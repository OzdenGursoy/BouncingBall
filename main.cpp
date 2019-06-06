#include "Angel.h"
#include "time.h"
#include "models.h"

int ShadingInfo = 0;
int lightBehavior = 0;

using namespace models;

GLuint Color;

// timing related variables
GLfloat currTime = 0;
const GLfloat deltaTime = .033;

// physics //
	// initial values for simulation
const vec3 ini_pos = vec3(-1.0, 1.0, -3.0);
const vec3 ini_vel = vec3(0.8, 0.0, 0.0);

vec3 light_location;
vec3 pos = ini_pos;
vec3 vel = ini_vel;
const vec3 acc = vec3(0.0, -2, 0.0);
	// velocity multiplier for each collision 
const GLfloat dampFactor = 0.9;

// bounds for side collision and resizing
typedef struct bounds {
	GLfloat xMin;
	GLfloat xMax;
	GLfloat yMin;
	GLfloat yMax;
} Bounds;

Bounds bounds = { -1, 1, -1, 1 };

// scale of the model 
const GLfloat defaultModelScale = 0.20;
// global variable to control scale for different models
GLfloat modelScale = defaultModelScale;
GLuint program;

// shader uniforms 
GLuint modelviewUniLoc;
GLuint projectionUniLoc;
GLuint colorUniLoc;
GLuint lightLocation;

bool textureFlag = false; //enable texture mapping
GLuint  TextureFlag; // texture flag uniform location

void loadTextureData(FILE *fd);

//----------------------------------------------------------------------------
void 
usage() {
	cout <<
		"---------------------------" << endl <<
		"   Bouncing Object Program" << endl <<
		"   Usage: " << endl <<
		"      Click to access the menu" << endl <<
		"      i - initialize pose at top left" << endl <<
		"      r - add a random velocity to the ball" << endl <<
		"      q - quit program" << endl <<
		"      h - print this help message" << endl;
}


// OpenGL initialization
void
init()
{
	// print help to console
	usage();

	// load model data into arrays (from models.h)
	sphere();


	glGenTextures(2, textures);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	loadTextureData(fopen("earth.ppm", "r"));
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	loadTextureData(fopen("basketball.ppm", "r"));


	// Create a vertex array object
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create and initialize a buffer object
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(spherePoints) + sizeof(sphereNormals) + sizeof(tex_coords),
		NULL, GL_STATIC_DRAW);


	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(spherePoints), spherePoints);

	glBufferSubData(GL_ARRAY_BUFFER, sizeof(spherePoints),
		sizeof(sphereNormals), sphereNormals);

	glBufferSubData(GL_ARRAY_BUFFER, sizeof(spherePoints)+sizeof(sphereNormals),
		sizeof(tex_coords), tex_coords);

	// Load shaders and use the resulting shader program
	program = InitShader("vshader.glsl", "fshader.glsl");
	glUseProgram(program);

	// set up vertex arrays
	GLuint vPosition = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(0));

	GLuint vNormal = glGetAttribLocation(program, "vNormal");
	glEnableVertexAttribArray(vNormal);
	glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(sizeof(spherePoints)));

	GLuint vTexCoord = glGetAttribLocation(program, "vTexCoord");
	glEnableVertexAttribArray(vTexCoord);
	glVertexAttribPointer(vTexCoord, 2, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(sizeof(spherePoints) + sizeof(sphereNormals)));

	// Initialize shader lighting parameters
	point4 light_position(-1.0, 0.0, 0.0, 1.0);
	color4 light_ambient(0.2, 0.2, 0.2, 1.0);
	color4 light_diffuse(1.0, 1.0, 1.0, 1.0);
	color4 light_specular(1.0, 1.0, 1.0, 1.0);

	color4 material_ambient(1.0, 0.0, 1.0, 1.0);
	color4 material_diffuse(1.0, 0.8, 0.0, 1.0);
	color4 material_specular(1.0, 0.8, 0.0, 1.0);
	float  material_shininess = 100.0;

	color4 ambient_product = light_ambient * material_ambient;
	color4 diffuse_product = light_diffuse * material_diffuse;
	color4 specular_product = light_specular * material_specular;

	glUniform4fv(glGetUniformLocation(program, "AmbientProduct"),
		1, ambient_product);
	glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"),
		1, diffuse_product);
	glUniform4fv(glGetUniformLocation(program, "SpecularProduct"),
		1, specular_product);

	glUniform4fv(glGetUniformLocation(program, "LightPosition"),
		1, light_position);

	glUniform1f(glGetUniformLocation(program, "Shininess"),
		material_shininess);


	glUniform1i(glGetUniformLocation(program, "ShadingInfo"),
		ShadingInfo);

	// Retrieve transformation uniform variable locations
	modelviewUniLoc = glGetUniformLocation(program, "ModelView");
	projectionUniLoc = glGetUniformLocation(program, "Projection");
	lightLocation = glGetUniformLocation(program, "LightLocation");

	TextureFlag = glGetUniformLocation(program, "TextureFlag");
	glUniform1i(TextureFlag, textureFlag);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glClearColor(1.0, 1.0, 1.0, 1.0);
}

//----------------------------------------------------------------------------

void
display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// update the transform of the object 
	mat4 transform = Translate(pos)
		* RotateX(currTime * 72) // constant rotation around 2 axes
		* RotateY(currTime * 20) // to appriciate all 3 dimensions
		* Scale(modelScale);

	mat4 lTransform = Translate(light_location);

	// pass transform into shader
	glUniformMatrix4fv(modelviewUniLoc, 1, GL_TRUE, transform);
	glUniformMatrix4fv(lightLocation, 1, GL_TRUE, lTransform);


	// select data to be drawn
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(spherePoints), spherePoints);
	
	// draw to backbuffer
	glDrawArrays(GL_TRIANGLES, 0, sphereNumVertices);
	// show rendered buffer
	glutSwapBuffers();
}

//----------------------------------------------------------------------------
void loadTextureData(FILE *fd) {

	//provided and fixed ppm reader
	int k, n, m;
	char c, b[100];
	fscanf(fd, "%[^\n] ", b);
	fscanf(fd, "%c", &c);
	while (c == '#') {
		fscanf(fd, "%[^\n] ", b);
		fscanf(fd, "%c", &c);
	}
	ungetc(c, fd);
	fscanf(fd, "%d %d %d", &n, &m, &k);
	GLubyte *image = (GLubyte *)malloc(3 * sizeof(GLubyte) * n * m);
	int red, green, blue;
	for (unsigned int i = n * m; i > 0; i--) {
		fscanf(fd, "%d %d %d", &red, &green, &blue);
		image[3 * n * m - 3 * i] = red;
		image[3 * n * m - 3 * i + 1] = green;
		image[3 * n * m - 3 * i + 2] = blue;
	}

	//init textures and texture parameters
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, n, m, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

}

void
keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	// quit program
	case 'q': case 'Q': case 033:  // Escape key
		exit(EXIT_SUCCESS);
		break;

	// return to inital state
	case 'i': case 'I':
		pos = vec3(bounds.xMin, bounds.yMax, -3.0);;
		vel = ini_vel;
		break;

	// add a random velocity
	case 'r': case 'R':
		vel.x = (GLfloat) 2.0 * (((rand() % 1000) / 1000.0) - 0.5);
		vel.y += (GLfloat) 2.0 * (rand() % 1000 / 1000.0);
		break;

	// print help to console
	case 'h': case 'H':
		usage();
		break;
	}
}

//----------------------------------------------------------------------------

void timer(int p) {
	currTime += deltaTime;

	// basic forward Euler integration 
	vel += acc * deltaTime;
	pos += vel * deltaTime;

	// bounds checking and bounce with damping
	if (pos.y < bounds.yMin + modelScale) {
		pos.y = bounds.yMin + modelScale;
		vel.y *= -1;
		vel *= dampFactor;
	}
	else if (pos.y > bounds.yMax - modelScale) {
		pos.y = bounds.yMax - modelScale;
		vel.y *= -1;
		vel *= dampFactor;
	}

	if (pos.x < bounds.xMin + modelScale) {
		pos.x = bounds.xMin + modelScale;
		vel.x *= -1;
		vel *= dampFactor;
	}
	else if (pos.x > bounds.xMax - modelScale) {
		pos.x = bounds.xMax - modelScale;
		vel.x *= -1;
		vel *= dampFactor;
	}

	if (lightBehavior == 1) {
		light_location.x = pos.x;
		light_location.y = pos.y;
		light_location.z = 10.0;
	}
	if (lightBehavior == 0) {
		light_location.x = 10.0;
		light_location.y = 10.0;
		light_location.z = 10.0;
	}
	
	glutPostRedisplay();

	glutTimerFunc(33, timer, 0);
}

//----------------------------------------------------------------------------

void reshape(int w, int h)
{
	glViewport(0, 0, w, h);

	GLfloat aspect = GLfloat(w) / h;

	mat4  projection = Perspective(45.0, aspect, 0.5, 6.0);
	glUniformMatrix4fv(projectionUniLoc, 1, GL_TRUE, projection);
}


////// menu callbacks //////

void doMetal() {
	color4 light_ambient(0.2, 0.2, 0.2, 1.0);
	color4 light_diffuse(1.0, 1.0, 1.0, 1.0);
	color4 light_specular(1.0, 1.0, 1.0, 1.0);

	color4 material_ambient(0.19225, 0.19225, 0.19225, 1.0);
	color4 material_diffuse(0.50754, 0.50754, 0.50754, 1.0);
	color4 material_specular(0.508273, 0.508273, 0.508273, 1.0);
	float  material_shininess = 1.4;

	color4 ambient_product = light_ambient * material_ambient;
	color4 diffuse_product = light_diffuse * material_diffuse;
	color4 specular_product = light_specular * material_specular;

	glUniform4fv(glGetUniformLocation(program, "AmbientProduct"),
		1, ambient_product);
	glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"),
		1, diffuse_product);
	glUniform4fv(glGetUniformLocation(program, "SpecularProduct"),
		1, specular_product);
	glUniform1f(glGetUniformLocation(program, "Shininess"),
		material_shininess);
}

void doPlastic() {
	color4 light_ambient(0.2, 0.2, 0.2, 1.0);
	color4 light_diffuse(1.0, 1.0, 1.0, 1.0);
	color4 light_specular(1.0, 1.0, 1.0, 1.0);

	color4 material_ambient(1.0, 0.0, 1.0, 1.0);
	color4 material_diffuse(1.0, 0.8, 0.0, 1.0);
	color4 material_specular(1.0, 0.8, 0.0, 1.0);
	float  material_shininess = 100.0;

	color4 ambient_product = light_ambient * material_ambient;
	color4 diffuse_product = light_diffuse * material_diffuse;
	color4 specular_product = light_specular * material_specular;

	glUniform4fv(glGetUniformLocation(program, "AmbientProduct"),
		1, ambient_product);
	glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"),
		1, diffuse_product);
	glUniform4fv(glGetUniformLocation(program, "SpecularProduct"),
		1, specular_product);
	glUniform1f(glGetUniformLocation(program, "Shininess"),
		material_shininess);
}

void doEmerald() {
	color4 light_ambient(0.2, 0.2, 0.2, 1.0);
	color4 light_diffuse(1.0, 1.0, 1.0, 1.0);
	color4 light_specular(1.0, 1.0, 1.0, 1.0);

	color4 material_ambient(0.0215, 0.1745, 0.0215, 1.0);
	color4 material_diffuse(0.07568, 0.61424, 0.07568, 1.0);
	color4 material_specular(0.633, 0.727811, 0.633, 1.0);
	float  material_shininess = 2.4;

	color4 ambient_product = light_ambient * material_ambient;
	color4 diffuse_product = light_diffuse * material_diffuse;
	color4 specular_product = light_specular * material_specular;

	glUniform4fv(glGetUniformLocation(program, "AmbientProduct"),
		1, ambient_product);
	glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"),
		1, diffuse_product);
	glUniform4fv(glGetUniformLocation(program, "SpecularProduct"),
		1, specular_product);
	glUniform1f(glGetUniformLocation(program, "Shininess"),
		material_shininess);
}
void
materialPropCallback(int id) {
	if (id == 0) doPlastic();	//Plastic
	if (id == 1) doMetal(); //Metallic
	if (id == 2) doEmerald(); //Emerald
	
}

void
textureCallback(int id) {
	textureFlag = true;
	if (id == 0) glBindTexture(GL_TEXTURE_2D, textures[0]);	//Earth
	if (id == 1) glBindTexture(GL_TEXTURE_2D, textures[1]); //Basketball
	TextureFlag = glGetUniformLocation(program, "TextureFlag");
	glUniform1i(TextureFlag, textureFlag);
}

void
lightSourceCallback(int id) {
	if (id == 0) lightBehavior = 0; // Fix it
	if (id == 1) lightBehavior = 1; // Move with object
}


void
specularOnOffCallback(int id) {
	if (id == 0) {
		color4 light_specular(1.0, 1.0, 1.0, 1.0);
		color4 material_specular(1.0, 0.8, 0.0, 1.0);
		color4 specular_product = light_specular * material_specular;
		glUniform4fv(glGetUniformLocation(program, "SpecularProduct"),
			1, specular_product);

	}
	if (id == 1) {
		color4 light_specular(0.0, 0.0, 0.0, 0.0);
		color4 material_specular(1.0, 0.8, 0.0, 1.0);
		color4 specular_product = light_specular * material_specular;
		glUniform4fv(glGetUniformLocation(program, "SpecularProduct"),
			1, specular_product);
	}
}


void
diffuseOnOffCallback(int id) {
	if (id == 0) {
		color4 light_diffuse(1.0, 1.0, 1.0, 1.0);
		color4 material_diffuse(1.0, 0.8, 0.0, 1.0);
		color4 diffuse_product = light_diffuse * material_diffuse;
		glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"),
			1, diffuse_product);
	}
	if (id == 1) {
		color4 light_diffuse(0.0, 0.0, 0.0, 0.0);
		color4 material_diffuse(1.0, 0.8, 0.0, 1.0);
		color4 diffuse_product = light_diffuse * material_diffuse;
		glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"),
			1, diffuse_product);
	}
}



void
ambientOnOffCallback(int id) {
	if (id == 0) {
		color4 light_ambient(0.2, 0.2, 0.2, 1.0);
		color4 material_ambient(1.0, 0.0, 1.0, 1.0);
		color4 ambient_product = light_ambient * material_ambient;
		glUniform4fv(glGetUniformLocation(program, "AmbientProduct"),
			1, ambient_product);
	}
	if (id == 1) {
		color4 light_ambient(0.0, 0.0, 0.0, 0.0);
		color4 material_ambient(1.0, 0.0, 1.0, 1.0);
		color4 ambient_product = light_ambient * material_ambient;
		glUniform4fv(glGetUniformLocation(program, "AmbientProduct"),
			1, ambient_product);
	}
}


void
componentSetupMenuCallback(int id) {
}

void
shadingMenuCallback(int id) {
	textureFlag = false;
	TextureFlag = glGetUniformLocation(program, "TextureFlag");
	glUniform1i(TextureFlag, textureFlag);
	if (id == 0)
		ShadingInfo = 0; 	// phong
	if (id == 1) ShadingInfo = 1; //Gouraud
	if (id == 2) ShadingInfo = 2; //Modified Phong
	//Send Shading Information
	glUniform1i(glGetUniformLocation(program, "ShadingInfo"),
		ShadingInfo);
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );	// solid
}

void
mainMenu(int id) { 
	if (id == 0) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); 	// wireframe
		textureFlag = false;
		TextureFlag = glGetUniformLocation(program, "TextureFlag");
		glUniform1i(TextureFlag, textureFlag);
	}
}

// setting up the right click menu
void initMenus() {
	int textureMenu = glutCreateMenu(textureCallback);
	glutAddMenuEntry("Earth", 0);
	glutAddMenuEntry("Basketball", 1);

	int MaterialPropMenu = glutCreateMenu(materialPropCallback);
	glutAddMenuEntry("Plastic", 0);
	glutAddMenuEntry("Metallic", 1);
	glutAddMenuEntry("Emerald", 2);
	//TODO you can add more here if you want

	int LightSourceMenu = glutCreateMenu(lightSourceCallback);
	glutAddMenuEntry("Fix it", 0);
	glutAddMenuEntry("Move with Object", 1);

	int specularMenu = glutCreateMenu(specularOnOffCallback);
	glutAddMenuEntry("On", 0);
	glutAddMenuEntry("Off", 1);


	int diffuseMenu = glutCreateMenu(diffuseOnOffCallback);
	glutAddMenuEntry("On", 0);
	glutAddMenuEntry("Off", 1);


	int ambientMenu = glutCreateMenu(ambientOnOffCallback);
	glutAddMenuEntry("On", 0);
	glutAddMenuEntry("Off", 1);


	int componentSetupMenu = glutCreateMenu(componentSetupMenuCallback);
	glutAddSubMenu("Specular", specularMenu);
	glutAddSubMenu("Diffuse", diffuseMenu);
	glutAddSubMenu("Ambient", ambientMenu);


	int shadingMenu = glutCreateMenu(shadingMenuCallback);
	glutAddMenuEntry("Phong", 0);
	glutAddMenuEntry("Gouraud", 1);
	glutAddMenuEntry("Modified Phong", 2);
	glutAddSubMenu("Components Setup", componentSetupMenu);
	glutAddSubMenu("Light Source Position", LightSourceMenu);
	glutAddSubMenu("Material Properties", MaterialPropMenu);

	int rmenu = glutCreateMenu(mainMenu);
	glutAddMenuEntry("Wireframe", 0);
	glutAddSubMenu("Shading", shadingMenu);
	glutAddSubMenu("Texture", textureMenu);

	// any click will open up the pop up menu
	glutAttachMenu(GLUT_RIGHT_BUTTON);
	glutAttachMenu(GLUT_LEFT_BUTTON);
	glutAttachMenu(GLUT_MIDDLE_BUTTON);
}


////// main //////
int
main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(512, 512);
	glutInitWindowPosition(50, 50);
	glutCreateWindow("Bouncing Object");

	printf("Supported GLSL version is %s.\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	printf("%s\n%s\n%s\n",
		glGetString(GL_RENDERER),  // e.g. Intel HD Graphics 3000 OpenGL Engine
		glGetString(GL_VERSION),    // e.g. 3.2 INTEL-8.0.61
		glGetString(GL_SHADING_LANGUAGE_VERSION));

	//glewExperimental = GL_TRUE;
	glewInit();

	// initialize models and OpenGL
	init();
	initMenus();

	// bind callbacks
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutTimerFunc(33, timer, 0);
	glutReshapeFunc(reshape);

	glutMainLoop();

	return 0;
}
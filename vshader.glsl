
#version 410

in   vec4 vPosition;
in   vec3 vNormal;
in  vec2 vTexCoord;
in  vec4 vColor;


// output values that will be interpretated per-fragment
out  vec3 fN;
out  vec3 fV;
out  vec3 fL;
out vec2 texCoord;
out vec4 vertColor;

uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform vec4 LightPosition;
uniform float Shininess;
uniform int ShadingInfo;
uniform vec4 color;
uniform mat4 ModelView;
uniform mat4 Projection;
uniform mat4 LightLocation;
uniform int TextureFlag;

void main()
{	

vec4 lPos = LightLocation * LightPosition; 

	if(TextureFlag == 0){
		if(ShadingInfo == 0){ //Phong Shading
			// Transform vertex position into camera (eye) coordinates
			vec3 pos = (ModelView * vPosition).xyz;
		
			fN = (ModelView * vec4(vNormal, 0.0)).xyz; // normal direction in camera coordinates
		
			fV = -pos; //viewer direction in camera coordinates
		
			fL = lPos.xyz; // light direction
		
			if( LightPosition.w != 0.0 ) {
			    fL = lPos.xyz - pos;  //directional light source
			}
		
			gl_Position = Projection * ModelView * vPosition;
			vertColor = color;
		}

		else if(ShadingInfo == 1){ //Gouraud Shading
			// Transform vertex position into camera (eye) coordinates
			vec3 pos = (ModelView * vPosition).xyz;
	
			vec3 L = normalize( lPos.xyz - pos ); //light direction
			vec3 V = normalize( -pos ); // viewer direction
			vec3 H = normalize( L + V ); // halfway vector

			// Transform vertex normal into camera coordinates
			vec3 N = normalize( ModelView * vec4(vNormal, 0.0) ).xyz;

			// Compute terms in the illumination equation
			vec4 ambient = AmbientProduct;

			float Kd = max( dot(L, N), 0.0 ); //set diffuse to 0 if light is behind the surface point
			vec4  diffuse = Kd*DiffuseProduct;

			float Ks = pow( max(dot(N, H), 0.0), Shininess );
			vec4  specular = Ks * SpecularProduct;
    
			//ignore also specular component if light is behind the surface point
			if( dot(L, N) < 0.0 ) {
				specular = vec4(0.0, 0.0, 0.0, 1.0);
			} 

			gl_Position = Projection * ModelView * vPosition;
		
			vertColor = ambient + diffuse + specular;
			vertColor.a = 1.0;

		}
		else if(ShadingInfo == 2){ //Modified Phong Shading
			// Transform vertex position into camera (eye) coordinates
			vec3 pos = (ModelView * vPosition).xyz;
		
			fN = (ModelView * vec4(vNormal, 0.0)).xyz; // normal direction in camera coordinates
		
			fV = -pos; //viewer direction in camera coordinates
		
			fL = lPos.xyz; // light direction
		
			if( LightPosition.w != 0.0 ) {
			    fL = lPos.xyz - pos;  //directional light source
			}
		
			gl_Position = Projection * ModelView * vPosition;
			vertColor = color;
		}
	}
	else if (TextureFlag == 1){
		vertColor       = vColor;
		
		gl_Position = Projection * ModelView * vPosition;
		texCoord = vec2(atan (vPosition.y, vPosition.x)/(2*3.145)+0.5, acos(vPosition.z/sqrt(length(vPosition.xyz)))/3.14);
	}
	
}

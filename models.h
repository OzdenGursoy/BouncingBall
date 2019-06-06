#pragma once
#include "Angel.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

typedef Angel::vec4  color4;
typedef Angel::vec4  point4;


// Texture objects and storage for texture image

const int  TextureSize = 64;
GLuint textures[2];

GLubyte image[TextureSize][TextureSize][3];
GLubyte image2[TextureSize][TextureSize][3];



using namespace std;

namespace models {
	enum { CUBE = 0, NUM_MODELS = 3 };
	////// SPHERE //////

	const int NumTimesToSubdivide = 3;
	const int NumTriangles = 4096; // (4 faces)^(NumTimesToSubdivide + 1)
	const int sphereNumVertices = 3 * NumTriangles;
	point4 spherePoints[sphereNumVertices];
	vec3   sphereNormals[sphereNumVertices];
	vec2 tex_coords[sphereNumVertices];

	int sphereIndex = 0;

	double uCalculate(double x, double r) {
		double temp = x / r;
		double arcIn = acos(temp);
		double u = arcIn / (2 * M_PI);
		return u;
	}

	double vCalculate(double y, double z) {
		double temp = z / y;
		double tanIn = atan(temp);
		double v = tanIn / (2 * M_PI);
		return v;
	}

	void
	triangle(const point4& a, const point4& b, const point4& c)
	{
		vec3 vecA = vec3(a.x, a.y, a.z);
		double r1 = length(vec3(a.x, a.y, a.z));
		vec2 texVec1 = vec2(uCalculate(a.x,r1), vCalculate(a.y,a.z));

		vec3 vecB = vec3(b.x, b.y, b.z);
		double r2 = length(vec3(b.x, b.y, b.z));
		vec2 texVec2 = vec2(uCalculate(b.x, r2), vCalculate(b.y, b.z));

		vec3 vecC = vec3(c.x, c.y, c.z);
		double r3 = length(vec3(c.x, c.y, c.z));
		vec2 texVec3 = vec2(uCalculate(c.x, r3), vCalculate(c.y, c.z));


		tex_coords[sphereIndex] = texVec1; sphereNormals[sphereIndex] = vecA; spherePoints[sphereIndex] = a; sphereIndex++;
		tex_coords[sphereIndex] = texVec2; sphereNormals[sphereIndex] = vecB; spherePoints[sphereIndex] = b; sphereIndex++;
		tex_coords[sphereIndex] = texVec3; sphereNormals[sphereIndex] = vecC; spherePoints[sphereIndex] = c; sphereIndex++;
	}

	point4
	unit(const point4& p)
	{ 
		float len = p.x*p.x + p.y*p.y + p.z*p.z;
		point4 t;
		if (len > DivideByZeroTolerance) {
			t = p / sqrt(len);
			t.w = 1.0;
		}
		return t;
	}
	void
	divide_triangle(const point4& a, const point4& b,
			const point4& c, int count)
	{
		if (count > 0) {
			point4 v1 = unit(a + b);
			point4 v2 = unit(a + c);
			point4 v3 = unit(b + c);
			divide_triangle(a, v1, v2, count - 1);
			divide_triangle(c, v2, v3, count - 1);
			divide_triangle(b, v3, v1, count - 1);
			divide_triangle(v1, v3, v2, count - 1);
		}
		else {
			triangle(a, b, c);
		}
	}

	void
	tetrahedron(int count)
	{
		point4 v[4] = {
			vec4(0.0, 0.0, 1.0, 1.0),
			vec4(0.0, 0.942809, -0.333333, 1.0),
			vec4(-0.816497, -0.471405, -0.333333, 1.0),
			vec4(0.816497, -0.471405, -0.333333, 1.0)
		};
		divide_triangle(v[0], v[1], v[2], count);
		divide_triangle(v[3], v[2], v[1], count);
		divide_triangle(v[0], v[3], v[1], count);
		divide_triangle(v[0], v[2], v[3], count);
	}

	void
	sphere() {
		tetrahedron(NumTimesToSubdivide);
	}

	
	// array of pointers to models points
	point4 *model_pts = spherePoints;

	// sizes of point arrays (needed due to non-uniform array size in the above array)
	size_t model_pts_sizes = sizeof(spherePoints);

	// number of vertices for each model
	int NumVertices = sphereNumVertices;
}
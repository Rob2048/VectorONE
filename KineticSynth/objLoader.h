#pragma once

#include <stdint.h>

typedef uint16_t u16;

struct vec2
{
	float x;
	float y;

	bool operator==(const vec2& R) const;
};

inline bool vec2::operator==(const vec2& R) const
{
	return (x == R.x && y == R.y);
}

struct vec3
{
	float x;
	float y;
	float z;

	bool operator==(const vec3& R) const;
};

inline bool vec3::operator==(const vec3& R) const
{
	return (x == R.x && y == R.y && z == R.z);
}

struct vec4
{
	float x;
	float y;
	float z;
	float w;
};

struct vsTexVertex
{
	vec3 pos;
	vec2 uv;
};

struct vsVertex
{
	vec3 pos;
	vec2 uv;
	vec3 normal;
};

struct vsOBJModel
{
	vsVertex*	verts;
	u16*		indices;
	int			vertCount;
	int			indexCount;
};

vsOBJModel CreateOBJ(const char* FileName);

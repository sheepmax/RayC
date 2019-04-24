#include <math.h>

typedef struct vector3d 
{
	float x;
	float y;
	float z;
} VECTOR3D;

float Vmagnitude (VECTOR3D vec) 
{
	return sqrtf(powf(vec.x, 2) 
		+ powf(vec.y, 2) 
		+ powf(vec.z, 2));
}

VECTOR3D Vnormalize (VECTOR3D vec) 
{
	float magnitude = Vmagnitude(vec);
	float x = vec.x/magnitude;
	float y = vec.y/magnitude;
	float z = vec.z/magnitude;
	VECTOR3D new = {.x = x, .y = y, .z = z};

	return new;
}

VECTOR3D Vadd (VECTOR3D vecA, VECTOR3D vecB)
{
	float x = vecA.x + vecB.x;
	float y = vecA.y + vecB.y;
	float z = vecA.z + vecB.z;
	VECTOR3D new = {.x = x, .y = y, .z = z};

	return new;
}

VECTOR3D Vsub (VECTOR3D vecA, VECTOR3D vecB)
{
	float x = vecA.x - vecB.x;
	float y = vecA.y - vecB.y;
	float z = vecA.z - vecB.z;
	VECTOR3D new = {.x = x, .y = y, .z = z};

	return new;
}

float Vdot (VECTOR3D vecA, VECTOR3D vecB)
{
	return (vecA.x * vecB.x +
	               vecA.y * vecB.y +
                                    vecA.z * vecB.z);
}

VECTOR3D Vscale (VECTOR3D vec, float scalar) 
{
	float x = vec.x * scalar;
	float y = vec.y * scalar;
	float z = vec.z * scalar;
	VECTOR3D new = {.x = x, .y = y, .z = z};

	return new;
}

VECTOR3D VcompMult (VECTOR3D vecA, VECTOR3D vecB)
{
	float x = vecA.x * vecB.x;
	float y = vecA.y * vecB.y;
	float z = vecA.z * vecB.z;
	VECTOR3D new = {.x = x, .y = y, .z = z};

	return new;
}

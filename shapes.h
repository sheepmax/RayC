enum shape_types {SHAPE_SPHERE, SHAPE_PLANE};

typedef struct plane
{
	VECTOR3D position; // Only y will be used
	int reflectivity;
	VECTOR3D colour;
} PLANE;

typedef struct sphere
{
	VECTOR3D position;
	float radius;
	int reflectivity;
	VECTOR3D colour;
} SPHERE;

typedef struct light
{
	VECTOR3D position;
	int intensity;
	VECTOR3D colour;
} LIGHT;

union shape_u
{
	SPHERE sphere;
	PLANE plane;
};

typedef struct shape_t 
{
	int type;
	union shape_u shape;
} SHAPE_t;

float get_shape_distance (VECTOR3D point, SHAPE_t shape)
{
	switch (shape.type)
	{
		case SHAPE_SPHERE:
			return (Vmagnitude(Vsub(shape.shape.sphere.position, point)) - shape.shape.sphere.radius); // Vsub(shape.shape.sphere.position, point) = distance to sphere
																									   // C doesn't allow declarations after labels
		case SHAPE_PLANE:
			return (point.y - shape.shape.plane.position.y);
	}
}
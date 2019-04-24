#include "vectors.h"
#include "shapes.h"
#include "png_maker.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <string.h>

// ------ Constants ------ //
#define WIN_WIDTH 100
#define WIN_HEIGHT 100

#define MAX_ITERATIONS 1000
#define MINIMUM_DISTANCE 0.01
#define MAXIMUM_DISTANCE 1000
#define NORMAL_ACCURACY 0.001

#define MAX_SCENE_OBJECTS 10
#define MAX_SCENE_LIGHTS 10

#define FRAME_VALUES 10
// ----------------------- //

// ------- Macros -------- //
#define MAX(a, b) (((a) > (b)) ? (a) : (b))  
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
// Note to self - macro parameters must not have space after the name!
// #define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))
// ----------------------  //

// Vectors will be reused as RGB

Uint32 frametimes[FRAME_VALUES];
Uint32 frametimelast;
Uint32 framecount;
float framespersecond;

void fpsinit() {

// Set all frame times to 0ms.
	memset(frametimes, 0, sizeof(frametimes));
	framecount = 0;
	framespersecond = 0;
	frametimelast = SDL_GetTicks();

}

void fpsthink() {

	Uint32 frametimesindex;
	Uint32 getticks;
	Uint32 count;
	Uint32 i;
// frametimesindex is the position in the array. It ranges from 0 to FRAME_VALUES.
// This value rotates back to 0 after it hits FRAME_VALUES.
	frametimesindex = framecount % FRAME_VALUES;
// store the current time
	getticks = SDL_GetTicks();
// save the frame time value
	frametimes[frametimesindex] = getticks - frametimelast;
// save the last frame time for the next fpsthink
	frametimelast = getticks;
// increment the frame count
	framecount++;
// Work out the current framerate

// The code below could be moved into another function if you don't need the value every frame.

// I've included a test to see if the whole array has been written to or not. This will stop
// strange values on the first few (FRAME_VALUES) frames.
	if (framecount < FRAME_VALUES) {
		count = framecount;
	} else {
		count = FRAME_VALUES;
	}
// add up all the values and divide to get the average frame time.
	framespersecond = 0;
	for (i = 0; i < count; i++) {
		framespersecond += frametimes[i];
	}
	framespersecond /= count;
// now to make it an actual frames per second value...
	framespersecond = 1000.f / framespersecond;
}

// -------------- Scene stuff -------------- //
VECTOR3D camera_position = {0, 2, 0};
// ---------------------------------------------//

// ------- Other initial stuff ------- //
SHAPE_t scene_objects[MAX_SCENE_OBJECTS];
LIGHT scene_lights[MAX_SCENE_LIGHTS];
unsigned int scene_lights_index = 0;
unsigned int scene_objects_index = 0;
// ------------------------------------ //

void get_scene_distance (VECTOR3D point, SHAPE_t *return_shape, float *return_distance)
{
	*return_distance = get_shape_distance(point, scene_objects[0]);
	*return_shape = scene_objects[0];

	for (int i = 1; i < scene_objects_index; i++) 
	{
		SHAPE_t to_test = scene_objects[i];

		float to_distance = get_shape_distance(point, to_test);
		if (to_distance < *return_distance)
		{
			*return_distance = to_distance;
			*return_shape = to_test;
		}
	}
}

VECTOR3D get_scene_normal (VECTOR3D point)
{
	float initial_distance, delta_x, delta_y, delta_z;
	SHAPE_t *garbage = malloc(sizeof(SHAPE_t)); // Will not be used beyond being a filler parameter
	get_scene_distance(point, garbage, &initial_distance);
	get_scene_distance(Vsub(point, (VECTOR3D){NORMAL_ACCURACY, 0, 0}), garbage, &delta_x); // Bye bye readability ;-; 
	get_scene_distance(Vsub(point, (VECTOR3D){0, NORMAL_ACCURACY, 0}), garbage, &delta_y); // This is basically magic anyways
	get_scene_distance(Vsub(point, (VECTOR3D){0, 0, NORMAL_ACCURACY}), garbage, &delta_z);
	VECTOR3D scene_delta = {initial_distance - delta_x, initial_distance - delta_y, initial_distance - delta_z};
	free(garbage);
	return Vnormalize(scene_delta);
}

float calculate_basic_shade(VECTOR3D point, LIGHT target_light)
{
	VECTOR3D direction_to_light = Vnormalize(Vsub(target_light.position, point));
	VECTOR3D surface_normal = get_scene_normal(point);

	float light_intensity = Vdot(surface_normal, direction_to_light) + (0.01 * target_light.intensity);
	return MAX(light_intensity, 0);	
}

float calculate_soft_shadow (VECTOR3D point, LIGHT target_light)
{
	float shadow_multiplier = 1;
	float distance_marched = 0;
	float step_distance = 0;

	VECTOR3D surface_normal = get_scene_normal(point);
	VECTOR3D corrected_point = Vadd(point, Vscale(surface_normal, MINIMUM_DISTANCE));

	VECTOR3D vector_to_light = Vsub(target_light.position, point);
	float distance_to_light = Vmagnitude(vector_to_light);
	VECTOR3D direction_to_light = Vnormalize(vector_to_light);

	SHAPE_t *garbage = malloc(sizeof(SHAPE_t));

	for (int i = 0; i < MAX_ITERATIONS; i++)
	{
		get_scene_distance(corrected_point, garbage, &step_distance);
		distance_marched += step_distance;

		if (distance_marched > MAXIMUM_DISTANCE)
		{
			break;
		}
		else if (step_distance < MINIMUM_DISTANCE)
		{
			shadow_multiplier = 0;
			break;
		}

		corrected_point = Vadd(corrected_point, Vscale(direction_to_light, step_distance));
		shadow_multiplier = MIN(shadow_multiplier, step_distance/distance_marched);
	}
	free(garbage);
	return MAX(shadow_multiplier, 0);
}

void march_ray (VECTOR3D ray_direction, VECTOR3D *ray_origin, SHAPE_t **return_shape)
{
	// ray_origin reused to return intersection point
	// return_shape = intersected shape
	float distance_marched = 0;
	float step_distance = 0;

	for (int i = 0; i < MAX_ITERATIONS; i++)
	{
		get_scene_distance(*ray_origin, *return_shape, &step_distance);

		distance_marched += step_distance;

		if (step_distance < MINIMUM_DISTANCE)
		{
			break;
		}
		else if (distance_marched > MAXIMUM_DISTANCE)
		{
			*return_shape = NULL;
			break;
		}

		*ray_origin = Vadd(*ray_origin, Vscale(ray_direction, step_distance));
	}
}

VECTOR3D get_colour (VECTOR3D ray_direction, VECTOR3D ray_origin)
{
	VECTOR3D intersection_point = ray_origin;
	SHAPE_t *intersected_shape = malloc(sizeof(SHAPE_t));

	march_ray(ray_direction, &intersection_point, &intersected_shape);

	VECTOR3D accumulated_colour_ratio = {0, 0, 0}; // Will be returned as black, or used later

	if (!intersected_shape)
	{
		return accumulated_colour_ratio; // Black
	}
	else
	{
		for (int i = 0; i < scene_lights_index; i++)
		{
			LIGHT target_light = scene_lights[i];
			float basic_shade = calculate_basic_shade(intersection_point, target_light);
			float soft_shadow = calculate_soft_shadow(intersection_point, target_light);
			float total_shade = basic_shade * soft_shadow;
			// printf("%f\n", total_shade);
			accumulated_colour_ratio = Vadd(accumulated_colour_ratio, Vscale(target_light.colour, total_shade));
		}
		
		accumulated_colour_ratio.x = MIN(1, accumulated_colour_ratio.x);
		accumulated_colour_ratio.y = MIN(1, accumulated_colour_ratio.y);
		accumulated_colour_ratio.z = MIN(1, accumulated_colour_ratio.z);

		//printf("R: %d, G: %d, B: %d\n", accumulated_colour_ratio.x, accumulated_colour_ratio.y, accumulated_colour_ratio.z);

		VECTOR3D shape_colour;
		float shape_reflectivity;

		switch (intersected_shape->type)
		{
			case SHAPE_SPHERE:
				shape_colour = (intersected_shape->shape).sphere.colour;
				shape_reflectivity = (float)(intersected_shape->shape).sphere.reflectivity / 100.f;
				break;

			case SHAPE_PLANE:
				shape_colour = (intersected_shape->shape).plane.colour;
				shape_reflectivity = (float)(intersected_shape->shape).plane.reflectivity / 100.f;
				break;
		}

		free(intersected_shape);

		VECTOR3D shaded_colour = VcompMult(shape_colour, accumulated_colour_ratio);
		//if (intersected_shape->type == SHAPE_SPHERE) printf("R: %f, G: %f, B: %f\n", shaded_colour.x, shaded_colour.y, shaded_colour.z);
		if (shape_reflectivity > 0)
		{
			//printf("ref\n");
			VECTOR3D intersection_normal = get_scene_normal(intersection_point);
			VECTOR3D reflected_direction = Vsub(ray_direction, Vscale(intersection_normal, (Vdot(ray_direction, intersection_normal) * 2))); // Alright xD
			VECTOR3D reflected_colour = get_colour(reflected_direction, Vadd(intersection_point, Vscale(intersection_normal, MINIMUM_DISTANCE)));
			return Vadd(Vscale(shaded_colour, (1 - shape_reflectivity)), Vscale(reflected_colour, shape_reflectivity));
		}

		return shaded_colour;
	}

}


void add_sphere (float x, float y, float z, float radius, int reflectivity, uint8_t R, uint8_t G, uint8_t B)
{
	SHAPE_t new_sphere;
	new_sphere.type = SHAPE_SPHERE;

	SPHERE new_sphere_data = {.position = {x, y, z}, .radius = radius, .reflectivity = reflectivity, .colour = {R, G, B}};
	new_sphere.shape.sphere = new_sphere_data;

	scene_objects[scene_objects_index] = new_sphere;
	scene_objects_index++;
}

void add_plane (float y, int reflectivity, uint8_t R, uint8_t G, uint8_t B)
{
	SHAPE_t new_plane;
	new_plane.type = SHAPE_PLANE;

	PLANE new_plane_data = {.position = {0, y, 0}, .reflectivity = reflectivity, .colour = {R, G, B}};
	new_plane.shape.plane = new_plane_data;

	scene_objects[scene_objects_index] = new_plane;
	scene_objects_index++;
}

void add_light (float x, float y, float z, int intensity, uint8_t R, uint8_t G, uint8_t B)
{
	LIGHT new_light = {.position = {x, y, z}, .intensity = intensity, .colour = {R/255, G/255, B/255}};
	// The "colour" of the light is divided by 255
	// This gives the ratio of that colour that the light will light up
	// If a light is of colour (255, 0, 0), then it will light up all red and no other colour
	// 1 = 100%
	scene_lights[scene_lights_index] = new_light;
	scene_lights_index++;
}

int main (int argc, char *argv[]) {

	// Set up scene (no error checking) //
	add_sphere (0, 1, 5, 1, 50, 255, 255, 255);
	add_light  (0, 1, 2, 50, 255, 255, 255);
	add_light  (0, 2, 2, 50, 255, 255, 255);
	add_plane  (0, 1, 255, 255, 255);

	// refine lighting system to work with walls that are above light sources and stuff
	
	// -------------------------------- //

	// Check for errors when initializing
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0) 
	{
		printf("Error initializing SDL: %s!\n", SDL_GetError());
		return 1;
	}

	SDL_Event event;


	
	// Create window
	SDL_Window *window = SDL_CreateWindow ("Test window", // name
										100, 			// x
										100,			// y
										WIN_WIDTH,			// w
										WIN_HEIGHT,			// h
										SDL_WINDOW_RESIZABLE);	// flags

	if (!window)
	{
		printf("Could not create window: %s", SDL_GetError());
		SDL_Quit();
		return 1;
	}

 	SDL_Renderer *renderer = SDL_CreateRenderer(window, // Window the renderer is to affect
 												-1, // Driver to use (-1 uses the first compatible one)
 												SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); // Flags

 	if (!renderer)
 	{
 		printf("Could not create renderer: %s", SDL_GetError());
 		SDL_DestroyWindow(window);
 		SDL_Quit();
 		return 1;
 	}

 	SDL_Texture *texture = SDL_CreateTexture(renderer,
                               				 SDL_PIXELFORMAT_ARGB8888,
                              				 SDL_TEXTUREACCESS_STREAMING,
                               				 WIN_WIDTH,
                              				 WIN_HEIGHT);



 	unsigned char *pixels = calloc(WIN_HEIGHT, WIN_WIDTH * 4);
 	fpsinit();

	while (1)
	{
		for (int y = 0; y < WIN_HEIGHT; y++)
		{
			printf("Y %d\n", y);
			for (int x = 0; x < WIN_WIDTH; x++)
			{
				//printf("Working X %d\n", x);
				float pixel_x = (x - 0.5 * WIN_WIDTH) / WIN_HEIGHT;				// For square screen the coordinates are x = -0.5 on the left and y = 0.5 for the top
				float pixel_y = (-y + 0.5 * WIN_HEIGHT) / WIN_HEIGHT;
				VECTOR3D pixel_direction = Vnormalize((VECTOR3D){pixel_x, pixel_y, 1});
				VECTOR3D shaded_colour_vec = get_colour(pixel_direction, camera_position);
				
				//printf("R: %f, G: %f, B: %f\n", shaded_colour_vec.x, shaded_colour_vec.y, shaded_colour_vec.z);
				unsigned int offset = (WIN_WIDTH * 4 * y) + (x * 4);
				pixels[offset] = shaded_colour_vec.z;
				pixels[offset + 1] = shaded_colour_vec.y;
				pixels[offset + 2] = shaded_colour_vec.x;
				pixels[offset + 3] = 255;

				SDL_PollEvent(&event);
				switch (event.type)
				{
					case SDL_QUIT:
						goto Finish;
				}
			}
			SDL_UpdateTexture(texture, NULL, pixels, WIN_WIDTH*4);
			SDL_RenderCopy(renderer, texture, NULL, NULL);
			SDL_RenderPresent(renderer);
			
		}

		unsigned char temp;
		for (int i = 0; i < WIN_HEIGHT * WIN_WIDTH; i++)
		{
			temp = pixels[i * 4];
			pixels[i * 4] = pixels[i * 4 + 2];
			pixels[i * 4 + 2] = temp;
		}

		char title[15];

		snprintf(title, sizeof(title), "Frame_%d.png", framecount);

		make_png(title, WIN_WIDTH, WIN_HEIGHT, "Aframe", pixels);

		fpsthink();
		printf("Current FPS: %f\n", framespersecond);		
	}

	Finish:
	// Clean-up SDL resources
	free(pixels);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}



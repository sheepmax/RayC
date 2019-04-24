#include <png.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

int make_png(char *file_name, int width, int height, char* title, unsigned char *buffer)
{
	// Basically no error checking because I'm lazy
	FILE *file_ptr = fopen(file_name, "wb");

	if (!file_ptr) return 0;

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info_ptr = png_create_info_struct(png_ptr);

	png_init_io(png_ptr, file_ptr);

	png_set_IHDR(png_ptr, info_ptr, width, height, 8, 
				PNG_COLOR_TYPE_RGB_ALPHA, 
				PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_BASE,
				PNG_FILTER_TYPE_BASE);

	png_text title_text;
	title_text.compression = PNG_TEXT_COMPRESSION_NONE;
	title_text.key = "Title";
	title_text.text = title;
	png_set_text(png_ptr, info_ptr, &title_text, 1);
	png_write_info(png_ptr, info_ptr);

	png_bytepp row_pointers = png_malloc(png_ptr, height * sizeof(png_bytep));
	for (int y = 0; y < height; y++)
	{
		png_bytep row = png_malloc(png_ptr, sizeof(uint8_t) * width * 4);
		row_pointers[y] = row;
		for (int x = 0; x < width; x++)
		{
			row[x * 4]     = buffer[y * width * 4 + x * 4]    ;
			row[x * 4 + 1] = buffer[y * width * 4 + x * 4 + 1];
			row[x * 4 + 2] = buffer[y * width * 4 + x * 4 + 2];
			row[x * 4 + 3] = buffer[y * width * 4 + x * 4 + 3];
		}
	}	
	
	png_set_rows(png_ptr, info_ptr, row_pointers);

	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	fclose(file_ptr);
	png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	for (int y = 0; y < height; y++)
	{
		png_free(png_ptr, row_pointers[y]);
	}
	png_free(png_ptr, row_pointers);
	
	return 1;
}
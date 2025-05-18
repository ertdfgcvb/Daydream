#ifndef DAYDREAM_UTILS_H
#define DAYDREAM_UTILS_H

#include <stdint.h>
#include <math.h>
#include "etherdream.h"

/**
 * Structure to store RGB color values
 * Etherdream DACs expect 16-bit values for each color component [0..65535]
 */
struct color {
	uint16_t r;
	uint16_t g;
	uint16_t b;
} current_stroke;

/**
 * Clamps an integer value to the range 0-255
 *
 * @param i The integer value to clamp
 * @return The clamped value (0-255)
 */
uint8_t clamp8(int32_t i) {
	return (uint8_t) (i < 0 ? 0 : (i > 255 ? 255 : i));
}

/**
 * Converts a byte value to a 16-bit word value
 * Used to convert the 8 bit OSC color values to the Etherdream DAC color values
 *
 * @param b The byte value to convert
 * @return The converted 16-bit word value
 */
uint16_t byte2word(int32_t b) {
	return (uint16_t) (clamp8(b) * 257);
}

/**
 * Generates a circle pattern for testing purposes
 *
 * @param points Array to store the generated points
 * @param offset Starting index in the points array
 * @param num_points Number of points to generate
 * @param x Center X coordinate
 * @param y Center Y coordinate
 * @param radius Radius of the circle
 * @param phase Phase offset for animation
 */
void circle(struct etherdream_point *points, int offset, int num_points, int x, int y, int radius) {
	int i;
	for (i = 0; i < num_points; i++) {
		struct etherdream_point *pt = &points[offset + i];
		float ip = (float)i * 2.0 * M_PI / (float) num_points;

		pt->x = x + sin(ip) * radius;
		pt->y = y + cos(ip) * radius;
		pt->r = 65535;
		pt->g = 0;
		pt->b = 0;
	}
}

#endif /* DAYDREAM_UTILS_H */

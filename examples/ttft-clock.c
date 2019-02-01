/* ttft-clock.c -- clock application for use with ttft */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define HEIGHT 64
#define WIDTH 64

#define MIN(X, Y)  ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y)  ((X) > (Y) ? (X) : (Y))

/* set pixel x,y of buf */
void
put_pixel(int x, int y, uint8_t *buf)
{
	int pixeli = (y * WIDTH + x) % 8;
	int bytei = (y * WIDTH + x) / 8;
	int sft = 1 << (7 - pixeli);
	buf[bytei] |= sft;
	return;
}

/* draw a line from x0,y0 to x1,y1 on buf using Bresenham's algorithm */
void bresenham(int x0, int y0, int x1, int y1, uint8_t *buf)
{
	/* taken from Rosetta Code */
	int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
	int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
	int err = (dx>dy ? dx : -dy)/2, e2;

	for(;;){
		put_pixel(x0, y0, buf);
		if (x0==x1 && y0==y1) break;
		e2 = err;
		if (e2 >-dx) { err -= dy; x0 += sx; }
		if (e2 < dy) { err += dx; y0 += sy; }
	}
	return;
}

/* draw a line on buf starting at x,y with angle and length */
void
draw_line(int x0, int y0, int angle, int length, uint8_t *buf)
{
	double rads = (M_PI/180) * angle;
	int x1 = x0 + (length * cos(rads));
	int y1 = y0 + (length * sin(rads));
	bresenham(x0, y0, x1, y1, buf);

	return;
}

/* draw a circle of radius r on buf with centre at x,y */
void
draw_circle(int x0, int y0, int r, uint8_t *buf)
{
	/* taken from Wikipedia article Midpont Circle Algorithm */
	int x = r - 1;
	int y = 0;
	int dx = 1;
	int dy = 1;
	int err = dx - (r << 1);

	while (x >= y) {
		put_pixel(x0 + x, y0 + y, buf);
		put_pixel(x0 + y, y0 + x, buf);
		put_pixel(x0 - y, y0 + x, buf);
		put_pixel(x0 - x, y0 + y, buf);
		put_pixel(x0 - x, y0 - y, buf);
		put_pixel(x0 - y, y0 - x, buf);
		put_pixel(x0 + y, y0 - x, buf);
		put_pixel(x0 + x, y0 - y, buf);

		if (err <= 0) {
			y += 1;
			err += dy;
			dy += 2;
		}
		if (err > 0) {
			x -= 1;
			dx += 2;
			err += dx - (r << 1);
		}
	}
	return;
}

/* draw a analogue clock on buf displaying the given hour and minute */
void
draw_clock(int hour, int minute, uint8_t *buf)
{
	int x = WIDTH / 2;
	int y = HEIGHT / 2;
	int r = (MIN(HEIGHT, WIDTH)) / 2;
	/* multiply hour/minute to get an angle in degrees
	   & -90 to originate at 12 o'clock */
	int hourang = ((hour % 12) * 30) - 90;
	int minang = (minute * 6) - 90;

	draw_circle(x, y, r, buf);
	draw_line(x, y, hourang, r * 0.4, buf);
	draw_line(x, y, minang, r * 0.8, buf);
	return;
}

/* once a minute, draw a clock displaying the current time */
int
main(int argc, char **argv)
{
	const size_t bufsize = (HEIGHT*WIDTH)/8;
	uint8_t buf[bufsize];
	time_t rawtime;
	struct tm *currtime;

	while (1) {
		rawtime = time(NULL);
		currtime = localtime(&rawtime);
		memset(buf, 0, bufsize);
		draw_clock(currtime->tm_hour, currtime->tm_min, buf);
		fwrite(buf, 1, bufsize, stdout);
		fflush(stdout);
		sleep(60);
	}

	return 0;
}

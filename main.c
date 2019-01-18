/* main.c -- main file for tiny tft */

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>


/* convert mono1 array to RBG bitmap format for SDL
   return an array which must be freed. return null on failure */
uint8_t*
mono1torgb24(char *mono, int w, int h)
{
	uint8_t *rgb;
	int i, j, k;

	/* 3 bytes per pixel by width and height of buffer */
	if (!(rgb = malloc(3 * w * h))) {
		return 0;
	}

	/* i increments over bytes, j over bits and serves to build a mask
	   the highest bits come first */
	for (i = 0; i < (w*h) / 8; i++) {
		/* index into rbg - each mono byte corresponds to 24 rgb bytes,
		   each mono bit corresponds to 3 rgb bytes */
		k = i * 24;
		for (j = 7; j >= 0; j--) {
			rgb[k] = mono[i] & 1 << j ? 0xFF : 0x00;
			rgb[k + 1] = mono[i] & 1 << j ? 0xFF : 0x00;
			rgb[k + 2] = mono[i] & 1 << j ? 0xFF : 0x00;
			k += 3;
		}
	}

	return rgb;
}

/* render the current frame to the given streamable SDL texture
   return 0 on success; else -1 */
int
sdl_render_frame(char *mono, SDL_Texture *tex, int w, int h)
{
	uint8_t *memrgb24;
	int r = 0;

	if (!(memrgb24 = mono1torgb24(mono, w, h))) {
		perror("mono1torgb24 error");
		return -1;
	}

	/* if Update texture returns an error */
	if (SDL_UpdateTexture(tex, NULL, memrgb24, 3 * w)) {
		fprintf(stderr, "SDL_UpdateTexture Error: %s\n", SDL_GetError());
		r = -1;
	}

	free(memrgb24);
	return r;
}

/* draw the current frame to the screen */
void
sdl_present_frame(SDL_Renderer *rdr, SDL_Texture *tex)
{
	SDL_RenderClear(rdr);
	SDL_RenderCopy(rdr, tex, NULL, NULL);
	SDL_RenderPresent(rdr);
	return;
}

/* read a single frame from stdin and store it in input
   return -1 on error, else number of bytes read */
int
read_frame(char *input, int w, int h)
{
	int n;
	for (n = 0; n < (w*h) / 8;) {
		n += fread(input, 1, (w*h) / 8, stdin);

	}
	return n;
}

int
main(int argc, char **argv)
{
	SDL_Window *sdlwin;
	SDL_Renderer *sdlrdr;
	SDL_Texture *sdltex;
	SDL_Event e;
	int pause, quit;
	char c;
	int width, height, scale;
	char *input;

	width = height = scale = 0;
	while ((c = getopt (argc, argv, "w:h:s:")) != -1) {
		switch (c) {
		case 'w':
			width = atoi(optarg);
			break;
		case 'h':
			height = atoi(optarg);
			break;
		case 's':
			scale = atoi(optarg);
			break;
		default:
			exit(EXIT_FAILURE);
		}
	}

	if (width <= 0 || height <= 0) {
		fprintf(stderr, "usage: height and width must be provided\n");
		exit(EXIT_FAILURE);
	}

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
	}

	/* Open SDL window */
	sdlwin = SDL_CreateWindow("ttft", 0, 0, width*scale, height*scale,
							  SDL_WINDOW_SHOWN);
	if (!sdlwin) {
		fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
		SDL_Quit();
		exit(1);
	}

	/* Create SDL renderer */
	sdlrdr = SDL_CreateRenderer(sdlwin, -1,
								SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
	if (!sdlrdr) {
		SDL_DestroyWindow(sdlwin);
		fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
		SDL_Quit();
		exit(1);
	}

	/* create texture */
	sdltex = SDL_CreateTexture(sdlrdr, SDL_PIXELFORMAT_RGB24,
							   SDL_TEXTUREACCESS_STATIC, width, height);
	if (!sdltex) {
		fprintf(stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
		SDL_Quit();
		exit(1);
	}

	/* create input buffer, we need to account for the null byte */
	input = malloc((width*height / 8) + 1);

	int n = 0;
	pause = quit = 0;
	/* main loop */
	while (!quit) {
		/* first frame will be completely black */
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_QUIT:
				quit = 1;
				fprintf(stderr, "received user quit\n");
				break;
			case SDL_WINDOWEVENT:
				if (e.window.event == SDL_WINDOWEVENT_EXPOSED)
					sdl_present_frame(sdlrdr, sdltex);
			default:
				break;
			}
		}
		/* read from stdin and render given input */
		/* read_frame is blocking, so the loop won't run out of control */
		n = read_frame(input, width, height);
		sdl_render_frame(input, sdltex, width, height);
		sdl_present_frame(sdlrdr, sdltex);
		printf("rendered frame %d bytes\n", n);
	}

	SDL_DestroyTexture(sdltex);
	SDL_DestroyRenderer(sdlrdr);
	SDL_DestroyWindow(sdlwin);
	SDL_Quit();

	return 0;
}

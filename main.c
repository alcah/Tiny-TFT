/* main.c -- main file for tiny tft */

#include <SDL2/SDL.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <getopt.h>

#define SDL_CREATE(r, f)											\
	if (r) { fprintf(stderr, "%s Error: %s\n", f, SDL_GetError());	\
		SDL_Quit();													\
		exit(EXIT_FAILURE); }

struct Threadargs
{
	/* file descriptor to send data */
	int out;
	/* SDL event to raise on read */
	uint32_t event;
	/* frame size to read in bytes */
	size_t fsize;
	/* time to sleep between each read in ms */
	int wait;
};

/* print usage information and exit */
void
usage()
{
	printf("Usage: ttft -x ?? -y ??\n\
Tiny TFT: Display video data read from stdin\n\n\
 -h\tdisplay this help message\n\
 -x\tvideo width (mandatory)\n\
 -y\tvideo height (mandatory)\n\
 -s\tscaling factor\n\
 -w\tminimum wait time between frames (ms)\n\
 -f\tforeground (on) colour in rgb hex notation\n\
 -b\tbackground (off) colour in rgb hex notation\n");
	exit(0);
}

/* convert mono1 array to RBG bitmap format for SDL
   return an array which must be freed. return null on failure */
uint8_t*
mono1torgb24(uint8_t *mono, int w, int h, int fg, int bg)
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
			/* we set each colour component to the corresponding byte
			 * in fg or bg */
			rgb[k] = mono[i] & 1 << j ? fg >> 16 : bg >> 16;
			rgb[k + 1] = mono[i] & 1 << j ? (fg >> 8) & 0xFF : (bg >> 8) & 0xFF;
			rgb[k + 2] = mono[i] & 1 << j ? fg & 0xFF : bg & 0xFF;
			k += 3;
		}
	}

	return rgb;
}

void*
thread_read_input(void *targs)
{
	struct Threadargs *args = (struct Threadargs *)targs;
	uint8_t input[args->fsize];
	SDL_Event e;

	/* read from pipe -- if a frame is read feed it into next pipe */
	/* otherwise raise an SDL_QUIT event */
	SDL_memset(&e, 0, sizeof(e));
	e.type = args->event;

	while (fread(input, 1, args->fsize, stdin) == args->fsize) {
		write(args->out, input, args->fsize);
		SDL_PushEvent(&e);
		if (args->wait)
			usleep(1000 * args->wait);
	}

	e.type = SDL_QUIT;
	SDL_PushEvent(&e);

	pthread_exit(NULL);
}

/* render the current frame to the given streamable SDL texture
   return 0 on success; else -1 */
int
sdl_render_frame(uint8_t *mono, SDL_Texture *tex, int w, int h, int fg, int bg)
{
	uint8_t *memrgb24;
	int r = 0;

	if (!(memrgb24 = mono1torgb24(mono, w, h, fg, bg))) {
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

int
main(int argc, char **argv)
{
	SDL_Window *sdlwin;
	SDL_Renderer *sdlrdr;
	SDL_Texture *sdltex;
	SDL_Event e;
	int quit = 0;
	char arg;
	int width = -1, height = -1;
	int scale = 10;
	int wait = 0;
	int fg = 0xFFFFFF, bg = 0x000000;
	size_t fsize;
	uint8_t *input;
	pthread_t inthread;
	int pipefd[2];
	Uint32 sdl_new_frame;
	struct Threadargs args;

	while ((arg = getopt (argc, argv, "hx:y:s:w:f:b:")) != -1) {
		switch (arg) {
		case 'h':
			usage();
			break;
		case 'x':
			width = atoi(optarg);
			break;
		case 'y':
			height = atoi(optarg);
			break;
		case 's':
			scale = atoi(optarg);
			break;
		case 'w':
			wait = atoi(optarg);
			break;
		case 'f':
			if (optarg[0] == '#')
				optarg++;
			fg = strtol(optarg, NULL, 16);
			break;
		case 'b':
			if (optarg[0] == '#')
				optarg++;
			bg = strtol(optarg, NULL, 16);
			break;
		default:
			usage();
		}
	}

	/* check sanity of args */
	if (width <= 0 || height <= 0) {
		usage();
	}
	fsize = (width * height) / 8;
	input = calloc(1, fsize);

	/* initialise SDL */
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
	}
	SDL_CREATE(!(sdlwin = SDL_CreateWindow("ttft", 0, 0, width*scale,
										   height*scale, SDL_WINDOW_SHOWN)),
			   "SDL_CreateWindow");
	SDL_CREATE(!(sdlrdr = SDL_CreateRenderer(sdlwin, -1,
											 SDL_RENDERER_ACCELERATED)),
			   "SDL_CreateRenderer");
	SDL_CREATE(!(sdltex = SDL_CreateTexture(sdlrdr, SDL_PIXELFORMAT_RGB24,
											SDL_TEXTUREACCESS_STATIC,
											width, height)),
			   "SDL_CreateTexture");
	SDL_CREATE(((sdl_new_frame = SDL_RegisterEvents(1)) == (Uint32)-1),
			   "SDL_RegisterEvents");

	/* create pipe */
	if (pipe(pipefd)) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}

	/* set up thread's args */
	args.out = pipefd[1];
	args.event = sdl_new_frame;
	args.fsize = fsize;
	args.wait = wait;

	/* create thread to read from stdin and pass messages back to SDL */
	if (pthread_create(&inthread, NULL, thread_read_input, (void *)&args)) {
		perror("pthread_create");
		exit(EXIT_FAILURE);
	}

	/* initial frame */
	sdl_render_frame(input, sdltex, width, height, fg, bg);
	sdl_present_frame(sdlrdr, sdltex);
	/* main loop */
	while (!quit) {
		if (SDL_WaitEvent(&e)) {
			if (e.type == SDL_WINDOWEVENT &&
				e.window.event == SDL_WINDOWEVENT_EXPOSED)
				sdl_present_frame(sdlrdr, sdltex);
			else if (e.type == SDL_QUIT)
				quit = 1;
			else if (e.type == sdl_new_frame) {
				read(pipefd[0], input, fsize);
				sdl_render_frame(input, sdltex, width, height, fg, bg);
				sdl_present_frame(sdlrdr, sdltex);
			}
		}
	}

	SDL_DestroyTexture(sdltex);
	SDL_DestroyRenderer(sdlrdr);
	SDL_DestroyWindow(sdlwin);
	SDL_Quit();

	return 0;
}

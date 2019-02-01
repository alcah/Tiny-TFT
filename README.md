<!-- README.md -- Readme for ttft -->

# Tiny TFT
Tiny TFT is a small (< 250 loc), efficient C program that reads a continuous stream of binary video data from stdin and displays it in real time using SDL.
Tiny TFT uses separate threads to read input and handle SDL output events, ensuring the program is always responsive and uses a minimal amount of processor time, even when receiving a continuous stream of data.

Due to the large nature of video Tiny TFT is only expected to accommodate low-resolution, monochrome video, and so works best with a non-interactive, intermittently updating feed such as the example analogue clock program. It was originally written to accompany a Chip8 interpreter which uses a 64x32 display.

All that said, it's not particularly useful or easy to use. The simple idea of video output being handled by an independent program is more interesting than any of the code contained herein.

# Use
Input video data is expected to be a stream of bytes where each bit represents a pixel (1 being on/white). Bits are read left-to-right, ergo the highest-order (leftmost) bit of each byte corresponds to a pixel left of the lower bits.

When piping to ttft the -x and -y flags must be specified.

ttft currently supports the following options:

- -h: display help
- -x: width of input feed (mandatory)
- -y: height of input feed (mandatory)
- -s: scaling factor for video output (default: 10)
- -f: minimum wait time between frames in ms

# Possible Future Features
- Foreground and background colour options
- Set parameters via an initial message on the input feed
- Other input formats -- 2 bits per pixel would enable a Gameboy-style 4 tone palette.

# Examples
In the `examples` folder you can find a clock application intended to demonstrate ttft's use.

After building with `make clock` it can be run with `examples/clock | ./ttft -x64 -y64` to display an analogue clock that will show the current time and update once a minute.

The code in ttft-clock.c should be taken as a minimal working example only.

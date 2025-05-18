#define _GNU_SOURCE

#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>

// Dependencies
#include "etherdream.h"
#include "tinyosc.h"
#include "daydream_utils.h"

/**
 * Minimum number of points required per frame to maintain stable laser output
 */
#define MIN_POINTS_PER_FRAME 64

/**
 * Maximum number of points that can be stored in a single frame
 */
#define MAX_POINTS_PER_FRAME 16000

/**
 * Size of the buffer used for receiving OSC messages
 */
#define OSC_BUFFER_SIZE 65536

/**
 * Array to store laser points for the current frame
 */
struct etherdream_point points[MAX_POINTS_PER_FRAME];

/**
 * Buffer for receiving OSC messages
 */
char buffer[OSC_BUFFER_SIZE];

/**
 * OSC server configuration
 * osc_port: UDP port to listen for OSC messages (default: 9000)
 * pps: Points per second for laser output (default: 30000)
 * loop_count: Number of times to loop the current frame (1=once, -1=infinite, 0=invalid)
 * wait_for_ready: Whether to wait for DAC to be ready before sending new data
 */
int osc_port        = 9000;
int pps	            = 30000;
int loop_count      = 1;
bool wait_for_ready = true;

/**
 * Global flag to control the main loop
 * Set to false when SIGINT (Ctrl+C) is received
 */
static volatile bool running = true;

/**
 * Signal handler for SIGINT (Ctrl+C)
 * Sets the running flag to false to gracefully exit the program
 *
 * @param signo The signal number (unused)
 */
static void sigint_handler(int signo) {
	(void) signo;
 	running = false;
}

/**
 * Current number of points in the frame
 */
int pcount = 0;

/**
 * Handles incoming OSC messages
 *
 * Supported OSC messages:
 * - /p x y r g b : Add a point with specific coordinates and color
 * - /c r g b : Set the default color for subsequent points
 * - /x x y : Add a point using the default color
 * - /clear : Reset the point count
 * - /set_pps value : Set points per second
 * - /set_loop_count value : Set number of times to loop current frame
 * - /test_pattern : Generate a test circle pattern
 *
 * Example OSC messages:
 * 1. Add a red point at coordinates (1000, 2000):
 *    /p 1000 2000 255 0 0
 * 2. Set default color to white:
 *    /c 255 255 255
 * 3. Add a point using default color:
 *    /x 1500 2500
 * 4. Set points per second to 40000:
 *    /set_pps 40000
 * 5. Set infinite loop:
 *    /set_loop_count -1
 * 6. Clear all points:
 *    /clear
 * 7. Generate test pattern:
 *    /test_pattern
 *
 * @param osc The OSC message to process
 * @param dac Pointer to the EtherDream DAC structure
 */
void handle_osc_message(tosc_message osc){

	char* mess = tosc_getAddress(&osc);

	// "/p" : add an RGB point to the point count.
	if (strcmp(mess, "/p") == 0) {
		struct etherdream_point *pt = &points[pcount];
		pt->x = (int16_t)  tosc_getNextInt32(&osc);
		pt->y = (int16_t)  tosc_getNextInt32(&osc);
		pt->r = byte2word(tosc_getNextInt32(&osc));
		pt->g = byte2word(tosc_getNextInt32(&osc));
		pt->b = byte2word(tosc_getNextInt32(&osc));
		pcount++;

	// "/c" : sets the default color.
	} else if (strcmp(mess, "/c") == 0) {
		current_stroke.r = byte2word(tosc_getNextInt32(&osc));
		current_stroke.g = byte2word(tosc_getNextInt32(&osc));
		current_stroke.b = byte2word(tosc_getNextInt32(&osc));

	// "/x" : add a point by using the default color.
	} else if (strcmp(mess, "/x") == 0) {
		struct etherdream_point *pt = &points[pcount];
		pt->x = (int16_t)  tosc_getNextInt32(&osc);
		pt->y = (int16_t)  tosc_getNextInt32(&osc);
		pt->r = current_stroke.r;
		pt->g = current_stroke.g;
		pt->b = current_stroke.b;
		pcount++;

	// "/clear" : reset the point count.
	} else if (strcmp(mess, "/clear") == 0) {
		pcount = 0;

	// "/set_pps"
	} else if (strcmp(mess, "/set_pps") == 0) {
		pps = tosc_getNextInt32(&osc);
		printf("pps = %d\n", pps);

	// "/set_loop_count"
	} else if (strcmp(mess, "/set_loop_count") == 0) {
		loop_count = tosc_getNextInt32(&osc);
		printf("loop_count = %d\n", loop_count);


	// "/test_pattern"
	} else if (strcmp(mess, "/test_pattern") == 0) {
		pcount = 256;
		circle(points, pcount * 0, pcount, 0, 0, 32768 - 1);
		printf("test pattern\n");
	}
}

/**
 * Main entry point for the laser control program.
 *
 * Sets up OSC server, connects to EtherDream DAC, and processes incoming messages
 */
int main(int argc, char **argv) {

	if (argc > 1) osc_port = atoi(argv[1]);

	// Register the SIGINT handler (Ctrl+C)
	signal(SIGINT, &sigint_handler);

	// Open a socket to listen for datagrams (i.e. UDP packets) on port osc_port
	const int fd = socket(AF_INET, SOCK_DGRAM, 0);
	fcntl(fd, F_SETFL, O_NONBLOCK); // set the socket to non-blocking

	struct sockaddr_in sin;
	sin.sin_family      = AF_INET;
	sin.sin_port        = htons(osc_port);
	sin.sin_addr.s_addr = INADDR_ANY;
	bind(fd, (struct sockaddr *) &sin, sizeof(struct sockaddr_in));

	printf("Listening for OSC on port %d.\n", osc_port);

	// Start the etherdream lib
	etherdream_lib_start();

	// Sleep for a bit over a second, to ensure that we see broadcasts
	// from all available DACs.
	usleep(2000000);

	// No DACs... return.
	int cc = etherdream_dac_count();
	if (!cc) {
		printf("Etherdream DACs not found...\n");
		return 0;
	}

	// Connect to the first DAC.
	for ( int i = 0; i < cc; i++) {
		printf("%d: Ether Dream %06lx\n", i, etherdream_get_id(etherdream_get(i)));
	}
	struct etherdream *dac = etherdream_get(0);

	printf("Connecting to Ether Dream...\n");
	if (etherdream_connect(dac) < 0) {
		return 1;
	}

	current_stroke.r = 65535;
	current_stroke.g = 65535;
	current_stroke.b = 65535;

	// Main loop.
	while (running) {
		// Set up file descriptor set for select() to monitor UDP socket
		fd_set readSet;
		FD_ZERO(&readSet);
		FD_SET(fd, &readSet);
		// Set timeout to 0 for non-blocking operation
		struct timeval timeout = {0, 0};

		// Check if there are any OSC messages waiting to be processed
		if (select(fd+1, &readSet, NULL, NULL, &timeout) > 0) {
     		struct sockaddr sa; // Address of sender (can be cast to sockaddr_in)
     		socklen_t sa_len = sizeof(struct sockaddr_in);

			tosc_message osc;
     		int len = 0;
     		// Process all available messages in the buffer
     		while ((len = (int) recvfrom(fd, buffer, sizeof(buffer), 0, &sa, &sa_len)) > 0) {
				// Handle OSC bundles (messages containing multiple OSC messages)
				if (tosc_isBundle(buffer)) {
					tosc_bundle bundle;
          			tosc_parseBundle(&bundle, buffer, len);
          			// Process each message in the bundle
          			while (tosc_getNextMessage(&bundle, &osc)) {
            			handle_osc_message(osc);
          			}
				// Handle single OSC message
				} else {
					tosc_parseMessage(&osc, buffer, len);
					handle_osc_message(osc);
				}
     		}

     		// Ensure minimum number of points for stable laser output
			if (pcount < MIN_POINTS_PER_FRAME) {
				// Get the last valid point to use as reference
				struct etherdream_point *last = &points[pcount];
				// Fill remaining points with blank (off) points
				for (int i=pcount; i<MIN_POINTS_PER_FRAME; i++){
					struct etherdream_point *pt = &points[i];
					pt->x = last->x;  // Maintain last position
					pt->y = last->y;  // Maintain last position
					pt->r = 0;        // Laser off
					pt->g = 0;        // Laser off
					pt->b = 0;        // Laser off
				}
			}
		}

		// Check if DAC is ready to receive new data
		if (etherdream_is_ready(dac)) {
			// Send point data to DAC if we have points
			if (pcount > 0) {
				// Use actual point count if above minimum, otherwise use minimum
				int c = pcount > MIN_POINTS_PER_FRAME ? pcount : MIN_POINTS_PER_FRAME;
				// Send points to DAC with current PPS and loop settings
				etherdream_write(dac, points, c, pps, loop_count);
			} else {
				// Stop DAC if no points are available
				etherdream_stop(dac);
			}
		}
	}

	etherdream_stop(dac);
	etherdream_disconnect(dac);

	printf("\nBye!\n");
	return 0;
}

#define _GNU_SOURCE

#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>

// Dependencies
#include "etherdream.h"
#include "ws.h"
#include "daydream_utils.h"

/**
 * Minimum number of points required per frame to maintain stable laser output
 */
#define MIN_POINTS_PER_FRAME 200

/**
 * Maximum number of points that can be stored in a single frame
 */
#define MAX_POINTS_PER_FRAME 16000

/**
 * Array to store laser points for the current frame
 */
struct etherdream_point points[MAX_POINTS_PER_FRAME];

/**
 * WebSocket server configuration
 * ws_port: WebSocket port to listen for messages (default: 8080)
 * pps: Points per second for laser output (default: 30000)
 * loop_count: Number of times to loop the current frame (1=once, -1=infinite, 0=invalid)
 * wait_for_ready: Whether to wait for DAC to be ready before sending new data
 */
int ws_port = 8080;
int pps = 30000;
int loop_count = 1;
bool wait_for_ready = true;

/**
 * Current number of points in the frame
 */
int pcount = 0;

/**
 * Whether to enable writing to the DAC
 */
bool write_enabled = false;

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
 * Called when a client connects to the server.
 *
 * @param client Client connection.
 */
void onopen(ws_cli_conn_t client) {
	char *cli, *port;
	cli  = ws_getaddress(client);
	port = ws_getport(client);
	printf("Connection opened, addr: %s, port: %s\n", cli, port);
}

/**
 * Called when a client disconnects from the server.
 *
 * @param client Client connection.
 */
void onclose(ws_cli_conn_t client) {
	char *cli;
	cli = ws_getaddress(client);
	printf("Connection closed, addr: %s\n", cli);
}

/**
 * Parse a command string and execute the corresponding action
 *
 * @param cmd Command string to parse
 * @param dac Pointer to the EtherDream DAC structure
 */
void parse_command(const char *cmd, struct etherdream *dac) {
	// Skip leading whitespace
	while (*cmd == ' ') cmd++;

	// Get command label (first word)
	char label[32];
	int label_len = 0;
	while (*cmd && *cmd != ' ' && label_len < 31) {
		label[label_len++] = *cmd++;
	}
	label[label_len] = '\0';

	// Skip whitespace after label
	while (*cmd == ' ') cmd++;

	if (strcmp(label, "/xyrgb") == 0) {
		// Process bundled points: "/xyrgb x y r g b x y r g b..."
		int x, y, r, g, b;
		const char* ptr = cmd;
		int read = 0;

		// Keep reading sets of 5 values until we run out or reach max points
		while (pcount < MAX_POINTS_PER_FRAME && sscanf(ptr, "%d %d %d %d %d %n", &x, &y, &r, &g, &b, &read) == 5) {
			// Add the point
			struct etherdream_point *pt = &points[pcount];
			pt->x = (int16_t)x;
			pt->y = (int16_t)y;
			pt->r = byte2word(r);
			pt->g = byte2word(g);
			pt->b = byte2word(b);
			pcount++;

			// Move pointer forward to next set
			ptr += read;

			// Skip any whitespace
			while (*ptr == ' ') ptr++;

			// If we've reached the end of the string, break
			if (*ptr == '\0') break;
		}
	}
	else if (strcmp(label, "/xy") == 0) {
		// Process bundled points: "/xy x y x y x y..."
		int x, y;
		const char* ptr = cmd;
		int read = 0;

		// Keep reading sets of 5 values until we run out or reach max points
		while (pcount < MAX_POINTS_PER_FRAME && (sscanf(ptr, "%d %d %n", &x, &y, &read)) == 2) {
			// Add the point
			struct etherdream_point *pt = &points[pcount];
			pt->x = (int16_t)x;
			pt->y = (int16_t)y;
			pt->r = current_stroke.r;
			pt->g = current_stroke.g;
			pt->b = current_stroke.b;
			pcount++;

			// Move pointer forward to next set
			ptr += read;

			// Skip any whitespace
			while (*ptr == ' ') ptr++;

			// If we've reached the end of the string, break
			if (*ptr == '\0') break;
		}
	}
	else if (strcmp(label, "/color") == 0) {
		int r, g, b;
		if (sscanf(cmd, "%d %d %d", &r, &g, &b) == 3) {
			current_stroke.r = byte2word(r);
			current_stroke.g = byte2word(g);
			current_stroke.b = byte2word(b);
			printf("current_stroke = %d %d %d\n", current_stroke.r, current_stroke.g, current_stroke.b);
		}
	}
	else if (strcmp(label, "/clear") == 0) {
		pcount = 0;
	}
	else if (strcmp(label, "/write") == 0) {
		write_enabled = true;
		printf("sending points: %d\n", pcount);
	}
	else if (strcmp(label, "/set_pps") == 0) {
		int new_pps;
		if (sscanf(cmd, "%d", &new_pps) == 1) {
			pps = new_pps;
			printf("pps = %d\n", pps);
		}
	}
	else if (strcmp(label, "/set_loop_count") == 0) {
		if (sscanf(cmd, "%d", &loop_count) == 1) {
			printf("loop_count = %d\n", loop_count);
		}
	}
	else if (strcmp(label, "/test_pattern") == 0) {
		pcount = 256;
		circle(points, 0, pcount, 0, 0, 32768 - 1);
	}
}

/**
 * Called when a message is received from a client.
 *
 * @param client Client connection.
 * @param msg Received message.
 * @param size Message size in bytes.
 * @param type Message type.
 */
void onmessage(ws_cli_conn_t client, const unsigned char *msg, uint64_t size, int type) {
	// char *cli;
	// cli = ws_getaddress(client);
	// printf("Received message: %.*s (size: %" PRId64 ", type: %d), from: %s\n", (int)size, msg, size, type, cli);

	// Parse and execute the command
	parse_command((char *)msg, NULL); // We'll need to pass the DAC pointer somehow
}


/**
 * Main entry point for the laser control program.
 *
 * Sets up WebSocket server, connects to EtherDream DAC, and processes incoming messages
 */
int main(int argc, char **argv) {
	if (argc > 1) ws_port = atoi(argv[1]);

	// Register the SIGINT handler (Ctrl+C)
	signal(SIGINT, &sigint_handler);

	printf("Starting WebSocket server on port %d.\n", ws_port);

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
	for (int i = 0; i < cc; i++) {
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

	// Start WebSocket server
	ws_socket(&(struct ws_server){
		.host = "0.0.0.0",
		.port = ws_port,
		.thread_loop = 1,  // Allow code to continue after ws_socket()
		.timeout_ms = 1000,
		.evs.onopen = &onopen,
		.evs.onclose = &onclose,
		.evs.onmessage = &onmessage
	});

	// Main loop for sending data to DAC
	while (running) {
		if (write_enabled) {
			// Check if DAC is ready to receive new data
			if (etherdream_is_ready(dac)) {
				// Send point data to DAC if we have points
				if (pcount > 0) {

					// Ensure minimum number of points for stable laser output
					while(pcount < MIN_POINTS_PER_FRAME) {
						// Get the last valid point to use as reference
						struct etherdream_point *last = &points[pcount];
						// Fill remaining points with blank (off) points
						for (int i=pcount; i<MIN_POINTS_PER_FRAME; i++){
							struct etherdream_point *pt = &points[i];
							pt->x = last->x;  // Maintain last position
							pt->y = last->y;  // Maintain last position
							pt->r = 0;        // Laser off
							pt->g = 0;
							pt->b = 0;
						}
						pcount++;
					}

					// Use actual point count if above minimum, otherwise use minimum
					int c = pcount > MIN_POINTS_PER_FRAME ? pcount : MIN_POINTS_PER_FRAME;
					// Send points to DAC with current PPS and loop settings
					etherdream_write(dac, points, c, pps, loop_count);
				} else {
					// Stop DAC if no points are available
					// etherdream_stop(dac);
				}
				pcount = 0;
				write_enabled = false;
			}
		}
	}

	// Cleanup
	etherdream_disconnect(dac);
	return 0;
}

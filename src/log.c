/* librist. Copyright 2019 SipRadius LLC. All right reserved.
 * Author: Kuldeep Singh Dhaka <kuldeep@madresistor.com>
 * Author: Sergio Ammirata, Ph.D. <sergio@ammirata.net>
 */

#include "network.h"
#include <librist.h>
#include "log-private.h"
#include "time-shim.h"
#include "stdio-shim.h"

static int loglevel = RIST_LOG_WARN;

#if defined(__unix__)
static int stats_fd = STDERR_FILENO;
#elif defined(_WIN32)
static int stats_fd = -1;
#else
static int stats_fd = 2;
#endif

static int stats_socket = 0;

void set_loglevel(int level)
{
	loglevel = level;
}

int rist_set_stats_fd(int fd)
{
	if (fd > -1) {
		stats_fd = fd;
		fprintf(stderr, "Statistic custom file handle set, #%d\n", stats_fd);
	}

	return 0;
}

int rist_set_stats_socket(int port)
{
	if (!port) {
		fprintf(stderr, "Invalid Statistic socket port %d requested\n", port);
		return -1;
	}

	if (!stats_socket) {
		stats_socket = udp_Connect("127.0.0.1", port, -1, 0, NULL);
		fprintf(stderr, "Statistic socket created on port %d (#%d)\n", port, stats_socket);
	} else {
		fprintf(stderr, "Sorry, statistic socket was already created on port %d (#%d)\n", port, stats_socket);
	}

	return 0;
}

void msg(intptr_t server_ctx, intptr_t client_ctx, int level, const char *format, ...)
{
	struct timeval tv;
	char *str_content;
	char *str_udp;

#ifdef _WIN32
	if (stats_fd == -1) {
		stats_fd = _fileno(stderr);
	}
#endif
	if (level > loglevel) {
		return;
	}

	gettimeofday(&tv, NULL);

	va_list args;
	va_start(args, format);
	vasprintf(&str_content, format, args);
	va_end(args);
	int udplen = asprintf(&str_udp, "%d.%6.6d|%ld.%ld|%d|%s", (int)tv.tv_sec,
		(int)tv.tv_usec, server_ctx, client_ctx, level, str_content);

	write(stats_fd, str_udp, udplen + 1);
	if (stats_socket > 0) {
		udp_Write(stats_socket, str_udp, udplen);
	}

	free(str_udp);
	free(str_content);
}
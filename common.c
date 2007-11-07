/*
 * dhcpcd - DHCP client daemon
 * Copyright 2006-2007 Roy Marples <roy@marples.name>
 * 
 * Distributed under the terms of the GNU General Public License v2
 */

#ifdef __linux__
# define _XOPEN_SOURCE 500 /* needed for pwrite */
#endif

#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "logger.h"

/* OK, this should be in dhcpcd.c
 * It's here to make dhcpcd more readable */
#ifdef __linux__
void srandomdev (void)
{
	int fd;
	unsigned long seed;

	fd = open ("/dev/urandom", 0);
	if (fd == -1 || read (fd,  &seed, sizeof (seed)) == -1) {
		logger (LOG_WARNING, "Could not load seed from /dev/urandom: %s",
				strerror (errno));
		seed = time (0);
	}
	if (fd >= 0)
		close(fd);

	srandom (seed);
}
#endif

/* strlcpy is nice, shame glibc does not define it */
#ifdef __GLIBC__
#  if ! defined (__UCLIBC__) && ! defined (__dietlibc__)
size_t strlcpy (char *dst, const char *src, size_t size)
{
	const char *s = src;
	size_t n = size;

	if (n && --n)
		do {
			if (! (*dst++ = *src++))
				break;
		} while (--n);

	if (! n) {
		if (size)
			*dst = '\0';
		while (*src++);
	}

	return (src - s - 1);
}
#  endif
#endif

/* Close our fd's */
void close_fds (void)
{
	int fd;

	if ((fd = open ("/dev/null", O_RDWR)) == -1) {
		logger (LOG_ERR, "open `/dev/null': %s", strerror (errno));
		return;
	}

	dup2 (fd, fileno (stdin));
	dup2 (fd, fileno (stdout));
	dup2 (fd, fileno (stderr));
	if (fd > 2)
		close (fd);
}

/* Handy function to get the time.
 * We only care about time advancements, not the actual time itself
 * Which is why we use CLOCK_MONOTONIC, but it is not available on all
 * platforms.
 */
int get_time (struct timeval *tp)
{
#if defined(_POSIX_MONOTONIC_CLOCK) && defined(CLOCK_MONOTONIC)
	struct timespec ts;
	static clockid_t posix_clock;
	static int posix_clock_set = 0;

	if (! posix_clock_set) {
		if (sysconf (_SC_MONOTONIC_CLOCK) >= 0)
			posix_clock = CLOCK_MONOTONIC;
		else
			posix_clock = CLOCK_REALTIME;
		posix_clock_set = 1;
	}

	if (clock_gettime (posix_clock, &ts) == -1) {
		logger (LOG_ERR, "clock_gettime: %s", strerror (errno));
		return (-1);
	}

	tp->tv_sec = ts.tv_sec;
	tp->tv_usec = ts.tv_nsec / 1000;
	return (0);
#else
	if (gettimeofday (tp, NULL) == -1) {
		logger (LOG_ERR, "gettimeofday: %s", strerror (errno));
		return (-1);
	}
	return (0);
#endif
}

time_t uptime (void)
{
	struct timeval tp;

	if (get_time (&tp) == -1)
		return (-1);

	return (tp.tv_sec);
}

void writepid (int fd, pid_t pid)
{
	char spid[16];
	if (ftruncate (fd, 0) == -1) {
		logger (LOG_ERR, "ftruncate: %s", strerror (errno));
	} else {
		snprintf (spid, sizeof (spid), "%u", pid);
		if (pwrite (fd, spid, strlen (spid), 0) != (ssize_t) strlen (spid))
			logger (LOG_ERR, "pwrite: %s", strerror (errno));
	}
}

void *xmalloc (size_t s)
{
	void *value = malloc (s);

	if (value)
		return (value);

	logger (LOG_ERR, "memory exhausted");
	exit (EXIT_FAILURE);
}

char *xstrdup (const char *str)
{
	char *value;

	if (! str)
		return (NULL);

	if ((value = strdup (str)))
		return (value);

	logger (LOG_ERR, "memory exhausted");
	exit (EXIT_FAILURE);
}


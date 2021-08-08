/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

char *argv0;

static void
verr(const char *fmt, va_list ap)
{
	if (argv0 && strncmp(fmt, "usage", sizeof("usage") - 1)) {
		fprintf(stderr, "%s: ", argv0);
	}

	vfprintf(stderr, fmt, ap);

	if (fmt[0] && fmt[strlen(fmt) - 1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}
}

void
warn(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	verr(fmt, ap);
	va_end(ap);
}

void
die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	verr(fmt, ap);
	va_end(ap);

	exit(1);
}

static int
evsnprintf(char *str, size_t size, const char *fmt, va_list ap)
{
	int ret;

	ret = vsnprintf(str, size, fmt, ap);

	if (ret < 0) {
		warn("vsnprintf:");
		return -1;
	} else if ((size_t)ret >= size) {
		warn("vsnprintf: Output truncated");
		return -1;
	}

	return ret;
}

int
esnprintf(char *str, size_t size, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = evsnprintf(str, size, fmt, ap);
	va_end(ap);

	return ret;
}

const char *
bprintf(const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = evsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	return (ret < 0) ? NULL : buf;
}

const char *
fmt_human(uintmax_t num, int base)
{
	double scaled;
	size_t i, prefixlen;
	const char **prefix;
	const char *prefix_1000[] = { "", "k", "M", "G", "T", "P", "E", "Z",
	                              "Y" };
	const char *prefix_1024[] = { "", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei",
	                              "Zi", "Yi" };

	switch (base) {
	case 1000:
		prefix = prefix_1000;
		prefixlen = LEN(prefix_1000);
		break;
	case 1024:
		prefix = prefix_1024;
		prefixlen = LEN(prefix_1024);
		break;
	default:
		warn("fmt_human: Invalid base");
		return NULL;
	}

	scaled = num;
	for (i = 0; i < prefixlen && scaled >= base; i++) {
		scaled /= base;
	}

	return bprintf("%.1f %s", scaled, prefix[i]);
}

int
pscanf(const char *path, const char *fmt, ...)
{
	FILE *fp;
	va_list ap;
	int n;

	if (!(fp = fopen(path, "r"))) {
		warn("fopen '%s':", path);
		return -1;
	}
	va_start(ap, fmt);
	n = vfscanf(fp, fmt, ap);
	va_end(ap);
	fclose(fp);

	return (n == EOF) ? -1 : n;
}

/* status2d */
int
ccToInt(const char* ch, int *ret)
{
	if (ch == NULL) {
		return 1;
	}

	char swap[100];

	if (esnprintf(swap, sizeof(swap),
		   	"%s", ch) == 0) {
		return 1;
	}

	if (sscanf(swap, "%d",
		   	ret) == 0) {
		return 1;
	}

	return 0;
}

void
intToHexColor(int num, char *color)
{
	snprintf(color, 7,
			"%6x", num);
	for (int i = 0; i < 7; i++) {
		if (color[i] == ' ') {
			color[i] = '0';
		}
	}
}

const char *
rgb2hex_c(int r, int g, int b)
{
	return bprintf("%x", ((r << 16) + (g << 8) + b));
}

int
rgb2hex_n(int r, int g, int b)
{
	return ((r << 16) + (g << 8) + b);
}

int
greenToRed(int current, int sep, int max)
{
	int steps = 10;
	int cost = 255/steps;
	int step = current*steps/max;
	int color_cotol = steps-(sep*steps/max);

	int green = 255;
	int red = step * cost * color_cotol; //TODO
	if (red > 255) {
		red = 255;
		green = POSITIVE_OR_ZERO(green - step * cost);
	}

	return rgb2hex_n(red, green, 0);
}

void
printBar(char *bar, int x, int y, int w, int h, int barlen, int backlen, const char* fg_color, const char* bg_color)
{
	if(esnprintf(bar, sizeof(char) * MAX_BAR_LEN,
			"^c#%6s^^r%d,%d,%d,%d^^c#%s^^r%d,%d,%d,%d^^d^^f%d^",
			bg_color,
			x, y, backlen, h,
			fg_color,
			x, y, w ,h,
			barlen) < 48) {
		bar = NULL;
	}
}

void
printDoubleBar(char *dbar, int x, int y, int w, int h, int sx, int sy, int sw, int sh, int barlen, const char *fir_color, const char *sec_color, const char* bg_color0, const char* bg_color1)
{
	char f[MAX_BAR_LEN], s[MAX_BAR_LEN];
	printBar(f,
			x, y, w, h,
			0, barlen, fir_color, bg_color0);
	printBar(s,
			sx, sy, sw, sh,
			barlen, barlen, sec_color, bg_color1);
	
	esnprintf(dbar, sizeof(f) + sizeof(s),
			"%s%s", f, s);
}

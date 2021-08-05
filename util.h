/* See LICENSE file for copyright and license details. */
#include <stdint.h>

/* status2d */
#define RED   16711680 //ff0000
#define GREEN 65280    //00ff00
#define BLUE  255      //0000ff

#define COLOR_MASK    RED+GREEN+BLUE

#define DEFAULT_BG    2236962
#define DEFAULT_BG_C  "222222"
#define DEFAULT_FG    1144746
#define DEFAULT_FG_C  "1177aa"
#define DEFAULT_BBG   12369084
#define DEFAULT_BBG_C "bcbcbc"
#define SPEC_COLOR    9076891
#define SPEC_COLOR_C  "8a809b"
#define DEFAULT_BAR_BG_C "444444"

#define MAX_HEIGHT    19
#define INDENT_HEIGHT 4
#define INDENT_WIDTH  2

#define CENTRED (MAX_HEIGHT - (INDENT_HEIGHT * 2))
#define DEFAULT_BAR_WIDTH 50

#define MAX_BAR_LEN   74

extern char buf[1024];

#define POSITIVE_OR_ZERO(x) (x > 0) ? x : 0
#define TO_255(x) (x <= 255) ? x : 255
#define LEN(x) (sizeof (x) / sizeof *(x))

extern char *argv0;

void warn(const char *, ...);
void die(const char *, ...);

int esnprintf(char *str, size_t size, const char *fmt, ...);
const char *bprintf(const char *fmt, ...);
const char *fmt_human(uintmax_t num, int base);
int pscanf(const char *path, const char *fmt, ...);

/* status2d */
int  *ccToInt(const char* ch);
void intToHexColor(int num, char *color);
int rgb2hex_n(int r, int g, int b);
const char *rgb2hex_c(int r, int g, int b);
int  greenToRed(int current, int sep, int max);
void printBar(char *ret, int x, int y, int w, int h, int barlen, int backlen, const char* fg_color, const char* bg_color);
void printDoubleBar(char *ret, int x, int y, int w, int h, int sx, int sy, int sw, int sh, int barlen, const char *fir_color, const char *sec_color, const char *bg_color0, const char *bg_color1);

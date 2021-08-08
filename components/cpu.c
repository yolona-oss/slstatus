/* See LICENSE file for copyright and license details. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <linux/limits.h>

#include "../util.h"

#if defined(__linux__)
	const char *
	cpu_freq(void)
	{
		uintmax_t freq;

		/* in kHz */
		if (pscanf("/sys/devices/system/cpu/cpu0/cpufreq/"
		           "scaling_cur_freq", "%ju", &freq) != 1) {
			return NULL;
		}

		return fmt_human(freq * 1000, 1000);
	}

	const char *
	cpu_perc(void)
	{
		static long double a[7];
		long double b[7], sum;

		memcpy(b, a, sizeof(b));
		/* cpu user nice system idle iowait irq softirq */
		if (pscanf("/proc/stat", "%*s %Lf %Lf %Lf %Lf %Lf %Lf %Lf",
		           &a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &a[6])
		    != 7) {
			return NULL;
		}
		if (b[0] == 0) {
			return NULL;
		}

		sum = (b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6]) -
		      (a[0] + a[1] + a[2] + a[3] + a[4] + a[5] + a[6]);

		if (sum == 0) {
			return NULL;
		}

		return bprintf("%d", (int)(100 *
				((b[0] + b[1] + b[2] + b[5] + b[6]) -
				(a[0] + a[1] + a[2] + a[5] + a[6])) / sum));
	}

	const char *
	cpu_char(void)
	{	
		int perc;
		
		int rc = ccToInt(cpu_perc(), &perc);
		if (rc != 0) {
			return NULL;
		}

		char *ret = NULL;
		if (perc < 50) {
			ret = "low";
		} else if (perc >= 50 && perc < 80) {
			ret = "med";
		} else if (perc >= 80) {
			ret = "high";
		}

		return ret;
	}

	const char *
	cpu_status2d(const char *path)
	{
		char tpath[PATH_MAX];
		int perc;
		int crit_temp, cur_temp, hitemp;

		int rc = ccToInt(cpu_perc(), &perc);

		if(rc != 0) {
			return NULL;
		}

		/* getting cpu temperature */
		esnprintf(tpath, sizeof(tpath),
				"%s_input", path);
		if (pscanf(tpath, "%d", &cur_temp) != 1) {
			return NULL;
		}
		esnprintf(tpath, sizeof(tpath),
				"%s_max", path);
		if (pscanf(tpath, "%d", &crit_temp) != 1) {
			return NULL;
		}

		cur_temp  /= 1000;
		crit_temp /= 1000;
		hitemp     = crit_temp - 10;
		
		int barlen;
		int x, y, cw, tw, ch, th;

		barlen = DEFAULT_BAR_WIDTH;
		x = INDENT_WIDTH;
		y = INDENT_HEIGHT;
		th = CENTRED / 4;
		ch = CENTRED - th;
		cw = barlen * perc / 100;
		tw = barlen * cur_temp / crit_temp;

		char cpucol[7] = DEFAULT_FG_C;
	   	char tempcol[7];

		int grad_color = greenToRed(cur_temp, hitemp, crit_temp);
		intToHexColor(grad_color, tempcol);

		char cpu[MAX_BAR_LEN * 2];
		printDoubleBar(cpu,
						x, y, cw, ch,
						x, ch+y, tw, th,
						barlen,
						cpucol, tempcol, DEFAULT_BAR_BG_C, DEFAULT_BG_C);

		return bprintf("%s", cpu);
	}

#elif defined(__OpenBSD__)
	#include <sys/param.h>
	#include <sys/sched.h>
	#include <sys/sysctl.h>

	const char *
	cpu_freq(void)
	{
		int freq, mib[2];
		size_t size;

		mib[0] = CTL_HW;
		mib[1] = HW_CPUSPEED;

		size = sizeof(freq);

		/* in MHz */
		if (sysctl(mib, 2, &freq, &size, NULL, 0) < 0) {
			warn("sysctl 'HW_CPUSPEED':");
			return NULL;
		}

		return fmt_human(freq * 1E6, 1000);
	}

	const char *
	cpu_perc(void)
	{
		int mib[2];
		static uintmax_t a[CPUSTATES];
		uintmax_t b[CPUSTATES], sum;
		size_t size;

		mib[0] = CTL_KERN;
		mib[1] = KERN_CPTIME;

		size = sizeof(a);

		memcpy(b, a, sizeof(b));
		if (sysctl(mib, 2, &a, &size, NULL, 0) < 0) {
			warn("sysctl 'KERN_CPTIME':");
			return NULL;
		}
		if (b[0] == 0) {
			return NULL;
		}

		sum = (a[CP_USER] + a[CP_NICE] + a[CP_SYS] + a[CP_INTR] + a[CP_IDLE]) -
		      (b[CP_USER] + b[CP_NICE] + b[CP_SYS] + b[CP_INTR] + b[CP_IDLE]);

		if (sum == 0) {
			return NULL;
		}

		return bprintf("%d", 100 *
		               ((a[CP_USER] + a[CP_NICE] + a[CP_SYS] +
		                 a[CP_INTR]) -
		                (b[CP_USER] + b[CP_NICE] + b[CP_SYS] +
		                 b[CP_INTR])) / sum);
	}
#elif defined(__FreeBSD__)
	#include <sys/param.h>
	#include <sys/sysctl.h>
	#include <devstat.h>

	const char *
	cpu_freq(void)
	{
		int freq;
		size_t size;

		size = sizeof(freq);
		/* in MHz */
		if (sysctlbyname("hw.clockrate", &freq, &size, NULL, 0) == -1
				|| !size) {
			warn("sysctlbyname 'hw.clockrate':");
			return NULL;
		}

		return fmt_human(freq * 1E6, 1000);
	}

	const char *
	cpu_perc(void)
	{
		size_t size;
		static long a[CPUSTATES];
		long b[CPUSTATES], sum;

		size = sizeof(a);
		memcpy(b, a, sizeof(b));
		if (sysctlbyname("kern.cp_time", &a, &size, NULL, 0) == -1
				|| !size) {
			warn("sysctlbyname 'kern.cp_time':");
			return NULL;
		}
		if (b[0] == 0) {
			return NULL;
		}

		sum = (a[CP_USER] + a[CP_NICE] + a[CP_SYS] + a[CP_INTR] + a[CP_IDLE]) -
		      (b[CP_USER] + b[CP_NICE] + b[CP_SYS] + b[CP_INTR] + b[CP_IDLE]);

		if (sum == 0) {
			return NULL;
		}

		return bprintf("%d", 100 *
		               ((a[CP_USER] + a[CP_NICE] + a[CP_SYS] +
		                 a[CP_INTR]) -
		                (b[CP_USER] + b[CP_NICE] + b[CP_SYS] +
		                 b[CP_INTR])) / sum);
	}
#endif

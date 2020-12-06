/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>

#include "../util.h"
/* #include "../slstatus.h" */

#if defined(__linux__)
	#include <stdint.h>

	const char *
	ram_free(void)
	{
		uintmax_t free;

		if (pscanf("/proc/meminfo",
		           "MemTotal: %ju kB\n"
		           "MemFree: %ju kB\n"
		           "MemAvailable: %ju kB\n",
		           &free, &free, &free) != 3) {
			return NULL;
		}

		return fmt_human(free * 1024, 1024);
	}

	const char *
	ram_perc(void)
	{
		uintmax_t total, free, buffers, cached;

		if (pscanf("/proc/meminfo",
		           "MemTotal: %ju kB\n"
		           "MemFree: %ju kB\n"
		           "MemAvailable: %ju kB\n"
		           "Buffers: %ju kB\n"
		           "Cached: %ju kB\n",
		           &total, &free, &buffers, &buffers, &cached) != 5) {
			return NULL;
		}

		if (total == 0) {
			return NULL;
		}

		return bprintf("%d", 100 * ((total - free) - (buffers + cached))
                               / total);
	}

	const char *
	ram_total(void)
	{
		uintmax_t total;

		if (pscanf("/proc/meminfo", "MemTotal: %ju kB\n", &total)
		    != 1) {
			return NULL;
		}

		return fmt_human(total * 1024, 1024);
	}

	const char *
	ram_used(void)
	{
		uintmax_t total, free, buffers, cached;

		if (pscanf("/proc/meminfo",
		           "MemTotal: %ju kB\n"
		           "MemFree: %ju kB\n"
		           "MemAvailable: %ju kB\n"
		           "Buffers: %ju kB\n"
		           "Cached: %ju kB\n",
		           &total, &free, &buffers, &buffers, &cached) != 5) {
			return NULL;
		}

		return fmt_human((total - free - buffers - cached) * 1024,
		                 1024);
	}

	static int
	get_swap_info(uintmax_t *s_total, uintmax_t *s_free, uintmax_t *s_cached)
	{
		FILE *fp;
		struct {
			const char *name;
			const size_t len;
			uintmax_t *var;
		} ent[] = {
			{ "SwapTotal",  sizeof("SwapTotal") - 1,  s_total  },
			{ "SwapFree",   sizeof("SwapFree") - 1,   s_free   },
			{ "SwapCached", sizeof("SwapCached") - 1, s_cached },
		};
		size_t line_len = 0, i, left;
		char *line = NULL;

		/* get number of fields we want to extract */
		for (i = 0, left = 0; i < LEN(ent); i++) {
			if (ent[i].var) {
				left++;
			}
		}

		if (!(fp = fopen("/proc/meminfo", "r"))) {
			warn("fopen '/proc/meminfo':");
			return 1;
		}

		/* read file line by line and extract field information */
		while (left > 0 && getline(&line, &line_len, fp) >= 0) {
			for (i = 0; i < LEN(ent); i++) {
				if (ent[i].var &&
				    !strncmp(line, ent[i].name, ent[i].len)) {
					sscanf(line + ent[i].len + 1,
					       "%lu kB\n", ent[i].var);
					left--;
					break;
				}
			}
		}
		free(line);
		if (ferror(fp)) {
			warn("getline '/proc/meminfo':");
			return 1;
		}

		fclose(fp);
		return 0;
	}

	const char *
	ram_status2d(void)
	{
		/* uintmax_t total, free, buffers, cached; */
		uintmax_t stotal, sfree, scached;
		int *ram, swap;

		/* getting swap usage info */
		if (get_swap_info(&stotal, &sfree, &scached)) {
			return NULL;
		}
		swap = (uintmax_t)(100 * (stotal - sfree - scached) / stotal);

		/* getting ram usage info */
		if ((ram = ccToInt(ram_perc())) == NULL) {
			return NULL;
		}

		int barlen;
		int x, y, rw, sw, rh, sh;

		barlen = DEFAULT_BAR_WIDTH;
		x = INDENT_WIDTH;
		y = INDENT_HEIGHT;
		sh = 2;
		rh = CENTRED - sh;
		rw = barlen * *ram / 100;
		sw = barlen * swap / 100;

		char ramcol[7]  = DEFAULT_FG_C; // intToHexColor(DEFAULT_FG, ramcol);
		char swapcol[7] = "55CDFC";     // intToHexColor(5623292, swapcol);

		char mem[MAX_BAR_LEN * 2];
		printDoubleBar(mem,
				x, y, rw, rh,
				x, rh+y, sw, sh,
				barlen,
				ramcol, swapcol, DEFAULT_BG_C);

		return bprintf("%s", mem);
	}
#elif defined(__OpenBSD__)
	#include <stdlib.h>
	#include <sys/sysctl.h>
	#include <sys/types.h>
	#include <unistd.h>

	#define LOG1024 10
	#define pagetok(size, pageshift) (size_t)(size << (pageshift - LOG1024))

	inline int
	load_uvmexp(struct uvmexp *uvmexp)
	{
		int uvmexp_mib[] = {CTL_VM, VM_UVMEXP};
		size_t size;

		size = sizeof(*uvmexp);

		if (sysctl(uvmexp_mib, 2, uvmexp, &size, NULL, 0) >= 0) {
			return 1;
		}

		return 0;
	}

	const char *
	ram_free(void)
	{
		struct uvmexp uvmexp;
		int free_pages;

		if (load_uvmexp(&uvmexp)) {
			free_pages = uvmexp.npages - uvmexp.active;
			return fmt_human(pagetok(free_pages, uvmexp.pageshift) *
			                 1024, 1024);
		}

		return NULL;
	}

	const char *
	ram_perc(void)
	{
		struct uvmexp uvmexp;
		int percent;

		if (load_uvmexp(&uvmexp)) {
			percent = uvmexp.active * 100 / uvmexp.npages;
			return bprintf("%d", percent);
		}

		return NULL;
	}

	const char *
	ram_total(void)
	{
		struct uvmexp uvmexp;

		if (load_uvmexp(&uvmexp)) {
			return fmt_human(pagetok(uvmexp.npages,
			                         uvmexp.pageshift) * 1024,
			                 1024);
		}

		return NULL;
	}

	const char *
	ram_used(void)
	{
		struct uvmexp uvmexp;

		if (load_uvmexp(&uvmexp)) {
			return fmt_human(pagetok(uvmexp.active,
			                         uvmexp.pageshift) * 1024,
			                 1024);
		}

		return NULL;
	}
#elif defined(__FreeBSD__)
	#include <sys/sysctl.h>
	#include <sys/vmmeter.h>
	#include <unistd.h>
	#include <vm/vm_param.h>

	const char *
	ram_free(void) {
		struct vmtotal vm_stats;
		int mib[] = {CTL_VM, VM_TOTAL};
		size_t len;

		len = sizeof(struct vmtotal);
		if (sysctl(mib, 2, &vm_stats, &len, NULL, 0) == -1
				|| !len)
			return NULL;

		return fmt_human(vm_stats.t_free * getpagesize(), 1024);
	}

	const char *
	ram_total(void) {
		long npages;
		size_t len;

		len = sizeof(npages);
		if (sysctlbyname("vm.stats.vm.v_page_count", &npages, &len, NULL, 0) == -1
				|| !len)
			return NULL;

		return fmt_human(npages * getpagesize(), 1024);
	}

	const char *
	ram_perc(void) {
		long npages;
		long active;
		size_t len;

		len = sizeof(npages);
		if (sysctlbyname("vm.stats.vm.v_page_count", &npages, &len, NULL, 0) == -1
				|| !len)
			return NULL;

		if (sysctlbyname("vm.stats.vm.v_active_count", &active, &len, NULL, 0) == -1
				|| !len)
			return NULL;

		return bprintf("%d", active * 100 / npages);
	}

	const char *
	ram_used(void) {
		long active;
		size_t len;

		len = sizeof(active);
		if (sysctlbyname("vm.stats.vm.v_active_count", &active, &len, NULL, 0) == -1
				|| !len)
			return NULL;

		return fmt_human(active * getpagesize(), 1024);
	}
#endif

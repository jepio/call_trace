#include "call_trace.h"

#include <stdlib.h>
#include <stdio.h>
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <demangle.h>

/*
 * Prints a backtrace using libunwind. The printed format is:
 *
 *	#<number> <demangled function name>+<hexadecimal pointer offset>
 *
 */
void show_backtrace(void)
{
	int ctr = 0;
	unw_cursor_t cursor;
	unw_context_t uc;
	unw_word_t ip, sp, offp;

	unw_getcontext(&uc);
	unw_init_local(&cursor, &uc);

	while (unw_step(&cursor) > 0) {
		char fname[256];

		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		unw_get_reg(&cursor, UNW_REG_SP, &sp);
		unw_get_proc_name(&cursor, fname, 256, &offp);
		char *realname = cplus_demangle(fname, DMGL_PARAMS|DMGL_AUTO);
		char *name = realname ? realname : fname;
		printf("#%d %s+%#lx\n", ctr, name, offp);
		free(realname);

		++ctr;
	}
}

/* vim: set noet ts=8 sw=8: */

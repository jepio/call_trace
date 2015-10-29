#include "call_trace.h"

#include <stdlib.h>
#include <stdio.h>
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <demangle.h>

/*
 * Retrieves the full name of the procedure name from cursor using
 * unw_get_proc_name multiple times if necessary. Returns NULL if procedure
 * name can't be retrieved. The returned pointer needs to be freed.
 */
static char *get_full_proc_name(unw_cursor_t *cursor, unw_word_t *offp)
{
	int len = 256;
	char *name = malloc(sizeof(*name) * len);
	int ret = unw_get_proc_name(cursor, name, len, offp);

	if (ret == UNW_ENOMEM) {
		do {
			len *= 1.4;
			name = realloc(name, len);
			ret = unw_get_proc_name(cursor, name, len, offp);
		} while (ret == UNW_ENOMEM);
	} else if (ret == UNW_ENOINFO || ret == UNW_EUNSPEC) {
		perror("unw_get_proc_name");
		free(name);
		name = NULL;
	}
	return name;
}

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
		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		unw_get_reg(&cursor, UNW_REG_SP, &sp);

		char *fname = get_full_proc_name(&cursor, &offp);
		char *realname = cplus_demangle(fname, DMGL_PARAMS | DMGL_AUTO);
		const char *name = realname ? realname : fname;

		printf("#%d %s+%#lx\n", ctr, name, (long)offp);

		++ctr;
		free(realname);
		free(fname);
	}
}

/* vim: set noet ts=8 sw=8: */

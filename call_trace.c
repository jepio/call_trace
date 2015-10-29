#include "call_trace.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#define UNW_LOCAL_ONLY
#include <libunwind.h>

struct sized_ptr {
	char *data;
	int len;
};

static int program_path(struct sized_ptr ret)
{
	assert(ret.data != NULL);
	assert(ret.len > 0);

	int len = readlink("/proc/self/exe", ret.data, ret.len - 1);

	if (len != -1) {
		ret.data[len] = '\0';
		return 0;
	} else {
		return -1;
	}
}

static FILE *open_addr2line(const char *exe_name, unw_word_t addr)
{
	assert(exe_name != NULL);

	char buf[256];
	int ret =
	    snprintf(buf, 256, "addr2line -C -e %s -f -i %lx", exe_name, addr);

	if (ret < 0)
		return NULL;
	return popen(buf, "r");
}

/*
 * fail: returns -1 and leaves *fname unchanged.
 * success: returns strlen(*fname) and the function_name in *fname
 */
static ssize_t parse_funname(FILE *f, char **fname)
{
	assert(f);
	assert(fname);

	char *name = NULL;
	size_t n = 0;
	ssize_t ret = getline(&name, &n, f);

	if (ret == -1) {
		perror("Reading function name");
	} else {
		name[ret - 1] = '\0';
		*fname = name;
	}
	return ret;
}

/*
 * fail: returns -1 and *filename / *line unchanged
 * success: return number of characters read from f, and data in *filename /
 * *line
 */
static ssize_t parse_filename(FILE *f, char *filename, int *line)
{
	assert(f);
	assert(filename);
	assert(line);

	char *buf = NULL;
	size_t n = 0;
	ssize_t ret = getline(&buf, &n, f);

	if (ret == -1) {
		perror("Reading file name and line number");
	} else {
		/* split into two string at ':' */
		char *p = strchr(buf, ':');
		p[0] = '\0';
		++p;
		strcpy(filename, buf);
		*line = strtol(p, NULL, 0);
	}
	free(buf);
	return ret;
}

static void get_stack(unw_word_t addr, char **fname, char *file, int *line)
{
	/* Returns in case of fail */
	*fname = NULL;
	file[0] = '\0';
	*line = 0;

	/* get name of current application */
	char exe_buffer[100];
	struct sized_ptr exe_name = {.data = exe_buffer, .len = 100};

	if (program_path(exe_name) != 0)
		return;

	FILE *f = open_addr2line(exe_name.data, addr);

	if (f == NULL)
		return;

	parse_funname(f, fname);
	parse_filename(f, file, line);

	pclose(f);
}

/*
 * Prints a backtrace using libunwind. The printed format is:
 *
 *	#<number> <demangled function name>+<hexadecimal pointer offset>
 *
 */
void show_backtrace(void)
{
	char file[256];
	char *name;
	int ctr = 0;
	int line = 0;
	unw_cursor_t cursor;
	unw_context_t uc;
	unw_word_t ip, sp;

	unw_getcontext(&uc);
	unw_init_local(&cursor, &uc);

	while (unw_step(&cursor) > 0) {
		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		unw_get_reg(&cursor, UNW_REG_SP, &sp);

		get_stack((long)ip, &name, file, &line);
		printf("#%d %s\n   in %s:%d\n", ctr, name, file, line);
		++ctr;
		free(name);
	}
}

/* vim: set noet ts=8 sw=8: */

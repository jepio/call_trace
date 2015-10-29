#include "call_trace.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#define UNW_LOCAL_ONLY
#include <libunwind.h>

static char *program_path()
{
	const int PATH_MAX = 100;
	char *path = (char *)malloc(PATH_MAX);
	if (path != NULL) {
		int len = readlink("/proc/self/exe", path, PATH_MAX - 1);
		if (len == -1) {
			free(path);
			path = NULL;
		} else {
			path[len] = '\0';
		}
	}
	return path;
}

static void getNameFileAndLine(unw_word_t addr, char **fname, char *file,
			       int *line)
{
	char buf[256];
	char *exe_name = program_path();
	if (!exe_name) {
		exe_name = "./a.out";
	}

	// prepare command to be executed
	int ret = sprintf(buf, "/usr/bin/addr2line -C -e %s -f -i %lx",
			  exe_name, addr);
	free(exe_name);
	assert(ret < 255);
	FILE *f = popen(buf, "r");

	if (f == NULL) {
		perror(buf);
		return;
	}

	// get function name
	int read_size = 256;
	fgets(buf, read_size, f);
	char *name = (char *)malloc(read_size * sizeof(*fname));

	char *endline_pos = strchr(buf, '\n');
	if (endline_pos) {
		*endline_pos = '\0';
		strcpy(name, buf);
	} else {
		int name_len = read_size;
		int name_strlen = 255;
		char *ptr = name;
		*ptr = '\0';
		strncat(ptr, buf, 256);

		do {
			name_len += 256;
			name = (char *)realloc(name, name_len);
			ptr = name + name_strlen;
			fgets(buf, read_size, f);
			strncat(ptr, buf, 256);
			name_strlen += 255;
		} while (!strchr(buf, '\n'));
	}
	*fname = name;

	// get file and line
	fgets(buf, 256, f);
	{
		char *p = buf;
		// file name is until ':'
		while (*p != ':') ++p;

		*(p++) = '\0';
		// after file name follows line number
		strcpy(file, buf);
		sscanf(p, "%d", line);
	}

	pclose(f);
}

void show_backtrace(void)
{
	char file[256];
	int ctr = 0;
	unw_cursor_t cursor;
	unw_context_t uc;
	unw_word_t ip, sp;

	unw_getcontext(&uc);
	unw_init_local(&cursor, &uc);

	while (unw_step(&cursor) > 0) {
		file[0] = '\0';

		int line = 0;

		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		unw_get_reg(&cursor, UNW_REG_SP, &sp);

		char *name[1] = {NULL};
		getNameFileAndLine((long)ip, name, file, &line);
		printf("#%d %s in %s:%d\n", ctr, *name, file, line);
		++ctr;
		free(name[0]);
	}
}

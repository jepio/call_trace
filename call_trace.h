/*
 * This is based on the solution given at
 *
 *     http://blog.bigpixel.ro/2010/09/stack-unwinding-stack-trace-with-gcc/
 *
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void show_backtrace(void);

#ifdef __cplusplus
}
#endif

#ifndef MINGW_WORKAROUND_H
#define MINGW_WORKAROUND_H

#if defined(__MINGW32__) && defined(__cplusplus)

#include <time.h>

#ifndef TIME_UTC
#define TIME_UTC 1
#endif

extern "C" {
void quick_exit(int status) noexcept;
int at_quick_exit(void (*func)(void)) noexcept;
int timespec_get(struct timespec *ts, int base) noexcept;
}

#endif

#endif // MINGW_WORKAROUND_H

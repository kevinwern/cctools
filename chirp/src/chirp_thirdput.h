#ifndef CHIRP_THIRDPUT_H
#define CHIRP_THIRDPUT_H

#include "int_sizes.h"
#include <sys/time.h>

INT64_T chirp_thirdput( const char *subject, const char *lpath, const char *hostname, const char *rpath, time_t stoptime );

#endif

#ifndef __UTILS_H__
#define __UTILS_H__

#include <time.h>
#include <stdint.h>
#include "libavutil/rational.h"

int64_t timespecDiff(struct timespec *timeA_p, struct timespec *timeB_p);
int64_t get_pts(int64_t time, AVRational clock_rate);

#endif /* __UTILS_H__ */

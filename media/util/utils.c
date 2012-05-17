
#include "utils.h"

int64_t
timespecDiff(struct timespec *timeA_p, struct timespec *timeB_p)
{
	return ( ((timeA_p->tv_sec * 1000000000) + timeA_p->tv_nsec) -
		((timeB_p->tv_sec * 1000000000) + timeB_p->tv_nsec) )/1000000;
}

int64_t
get_pts(int64_t time, AVRational clock_rate)
{
	return (time * clock_rate.den) / (clock_rate.num * 1000);
}


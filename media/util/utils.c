
#include "utils.h"

int64_t
get_pts(int64_t time, AVRational clock_rate)
{
	return (time * clock_rate.den) / (clock_rate.num * 1000);
}


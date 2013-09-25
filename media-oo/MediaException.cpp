
#include "MediaException.h"
#include <stdio.h>

using namespace media;

MediaException::MediaException(const char *fmt, ...)
{
	va_list vl;

	va_start(vl, fmt);
	vsnprintf(_message, sizeof(_message), fmt, vl);
	va_end(vl);
}

const char*
MediaException::what() const throw()
{
   return _message;
}

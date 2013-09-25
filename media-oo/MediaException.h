
#ifndef __MEDIA_EXCEPTION_H__
#define __MEDIA_EXCEPTION_H__

#include <exception>
#include <stdarg.h>

using namespace std;

namespace media {
	class MediaException : public exception {
	private:
		char _message[256];

	public:
		MediaException(const char *fmt, ...);

		const char* what() const throw();
	};
}

#endif /* __MEDIA_EXCEPTION_H__ */


#ifndef __MEDIA_H__
#define __MEDIA_H__

extern "C" {
#include <util/log.h>
}

#include "MediaException.h"

namespace media {
	class Media {
	protected:
		const char* LOG_TAG;
	public:
		Media() throw(MediaException);
		~Media();
	};
}

void set_interrrupt_cb(int i);

#endif /* __MEDIA_H__ */

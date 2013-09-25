
#ifndef __MEDIA_PORT_H__
#define __MEDIA_PORT_H__

extern "C" {
#include "libavformat/avformat.h"
}

#include "Media.h"
#include "util/Lock.h"

namespace media {
	class MediaPort : public Media {
	private:
		int _port;
		AVFormatContext *_formatContext;
		URLContext *_urlContext;

		Lock *_mutex;
		int _n_users;

	public:
		MediaPort();
		MediaPort(int port);
		~MediaPort();

		int getPort();
		URLContext* getConnection();
		void closeContext(AVFormatContext *s);
		int close();
	private:
		AVFormatContext* open(const char *rtp);
	};
}

#endif /* __MEDIA_PORT_H__ */

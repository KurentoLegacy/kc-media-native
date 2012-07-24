
#ifndef __MEDIA_RX_H__
#define __MEDIA_RX_H__

#include "Media.h"
#include "util/Lock.h"

extern "C" {
#include "libavformat/avformat.h"
}

namespace media {
	class MediaRx : public Media {
	private:
		bool _receive;

	protected:
		const char *_sdp;
		int _max_delay;

		Lock *_mutex;
		Lock *_freeLock;

	public:
		MediaRx(const char* sdp, int max_delay);
		virtual ~MediaRx();

		virtual int start() = 0;
		virtual int stop() = 0;

	protected:
		bool getReceive();
		void setReceive(bool receive);

		virtual int openFormatContext(AVFormatContext **c);
		virtual void processPacket(AVPacket avpkt, int64_t rx_time) = 0;
	};
}

#endif /* __MEDIA_RX_H__ */

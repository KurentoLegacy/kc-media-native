
#ifndef __MEDIA_RX_H__
#define __MEDIA_RX_H__

#include "Media.h"
#include "util/Lock.h"
#include "MediaPort.h"

extern "C" {
#include "libavformat/avformat.h"
}

namespace media {
	class MediaRx : public Media {
	private:
		bool _receive;
		MediaPort *_mediaPort;
		CodecType _codec_type;

	protected:
		const char *_sdp;
		int _max_delay;

		Lock *_mutex;
		Lock *_freeLock;

		AVFormatContext *_pFormatCtx;
		AVCodecContext *_pDecodecCtx;
		int _stream;

	public:
		MediaRx(MediaPort* mediaPort, const char* sdp, int max_delay,
							CodecType codec_type);
		virtual ~MediaRx();

		int start();
		int stop();

	protected:
		bool getReceive();
		void setReceive(bool receive);
		virtual void processPacket(AVPacket avpkt, int64_t rx_time) = 0;

	private:
		int openFormatContext(AVFormatContext **c);
	};
}

#endif /* __MEDIA_RX_H__ */

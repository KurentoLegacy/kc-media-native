/*
 * (C) Copyright 2013 Kurento (http://kurento.org/)
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */

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
					CodecType codec_type) throw(MediaException);
		virtual ~MediaRx();

		void start() throw(MediaException);
		int stop();

	protected:
		bool getReceive();
		void setReceive(bool receive);
		virtual void processPacket(AVPacket avpkt, int64_t rx_time) = 0;

	private:
		void openFormatContext(AVFormatContext **c) throw(MediaException);
		void release();
	};
}

#endif /* __MEDIA_RX_H__ */

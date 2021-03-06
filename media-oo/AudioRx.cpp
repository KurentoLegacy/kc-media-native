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

#include "AudioRx.h"

extern "C" {
#include "libavformat/avformat.h"
}

enum {
	DATA_SIZE = (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2,
};

using namespace media;

AudioRx::AudioRx(MediaPort* mediaPort, const char* sdp, int max_delay,
						put_audio_samples_rx callback)
: MediaRx(mediaPort, sdp, max_delay, CODEC_TYPE_AUDIO)
{
	_callback = callback;
	LOG_TAG = "media-audio-rx";
}

AudioRx::~AudioRx()
{

}

void
AudioRx::processPacket(AVPacket avpkt, int64_t rx_time)
{
	int out_size, len;
	DecodedAudioSamples decoded_samples, *ds = &decoded_samples;
	uint8_t outbuf[DATA_SIZE];

	//Is this a avpkt from the audio stream?
	if (avpkt.stream_index != _stream)
		return;

	while (avpkt.size > 0) {
		//Decode audio frame
		//FIXME: do not reuse outbuf.
		out_size = DATA_SIZE;
		len = avcodec_decode_audio3(_pDecodecCtx, (int16_t*)outbuf, &out_size, &avpkt);
		if (len < 0) {
			media_log(MEDIA_LOG_ERROR, LOG_TAG, "Error in audio decoding.");
			break;
		}

		if (out_size > 0) {
			ds->samples = outbuf;
			ds->size = out_size;
			ds->time_base = _pFormatCtx->streams[_stream]->time_base;
			ds->pts = avpkt.pts;
			ds->start_time = _pFormatCtx->streams[_stream]->start_time;
			ds->rx_time = rx_time;
			ds->encoded_size = len;
			_callback(ds);
		}

		avpkt.size -= len;
		avpkt.data += len;
	}
}

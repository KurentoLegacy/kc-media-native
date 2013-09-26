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

#ifndef __AUDIO_RX_H__
#define __AUDIO_RX_H__

#include "MediaRx.h"
#include "util/Lock.h"

typedef struct DecodedAudioSamples {
	uint8_t *samples;
	int size;
	AVRational time_base;
	uint64_t pts;
	int64_t start_time;
	int64_t rx_time;
	int encoded_size;
	void *priv_data; /* User private data */
} DecodedAudioSamples;

typedef void (*put_audio_samples_rx)(DecodedAudioSamples* decoded_audio_samples);

namespace media {
	class AudioRx : public MediaRx {
	private:
		put_audio_samples_rx _callback;

	public:
		AudioRx(MediaPort* mediaPort, const char* sdp, int max_delay,
						put_audio_samples_rx callback);
		~AudioRx();

	private:
		void processPacket(AVPacket avpkt, int64_t rx_time);
	};
}

#endif /* __AUDIO_RX_H__ */

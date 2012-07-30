
#ifndef __AUDIO_TX_H__
#define __AUDIO_TX_H__

extern "C" {
#include "libavformat/avformat.h"
}

#include "Media.h"
#include "util/Lock.h"
#include "MediaPort.h"

namespace media {
	class AudioTx : public Media {
	private:
		MediaPort* _mediaPort;

		AVOutputFormat *_fmt;
		AVFormatContext *_oc;
		AVStream *_audio_st;

		uint8_t *_audio_outbuf;
		int _audio_outbuf_size;
		int _frame_size;

		Lock *_mutex;

	public:
		AudioTx(const char* outfile, enum CodecID codec_id,
				int sample_rate, int bit_rate, int payload_type,
				MediaPort* mediaPort) throw(MediaException);
		~AudioTx();
		int putAudioSamplesTx(int16_t* samples, int n_samples,
					int64_t time) throw(MediaException);
		int getFrameSize();
	private:
		AVStream* addAudioStream(AVFormatContext *oc, enum CodecID codec_id,
							int sample_rate, int bit_rate);
		void openAudio() throw(MediaException);
		int writeAudioFrame(AVFormatContext *oc, AVStream *st,
			int16_t *samples, int64_t time) throw(MediaException);
		void release();
	};
}

#endif /* __AUDIO_TX_H__ */

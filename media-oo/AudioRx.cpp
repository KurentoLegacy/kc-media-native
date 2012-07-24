
#include "AudioRx.h"

extern "C" {
#include <socket-manager.h>

#include "libavformat/avformat.h"
}

enum {
	DATA_SIZE = (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2,
};

using namespace media;

AudioRx::AudioRx(const char* sdp, int max_delay, put_audio_samples_rx callback)
: MediaRx(sdp, max_delay)
{
	_callback = callback;
	LOG_TAG = "media-audio-rx";
}

AudioRx::~AudioRx()
{

}

int
AudioRx::start()
{
	AVCodec *pDecodecAudio = NULL;

	AVPacket avpkt;
	uint8_t *avpkt_data_init;

	int i, ret;
	int64_t rx_time;

	_freeLock->lock();
	this->setReceive(true);
	if ((ret = MediaRx::openFormatContext(&pFormatCtx)) < 0)
		goto end;
	media_log(MEDIA_LOG_INFO, LOG_TAG, "max_delay: %d ms", pFormatCtx->max_delay/1000);

	// Find the first audio stream
	_audioStream = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) {
			_audioStream = i;
			break;
		}
	}
	if (_audioStream == -1) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Didn't find a audio stream");
		ret = -4;
		goto end;
	}

	// Get a pointer to the codec context for the audio stream
	pDecodecCtxAudio = pFormatCtx->streams[_audioStream]->codec;

	// Find the decoder for the video stream
	pDecodecAudio = avcodec_find_decoder(pDecodecCtxAudio->codec_id);

	if (pDecodecAudio == NULL) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Unsupported audio codec");
		ret = -5;
		goto end;
	}

	media_log(MEDIA_LOG_INFO, LOG_TAG, "audio codec: %s", pDecodecAudio->name);

	// Open audio codec
	if (avcodec_open(pDecodecCtxAudio, pDecodecAudio) < 0) {
		ret = -6; // Could not open codec
		goto end;
	}

	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "Sample Rate: %d; Frame Size: %d; Bit Rate: %d",
		pDecodecCtxAudio->sample_rate, pDecodecCtxAudio->frame_size,
		pDecodecCtxAudio->bit_rate);

	//READING THE DATA
	for(;;) {
		if (!this->getReceive())
			break;

		if (av_read_frame(pFormatCtx, &avpkt) >= 0) {
			rx_time = av_gettime() / 1000;
			avpkt_data_init = avpkt.data;
			this->processPacket(avpkt, rx_time);
			//Free the packet that was allocated by av_read_frame
			avpkt.data = avpkt_data_init;
			av_free_packet(&avpkt);
		}
	}

	ret = 0;

end:
	if (pDecodecCtxAudio)
		avcodec_close(pDecodecCtxAudio);
	close_context(pFormatCtx);
	_freeLock->unlock();

	return ret;
}

int
AudioRx::stop()
{
	set_interrrupt_cb(1);
	this->setReceive(false);
	_freeLock->lock();
	_freeLock->unlock();

	return 0;
}

void
AudioRx::processPacket(AVPacket avpkt, int64_t rx_time)
{
	int out_size, len;
	DecodedAudioSamples decoded_samples, *ds = &decoded_samples;
	uint8_t outbuf[DATA_SIZE];

	//Is this a avpkt from the audio stream?
	if (avpkt.stream_index == _audioStream) {
		while (avpkt.size > 0) {
			//Decode audio frame
			//FIXME: do not reuse outbuf.
			out_size = DATA_SIZE;
			len = avcodec_decode_audio3(pDecodecCtxAudio, (int16_t*)outbuf, &out_size, &avpkt);
			if (len < 0) {
				media_log(MEDIA_LOG_ERROR, LOG_TAG, "Error in audio decoding.");
				break;
			}

			if (!this->getReceive())
				break;

			if (out_size > 0) {
				ds->samples = outbuf;
				ds->size = out_size;
				ds->time_base = pFormatCtx->streams[_audioStream]->time_base;
				ds->pts = avpkt.pts;
				ds->start_time = pFormatCtx->streams[_audioStream]->start_time;
				ds->rx_time = rx_time;
				ds->encoded_size = len;
				_callback(ds);
			}

			avpkt.size -= len;
			avpkt.data += len;
		}
	}
}

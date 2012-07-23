
#include "AudioRx.h"

extern "C" {
#include <util/log.h>
#include <socket-manager.h>

#include "libavformat/avformat.h"

#include "sdp-manager.h"
}

static char* LOG_TAG = "media-audio-rx";

enum {
	DATA_SIZE = (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2,
};





using namespace media;

AudioRx::AudioRx(const char* sdp, int max_delay, put_audio_samples_rx callback)
: Media()
{
	_sdp = sdp;
	_max_delay = max_delay;
	_callback = callback;
	_mutex = new Lock();
	_freeLock = new Lock();
}

AudioRx::~AudioRx()
{
	delete _mutex;
	delete _freeLock;
}


int
AudioRx::start()
{
	char buf[256];

	AVFormatContext *pFormatCtx = NULL;
	AVFormatParameters params, *ap = &params;
	AVCodecContext *pDecodecCtxAudio = NULL;
	AVCodec *pDecodecAudio = NULL;

	AVPacket avpkt;
	uint8_t *avpkt_data_init;
	uint8_t outbuf[DATA_SIZE];
	DecodedAudioSamples decoded_samples, *ds = &decoded_samples;

	int i, ret, audioStream, out_size, len;
	int64_t rx_time;

	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "sdp: %s", _sdp);

	_freeLock->lock();
//	pthread_mutex_lock(&mutex);
	_mutex->lock();
	_receive = 1;
//	pthread_mutex_unlock(&mutex);
	_mutex->unlock();
	for(;;) {
		_mutex->lock();
//		pthread_mutex_lock(&mutex);
		if (!_receive) {
//			pthread_mutex_unlock(&mutex);
			_mutex->unlock();
			goto end;
		}
//		pthread_mutex_unlock(&mutex);
		_mutex->unlock();

		pFormatCtx = avformat_alloc_context();
		pFormatCtx->max_delay = _max_delay * 1000;
		ap->prealloced_context = 1;

		// Open audio file
		if ( (ret = av_open_input_sdp(&pFormatCtx, _sdp, ap)) != 0 ) {
			av_strerror(ret, buf, sizeof(buf));
			media_log(MEDIA_LOG_ERROR, LOG_TAG, "Couldn't process sdp: %s", buf);
			ret = -2;
			goto end;
		}

		// Retrieve stream information
		if ( (ret = av_find_stream_info(pFormatCtx)) < 0) {
			av_strerror(ret, buf, sizeof(buf));
			media_log(MEDIA_LOG_WARN, LOG_TAG, "Couldn't find stream information: %s", buf);
			close_context(pFormatCtx);
		} else
			break;
	}

	// Find the first audio stream
	audioStream = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) {
			audioStream = i;
			break;
		}
	}
	if (audioStream == -1) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Didn't find a audio stream");
		ret = -4;
		goto end;
	}

	// Get a pointer to the codec context for the audio stream
	pDecodecCtxAudio = pFormatCtx->streams[audioStream]->codec;

	// Find the decoder for the video stream
	if(pDecodecCtxAudio->codec_id == CODEC_ID_AMR_NB) {
		pDecodecAudio = avcodec_find_decoder_by_name("libopencore_amrnb");
	}
	else {
		pDecodecAudio = avcodec_find_decoder(pDecodecCtxAudio->codec_id);
	}

	if (pDecodecAudio == NULL) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Unsupported audio codec");
		ret = -5;
		goto end;
	}

	media_log(MEDIA_LOG_INFO, LOG_TAG, "max_delay: %d ms", pFormatCtx->max_delay/1000);
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
//		pthread_mutex_lock(&mutex);
		_mutex->lock();
		if (!_receive) {
//			pthread_mutex_unlock(&mutex);
			_mutex->unlock();
			break;
		}
//		pthread_mutex_unlock(&mutex);
		_mutex->unlock();

		if (av_read_frame(pFormatCtx, &avpkt) >= 0) {
			rx_time = av_gettime() / 1000;
			avpkt_data_init = avpkt.data;
			//Is this a avpkt from the audio stream?
			if (avpkt.stream_index == audioStream) {
				while (avpkt.size > 0) {
					//Decode audio frame
					//FIXME: do not reuse outbuf.
					out_size = DATA_SIZE;
					len = avcodec_decode_audio3(pDecodecCtxAudio, (int16_t*)outbuf, &out_size, &avpkt);
					if (len < 0) {
						media_log(MEDIA_LOG_ERROR, LOG_TAG, "Error in audio decoding.");
						break;
					}

//					pthread_mutex_lock(&mutex);
					_mutex->lock();
					if (!_receive) {
//						pthread_mutex_unlock(&mutex);
						_mutex->unlock();
						break;
					}
					if (out_size > 0) {
						ds->samples = outbuf;
						ds->size = out_size;
						ds->time_base = pFormatCtx->streams[audioStream]->time_base;
						ds->pts = avpkt.pts;
						ds->start_time = pFormatCtx->streams[audioStream]->start_time;
						ds->rx_time = rx_time;
						ds->encoded_size = len;
						_callback(ds);
					}
//					pthread_mutex_unlock(&mutex);
					_mutex->unlock();

					avpkt.size -= len;
					avpkt.data += len;
				}
			}
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
//	pthread_mutex_lock(&mutex);
	_mutex->lock();
	_receive = 0;
	set_interrrupt_cb(1);
//	pthread_mutex_unlock(&mutex);
	_mutex->unlock();
	_freeLock->lock();
	_freeLock->unlock();

	return 0;
}

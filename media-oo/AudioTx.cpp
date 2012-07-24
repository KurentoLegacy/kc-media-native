
#include "AudioTx.h"

extern "C" {
#include <util/log.h>
#include <util/utils.h>
#include <my-cmdutils.h>
#include <socket-manager.h>

#include "libswscale/swscale.h"
#include "libavcodec/opt.h"
#include "libavformat/rtpenc.h"
#include "libavformat/rtpdec.h"
#include "libavformat/url.h"
}

enum {
	DEFAULT_FRAME_SIZE = 20, //milliseconds
};

using namespace media;
//TODO: methods as synchronized
AudioTx::AudioTx(const char* outfile, enum CodecID codec_id,
				int sample_rate, int bit_rate, int payload_type)
: Media()
{
	int ret;
	URLContext *urlContext;
	RTPMuxContext *rptmc;

	LOG_TAG = "media-audio-tx";

	this->_fmt = av_guess_format(NULL, outfile, NULL);
	if (!_fmt) {
		media_log(MEDIA_LOG_DEBUG, LOG_TAG, "Could not deduce output "
					"format from file extension: using RTP.");
		_fmt = av_guess_format("rtp", NULL, NULL);
	}
	if (!_fmt) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not find suitable "
								"output format");
		ret = -1;
		goto end;
	}
	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "Format established: %s", _fmt->name);
	_fmt->audio_codec = codec_id;


	/* allocate the output media context */
	_oc = avformat_alloc_context();
	if (!_oc) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Memory error: Could not alloc context");
		ret = -2;
		goto end;
	}
	_oc->oformat = _fmt;
	snprintf(_oc->filename, sizeof(_oc->filename), "%s", outfile);

	/* add the audio stream using the default format codecs
	and initialize the codecs */
	_audio_st = NULL;

	if (_fmt->audio_codec != CODEC_ID_NONE) {
		_audio_st = addAudioStream(_oc, _fmt->audio_codec, sample_rate, bit_rate);
	}

	/* set the output parameters (must be done even if no
	parameters). */
	if (av_set_parameters(_oc, NULL) < 0) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Invalid output format parameters");
		ret = -3;
		goto end;
	}

	av_dump_format(_oc, 0, outfile, 1);

	/* now that all the parameters are set, we can open the
	audio codec and allocate the necessary encode buffers */
	if (_audio_st) {
		if((ret = openAudio()) < 0) {
			media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not open audio");
			goto end;
		}
	}

	/* open the output file, if needed */
	if (!(_fmt->flags & AVFMT_NOFILE)) {
		if ((ret = avio_open(&_oc->pb, outfile, URL_WRONLY)) < 0) {
			media_log(MEDIA_LOG_ERROR, LOG_TAG,
				  "Could not open '%s' AVERROR_NOENT:%d", outfile, AVERROR_NOENT);
			goto end;
		}
	}

	//Free old URLContext
	if ((ret=ffurl_close((URLContext*)_oc->pb->opaque)) < 0) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not free URLContext");
		goto end;
	}

	urlContext = (URLContext*)get_audio_connection(0);
	if ((ret = rtp_set_remote_url (urlContext, outfile)) < 0) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not open '%s'", outfile);
		goto end;
	}

	_oc->pb->opaque = urlContext;

	/* write the stream header, if any */
	av_write_header(_oc);

	rptmc = (RTPMuxContext*)_oc->priv_data;
	rptmc->payload_type = payload_type;
	rptmc->max_frames_per_packet = 1;

	media_log(MEDIA_LOG_INFO, LOG_TAG, "Frames per packet: %d", rptmc->max_frames_per_packet);

	if(_audio_st->codec->frame_size > 1)
		_frame_size = _audio_st->codec->frame_size;
	else
		_frame_size = sample_rate * DEFAULT_FRAME_SIZE / 1000;
	ret = _frame_size;
	media_log(MEDIA_LOG_INFO, LOG_TAG, "Audio frame size: %d", _frame_size);

end:
	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "Constructor ret: %d", ret);
	_mutex = new Lock();
	//return ret;
	;
}

AudioTx::~AudioTx()
{
	int i;

	_mutex->lock();
	/* write the trailer, if any.  the trailer must be written
	* before you close the CodecContexts open when you wrote the
	* header; otherwise write_trailer may try to use memory that
	* was freed on av_codec_close() */
	if(_oc) {
		av_write_trailer(_oc);
		/* close codec */
		if (_audio_st) {
			avcodec_close(_audio_st->codec);
			av_free(_audio_outbuf);
		}
		/* free the streams */
		for(i = 0; i < _oc->nb_streams; i++) {
			av_freep(&_oc->streams[i]->codec);
			av_freep(&_oc->streams[i]);
		}
		media_log(MEDIA_LOG_DEBUG, LOG_TAG, "Close the context...");
		close_context(_oc);
		_oc = NULL;
		media_log(MEDIA_LOG_DEBUG, LOG_TAG, "ok");
	}

	_mutex->unlock();
	delete _mutex;
}

int
AudioTx::writeAudioFrame(AVFormatContext *oc, AVStream *st, int16_t *samples, int64_t time)
{
	AVCodecContext *c;
	AVPacket pkt;
	av_init_packet(&pkt);
	int ret;

	c = st->codec;

	pkt.size = avcodec_encode_audio(c, _audio_outbuf, _frame_size, samples);
	pkt.pts = get_pts(time, st->time_base);
	pkt.flags |= AV_PKT_FLAG_KEY;
	pkt.stream_index = st->index;
	pkt.data = _audio_outbuf;

	/* write the compressed frame in the media file */
	ret = av_write_frame(oc, &pkt);
	if (ret != 0) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Error while writing audio frame");
		return ret;
	}

	return pkt.size;
}

int
AudioTx::putAudioSamplesTx(int16_t* samples, int n_samples, int64_t time)
{
	int i, ret, nframes, total_size;

	_mutex->lock();
	total_size = 0;
	if (!_oc) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "No audio initiated.");
		ret = -1;
		goto end;
	}

	nframes = n_samples / _frame_size;
	for (i=0; i<nframes; i++) {
		if( (ret=writeAudioFrame(_oc, _audio_st, &samples[i*_frame_size], time)) < 0) {
			media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not write audio frame");
			break;
		}
		total_size += ret;
	}

	ret = total_size;

end:
	_mutex->unlock();
	return ret;
}

AVStream*
AudioTx::addAudioStream(AVFormatContext *oc, enum CodecID codec_id,
						int sample_rate, int bit_rate)
{
	AVCodecContext *c;
	AVStream *st;

	st = av_new_stream(oc, 1);
	if (!st) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not alloc stream");
		return NULL;
	}

	c = st->codec;
	c->codec_id = codec_id;
	c->codec_type = AVMEDIA_TYPE_AUDIO;

	/* put sample parameters */
	c->bit_rate = bit_rate;//AMR:12200;//MP2:64000
	c->sample_rate = sample_rate;//AMR:8000;//MP2:44100;
	c->channels = 1;
	c->sample_fmt = SAMPLE_FMT_S16;//AV_SAMPLE_FMT_S16;

	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "bit_rate: %d", c->bit_rate);
	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "sample_rate: %d", c->sample_rate);

	// some formats want stream headers to be separate
	if(oc->oformat->flags & AVFMT_GLOBALHEADER)
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;

	return st;
}

int
AudioTx::openAudio()
{
	AVCodec *codec;
	AVCodecContext *c;
	int ret;

	c = _audio_st->codec;

	/* find the audio encoder */
	codec = avcodec_find_encoder(c->codec_id);
	if (!codec) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Codec not found");
		return -1;
	}

	/* open the codec */
	if ((ret = avcodec_open(c, codec)) < 0) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Could not open codec");
		return ret;
	}

	_audio_outbuf_size = 100000;
	_audio_outbuf = (uint8_t*)av_malloc(_audio_outbuf_size);

	return 0;
}

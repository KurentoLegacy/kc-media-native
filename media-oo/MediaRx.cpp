
#include "MediaRx.h"
#include "MediaPortManager.h"

extern "C" {
#include "sdp-manager.h"
}

using namespace media;

MediaRx::MediaRx(MediaPort* mediaPort, const char* sdp, int max_delay,
							CodecType codec_type)
: Media()
{
	_sdp = sdp;
	_mediaPort = mediaPort;
	_max_delay = max_delay;
	_codec_type = codec_type;
	_mutex = new Lock();
	_freeLock = new Lock();
	LOG_TAG = "media-rx";
}

MediaRx::~MediaRx()
{
	delete _mutex;
	delete _freeLock;
}

bool
MediaRx::getReceive()
{
	bool ret;
	_mutex->lock();
	ret = _receive;
	_mutex->unlock();

	return ret;
}

void
MediaRx::setReceive(bool receive)
{
	_mutex->lock();
	_receive = receive;
	_mutex->unlock();
}

int
MediaRx::openFormatContext(AVFormatContext **c)
{
	AVFormatContext *pFormatCtx = NULL;
	AVFormatParameters params, *ap = &params;
	URLContext *urlContext;
	int ret;
	char buf[256];

	media_log(MEDIA_LOG_DEBUG, LOG_TAG, "sdp: %s", _sdp);

	urlContext = _mediaPort->getConnection();

	for(;;) {
		if (!this->getReceive())
			return -1;

		pFormatCtx = avformat_alloc_context();
		pFormatCtx->max_delay = _max_delay * 1000;
		ap->prealloced_context = 1;

		// Open sdp
		if ((ret = av_open_input_sdp(&pFormatCtx, _sdp, ap, urlContext)) != 0 ) {
			av_strerror(ret, buf, sizeof(buf));
			media_log(MEDIA_LOG_ERROR, LOG_TAG, "Couldn't process sdp: %s", buf);
			return ret;
		}

		// Retrieve stream information
		if ((ret = av_find_stream_info(pFormatCtx)) < 0) {
			av_strerror(ret, buf, sizeof(buf));
			media_log(MEDIA_LOG_WARN, LOG_TAG, "Couldn't find stream information: %s", buf);
			_mediaPort->closeContext(pFormatCtx);

		} else
			break;
	}
	*c = pFormatCtx;

	return 0;
}


int
MediaRx::start()
{
	AVCodec *pDecodec = NULL;

	AVPacket avpkt;
	uint8_t *avpkt_data_init;

	int i, ret;
	int64_t rx_time;

	_freeLock->lock();
	this->setReceive(true);
	if ((ret = MediaRx::openFormatContext(&_pFormatCtx)) < 0)
		goto end;
	media_log(MEDIA_LOG_INFO, LOG_TAG, "max_delay: %d ms", _pFormatCtx->max_delay/1000);

	// Find the first stream
	_stream = -1;
	for (i = 0; i < _pFormatCtx->nb_streams; i++) {
		if (_pFormatCtx->streams[i]->codec->codec_type == _codec_type) {
			_stream = i;
			break;
		}
	}
	if (_stream == -1) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Didn't find a stream");
		ret = -4;
		goto end;
	}

	// Get a pointer to the codec context for the stream
	_pDecodecCtx = _pFormatCtx->streams[_stream]->codec;

	// Find the decoder for the stream
	pDecodec = avcodec_find_decoder(_pDecodecCtx->codec_id);
	if (pDecodec == NULL) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "Unsupported codec");
		ret = -5; // Codec not found
		goto end;
	}

	// Open video codec
	if (avcodec_open(_pDecodecCtx, pDecodec) < 0) {
		ret = -6; // Could not open codec
		goto end;
	}

	//READING THE DATA
	for(;;) {
		if (!this->getReceive())
			break;

		if (av_read_frame(_pFormatCtx, &avpkt) >= 0) {
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
	if (_pDecodecCtx)
		avcodec_close(_pDecodecCtx);
	_mediaPort->closeContext(_pFormatCtx);
	MediaPortManager::releaseMediaPort(_mediaPort);
	_freeLock->unlock();

	return ret;
}

int
MediaRx::stop()
{
	set_interrrupt_cb(1);
	this->setReceive(false);
	_freeLock->lock();
	_freeLock->unlock();

	return 0;
}

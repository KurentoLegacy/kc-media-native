
#include "MediaRx.h"
#include "MediaPortManager.h"

extern "C" {
#include "util/sdp-manager.h"
}

using namespace media;

MediaRx::MediaRx(MediaPort* mediaPort, const char* sdp, int max_delay,
				CodecType codec_type) throw(MediaException)
: Media()
{
	LOG_TAG = "media-rx";
	_sdp = sdp;
	_mediaPort = mediaPort;
	_max_delay = max_delay;
	_codec_type = codec_type;

	_pFormatCtx = NULL;
	_pDecodecCtx = NULL;

	_mutex = new Lock();
	_freeLock = new Lock();
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

void
MediaRx::openFormatContext(AVFormatContext **c) throw(MediaException)
{
	AVFormatContext *pFormatCtx = NULL;
	AVFormatParameters params, *ap = &params;
	URLContext *urlContext;
	int ret;
	char buf[256];

	media_log(MEDIA_LOG_INFO, LOG_TAG, "sdp: %s", _sdp);

	urlContext = _mediaPort->getConnection();

	while(this->getReceive()) {
		pFormatCtx = avformat_alloc_context();
		pFormatCtx->max_delay = _max_delay * 1000;
		ap->prealloced_context = 1;

		// Open sdp
		ret = av_open_input_sdp(&pFormatCtx, _sdp, ap, urlContext);
		if (ret != 0 ) {
			av_strerror(ret, buf, sizeof(buf));
			throw MediaException("Couldn't process sdp: %s", buf);
		}

		// Retrieve stream information
		if ((ret = av_find_stream_info(pFormatCtx)) < 0) {
			av_strerror(ret, buf, sizeof(buf));
			media_log(MEDIA_LOG_WARN, LOG_TAG,
				"Couldn't find stream information: %s", buf);
			_mediaPort->closeContext(pFormatCtx);
			pFormatCtx = NULL;
		} else
			break;
	}
	*c = pFormatCtx;
}

void
MediaRx::release()
{
	if (_pDecodecCtx)
		avcodec_close(_pDecodecCtx);
	_mediaPort->closeContext(_pFormatCtx);
	MediaPortManager::releaseMediaPort(_mediaPort);
}

void
MediaRx::start() throw(MediaException)
{
	AVCodec *pDecodec = NULL;

	AVPacket avpkt;
	uint8_t *avpkt_data_init;

	int i;
	int64_t rx_time;

	_freeLock->lock();
	try {
		this->setReceive(true);
		openFormatContext(&_pFormatCtx);

		if (!_pFormatCtx)
			throw MediaException("Could not open context");

		// Find the first stream
		_stream = -1;
		for (i = 0; i < _pFormatCtx->nb_streams; i++) {
			if (_pFormatCtx->streams[i]->codec->codec_type == _codec_type) {
				_stream = i;
				break;
			}
		}
		if (_stream == -1)
			throw MediaException("Didn't find a stream");

		// Get a pointer to the codec context for the stream
		_pDecodecCtx = _pFormatCtx->streams[_stream]->codec;

		// Find the decoder for the stream
		pDecodec = avcodec_find_decoder(_pDecodecCtx->codec_id);
		if (pDecodec == NULL)
			throw MediaException("Unsupported codec");

		// Open video codec
		if (avcodec_open(_pDecodecCtx, pDecodec) < 0)
			throw MediaException("Could not open codec");

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
	}
	catch(MediaException &e) {
		media_log(MEDIA_LOG_ERROR, LOG_TAG, "%s", e.what());
		release();
		_freeLock->unlock();
		throw;
	}

	release();
	_freeLock->unlock();
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

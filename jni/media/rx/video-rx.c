/*
 * Kurento Android Media: Android Media Library based on FFmpeg.
 * Copyright (C) 2011  Tikal Technologies
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * 
 * @author Miguel París Díaz
 * 
 */

#include <init-media.h>
#include <socket-manager.h>

#include <jni.h>
#include <pthread.h>
#include <android/log.h>

#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

#include "sdp-manager.h"

#include <utils.h>

static char buf[256]; //Log
static char* LOG_TAG = "NDK-video-rx";

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int receive = 0;
static int sws_flags = SWS_BICUBIC;

enum {
	JINT_SIZE = 4, // size in bytes
};

enum {
	QUEUE_SIZE = 1, // FIXME: Coupled with VideoRecorderComponent
};

typedef struct DecodedFrame{
	AVFrame* pFrameRGB;
	jintArray out_buffer_video;
	uint8_t *buffer;
} DecodedFrame;


jint
Java_com_kurento_kas_media_rx_MediaRx_stopVideoRx(JNIEnv* env,
				jobject thiz)
{
	pthread_mutex_lock(&mutex);
	receive = 0;
	pthread_mutex_unlock(&mutex);
	return 0;
}


jint
Java_com_kurento_kas_media_rx_MediaRx_startVideoRx(JNIEnv* env, jobject thiz,
				jstring sdp_str, jint maxDelay, jobject videoPlayer)
{
	
	const char *pSdpString = NULL;

	AVFormatContext *pFormatCtx = NULL;
	AVFormatParameters params, *ap = &params;
	AVCodecContext *pDecodecCtxVideo = NULL;
	AVCodec *pDecodecVideo = NULL;
	AVFrame *pFrame = NULL;
	DecodedFrame decoded_frames[QUEUE_SIZE+1];

	AVPacket avpkt;
	uint8_t *avpkt_data_init;

	int i, j, ret, videoStream, buffer_nbytes, picture_nbytes, len, got_picture;
	int current_width, current_height;
	int n_frame;
	
	struct SwsContext *img_convert_ctx;

	struct timespec start, end, t1, t2;
	uint64_t time;
	uint64_t total_time = 0;


	pSdpString = (*env)->GetStringUTFChars(env, sdp_str, NULL);
	if (pSdpString == NULL) {
		ret = -1; // OutOfMemoryError already thrown
		goto end;
	}

	snprintf(buf, sizeof(buf), "pSdpString: %s", pSdpString);
	__android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, buf);

	if( (ret= init_media()) != 0) {
		__android_log_write(ANDROID_LOG_ERROR, LOG_TAG, "Couldn't init media");
		goto end;
	}

	pthread_mutex_lock(&mutex);
	receive = 1;
	pthread_mutex_unlock(&mutex);
	for(;;) {
		pthread_mutex_lock(&mutex);
		if (!receive) {
			pthread_mutex_unlock(&mutex);
			goto end;
		}
		pthread_mutex_unlock(&mutex);

		pFormatCtx = avformat_alloc_context();
		pFormatCtx->max_delay = maxDelay * 1000;
		ap->prealloced_context = 1;

		// Open video file
		if ( (ret = av_open_input_sdp(&pFormatCtx, pSdpString, ap)) != 0 ) {
			av_strerror(ret, buf, sizeof(buf));
			snprintf(buf, sizeof(buf), "%s: Couldn't process sdp.", buf);
			__android_log_write(ANDROID_LOG_ERROR, LOG_TAG, buf);
			ret = -2;
			goto end;
		}

		// Retrieve stream information
		if ( (ret = av_find_stream_info(pFormatCtx)) < 0) {
			av_strerror(ret, buf, sizeof(buf));
			snprintf(buf, sizeof(buf), "%s: Couldn't find stream information.", buf);
			__android_log_write(ANDROID_LOG_WARN, LOG_TAG, buf);
			close_context(pFormatCtx);
		} else
			break;
	}

	snprintf(buf, sizeof(buf), "max_delay: %d ms", pFormatCtx->max_delay/1000);
	__android_log_write(ANDROID_LOG_INFO, LOG_TAG, buf);

	// Find the first video stream
	videoStream = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	if (videoStream == -1) {
		__android_log_write(ANDROID_LOG_ERROR, LOG_TAG, "Didn't find a video stream");
		ret = -4;
		goto end;
	}

	// Get a pointer to the codec context for the video stream
	pDecodecCtxVideo = pFormatCtx->streams[videoStream]->codec;

	// Find the decoder for the video stream
	pDecodecVideo = avcodec_find_decoder(pDecodecCtxVideo->codec_id);
	if (pDecodecVideo == NULL) {
		__android_log_write(ANDROID_LOG_ERROR, LOG_TAG, "Unsupported codec!");
		ret = -5; // Codec not found
		goto end;
	}
	
	// Open video codec
	if (avcodec_open(pDecodecCtxVideo, pDecodecVideo) < 0) {
		ret = -6; // Could not open codec
		goto end;
	}

	//STORING THE DATA
	//Allocate video frame
	pFrame = avcodec_alloc_frame();

	//Allocate AVFrames structures
	for (i=0; i<QUEUE_SIZE+1; i++) {
		decoded_frames[i].pFrameRGB = avcodec_alloc_frame();
		if (decoded_frames[j].pFrameRGB == NULL) {
			ret = -7;
			goto end;
		}
		decoded_frames[i].out_buffer_video = NULL;
		decoded_frames[i].buffer = NULL;
	}

	//Prepare Call to Method Java.
	jclass cls = (*env)->GetObjectClass(env, videoPlayer);
	
	jmethodID midVideo = (*env)->GetMethodID(env, cls, "putVideoFrameRx", "([IIII)V");
	if (midVideo == 0) {
		__android_log_write(ANDROID_LOG_ERROR, LOG_TAG, "putVideoFrameRx([IIII)V no exist!");
		ret = -8;
		goto end;
	}

	picture_nbytes = 0;

	current_width = -1;
	current_height = -1;
	buffer_nbytes = -1;

i = 0;
int n_packet = 0;

	//READING THE DATA
	for(;;) {
		pthread_mutex_lock(&mutex);
		if (!receive) {
			pthread_mutex_unlock(&mutex);
			break;
		}
		pthread_mutex_unlock(&mutex);
		if (av_read_frame(pFormatCtx, &avpkt) >= 0) {
			avpkt_data_init = avpkt.data;
			//Is this a avpkt from the video stream?
			if (avpkt.stream_index == videoStream) {
snprintf(buf, sizeof(buf), "%d -------------------", n_packet++);
__android_log_write(ANDROID_LOG_INFO, LOG_TAG, buf);
snprintf(buf, sizeof(buf), "avpkt->pts: %lld", avpkt.pts);
__android_log_write(ANDROID_LOG_INFO, LOG_TAG, buf);
snprintf(buf, sizeof(buf), "avpkt->dts: %lld", avpkt.dts);
__android_log_write(ANDROID_LOG_INFO, LOG_TAG, buf);
snprintf(buf, sizeof(buf), "avpkt->size: %d", avpkt.size);
__android_log_write(ANDROID_LOG_INFO, LOG_TAG, buf);
				while (avpkt.size > 0) {
					n_frame = i % (QUEUE_SIZE+1);
clock_gettime(CLOCK_MONOTONIC, &start);
					//Decode video frame
					len = avcodec_decode_video2(pDecodecCtxVideo, pFrame, &got_picture, &avpkt);
clock_gettime(CLOCK_MONOTONIC, &t2);
time = timespecDiff(&t2, &start);
snprintf(buf, sizeof(buf), "decode time: %llu ms", time);
__android_log_write(ANDROID_LOG_INFO, LOG_TAG, buf);
					if (len < 0) {
						__android_log_write(ANDROID_LOG_ERROR, LOG_TAG, "Error in video decoding.");
						break;
					}
					pthread_mutex_lock(&mutex);
					if (!receive) {
						pthread_mutex_unlock(&mutex);
						break;
					}
					//Did we get a video frame?
					if (got_picture) {
						if ( (buffer_nbytes < 0) || (current_width != pDecodecCtxVideo->width) || (current_height != pDecodecCtxVideo->height) ) {
							current_width = pDecodecCtxVideo->width;
							current_height = pDecodecCtxVideo->height;
							// Determine required picture size
							picture_nbytes = avpicture_get_size(PIX_FMT_RGB32, current_width, current_height);
							if (picture_nbytes > buffer_nbytes) {
								for (j=0; j<QUEUE_SIZE+1; j++) {
									(*env)->DeleteLocalRef(env, decoded_frames[j].out_buffer_video);
									decoded_frames[j].out_buffer_video = (jintArray)(*env)->NewIntArray(env, picture_nbytes/JINT_SIZE);
									decoded_frames[j].buffer = (*env)->GetIntArrayElements(env, decoded_frames[j].out_buffer_video, NULL);
									(*env)->ReleaseIntArrayElements(env, decoded_frames[j].out_buffer_video, (jint*)(decoded_frames[j].buffer), 0);
								}
								buffer_nbytes = picture_nbytes;
							}
							//Assign appropriate parts of buffer to image planes in pFrameRGB
							//Note that pFrameRGB is an AVFrame, but AVFrame is a superset of AVPicture
							for (j=0; j<QUEUE_SIZE+1; j++) {
								avpicture_fill((AVPicture*) decoded_frames[j].pFrameRGB, decoded_frames[j].buffer, PIX_FMT_RGB32, current_width, current_height);
							}
							snprintf(buf, sizeof(buf), "current_width: %d\tcurrent_height: %d\tpicture_nbytes: %d", current_width, current_height, picture_nbytes);
							__android_log_write(ANDROID_LOG_INFO, LOG_TAG, buf);
						}
						//Convert the image from its native format to RGB
						img_convert_ctx = sws_getContext(current_width,
								current_height, pDecodecCtxVideo->pix_fmt,
								current_width, current_height, PIX_FMT_RGB32,
								sws_flags, NULL, NULL, NULL);
						if (img_convert_ctx == NULL) {
							ret = -9;
							goto end;
						}
						sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0,
								current_height, ((AVPicture*) decoded_frames[n_frame].pFrameRGB)->data,
								((AVPicture*) decoded_frames[n_frame].pFrameRGB)->linesize);
						sws_freeContext(img_convert_ctx);
						(*env)->CallVoidMethod(env, videoPlayer, midVideo, decoded_frames[n_frame].out_buffer_video, current_width, current_height, i);
					}
					pthread_mutex_unlock(&mutex);
					
					avpkt.size -= len;
					avpkt.data += len;
					i++;
clock_gettime(CLOCK_MONOTONIC, &end);
time = timespecDiff(&end, &start);
total_time += time;
snprintf(buf, sizeof(buf), "time: %llu ms; average: %llu ms", time, total_time/i);
__android_log_write(ANDROID_LOG_INFO, LOG_TAG, buf);
				}
			}
			//Free the packet that was allocated by av_read_frame
			avpkt.data = avpkt_data_init;
			av_free_packet(&avpkt);
		}
__android_log_write(ANDROID_LOG_INFO, LOG_TAG, "next");
	}

	ret = 0;

end:
	(*env)->ReleaseStringUTFChars(env, sdp_str, pSdpString);


	for (j=0; j<QUEUE_SIZE+1; j++) {
		(*env)->DeleteLocalRef(env, decoded_frames[j].out_buffer_video);
		av_free(decoded_frames[j].pFrameRGB);
	}

	//Free the YUV frame
	av_free(pFrame);

	//Close the codec
	if (pDecodecCtxVideo)
		avcodec_close(pDecodecCtxVideo);

	//Close the video file
	__android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, "Close the context...");
	close_context(pFormatCtx);
	__android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, "ok");

	return ret;
}


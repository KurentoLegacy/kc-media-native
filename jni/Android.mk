LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

export MY_FFMPEG_SOURCE := $(NDK_PROJECT_PATH)/jni/external/ffmpeg
export MY_FFMPEG_INSTALL := $(MY_FFMPEG_SOURCE)

export MY_AMR_INSTALL := $(NDK_PROJECT_PATH)/jni/target/opencore-amr_install
export MY_AMR_C_INCLUDE := $(MY_AMR_INSTALL)/include
export MY_AMR_LDLIB := -L$(MY_AMR_INSTALL)/lib -lopencore-amrnb

ifdef ENABLE_X264
	export MY_X264_INSTALL := $(NDK_PROJECT_PATH)/jni/target/x264_install
	export MY_X264_C_INCLUDE := $(MY_X264_INSTALL)/include
	export MY_X264_LDLIB := -L$(MY_X264_INSTALL)/lib -lx264
	LOCAL_CFLAGS += -DUSE_X264
endif


#$(shell ./external/configure-make-all.sh)


# These need to be in the right order
FFMPEG_LIBS := $(addprefix $(MY_FFMPEG_SOURCE)/, \
	libavformat/libavformat.a \
	libavcodec/libavcodec.a \
	libswscale/libswscale.a \
	libavutil/libavutil.a)

LOCAL_CFLAGS += -Wall -pedantic -std=c99

LOCAL_LDLIBS += $(FFMPEG_LIBS) $(MY_AMR_LDLIB) $(MY_X264_LDLIB) \
		-llog -lc -lm -ldl -lgcc -lz


LOCAL_C_INCLUDES := 	$(MY_FFMPEG_INSTALL) \
			$(MY_AMR_C_INCLUDE) \
			$(MY_X264_C_INCLUDE) \
			$(LOCAL_PATH)/media \
			$(LOCAL_PATH)/media/rx

LOCAL_MODULE := android-media
LOCAL_SRC_FILES :=	media/my-cmdutils.c media/init-media.c media/socket-manager.c	\
			media/tx/video-tx.c media/tx/audio-tx.c		\
			media/rx/sdp-manager.c media/rx/video-rx.c media/rx/audio-rx.c

include $(BUILD_SHARED_LIBRARY)


# Create toolchain


LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# These need to be in the right order
FFMPEG_LIBS := $(addprefix $(NDK_PROJECT_PATH)/jni/target/ffmpeg_install/lib/, \
	libavformat.a \
	libavcodec.a \
	libswscale.a \
	libavutil.a)
#libavdevice/libavdevice.a
#libavfilter/libavfilter.a
#libpostproc/libpostproc.a

export MY_FFMPEG_SOURCE := $(NDK_PROJECT_PATH)/jni/external/ffmpeg
export MY_FFMPEG_INSTALL := $(MY_FFMPEG_SOURCE)
#export MY_FFMPEG_INSTALL := $(NDK_PROJECT_PATH)/jni/target/ffmpeg_install
#export MY_AMR_SOURCE := $(MY_FFMPEG_SOURCE)/opencore-amr-0.1.2
export MY_AMR_INSTALL := $(NDK_PROJECT_PATH)/jni/target/opencore-amr_install

#ifdef USE_X264_TREE
#	export MY_X264_INSTALL := $(NDK_PROJECT_PATH)/jni/external/x264
#	export MY_X264_C_INCLUDE := $(MY_X264_INSTALL)/include
#	export MY_X264_LDLIB := -L$(MY_X264_INSTALL)/lib -lx264
#	LOCAL_CFLAGS += -DUSE_X264
#endif


#$(shell ./external/configure-make-all.sh)


# ffmpeg uses its own deprecated functions liberally, so turn off that annoying noise
LOCAL_CFLAGS += -Wall -pedantic -std=c99 -Wno-deprecated-declarations
LOCAL_LDLIBS += $(FFMPEG_LIBS) -L$(MY_AMR_INSTALL)/lib \
		-llog -lc -lm -ldl -lgcc -lz -lopencore-amrnb

LOCAL_C_INCLUDES := 	$(MY_FFMPEG_SOURCE) \
			$(MY_FFMPEG_INSTALL) \
			$(MY_X264_C_INCLUDE) \
			$(MY_AMR_INSTALL)/include \
			$(LOCAL_PATH)/media \
			$(LOCAL_PATH)/media/rx
#$(MY_FFMPEG_INSTALL)/include

LOCAL_MODULE := android-media
LOCAL_SRC_FILES :=	media/my-cmdutils.c media/init-media.c media/socket-manager.c	\
			media/tx/video-tx.c media/tx/audio-tx.c		\
			media/rx/sdp-manager.c media/rx/video-rx.c media/rx/audio-rx.c

include $(BUILD_SHARED_LIBRARY)


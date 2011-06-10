
export MY_FFMPEG_SOURCE := $(NDK_PROJECT_PATH)/jni/ffmpeg-0.7-rc1
export MY_FFMPEG_INSTALL := $(MY_FFMPEG_SOURCE)
export MY_AMR_SOURCE := $(MY_FFMPEG_SOURCE)/opencore-amr-0.1.2
export MY_AMR_INSTALL := $(MY_FFMPEG_INSTALL)/opencore-amr_install

ifdef USE_X264_TREE
	export MY_X264_INSTALL := $(MY_FFMPEG_INSTALL)/x264_install
	export MY_X264_C_INCLUDE := $(MY_X264_INSTALL)/include
	export MY_X264_LDLIB := -L$(MY_X264_INSTALL)/lib -lx264
endif

%/config.mak: force
	cd $(MY_FFMPEG_SOURCE) && $(MY_FFMPEG_SOURCE)/config-ffmpeg.sh
force: ;

TOP_LOCAL_PATH := $(call my-dir)
include $(call all-subdir-makefiles)
LOCAL_PATH := $(TOP_LOCAL_PATH)
include $(CLEAR_VARS)

LOCAL_CFLAGS := -Wall -pedantic -std=c99

ifdef USE_X264_TREE
	LOCAL_CFLAGS += -DUSE_X264
endif

LOCAL_C_INCLUDES := 	$(MY_FFMPEG_INSTALL)	\
			$(MY_X264_C_INCLUDE)	\
			$(MY_AMR_INSTALL)include	\
			$(LOCAL_PATH)/media	\
			$(LOCAL_PATH)/media/rx

LOCAL_STATIC_LIBRARIES := libavformat libavcodec libavutil libpostproc libswscale 
LOCAL_LDLIBS :=	-llog $(MY_X264_LDLIB)	\
		-L$(MY_AMR_INSTALL)/lib		\
		-lc -lm -ldl -lgcc -lz -lopencore-amrnb

LOCAL_MODULE := android-media
LOCAL_SRC_FILES :=	media/my-cmdutils.c media/init-media.c media/socket-manager.c	\
			media/tx/video-tx.c media/tx/audio-tx.c		\
			media/rx/sdp-manager.c media/rx/video-rx.c media/rx/audio-rx.c
			

include $(BUILD_SHARED_LIBRARY)


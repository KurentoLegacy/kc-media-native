/*
 * (C) Copyright 2013 Kurento (http://kurento.org/)
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */

#ifndef __MEDIA_PORT_H__
#define __MEDIA_PORT_H__

extern "C" {
#include "libavformat/avformat.h"
}

#include "Media.h"
#include "util/Lock.h"

namespace media {
	class MediaPort : public Media {
	private:
		int _port;
		AVFormatContext *_formatContext;
		URLContext *_urlContext;

		Lock *_mutex;
		int _n_users;

	public:
		MediaPort();
		MediaPort(int port);
		MediaPort(const char* address, int port);
		~MediaPort();

		int getPort();
		URLContext* getConnection();
		void closeContext(AVFormatContext *s);
		int close();
	private:
		AVFormatContext* open(const char *rtp);
	};
}

#endif /* __MEDIA_PORT_H__ */

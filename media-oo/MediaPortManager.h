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

#ifndef __MEDIA_PORT_MANAGER_H__
#define __MEDIA_PORT_MANAGER_H__

#include "Media.h"
#include "MediaPort.h"

namespace media {
	class MediaPortManager : public Media {
	public:
		static MediaPort* takeMediaPort();
		static MediaPort* takeMediaPort(int port);
		static MediaPort* takeMediaPort(const char* address, int port);
		static int releaseMediaPort(MediaPort *mediaPort);
	};
}

#endif /* __MEDIA_PORT_MANAGER_H__ */

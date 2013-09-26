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

#ifndef __MEDIA_H__
#define __MEDIA_H__

extern "C" {
#include <util/log.h>
}

#include "MediaException.h"

namespace media {
	class Media {
	protected:
		const char* LOG_TAG;
	public:
		Media() throw(MediaException);
		~Media();
	};
}

void set_interrrupt_cb(int i);

#endif /* __MEDIA_H__ */

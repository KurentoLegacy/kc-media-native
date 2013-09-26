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

#ifndef __MEDIA_EXCEPTION_H__
#define __MEDIA_EXCEPTION_H__

#include <exception>
#include <stdarg.h>

using namespace std;

namespace media {
	class MediaException : public exception {
	private:
		char _message[256];

	public:
		MediaException(const char *fmt, ...);

		const char* what() const throw();
	};
}

#endif /* __MEDIA_EXCEPTION_H__ */

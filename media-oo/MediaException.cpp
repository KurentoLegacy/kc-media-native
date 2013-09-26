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

#include "MediaException.h"
#include <stdio.h>

using namespace media;

MediaException::MediaException(const char *fmt, ...)
{
	va_list vl;

	va_start(vl, fmt);
	vsnprintf(_message, sizeof(_message), fmt, vl);
	va_end(vl);
}

const char*
MediaException::what() const throw()
{
   return _message;
}

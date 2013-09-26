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

#ifndef __LOCK_H__
#define __LOCK_H__

#include <pthread.h>

class Lock {
public:
	Lock() { pthread_mutex_init(&plock, NULL); }
	~Lock() { pthread_mutex_unlock(&plock); }

	void lock() { pthread_mutex_lock(&plock); }
	void unlock() { pthread_mutex_unlock(&plock); }

private:
	pthread_mutex_t plock;
};

#endif /* __LOCK_H__ */

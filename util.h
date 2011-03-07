/*
 *  Copyright (C) 2007  Iain Wade <iwade@optusnet.com.au>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __PSAN_UTIL_H__
#define __PSAN_UTIL_H__

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define tv2dbl(tv) ((tv).tv_sec + (tv).tv_usec / 1000000.0)
struct timeval dbl2tv(double d);

void *copy(void *buf, int len);
#define dup_struct(type, ...) (type *)copy((void *)&(type){ __VA_ARGS__ }, sizeof(type))

void dump_hex(uint8_t *buf, int len);

uint64_t get_uint48(uint8_t *buf);
uint64_t ntohll(uint64_t a);
#define htonll ntohll

#endif /* __PSAN_UTIL_H__ */

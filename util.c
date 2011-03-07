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

#include "util.h"

/* Convert a double to a struct timeval */
struct timeval dbl2tv(double d)
{
    struct timeval tv;

    tv.tv_sec = (long) d;
    tv.tv_usec = (long) ((d - (long) d) * 1000000.0);
  
    return tv;
}

void *copy(void *buf, int len)
{
    void *ret;
    if (!(ret = malloc(len)))
	return NULL;
    memcpy(ret, buf, len);
    return ret;
}

void dump_hex(uint8_t *buf, int len)
{
    fprintf(stdout, "DUMP:\n");

    for (int i = 0; i < len; i += 32)
    {
	for (int n = 0; n < 32; n++)
	    if (i + n < len)
		fprintf(stdout, "%02hhx", buf[i+n]);
	    else
		fprintf(stdout, "  ");

	fprintf(stdout, "\t");

	for (int n = 0; n < 32 && i + n < len; n++)
	    fprintf(stdout, "%c", isprint(buf[i+n]) ? buf[i+n] : '.');

	fprintf(stdout, "\n");
    }
}

uint64_t get_uint48(uint8_t *buf)
{
    off_t ret = 0;
    for (int i = 0; i < 6; i++)
	ret = (ret << 8) | buf[i];
    return ret;
}

uint64_t ntohll(uint64_t a)
{
    uint32_t lo = a & 0xffffffff;
    uint32_t hi = a >> 32U;
    lo = ntohl(lo);
    hi = ntohl(hi);
    return ((uint64_t) lo) << 32U | hi;
}

#define htonll ntohll

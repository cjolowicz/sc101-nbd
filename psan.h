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

#ifndef __PSAN_H__
#define __PSAN_H__

#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include "queue.h"

#include "util.h"

extern int sock;

SLIST_HEAD(disks_t, disk_t);

struct disk_t {
    struct sockaddr_in root_addr;
    SLIST_ENTRY(disk_t) entries;
};

struct disk_info_t {
    char *version;
    char *label;
    uint64_t total_size;
    uint64_t free_size;
    uint8_t partitions;
};

struct part_addr_t {
    struct sockaddr_in root_addr;
    struct sockaddr_in part_addr;
};

struct part_info_t {
    char *id;
    char *label;
    uint64_t size;
};

void psan_init(char *dev);
void psan_cleanup(void);

struct disks_t *psan_find_disks(void);
void free_disks(struct disks_t *disks);

struct disk_info_t *psan_query_disk(struct sockaddr_in *dest);
void free_disk_info(struct disk_info_t *disk_info);

struct part_info_t *psan_query_part(struct sockaddr_in *dest);
struct part_info_t *psan_query_root(struct sockaddr_in *dest, int partition);
void free_part_info(struct part_info_t *part_info);

struct part_addr_t *psan_resolve_id(char *id);
void free_part_addr(struct part_addr_t *part_addr);

uint16_t psan_next_seq(void);
void *wait_for_packet(int sock, uint8_t cmd, uint16_t seq, uint16_t len, struct timeval *timeout, struct sockaddr *from, socklen_t *from_len);

#endif /* __PSAN_H__ */

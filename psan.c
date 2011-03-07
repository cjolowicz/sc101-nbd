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

#include "psan.h"
#include "psan_wireformat.h"

char *strndup_x(const char *string, size_t n)
{
  char *ret;
  int i;
  if (!(ret = malloc(n)))
    return NULL;
  for (i = 0; string[i] && i < (n - 1); i++)
    ret[i] = string[i];
  ret[i] = 0;
  return ret;
}

void psan_init(char *dev)
{
    if (sock)
	return;

    if (!(sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)))
	err(EXIT_FAILURE, "socket");

    int bufsize = 8*1024*1024;

#ifndef SO_RCVBUFFORCE
#define SO_RCVBUFFORCE SO_RCVBUF
#endif

    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUFFORCE, &bufsize, sizeof(bufsize)) != 0 && errno != EPERM)
	warn("setsockopt(fd, SOL_SOCKET, SO_RCVBUFFORCE, %u)", bufsize);

#ifndef SO_SNDBUFFORCE
#define SO_SNDBUFFORCE SO_SNDBUF
#endif

    if (setsockopt(sock, SOL_SOCKET, SO_SNDBUFFORCE, &bufsize, sizeof(bufsize)) != 0 && errno != EPERM)
	warn("setsockopt(fd, SOL_SOCKET, SO_RCVBUFFORCE, %u)", bufsize);

    int on = 1;

    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) != 0)
	err(EXIT_FAILURE, "setsockopt(fd, SOL_SOCKET, SO_BROADCAST)");

#ifdef SO_BINDTODEVICE
    if (dev && (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, dev, strlen(dev)+1) < 0))
	err(EXIT_FAILURE, "setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, %s)", dev);
#endif

    if (bind(sock, (struct sockaddr *)&(struct sockaddr_in){ .sin_family=AF_INET, .sin_port=htons(20001) }, sizeof(struct sockaddr_in)) < 0 && errno != EADDRINUSE)
	warn("bind(fd, {sa_family=AF_INET, sin_port=htons(20001)})");
}

void psan_cleanup(void)
{
    if (sock)
	close(sock);
}

struct disks_t *psan_find_disks(void)
{
    struct sockaddr_in broadcast = {
	.sin_family = AF_INET,
	.sin_port   = htons(20001),
	.sin_addr   = { .s_addr = INADDR_BROADCAST }
    };
    socklen_t broadcast_len = sizeof(struct sockaddr_in);

    /* send broadcast FIND message */
    uint16_t expected_seq = psan_next_seq();
    struct psan_find_t find = {
	.ctrl = { .cmd = PSAN_FIND, .seq = htons(expected_seq) }
    };
    size_t find_len = sizeof(struct psan_find_t);
    sendto(sock, (void *)&find, find_len, 0, (struct sockaddr *)&broadcast, broadcast_len);

    /* collect initial answer within 1 second */
    struct timeval timeout = { .tv_sec = 1 };
    struct psan_find_response_t *pfr;
    struct disks_t *disks = NULL;

    while ((pfr = wait_for_packet(sock, PSAN_FIND_RESPONSE, expected_seq, sizeof(struct psan_find_response_t), &timeout, NULL, 0)))
    {
	/* subsequent responses within 1/10 of a second of the fastest */
	if (!disks)
	{
	    disks = malloc(sizeof(struct disks_t));
	    bzero(disks, sizeof(struct disks_t));
	    SLIST_INIT(disks);
	    timeout = (struct timeval){ .tv_sec = 0, .tv_usec = 100000 };
	}

	struct disk_t *disk = dup_struct(struct disk_t,
	    .root_addr = (struct sockaddr_in){
		.sin_family = AF_INET,
		.sin_port = htons(20001),
		.sin_addr = pfr->ip4
	    }
	);

	SLIST_INSERT_HEAD(disks, disk, entries);
    }

    return disks;
}

void free_disks(struct disks_t *disks)
{
    struct disk_t *disk;

    while ((disk = SLIST_FIRST(disks)))
    {
	SLIST_REMOVE_HEAD(disks, entries);
	free(disk);
    }

    free(disks);
}

struct disk_info_t *psan_query_disk(struct sockaddr_in *dest)
{
    /* query disk information from root IP */
    uint16_t expected_seq = psan_next_seq();
    struct psan_get_t get = {
	.ctrl = { .cmd = PSAN_GET, .seq = htons(expected_seq), .len_power = 9 },
	.sector = 0,
	.info = 0
    };
    size_t get_len = sizeof(struct psan_get_t);
    sendto(sock, (void *)&get, get_len, 0, (struct sockaddr *)dest, sizeof(struct sockaddr_in));

    struct timeval timeout = { .tv_sec = 1 };
    struct psan_get_response_disk_t *ret;

    if (!(ret = wait_for_packet(sock, PSAN_GET_RESPONSE, expected_seq, sizeof(struct psan_get_response_disk_t), &timeout, NULL, 0)))
	return NULL;

    return dup_struct(struct disk_info_t,
	.version    = strndup_x(ret->version, sizeof(ret->version)),
	.label      = strndup_x(ret->label, sizeof(ret->label)),
	.total_size = get_uint48(ret->sector_total) << 9,
	.free_size  = get_uint48(ret->sector_free) << 9,
	.partitions = ret->partitions
    );
}

void free_disk_info(struct disk_info_t *disk_info)
{
    free(disk_info->version);
    free(disk_info->label);
    free(disk_info);
}

struct part_info_t *psan_query_part(struct sockaddr_in *dest)
{
    /* query partition information from root IP */
    uint16_t expected_seq = psan_next_seq();
    struct psan_identify_t identify = {
	.ctrl = { .cmd = PSAN_IDENTIFY, .seq = htons(expected_seq) },
    };
    size_t identify_len = sizeof(struct psan_identify_t);
    sendto(sock, (void *)&identify, identify_len, 0, (struct sockaddr *)dest, sizeof(struct sockaddr_in));

    struct timeval timeout = { .tv_sec = 1 };
    struct psan_get_response_partition_t *ret;

    if (!(ret = wait_for_packet(sock, PSAN_GET_RESPONSE, expected_seq, sizeof(struct psan_get_response_disk_t), &timeout, NULL, 0)))
	return NULL;

    return dup_struct(struct part_info_t,
	.id    = strndup_x(ret->id, sizeof(ret->id)),
	.label = strndup_x(ret->label, sizeof(ret->label)),
	.size  = get_uint48(ret->sector_size) << 9
    );
}

struct part_info_t *psan_query_root(struct sockaddr_in *dest, int partition)
{
    /* query partition information from root IP */
    uint16_t expected_seq = psan_next_seq();
    struct psan_get_t get = {
	.ctrl = { .cmd = PSAN_GET, .seq = htons(expected_seq), .len_power = 9 },
	.sector = htonl(partition),
	.info = 0
    };
    size_t get_len = sizeof(struct psan_get_t);
    sendto(sock, (void *)&get, get_len, 0, (struct sockaddr *)dest, sizeof(struct sockaddr_in));

    struct timeval timeout = { .tv_sec = 1 };
    struct psan_get_response_partition_t *ret;

    if (!(ret = wait_for_packet(sock, PSAN_GET_RESPONSE, expected_seq, sizeof(struct psan_get_response_disk_t), &timeout, NULL, 0)))
	return NULL;

    return dup_struct(struct part_info_t,
	.id    = strndup_x(ret->id, sizeof(ret->id)),
	.label = strndup_x(ret->label, sizeof(ret->label)),
	.size  = get_uint48(ret->sector_size) << 9
    );
}

void free_part_info(struct part_info_t *part_info)
{
    free(part_info->id);
    free(part_info->label);
    free(part_info);
}

struct part_addr_t *psan_resolve_id(char *id)
{
    struct sockaddr_in broadcast = {
	.sin_family = AF_INET,
	.sin_port   = htons(20001),
	.sin_addr   = { .s_addr = INADDR_BROADCAST }
    };
    socklen_t broadcast_len = sizeof(struct sockaddr_in);

    /* query partition information from root IP */
    uint16_t expected_seq = psan_next_seq();
    struct psan_resolve_t resolve = {
	.ctrl = { .cmd = PSAN_RESOLVE, .seq = htons(expected_seq) },
    };
    size_t resolve_len = sizeof(struct psan_resolve_t);
    strncpy(resolve.id, id, sizeof(resolve.id));
    sendto(sock, (void *)&resolve, resolve_len, 0, (struct sockaddr *)&broadcast, broadcast_len);

    struct timeval timeout = { .tv_sec = 1 };
    struct psan_resolve_response_t *ret;
    struct sockaddr_in from;
    socklen_t from_len = sizeof(from);

    if (!(ret = wait_for_packet(sock, PSAN_RESOLVE_RESPONSE, expected_seq, sizeof(struct psan_resolve_response_t), &timeout, (struct sockaddr *)&from, &from_len)))
	return NULL;

    return dup_struct(struct part_addr_t,
	.root_addr = from,
	.part_addr = (struct sockaddr_in){
	    .sin_family = AF_INET,
	    .sin_port = htons(20001),
	    .sin_addr = ret->ip4
	}
    );
}

void free_part_addr(struct part_addr_t *part_addr)
{
    free(part_addr);
}

/* 15 bits of sequence number are usable */
uint16_t psan_next_seq(void)
{
    static uint16_t seq = 1<<15;

    if (seq & (1 << 15))
    {
	srand(time(NULL) ^ getpid());
	seq = rand() % (1 << 15);
    }

    seq++;

    if (seq & (1 << 15))
	seq = 0;

    return seq;
}

void *wait_for_packet(int sock, uint8_t cmd, uint16_t seq, uint16_t len, struct timeval *timeout, struct sockaddr *from, socklen_t *from_len)
{
    static char buf[65536];
    fd_set set;
    int ret;

    for (;;)
    {
	FD_ZERO(&set);
	FD_SET(sock, &set);

	if ((ret = select(sock+1, &set, NULL, NULL, timeout)) < 0)
	    err(EXIT_FAILURE, "select");

	if (!ret)
	    break;

	if ((ret = recvfrom(sock, buf, sizeof(buf), 0, from, from_len)) < 0)
	    err(EXIT_FAILURE, "recv");

	if (ret < sizeof(struct psan_ctrl_t))
	    continue;

	struct psan_ctrl_t *ctrl = (struct psan_ctrl_t *)buf;

	if (ret != len || ctrl->cmd != cmd || ntohs(ctrl->seq) != seq)
	    continue;

	return buf;
    }

    return NULL;
}

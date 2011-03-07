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

#ifndef __PSAN_WIREFORMAT_H__
#define __PSAN_WIREFORMAT_H__

#include <stdint.h>
#include <netinet/in.h>

#define PSAN_FIND 0x0d
#define PSAN_FIND_RESPONSE 0x0e
#define PSAN_RESOLVE 0x0f
#define PSAN_RESOLVE_RESPONSE 0x10
#define PSAN_IDENTIFY 0x13
#define PSAN_GET 0x00
#define PSAN_GET_RESPONSE 0x11
#define PSAN_PUT 0x01
#define PSAN_PUT_RESPONSE 0x04

struct psan_ctrl_t {
    uint8_t cmd;
    uint8_t len_power;
    uint16_t seq;
} __attribute__((__packed__));

struct psan_find_t {
    struct psan_ctrl_t ctrl;
} __attribute__((__packed__));

struct psan_find_response_t {
    struct psan_ctrl_t ctrl;
    uint8_t unknown1[12];
    struct in_addr ip4;
} __attribute__((__packed__));

struct psan_resolve_t {
    struct psan_ctrl_t ctrl;
    char id[64];
} __attribute__((__packed__));

struct psan_resolve_response_t {
    struct psan_ctrl_t ctrl;
    uint8_t unknown1[76];
    struct in_addr ip4;
    uint8_t unknown2[20];
} __attribute__((__packed__));

struct psan_identify_t {
    struct psan_ctrl_t ctrl;
    uint8_t unknown1[24];
} __attribute__((__packed__));

struct psan_get_t {
    struct psan_ctrl_t ctrl;
    uint8_t unknown1[12];
    struct in_addr ip4;
    uint8_t unknown2[2];
    uint32_t sector;
    uint8_t unknown3;
    uint8_t info;
} __attribute__((__packed__));

struct psan_get_response_t {
    struct psan_get_t get;
    uint8_t buffer[];
} __attribute__((__packed__));

struct psan_get_response_disk_t {
    struct psan_get_t get;
    char version[16];
    uint8_t market_class[2];
    uint8_t manufacturer_code[3];
    uint8_t part_number[3];
    uint8_t sector_total[6];
    uint8_t sector_free[6];
    uint8_t unknown1[5];
    uint8_t partitions;
    uint8_t unknown2[4];
    char label[56]; // UI indicates 28-chars max - chars stored as UTF16
    uint8_t unknown3[410];
} __attribute__((__packed__));

struct psan_get_response_partition_t {
    struct psan_get_t get;
    uint8_t unknown1[6];
    char label[28];
    uint8_t unknown2[100];
    uint8_t sector_size[6];
    uint8_t unknown3[38];
    char id[64];
    uint8_t unknown4[270];
} __attribute__((__packed__));

struct psan_put_t {
    struct psan_ctrl_t ctrl;
    uint8_t unknown1[12];
    struct in_addr ip4;
    uint8_t unknown2[2];
    uint32_t sector;
    uint8_t unknown3;
    uint8_t info;
    uint8_t buffer[];
} __attribute__((__packed__));

struct psan_put_response_t {
    struct psan_ctrl_t ctrl;
    uint8_t unknown1[12];
    struct in_addr ip4;
    uint8_t unknown2[2];
    uint32_t sector;
    uint8_t unknown3;
    uint8_t info;
} __attribute__((__packed__));

#endif /* __PSAN_WIREFORMAT_H__ */

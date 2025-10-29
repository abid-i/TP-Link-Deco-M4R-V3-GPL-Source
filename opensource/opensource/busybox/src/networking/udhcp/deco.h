//
// Created by xfs on 12/19/17.
//

#include "common.h"

#include <libubox/kvlist.h>

#include <elf.h>

#ifndef BUSYBOX_UDHCP_DECO_H
#define BUSYBOX_UDHCP_DECO_H

#define SNAME_PREFIX "DECO_"
/*the len that sizeof calculated of the string constant contains the '\0', so we need subtract 1 */
#define SNAME_PREFIX_LEN (sizeof(SNAME_PREFIX) - 1)
#define SNAME_MAC_LEN (sizeof("0123456789ab") - 1)
#define OPT_MAC_SHIFT (OPT_DATA + sizeof(SNAME_PREFIX) - 1)
#define OPT_MAC_SHIFT_MAX (OPT_MAC_SHIFT + SNAME_MAC_LEN)
#define HOSTNAME_MAX_LEN (0xff + 1)

uint8_t *deco_gen_sname(void);
int deco_get_mac_from_sname(const char *opt, char *mac, uint8_t len);
struct kvlist *deco_get_mac_list(void);

#endif //BUSYBOX_UDHCP_DECO_H

/*
 **************************************************************************
 * Copyright (c) 2014-2015,2018-2020 The Linux Foundation. All rights reserved.
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all copies.
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 **************************************************************************
 */

/*
 * nss_nldtls.h
 *	NSS Netlink Dtls API definitions
 */
#ifndef __NSS_NLDTLS_H
#define __NSS_NLDTLS_H

#define NSS_NLDTLS_SKB_TAILROOM 192
#define NSS_NLDTLS_IPV4_SESSION 0
#define NSS_NLDTLS_MAX_TUNNELS 32
#define NSS_NLDTLS_VLAN_INVALID 0xFFF
#define NSS_NLDTLS_IP_VERS_4 4
#define NSS_NLDTLS_DUMMY_DATA 0xcc

/*
 * nss_nldtls_tun_ctx
 *	Per dtls tunnel context
 */
struct nss_nldtls_tun_ctx {
	struct list_head list;			/**< List for holding different tunnel info */
	struct nss_nldtls_rule *nl_rule;	/**< Dtls rule structure */
	char dev_name[IFNAMSIZ];		/**< Dtls session netdev */
};

/*
 * nss_nldtls_gbl_ctx
 *	Global context for dtls
 */
struct nss_nldtls_gbl_ctx {
	spinlock_t lock;
	atomic_t num_tun;
	struct list_head dtls_list_head;
	bool log_en;
	ktime_t first_rx_pkt_time;
	ktime_t first_tx_pkt_time;
	ktime_t last_rx_pkt_time;
	ktime_t last_tx_pkt_time;
};

bool nss_nldtls_init(void);
bool nss_nldtls_exit(void);

#if (CONFIG_NSS_NLDTLS == 1)
#define NSS_NLDTLS_INIT nss_nldtls_init
#define NSS_NLDTLS_EXIT nss_nldtls_exit
#else
#define NSS_NLDTLS_INIT 0
#define NSS_NLDTLS_EXIT 0
#endif /* !CONFIG_NSS_NLDTLS */
#endif /* __NSS_NLDTLS_H */

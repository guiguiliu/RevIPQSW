/*
 * Copyright (c) 2013, 2018-2020 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 **************************************************************************
 * 2013, Qualcomm Atheros, Inc.
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
 * osif_nss.c
 *
 * This file used for wifi redirect for NSS
 * ------------------------REVISION HISTORY-----------------------------
 * Qualcomm Atheros         23/sep/2013              Created
 */


#if  QCA_NSS_PLATFORM
#include "osif_private.h"
#include <wlan_opts.h>
#define OSIF_TO_NETDEV(_osif) (((osif_dev *)(_osif))->netdev)
#include <nss_api_if.h>
#if ATH_PERF_PWR_OFFLOAD
#include <ol_if_athvar.h>
#endif
#include <cdp_txrx_ctrl.h>

#if QCA_OL_GRO_ENABLE
/*
 * callback function that is registered with NSS
 * for the WLAN interface
 */
void osif_receive_from_nss(struct net_device *if_ctx, struct sk_buff *os_buf, struct napi_struct *napi)
{
    struct net_device *netdev;
    struct sk_buff *skb;
    osif_dev  *osifp;

    nbuf_debug_add_record(os_buf);
    if(if_ctx == NULL) {
        qdf_nofl_info(KERN_CRIT "%s , netdev is NULL, freeing skb", __func__);
        skb = (struct sk_buff *)os_buf;
        wbuf_free(skb);
        return;
    }

    netdev = (struct net_device *)if_ctx;
    dev_hold(netdev);
    osifp = netdev_priv(netdev);

    if (osifp->netdev != netdev) {
        qdf_nofl_info(KERN_CRIT "%s , netdev incorrect, freeing skb", __func__);
        skb = (struct sk_buff *)os_buf;
        wbuf_free(skb);
        dev_put(netdev);
        return;
    }

    skb = (struct sk_buff *)os_buf;
    skb->dev = netdev;
    skb->protocol = eth_type_trans(skb, netdev);
    nbuf_debug_del_record(skb);
    napi_gro_receive(napi, skb);

    dev_put(netdev);
}
#endif /* QCA_OL_GRO_ENABLE */

/*
 * osif_pltfrm_create_nss_802_3_redir_vap()
 *  Create VAP to that accepts packets in 802.3 format
 */
static void osif_pltfrm_create_nss_802_3_redir_vap(osif_dev *osif)
{
    struct net_device *dev = OSIF_TO_NETDEV(osif);
    osif->nss_redir_ctx = nss_virt_if_create_sync(dev);
#if QCA_OL_GRO_ENABLE
    /* osif->nssctx will be NULL for more than 16 VAPS */
    if (osif->nss_redir_ctx) {
        nss_virt_if_register(osif->nss_redir_ctx, osif_receive_from_nss, dev);
    }
#endif /* QCA_OL_GRO_ENABLE */
}

/*
 * osif_pltfrm_create_nss_native_wifi_redir_vap()
 *  Create VAP to that accepts packets in native wifi format
 */
static void osif_pltfrm_create_nss_native_wifi_redir_vap(osif_dev *osif)
{
    struct net_device *dev = OSIF_TO_NETDEV(osif);
    osif->nss_redir_ctx = nss_wifi_if_create_sync(dev);
#if QCA_OL_GRO_ENABLE
    /* osif->nssctx will be NULL for more than 16 VAPS */
    if (osif->nss_redir_ctx) {
        nss_wifi_if_register(osif->nss_redir_ctx, osif_receive_from_nss, dev);
    }
#endif /* QCA_OL_GRO_ENABLE */
}

/*
 * osif_pltfrm_create_vap
 *  Register vap with NSS
 */
void osif_pltfrm_create_vap(osif_dev *osif)
{
    if (osif->nss_nwifi) {
        osif_pltfrm_create_nss_native_wifi_redir_vap(osif);
        return;
    }

    osif_pltfrm_create_nss_802_3_redir_vap(osif);
}

/*
 * osif_pltfrm_delete_vap
 *  Unregister vap with NSS
 */
void osif_pltfrm_delete_vap(osif_dev *osif)
{
    if (!osif->nss_redir_ctx) {
        return;
    }

    if (osif->nss_nwifi) {
        nss_wifi_if_destroy_sync(osif->nss_redir_ctx);
    } else {
        nss_virt_if_destroy_sync(osif->nss_redir_ctx);
    }
}

#ifdef VDEV_PEER_PROT_COUNT
#define osif_rx_peer_protocol_cnt(osifp, soc, vdev_id, skb) \
{ \
    if ((osifp)->peer_protocol_cnt) { \
        cdp_txrx_peer_protocol_cnt((soc), (vdev_id), (skb),\
                               CDP_VDEV_PEER_PROT_IS_EGRESS, \
                               CDP_VDEV_PEER_PROT_IS_RX); \
    } \
}
#else
#define osif_rx_peer_protocol_cnt(osifp, soc, vdev_id, skb)
#endif

/*
 * osif_send_to_nss
 *  Send packets to the nss driver
 */
extern void transcap_nwifi_to_8023(qdf_nbuf_t msdu);
void
osif_send_to_nss(os_if_t osif, struct sk_buff *skb, int nwifi)
{
    struct net_device *dev = OSIF_TO_NETDEV(osif);
    osif_dev  *osifp = (osif_dev *) osif;
    nss_tx_status_t nss_tx_status;
    struct ol_txrx_nbuf_classify nbuf_class;
    ol_txrx_soc_handle soc_txrx_handle;
    struct net_device *comdev;
    struct ol_ath_softc_net80211 *scn;
    cdp_config_param_type value = {0};

    comdev = ((osif_dev *)osif)->os_comdev;
    scn = ath_netdev_priv(comdev);
    soc_txrx_handle = wlan_psoc_get_dp_handle(scn->soc->psoc_obj);
    cdp_txrx_get_pdev_param(soc_txrx_handle, wlan_objmgr_pdev_get_pdev_id(scn->sc_pdev),
                            CDP_CONFIG_VOW, &value);

    /*
     * Classify packet for Video and query nss driver action whether
     * to accelerate the packet or drop it
     */
    if (qdf_unlikely(value.cdp_pdev_param_cfg_vow)) {
        cdp_txrx_classify_and_update(soc_txrx_handle, wlan_vdev_get_id(osifp->ctrl_vdev), skb, rx_direction, &nbuf_class);

        if (nbuf_class.is_ipv4 &&
                (nss_ipv4_dscp_action_get(nbuf_class.pkt_dscp) == NSS_IPV4_DSCP_MAP_ACTION_DONT_ACCEL))
            goto out;
        else if (nbuf_class.is_ipv6 &&
                (nss_ipv6_dscp_action_get(nbuf_class.pkt_dscp) == NSS_IPV6_DSCP_MAP_ACTION_DONT_ACCEL))
            goto out;

        cdp_calculate_delay_stats(soc_txrx_handle, wlan_vdev_get_id(osifp->ctrl_vdev), skb);
    }

    if (!nwifi) {
        if (qdf_unlikely((((long)skb->data) & 3) != 2)) {
            /*
             * skb->data is not aligned to 2 byte boundary, send to stack directly
             */
            goto out;
        }
    }

    if (qdf_unlikely(!osifp->nss_redir_ctx)) {
        goto out;
    }

    /* if received skb is non-linear, linearize and send to NSS */
    if (qdf_unlikely(skb_has_frag_list(skb))) {
        /* if unable to linearize send the skb to netif_rx */
        if (skb_linearize(skb) == -ENOMEM) {
            goto out;
        }
        skb = qdf_nbuf_unshare(skb);
        if (skb == NULL) {
           return;
        }
    }

    skb->next =NULL;
    if (qdf_unlikely(skb_shared(skb))) {
        goto out;
    }

    /* Delete the record before giving the buffer to NSS
     *
     * There can be a race condition where buffer is freed by NSS
     * and same skb is allocated to WiFi driver in other context
     * in this case there could be false positives if we delete
     * record later
     */
    nbuf_debug_del_record(skb);
    if (qdf_unlikely(nwifi)) {
        nss_tx_status = nss_wifi_if_tx_buf(osifp->nss_redir_ctx, skb) ;
    } else {
        nss_tx_status = nss_virt_if_tx_buf(osifp->nss_redir_ctx, skb);
    }

    if (qdf_likely(nss_tx_status == NSS_TX_SUCCESS)) {
        return;
    }

    nbuf_debug_add_record(skb);
    if (nss_tx_status == NSS_TX_FAILURE_QUEUE) {
        qdf_nbuf_free(skb);
        return;
    }

out:
    if (qdf_unlikely(nwifi)) {
        transcap_nwifi_to_8023(skb);
    }

    osif_rx_peer_protocol_cnt(osifp, soc_txrx_handle,
                          wlan_vdev_get_id(osifp->ctrl_vdev), skb);
    skb->protocol = eth_type_trans(skb, dev);
    skb->dev = dev;
    nbuf_debug_del_record(skb);
    netif_receive_skb(skb);
    return ;
}
qdf_export_symbol(osif_send_to_nss);
#endif

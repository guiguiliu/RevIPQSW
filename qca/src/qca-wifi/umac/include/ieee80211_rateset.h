/*
 * Copyright (c) 2011, 2018, 2020 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * Copyright (c) 2008 Atheros Communications Inc.
 * All Rights Reserved.
 *
 * 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 *
 */

#ifndef _NET80211_IEEE80211_RATESET_H
#define _NET80211_IEEE80211_RATESET_H

#include <ieee80211_var.h>

/*
 * Internal API's for rate/rateset handling
 */
int ieee80211_iserp_rateset(struct ieee80211com *, struct ieee80211_rateset *);
void ieee80211_set11gbasicrates(struct ieee80211_rateset *, enum ieee80211_phymode);
void ieee80211_setbasicrates(struct ieee80211_rateset *rs, struct ieee80211_rateset *rs_sup);
void ieee80211_setpuregbasicrates(struct ieee80211_rateset *);
int ieee80211_find_puregrate(u_int8_t rate);

/* flags for ieee80211_fix_rate() */
#define	IEEE80211_F_DOSORT	0x00000001	/* sort rate list */
#define	IEEE80211_F_DOFRATE	0x00000002	/* use fixed rate */
#define	IEEE80211_F_DOXSECT	0x00000004	/* intersection of rates  */
#define	IEEE80211_F_DOBRS	0x00000008	/* check for basic rates */

int ieee80211_setup_rates(struct ieee80211_node *ni, const u_int8_t *rates, const u_int8_t *xrates, int flags);
int ieee80211_setup_ht_rates(struct ieee80211_node *ni, u_int8_t *ie, int flags);
void ieee80211_setup_basic_ht_rates(struct ieee80211_node *,u_int8_t *);
int ieee80211_setup_vht_rates(struct ieee80211_node *ni, u_int8_t *ie, int flags);
void ieee80211_rateset_vattach(struct ieee80211vap *vap);
void ieee80211_get_nss_from_hecap_mcsnssset_6g(struct ieee80211_node *ni);

void ieee80211_init_rateset(struct ieee80211com *ic);
void ieee80211_init_node_rates(struct ieee80211_node *ni, struct ieee80211_ath_channel *chan);
int ieee80211_check_node_rates(struct ieee80211_node *ni);

int ieee80211_check_rate(struct ieee80211vap *vap, struct ieee80211_ath_channel *, struct ieee80211_rateset *, int is_ht);
int ieee80211_check_ht_rate(struct ieee80211vap *vap, struct ieee80211_ath_channel *, struct ieee80211_rateset *);
int ieee80211_fixed_htrate_check(struct ieee80211_node *ni, struct ieee80211_rateset *nrs);
int ieee80211_node_has_11g_rate(struct ieee80211_node *ni);
void ieee80211_set_mcast_rate(struct ieee80211vap *vap);
u_int8_t ieee80211_is_multistream(struct ieee80211_node *ni);
u_int8_t ieee80211_getstreams(struct ieee80211com *ic, u_int8_t chainmask);
int ieee80211_check_11b_rates(struct ieee80211vap *vap, struct ieee80211_rateset *rrs);
u_int8_t ieee80211_get_txstreams(struct ieee80211com *ic, struct ieee80211vap *vap);
u_int8_t ieee80211_get_rxstreams(struct ieee80211com *ic, struct ieee80211vap *vap);
u_int8_t ieee80211_compute_nss(struct ieee80211com *ic, u_int8_t chainmask, struct ieee80211_bwnss_map *nssmap);

#define IEEE80211_SUPPORTED_RATES(_ic, _mode)   (&((_ic)->ic_sup_rates[(_mode)]))
#define IEEE80211_HT_RATES(_ic, _mode)          (&((_ic)->ic_sup_ht_rates[(_mode)]))
#define IEEE80211_XR_RATES(_ic)                 (&((_ic)->ic_sup_xr_rates))
#define IEEE80211_HALF_RATES(_ic)               (&((_ic)->ic_sup_half_rates))
#define IEEE80211_QUARTER_RATES(_ic)            (&((_ic)->ic_sup_quarter_rates))

static INLINE u_int64_t
ieee80211_rate2linkpeed(u_int8_t rate)
{
    return ((u_int64_t)500000) * rate;
}

#define  MAX_VHT_STREAMS 8
typedef struct ieee80211_vht_rate_s {
    int num_streams;
    u_int8_t rates[MAX_VHT_STREAMS];
}ieee80211_vht_rate_t;

u_int16_t ieee80211_get_vht_rate_map(ieee80211_vht_rate_t *vht);
void ieee80211_get_vht_rates(u_int16_t map, ieee80211_vht_rate_t *vht);

#define DEFAULT_LOWEST_RATE_2G  (1000)    //kbps
#define DEFAULT_LOWEST_RATE_5G  (6000)    //kbps

int ieee80211_disable_legacy_rates(struct ieee80211vap *vap);

enum  ratecode_preamble {
    RATECODE_PREAM_OFDM,
    RATECODE_PREAM_CCK,
    RATECODE_PREAM_HT,
    RATECODE_PREAM_VHT,
};

/* Derive NSS values supported by peer based on VHT capabilities
 * advertised
 */
bool ieee80211_derive_nss_from_cap(struct ieee80211_node *ni, ieee80211_vht_rate_t *rx_rrs);

/* macro indicating max available IEEE80211_RTSCTS_PROFILE. To be updated as and when a new profile is added */
#define IEEE80211_RTSCTS_PROFILE_MAX_VALUE 4

/* macro indicating max available IEEE80211_RTS_CTS flag. To be updated as and when a new flag is added */
#define IEEE80211_RTSCTS_FLAG_MAX_VALUE 2

typedef enum {
    /* Neither of the rate-series should use RTS-CTS */
    IEEE80211_RTSCTS_FOR_NO_RATESERIES = 0,
    /* Only the second rate-series will use RTS-CTS */
    IEEE80211_RTSCTS_FOR_SECOND_RATESERIES = 1,
    /* Only the second rate-series will use RTS-CTS, but if there's a
     * sw retry, both the rate series will use RTS-CTS */
    IEEE80211_RTSCTS_ACROSS_SW_RETRIES = 2,
    /* RTS/CTS used for ERP protection for every PPDU. */
    IEEE80211_RTSCTS_ERP = 3,
    /* Enable  RTS-CTS for all rate series */
    IEEE80211_RTSCTS_FOR_ALL_RATESERIES = 4,
    /* Add new profiles before this.. */
    IEEE80211_RTSCTS_PROFILE_MAX = 15
} IEEE80211_RTSCTS_PROFILE;

typedef enum {
    /* RTS-CTS disabled */
    IEEE80211_RTS_CTS_DISABLED = 0,
    /* RTS-CTS enabled */
    IEEE80211_USE_RTS_CTS = 1,
    /* CTS-to-self enabled */
    IEEE80211_USE_CTS2SELF = 2,
} IEEE80211_RTS_CTS;

#define IEEE80211_RTS_CTS_MASK          0x0f
#define IEEE80211_RTS_CTS_SHIFT         0
#define IEEE80211_RTS_CTS_PROFILE_MASK  0xf0
#define IEEE80211_RTS_CTS_PROFILE_SHIFT 4

#define IEEE80211_RTSCTS_DISABLED \
    (((IEEE80211_RTS_CTS_DISABLED << IEEE80211_RTS_CTS_SHIFT) & IEEE80211_RTS_CTS_MASK) | \
     ((IEEE80211_RTSCTS_FOR_NO_RATESERIES << IEEE80211_RTS_CTS_PROFILE_SHIFT) & IEEE80211_RTS_CTS_PROFILE_MASK))

#define IEEE80211_RTSCTS_ENABLED_4_SECOND_RATESERIES \
    (((IEEE80211_USE_RTS_CTS << IEEE80211_RTS_CTS_SHIFT) & IEEE80211_RTS_CTS_MASK) | \
     ((IEEE80211_RTSCTS_FOR_SECOND_RATESERIES << IEEE80211_RTS_CTS_PROFILE_SHIFT) & IEEE80211_RTS_CTS_PROFILE_MASK))

#define CTS2SELF_ENABLED_4_SECOND_RATESERIES \
    (((IEEE80211_USE_CTS2SELF << IEEE80211_RTS_CTS_SHIFT) & IEEE80211_RTS_CTS_MASK) | \
     ((IEEE80211_RTSCTS_FOR_SECOND_RATESERIES << IEEE80211_RTS_CTS_PROFILE_SHIFT) & IEEE80211_RTS_CTS_PROFILE_MASK))

#define RTSCTS_ENABLED_4_SWRETRIES \
    (((IEEE80211_USE_RTS_CTS << IEEE80211_RTS_CTS_SHIFT) & IEEE80211_RTS_CTS_MASK) | \
     ((IEEE80211_RTSCTS_ACROSS_SW_RETRIES << IEEE80211_RTS_CTS_PROFILE_SHIFT) & IEEE80211_RTS_CTS_PROFILE_MASK))

#define CTS2SELF_ENABLED_4_SWRETRIES \
    (((IEEE80211_USE_CTS2SELF << IEEE80211_RTS_CTS_SHIFT) & IEEE80211_RTS_CTS_MASK) | \
     ((IEEE80211_RTSCTS_ACROSS_SW_RETRIES << IEEE80211_RTS_CTS_PROFILE_SHIFT) & IEEE80211_RTS_CTS_PROFILE_MASK))
#endif /* _NET80211_IEEE80211_RATESET_H */

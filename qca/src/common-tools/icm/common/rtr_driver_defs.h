/*
 * Copyright (c) 2017,2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 * Notifications and licenses are retained for attribution purposes only
 *
 * Copyright (c) 2008 Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * This is to be used for MBL only. In future we shall remove all RTR specific
 * definitions and use common definitions for both rtr/mbl.
 */

#ifndef ICM_RTR_DRIVER

#define TRUE 1
#define FALSE 0

/*
 * Channels are specified by frequency and attributes.
 */
struct ieee80211_ath_channel {
    u_int16_t       ic_freq;        /* setting in Mhz */
    u_int64_t       ic_flags;       /* see below */
    u_int16_t       ic_flagext;     /* see below */
    u_int8_t        ic_ieee;        /* IEEE channel number */
    int8_t          ic_maxregpower; /* maximum regulatory tx power in dBm */
    int8_t          ic_maxpower;    /* maximum tx power in dBm */
    int8_t          ic_minpower;    /* minimum tx power in dBm */
    u_int8_t        ic_regClassId;  /* regClassId of this channel */
    u_int8_t        ic_antennamax;  /* antenna gain max from regulatory */

    u_int8_t        ic_vhtop_ch_num_seg1;         /* Seg1 center channel index */
    u_int8_t        ic_vhtop_ch_num_seg2;         /* Seg2 center channel index for 80+80 MHz mode or
                                                     center channel index of operating span for 160 MHz mode */
    uint16_t        ic_vhtop_freq_seg1;           /* Seg1 center channel frequency */
    uint16_t        ic_vhtop_freq_seg2;           /* Seg2 center channel frequency index for 80+80 MHz mode or
                                                     center channel frequency of operating span for 160 MHz mode */
};

#define IEEE80211_2GCSA_TBTTCOUNT        3

/* bits 0-3 are for private use by drivers */
/* channel attributes */
#define IEEE80211_CHAN_TURBO            0x0000000000000010 /* Turbo channel */
#define IEEE80211_CHAN_CCK              0x0000000000000020 /* CCK channel */
#define IEEE80211_CHAN_OFDM             0x0000000000000040 /* OFDM channel */
#define IEEE80211_CHAN_2GHZ             0x0000000000000080 /* 2 GHz spectrum channel. */
#define IEEE80211_CHAN_5GHZ             0x0000000000000100 /* 5 GHz spectrum channel */
#define IEEE80211_CHAN_PASSIVE          0x0000000000000200 /* Only passive scan allowed */
#define IEEE80211_CHAN_DYN              0x0000000000000400 /* Dynamic CCK-OFDM channel */
#define IEEE80211_CHAN_GFSK             0x0000000000000800 /* GFSK channel (FHSS PHY) */
#define IEEE80211_CHAN_DFS_RADAR        0x0000000000001000 /* Radar found on channel */
#define IEEE80211_CHAN_STURBO           0x0000000000002000 /* 11a static turbo channel only */
#define IEEE80211_CHAN_HALF             0x0000000000004000 /* Half rate channel */
#define IEEE80211_CHAN_QUARTER          0x0000000000008000 /* Quarter rate channel */
#define IEEE80211_CHAN_HT20             0x0000000000010000 /* HT 20 channel */
#define IEEE80211_CHAN_HT40PLUS         0x0000000000020000 /* HT 40 with extension channel above */
#define IEEE80211_CHAN_HT40MINUS        0x0000000000040000 /* HT 40 with extension channel below */
#define IEEE80211_CHAN_HT40INTOL        0x0000000000080000 /* HT 40 Intolerant */
#define IEEE80211_CHAN_VHT20            0x0000000000100000 /* VHT 20 channel */
#define IEEE80211_CHAN_VHT40PLUS        0x0000000000200000 /* VHT 40 with extension channel above */
#define IEEE80211_CHAN_VHT40MINUS       0x0000000000400000 /* VHT 40 with extension channel below */
#define IEEE80211_CHAN_VHT80            0x0000000000800000 /* VHT 80 channel */
#define IEEE80211_CHAN_HT40INTOLMARK    0x0000000001000000 /* HT 40 Intolerant mark bit for ACS use */
#define IEEE80211_CHAN_BLOCKED          0x0000000002000000 /* channel temporarily blocked due to noise */
#define IEEE80211_CHAN_VHT160           0x0000000004000000 /* VHT 160 channel */
#define IEEE80211_CHAN_VHT80_80         0x0000000008000000 /* VHT 80_80 channel */
#define IEEE80211_CHAN_HE20             0x0000000010000000 /* HE 20 channel */
#define IEEE80211_CHAN_HE40PLUS         0x0000000020000000 /* HE 40 with extension channel above */
#define IEEE80211_CHAN_HE40MINUS        0x0000000040000000 /* HE 40 with extension channel below */
#define IEEE80211_CHAN_HE40INTOL        0x0000000080000000 /* HE 40 Intolerant */
#define IEEE80211_CHAN_HE40INTOLMARK    0x0000000100000000 /* HE 40 Intolerant mark bit for ACS use */
#define IEEE80211_CHAN_HE80             0x0000000200000000 /* HE 80 channel */
#define IEEE80211_CHAN_HE160            0x0000000400000000 /* HE 160 channel */
#define IEEE80211_CHAN_HE80_80          0x0000000800000000 /* HE 80_80 channel */
#define IEEE80211_CHAN_6GHZ             0x0000001000000000 /* 6 GHz spectrum channel */
/* Do not add anything here unrelated to PHY mode and width processing. */

/* flagext */
#define IEEE80211_CHAN_DFS_RADAR_FOUND  0x01
#define IEEE80211_CHAN_DFS              0x0002  /* DFS required on channel */
#define IEEE80211_CHAN_DFS_CFREQ2       0x0004  /* DFS required on channel for 2nd band of 80+80*/
#define IEEE80211_CHAN_DFS_CLEAR        0x0008  /* if channel has been checked for DFS */
#define IEEE80211_CHAN_11D_EXCLUDED     0x0010  /* excluded in 11D */
#define IEEE80211_CHAN_CSA_RECEIVED     0x0020  /* Channel Switch Announcement received on this channel */
#define IEEE80211_CHAN_DISALLOW_ADHOC   0x0040  /* ad-hoc is not allowed */
#define IEEE80211_CHAN_DISALLOW_HOSTAP  0x0080  /* Station only channel */
#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
#define IEEE80211_CHAN_HISTORY_RADAR    0x0100  /* DFS radar history for slave device(STA mode) */
#define IEEE80211_CHAN_CAC_VALID        0x0200  /* DFS CAC valid for  slave device(STA mode) */
#endif
#define IEEE80211_CHAN_PSC              0x0400  /* Channel is Prefered Scanning Channel */

/*
 * Useful combinations of channel characteristics.
 */
#define IEEE80211_CHAN_FHSS \
    (IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_GFSK)
#define IEEE80211_CHAN_A \
    (IEEE80211_CHAN_5GHZ | IEEE80211_CHAN_OFDM)
#define IEEE80211_CHAN_B \
    (IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_CCK)
#define IEEE80211_CHAN_PUREG \
    (IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_OFDM)
#define IEEE80211_CHAN_G \
    (IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_DYN)
#define IEEE80211_CHAN_108A \
    (IEEE80211_CHAN_5GHZ | IEEE80211_CHAN_OFDM | IEEE80211_CHAN_TURBO)
#define IEEE80211_CHAN_108G \
    (IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_OFDM | IEEE80211_CHAN_TURBO)
#define IEEE80211_CHAN_ST \
    (IEEE80211_CHAN_108A | IEEE80211_CHAN_STURBO)

#define IEEE80211_CHAN_ALL \
    (IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_5GHZ | IEEE80211_CHAN_6GHZ | IEEE80211_CHAN_GFSK | \
    IEEE80211_CHAN_CCK | IEEE80211_CHAN_OFDM | IEEE80211_CHAN_DYN | \
    IEEE80211_CHAN_HT20 | IEEE80211_CHAN_HT40PLUS | IEEE80211_CHAN_HT40MINUS | \
    IEEE80211_CHAN_VHT20 | IEEE80211_CHAN_VHT40PLUS | IEEE80211_CHAN_VHT40MINUS | IEEE80211_CHAN_VHT80 | IEEE80211_CHAN_VHT160 | IEEE80211_CHAN_VHT80_80 | \
    IEEE80211_CHAN_HE20 | IEEE80211_CHAN_HE40PLUS | IEEE80211_CHAN_HE40MINUS | \
    IEEE80211_CHAN_HE80 | IEEE80211_CHAN_HE160 | IEEE80211_CHAN_HE80_80 | \
    IEEE80211_CHAN_HALF | IEEE80211_CHAN_QUARTER)
#define IEEE80211_CHAN_ALLTURBO \
    (IEEE80211_CHAN_ALL | IEEE80211_CHAN_TURBO | IEEE80211_CHAN_STURBO)

#define IEEE80211_CHAN_HE_ALL \
    (IEEE80211_CHAN_HE20 | IEEE80211_CHAN_HE40PLUS | IEEE80211_CHAN_HE40MINUS | \
    IEEE80211_CHAN_HE80 | IEEE80211_CHAN_HE160 | IEEE80211_CHAN_HE80_80)

#define IEEE80211_IS_CHAN_FHSS(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_FHSS) == IEEE80211_CHAN_FHSS)
#define IEEE80211_IS_CHAN_A(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_A) == IEEE80211_CHAN_A)
#define IEEE80211_IS_CHAN_B(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_B) == IEEE80211_CHAN_B)
#define IEEE80211_IS_CHAN_PUREG(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_PUREG) == IEEE80211_CHAN_PUREG)
#define IEEE80211_IS_CHAN_G(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_G) == IEEE80211_CHAN_G)
#define IEEE80211_IS_CHAN_ANYG(_c) \
    (IEEE80211_IS_CHAN_PUREG(_c) || IEEE80211_IS_CHAN_G(_c))
#define IEEE80211_IS_CHAN_ST(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_ST) == IEEE80211_CHAN_ST)
#define IEEE80211_IS_CHAN_108A(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_108A) == IEEE80211_CHAN_108A)
#define IEEE80211_IS_CHAN_108G(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_108G) == IEEE80211_CHAN_108G)

#define IEEE80211_IS_CHAN_2GHZ(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_2GHZ) != 0)
#define IEEE80211_IS_CHAN_5GHZ(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_5GHZ) != 0)
#define IEEE80211_IS_CHAN_6GHZ(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_6GHZ) != 0)
#define IEEE80211_IS_CHAN_5GHZ_6GHZ(_c) \
    (((_c)->ic_flags & (IEEE80211_CHAN_5GHZ | IEEE80211_CHAN_6GHZ)) != 0)

#define IEEE80211_IS_CHAN_OFDM(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_OFDM) != 0)
#define IEEE80211_IS_CHAN_CCK(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_CCK) != 0)
#define IEEE80211_IS_CHAN_GFSK(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_GFSK) != 0)
#define IEEE80211_IS_CHAN_TURBO(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_TURBO) != 0)
#define IEEE80211_IS_CHAN_WEATHER_RADAR(_c) \
    ((((_c)->ic_freq >= 5600) && ((_c)->ic_freq <= 5650)) \
     || (((_c)->ic_flags & IEEE80211_CHAN_HT40PLUS) && (5580 == (_c)->ic_freq)))
#define IEEE80211_IS_CHAN_STURBO(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_STURBO) != 0)
#define IEEE80211_IS_CHAN_DTURBO(_c) \
    (((_c)->ic_flags & \
    (IEEE80211_CHAN_TURBO | IEEE80211_CHAN_STURBO)) == IEEE80211_CHAN_TURBO)
#define IEEE80211_IS_CHAN_HALF(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_HALF) != 0)
#define IEEE80211_IS_CHAN_QUARTER(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_QUARTER) != 0)
#define IEEE80211_IS_CHAN_PASSIVE(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_PASSIVE) != 0)

#define IEEE80211_IS_PRIMARY_OR_SECONDARY_CHAN_DFS(_c) \
    (IEEE80211_IS_CHAN_DFS(_c) ||       \
     ((IEEE80211_IS_CHAN_11AC_VHT160(_c) || \
       IEEE80211_IS_CHAN_11AC_VHT80_80(_c) || \
       IEEE80211_IS_CHAN_11AXA_HE160(_c) || \
       IEEE80211_IS_CHAN_11AXA_HE80_80(_c)) \
      && IEEE80211_IS_CHAN_DFS_CFREQ2(_c)))

#define IEEE80211_IS_CHAN_DFS(_c) \
    (((_c)->ic_flagext & (IEEE80211_CHAN_DFS|IEEE80211_CHAN_DFS_CLEAR)) == IEEE80211_CHAN_DFS)
#define IEEE80211_IS_CHAN_DFS_CFREQ2(_c) \
    (((_c)->ic_flagext & (IEEE80211_CHAN_DFS_CFREQ2|IEEE80211_CHAN_DFS_CLEAR)) == IEEE80211_CHAN_DFS_CFREQ2)
#define IEEE80211_IS_CHAN_DFSFLAG(_c) \
    (((_c)->ic_flagext & IEEE80211_CHAN_DFS) == IEEE80211_CHAN_DFS)
#define IEEE80211_IS_CHAN_DFSFLAG_CFREQ2(_c) \
    (((_c)->ic_flagext & IEEE80211_CHAN_DFS_CFREQ2) == IEEE80211_CHAN_DFS_CFREQ2)
#define IEEE80211_IS_CHAN_DISALLOW_ADHOC(_c) \
    (((_c)->ic_flagext & IEEE80211_CHAN_DISALLOW_ADHOC) != 0)
#define IEEE80211_IS_CHAN_11D_EXCLUDED(_c) \
    (((_c)->ic_flagext & IEEE80211_CHAN_11D_EXCLUDED) != 0)
#define IEEE80211_IS_CHAN_CSA(_c) \
    (((_c)->ic_flagext & IEEE80211_CHAN_CSA_RECEIVED) != 0)
#define IEEE80211_IS_CHAN_ODD(_c) \
    (((_c)->ic_freq == 5170) || ((_c)->ic_freq == 5190) || \
     ((_c)->ic_freq == 5210) || ((_c)->ic_freq == 5230))
#define IEEE80211_IS_CHAN_DISALLOW_HOSTAP(_c) \
    (((_c)->ic_flagext & IEEE80211_CHAN_DISALLOW_HOSTAP) != 0)
#define IEEE80211_IS_CHAN_PSC(_c) \
    (((_c)->ic_flagext & IEEE80211_CHAN_PSC) != 0)

#define IEEE80211_IS_CHAN_11NG_HT20(_c) \
    (IEEE80211_IS_CHAN_2GHZ((_c)) && (((_c)->ic_flags & (IEEE80211_CHAN_HT20)) != 0))
#define IEEE80211_IS_CHAN_11NA_HT20(_c) \
    (IEEE80211_IS_CHAN_5GHZ((_c)) && (((_c)->ic_flags & (IEEE80211_CHAN_HT20)) != 0))
#define IEEE80211_IS_CHAN_11NG_HT40PLUS(_c) \
    (IEEE80211_IS_CHAN_2GHZ((_c)) && (((_c)->ic_flags & (IEEE80211_CHAN_HT40PLUS)) != 0))
#define IEEE80211_IS_CHAN_11NG_HT40MINUS(_c) \
    (IEEE80211_IS_CHAN_2GHZ((_c)) && (((_c)->ic_flags & (IEEE80211_CHAN_HT40MINUS)) != 0))
#define IEEE80211_IS_CHAN_11NA_HT40PLUS(_c) \
    (IEEE80211_IS_CHAN_5GHZ((_c)) && (((_c)->ic_flags & (IEEE80211_CHAN_HT40PLUS)) != 0))
#define IEEE80211_IS_CHAN_11NA_HT40MINUS(_c) \
    (IEEE80211_IS_CHAN_5GHZ((_c)) && (((_c)->ic_flags & (IEEE80211_CHAN_HT40MINUS)) != 0))

#define IEEE80211_IS_CHAN_11N(_c) \
    (((_c)->ic_flags & (IEEE80211_CHAN_HT20 | IEEE80211_CHAN_HT40PLUS | IEEE80211_CHAN_HT40MINUS)) != 0)
#define IEEE80211_IS_CHAN_11N_HT20(_c) \
    (((_c)->ic_flags & (IEEE80211_CHAN_HT20)) != 0)
#define IEEE80211_IS_CHAN_11N_HT40(_c) \
    (((_c)->ic_flags & (IEEE80211_CHAN_HT40PLUS | IEEE80211_CHAN_HT40MINUS)) != 0)
#define IEEE80211_IS_CHAN_11NG(_c) \
    (IEEE80211_IS_CHAN_2GHZ((_c)) && IEEE80211_IS_CHAN_11N((_c)))
#define IEEE80211_IS_CHAN_11NA(_c) \
    (IEEE80211_IS_CHAN_5GHZ((_c)) && IEEE80211_IS_CHAN_11N((_c)))
#define IEEE80211_IS_CHAN_11N_HT40PLUS(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_HT40PLUS) != 0)
#define IEEE80211_IS_CHAN_11N_HT40MINUS(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_HT40MINUS) != 0)

#define IEEE80211_IS_CHAN_HT20_CAPABLE(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_HT20) == IEEE80211_CHAN_HT20)
#define IEEE80211_IS_CHAN_HT40PLUS_CAPABLE(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_HT40PLUS) == IEEE80211_CHAN_HT40PLUS)
#define IEEE80211_IS_CHAN_HT40MINUS_CAPABLE(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_HT40MINUS) == IEEE80211_CHAN_HT40MINUS)
#define IEEE80211_IS_CHAN_HT40_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_HT40PLUS_CAPABLE(_c) || IEEE80211_IS_CHAN_HT40MINUS_CAPABLE(_c))
#define IEEE80211_IS_CHAN_HT_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_HT20_CAPABLE(_c) || IEEE80211_IS_CHAN_HT40_CAPABLE(_c))
#define IEEE80211_IS_CHAN_11N_CTL_CAPABLE(_c)  IEEE80211_IS_CHAN_HT20_CAPABLE(_c)
#define IEEE80211_IS_CHAN_11N_CTL_U_CAPABLE(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_HT40PLUS) == IEEE80211_CHAN_HT40PLUS)
#define IEEE80211_IS_CHAN_11N_CTL_L_CAPABLE(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_HT40MINUS) == IEEE80211_CHAN_HT40MINUS)
#define IEEE80211_IS_CHAN_11N_CTL_40_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_11N_CTL_U_CAPABLE((_c)) || IEEE80211_IS_CHAN_11N_CTL_L_CAPABLE((_c)))


#define IEEE80211_IS_CHAN_VHT(_c) \
    (((_c)->ic_flags & (IEEE80211_CHAN_VHT20 | \
      IEEE80211_CHAN_VHT40PLUS | IEEE80211_CHAN_VHT40MINUS | IEEE80211_CHAN_VHT80 | IEEE80211_CHAN_VHT160 | IEEE80211_CHAN_VHT80_80)) != 0)
#define IEEE80211_IS_CHAN_11AC(_c) \
    ( IEEE80211_IS_CHAN_5GHZ((_c)) && IEEE80211_IS_CHAN_VHT((_c)) )

#define IEEE80211_IS_CHAN_11AC_VHT20(_c) \
    (IEEE80211_IS_CHAN_5GHZ((_c)) && (((_c)->ic_flags & (IEEE80211_CHAN_VHT20)) != 0))
#define IEEE80211_IS_CHAN_11AC_VHT40(_c) \
    (IEEE80211_IS_CHAN_5GHZ((_c)) && ((((_c)->ic_flags & (IEEE80211_CHAN_VHT40PLUS)) != 0) || ((_c)->ic_flags & (IEEE80211_CHAN_VHT40MINUS))))
#define IEEE80211_IS_CHAN_11AC_VHT40PLUS(_c) \
    (IEEE80211_IS_CHAN_5GHZ((_c)) && (((_c)->ic_flags & (IEEE80211_CHAN_VHT40PLUS)) != 0))
#define IEEE80211_IS_CHAN_11AC_VHT40MINUS(_c) \
    (IEEE80211_IS_CHAN_5GHZ((_c)) && (((_c)->ic_flags & (IEEE80211_CHAN_VHT40MINUS)) != 0))
#define IEEE80211_IS_CHAN_11AC_VHT80(_c) \
    (IEEE80211_IS_CHAN_5GHZ((_c)) && (((_c)->ic_flags & (IEEE80211_CHAN_VHT80)) != 0))
#define IEEE80211_IS_CHAN_11AC_VHT160(_c) \
    (IEEE80211_IS_CHAN_5GHZ((_c)) && (((_c)->ic_flags & (IEEE80211_CHAN_VHT160)) != 0))
#define IEEE80211_IS_CHAN_11AC_VHT80_80(_c) \
    (IEEE80211_IS_CHAN_5GHZ((_c)) && (((_c)->ic_flags & (IEEE80211_CHAN_VHT80_80)) != 0))

#define IEEE80211_IS_CHAN_HE(_c) \
    (((_c)->ic_flags & (IEEE80211_CHAN_HE20 | \
      IEEE80211_CHAN_HE40PLUS | IEEE80211_CHAN_HE40MINUS | \
      IEEE80211_CHAN_HE80 | IEEE80211_CHAN_HE160 |         \
      IEEE80211_CHAN_HE80_80)) != 0)

#define IEEE80211_IS_CHAN_11AX(_c) \
    ((IEEE80211_IS_CHAN_5GHZ_6GHZ((_c)) || IEEE80211_IS_CHAN_2GHZ((_c))) && \
     IEEE80211_IS_CHAN_HE((_c)))

#define IEEE80211_IS_CHAN_11AXG_HE20(_c) \
    (IEEE80211_IS_CHAN_2GHZ((_c)) && (((_c)->ic_flags & (IEEE80211_CHAN_HE20)) != 0))
#define IEEE80211_IS_CHAN_11AXA_HE20(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ((_c)) && (((_c)->ic_flags & (IEEE80211_CHAN_HE20)) != 0))
#define IEEE80211_IS_CHAN_11AXG_HE40PLUS(_c) \
    (IEEE80211_IS_CHAN_2GHZ((_c)) && (((_c)->ic_flags & (IEEE80211_CHAN_HE40PLUS)) != 0))
#define IEEE80211_IS_CHAN_11AXG_HE40MINUS(_c) \
    (IEEE80211_IS_CHAN_2GHZ((_c)) && (((_c)->ic_flags & (IEEE80211_CHAN_HE40MINUS)) != 0))
#define IEEE80211_IS_CHAN_11AXA_HE40PLUS(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ((_c)) && (((_c)->ic_flags & (IEEE80211_CHAN_HE40PLUS)) != 0))
#define IEEE80211_IS_CHAN_11AXA_HE40MINUS(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ((_c)) && (((_c)->ic_flags & (IEEE80211_CHAN_HE40MINUS)) != 0))
#define IEEE80211_IS_CHAN_11AXA_HE80(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ((_c)) && (((_c)->ic_flags & (IEEE80211_CHAN_HE80)) != 0))
#define IEEE80211_IS_CHAN_11AXA_HE160(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ((_c)) && (((_c)->ic_flags & (IEEE80211_CHAN_HE160)) != 0))
#define IEEE80211_IS_CHAN_11AXA_HE80_80(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ((_c)) && (((_c)->ic_flags & (IEEE80211_CHAN_HE80_80)) != 0))

#define IEEE80211_IS_CHAN_11AX_HE20(_c) \
    (((_c)->ic_flags & (IEEE80211_CHAN_HE20)) != 0)
#define IEEE80211_IS_CHAN_11AX_HE40(_c) \
    (((_c)->ic_flags & (IEEE80211_CHAN_HE40PLUS | IEEE80211_CHAN_HE40MINUS)) \
        != 0)
#define IEEE80211_IS_CHAN_11AXG(_c) \
    (IEEE80211_IS_CHAN_2GHZ((_c)) && IEEE80211_IS_CHAN_11AX((_c)))
#define IEEE80211_IS_CHAN_11AXA(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ((_c)) && IEEE80211_IS_CHAN_11AX((_c)))
#define IEEE80211_IS_CHAN_11AX_HE40PLUS(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_HE40PLUS) != 0)
#define IEEE80211_IS_CHAN_11AX_HE40MINUS(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_HE40MINUS) != 0)
#define IEEE80211_IS_CHAN_11AXG_HE40(_c) \
    (IEEE80211_IS_CHAN_2GHZ((_c)) && \
     ((((_c)->ic_flags & IEEE80211_CHAN_HE40PLUS) != 0) || ((_c)->ic_flags & IEEE80211_CHAN_HE40MINUS)))
#define IEEE80211_IS_CHAN_11AXA_HE40(_c) \
    (IEEE80211_IS_CHAN_5GHZ_6GHZ((_c)) && \
     ((((_c)->ic_flags & IEEE80211_CHAN_HE40PLUS) != 0) || ((_c)->ic_flags & IEEE80211_CHAN_HE40MINUS)))
#define IEEE80211_IS_CHAN_11AX_HE80(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_HE80) != 0)
#define IEEE80211_IS_CHAN_11AX_HE160(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_HE160) != 0)
#define IEEE80211_IS_CHAN_11AX_HE80_80(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_HE80_80) != 0)

#define IEEE80211_IS_CHAN_HE20_CAPABLE(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_HE20) == IEEE80211_CHAN_HE20)
#define IEEE80211_IS_CHAN_HE40PLUS_CAPABLE(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_HE40PLUS) == IEEE80211_CHAN_HE40PLUS)
#define IEEE80211_IS_CHAN_HE40MINUS_CAPABLE(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_HE40MINUS) == IEEE80211_CHAN_HE40MINUS)
#define IEEE80211_IS_CHAN_HE40_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_HE40PLUS_CAPABLE((_c)) || \
     IEEE80211_IS_CHAN_HE40MINUS_CAPABLE((_c)))
#define IEEE80211_IS_CHAN_HE80_CAPABLE(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_HE80) == IEEE80211_CHAN_HE80)
#define IEEE80211_IS_CHAN_HE160_CAPABLE(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_HE160) == IEEE80211_CHAN_HE160)
#define IEEE80211_IS_CHAN_HE80_80_CAPABLE(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_HE80_80) == IEEE80211_CHAN_HE80_80)

#define IEEE80211_IS_CHAN_HE_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_HE20_CAPABLE((_c)) ||  \
     IEEE80211_IS_CHAN_HE40_CAPABLE((_c)) ||  \
     IEEE80211_IS_CHAN_HE80_CAPABLE((_c)) ||  \
     IEEE80211_IS_CHAN_HE160_CAPABLE((_c)) || \
     IEEE80211_IS_CHAN_HE80_80_CAPABLE((_c)))

/* Check for HT20/VHT20/HE20 channel */
#define IEEE80211_IS_CHAN_HT20(_c)        \
    (IEEE80211_IS_CHAN_11NG_HT20(_c) ||   \
     IEEE80211_IS_CHAN_11NA_HT20(_c) ||   \
     IEEE80211_IS_CHAN_11AC_VHT20(_c) ||  \
     IEEE80211_IS_CHAN_11AXG_HE20(_c) ||  \
     IEEE80211_IS_CHAN_11AXA_HE20(_c))

/* Check for HT40/VHT40/HE40 PLUS/MINUS channel */
#define IEEE80211_IS_CHAN_HT40(_c)            \
    (IEEE80211_IS_CHAN_11NG_HT40PLUS(_c) ||   \
     IEEE80211_IS_CHAN_11NG_HT40MINUS(_c) ||  \
     IEEE80211_IS_CHAN_11NA_HT40PLUS(_c) ||   \
     IEEE80211_IS_CHAN_11NA_HT40MINUS(_c) ||  \
     IEEE80211_IS_CHAN_11AC_VHT40PLUS(_c) ||  \
     IEEE80211_IS_CHAN_11AC_VHT40MINUS(_c) || \
     IEEE80211_IS_CHAN_11AXG_HE40PLUS(_c) ||  \
     IEEE80211_IS_CHAN_11AXG_HE40MINUS(_c) || \
     IEEE80211_IS_CHAN_11AXA_HE40PLUS(_c) ||  \
     IEEE80211_IS_CHAN_11AXA_HE40MINUS(_c))

/* Check for HT40/VHT40/HE40 PLUS channel */
#define IEEE80211_IS_CHAN_HT40PLUS(_c)        \
    (IEEE80211_IS_CHAN_11NG_HT40PLUS(_c) ||   \
     IEEE80211_IS_CHAN_11NA_HT40PLUS(_c) ||   \
     IEEE80211_IS_CHAN_11AC_VHT40PLUS(_c) ||  \
     IEEE80211_IS_CHAN_11AXG_HE40PLUS(_c) ||  \
     IEEE80211_IS_CHAN_11AXA_HE40PLUS(_c))

/* Check for HT40/VHT40/HE40 MINUS channel */
#define IEEE80211_IS_CHAN_HT40MINUS(_c)       \
    (IEEE80211_IS_CHAN_11NG_HT40MINUS(_c) ||  \
     IEEE80211_IS_CHAN_11NA_HT40MINUS(_c) ||  \
     IEEE80211_IS_CHAN_11AC_VHT40MINUS(_c) || \
     IEEE80211_IS_CHAN_11AXG_HE40MINUS(_c) || \
     IEEE80211_IS_CHAN_11AXA_HE40MINUS(_c))

/* Check for VHT80/HE80 channel */
#define IEEE80211_IS_CHAN_VHT80(_c)         \
    (IEEE80211_IS_CHAN_11AC_VHT80(_c) ||    \
     IEEE80211_IS_CHAN_11AXA_HE80(_c))

/* Check for VHT160/HE160 channel */
#define IEEE80211_IS_CHAN_VHT160(_c)         \
    (IEEE80211_IS_CHAN_11AC_VHT160(_c) ||    \
     IEEE80211_IS_CHAN_11AXA_HE160(_c))

/* Check for VHT80_80/HE80_80 channel */
#define IEEE80211_IS_CHAN_VHT80_80(_c)       \
    (IEEE80211_IS_CHAN_11AC_VHT80_80(_c) ||  \
     IEEE80211_IS_CHAN_11AXA_HE80_80(_c))

#define IEEE80211_IS_CHAN_BW_80_80(_c)         \
    (IEEE80211_IS_CHAN_11AC_VHT80_80(_c) ||    \
     IEEE80211_IS_CHAN_11AXA_HE80_80(_c))

/* The below are based on equivalent 11n definitions.
 * 11AX TODO (Phase II) - Once more details emerge in the 802.11ax
 * specification, in case it turns out the below CTL based definitions are not
 * necessary, then remove.
 */
#define IEEE80211_IS_CHAN_11AX_CTL_CAPABLE(_c)  \
    IEEE80211_IS_CHAN_HE20_CAPABLE((_c))
#define IEEE80211_IS_CHAN_11AX_CTL_U_CAPABLE(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_HE40PLUS) == IEEE80211_CHAN_HE40PLUS)
#define IEEE80211_IS_CHAN_11AX_CTL_L_CAPABLE(_c) \
    (((_c)->ic_flags & IEEE80211_CHAN_HE40MINUS) == IEEE80211_CHAN_HE40MINUS)
#define IEEE80211_IS_CHAN_11AX_CTL_40_CAPABLE(_c) \
    (IEEE80211_IS_CHAN_11AX_CTL_U_CAPABLE((_c)) || \
     IEEE80211_IS_CHAN_11AX_CTL_L_CAPABLE((_c)))

#define IEEE80211_CH_HOPPING_SET_CHAN_BLOCKED(_c)    \
    ((_c)->ic_flags |= IEEE80211_CHAN_BLOCKED)
#define IEEE80211_CH_HOPPING_IS_CHAN_BLOCKED(_c)    \
    (((_c)->ic_flags & IEEE80211_CHAN_BLOCKED) == IEEE80211_CHAN_BLOCKED)
#define IEEE80211_CH_HOPPING_CLEAR_CHAN_BLOCKED(_c)    \
    ((_c)->ic_flags &= ~IEEE80211_CHAN_BLOCKED)
#define IEEE80211_IS_CHAN_RADAR(_c)    \
    (((_c)->ic_flags & IEEE80211_CHAN_DFS_RADAR) == IEEE80211_CHAN_DFS_RADAR)
#define IEEE80211_CHAN_SET_RADAR(_c)    \
    ((_c)->ic_flags |= IEEE80211_CHAN_DFS_RADAR)
#define IEEE80211_CHAN_CLR_RADAR(_c)    \
    ((_c)->ic_flags &= ~IEEE80211_CHAN_DFS_RADAR)
#define IEEE80211_CHAN_SET_DISALLOW_ADHOC(_c)   \
    ((_c)->ic_flagext |= IEEE80211_CHAN_DISALLOW_ADHOC)
#define IEEE80211_CHAN_SET_DISALLOW_HOSTAP(_c)   \
    ((_c)->ic_flagext |= IEEE80211_CHAN_DISALLOW_HOSTAP)
#define IEEE80211_CHAN_SET_DFS(_c)  \
    ((_c)->ic_flagext |= IEEE80211_CHAN_DFS)
#define IEEE80211_CHAN_SET_DFS_CLEAR(_c)  \
    ((_c)->ic_flagext |= IEEE80211_CHAN_DFS_CLEAR)
#define IEEE80211_CHAN_EXCLUDE_11D(_c)  \
    ((_c)->ic_flagext |= IEEE80211_CHAN_11D_EXCLUDED)

#if ATH_SUPPORT_DFS && ATH_SUPPORT_STA_DFS
#define IEEE80211_IS_CHAN_HISTORY_RADAR(_c)    \
    (((_c)->ic_flagext & IEEE80211_CHAN_HISTORY_RADAR) == IEEE80211_CHAN_HISTORY_RADAR)
#define IEEE80211_CHAN_SET_HISTORY_RADAR(_c)    \
    ((_c)->ic_flagext |= IEEE80211_CHAN_HISTORY_RADAR)
#define IEEE80211_CHAN_CLR_HISTORY_RADAR(_c)    \
    ((_c)->ic_flagext &= ~IEEE80211_CHAN_HISTORY_RADAR)

#define IEEE80211_IS_CHAN_CAC_VALID(_c)    \
    (((_c)->ic_flagext & IEEE80211_CHAN_CAC_VALID) == IEEE80211_CHAN_CAC_VALID)
#define IEEE80211_CHAN_SET_CAC_VALID(_c)    \
    ((_c)->ic_flagext |= IEEE80211_CHAN_CAC_VALID)
#define IEEE80211_CHAN_CLR_CAC_VALID(_c)    \
    ((_c)->ic_flagext &= ~IEEE80211_CHAN_CAC_VALID)
#endif

#endif /* ICM_RTR_DRIVER */

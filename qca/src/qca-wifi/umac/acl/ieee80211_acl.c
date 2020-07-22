/*
 * Copyright (c) 2017 Qualcomm Innovation Center, Inc.
 * All Rights Reserved
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * Copyright (c) 2010, Atheros Communications Inc.
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
 */


#include <ieee80211_var.h>
#include "ieee80211_ioctl.h"
#include <wlan_son_pub.h>

/*! \file ieee80211_acl.c
**  \brief 
**
** Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
** Copyright (c) 2004-2007 Atheros Communications, Inc.
**
 */

/*
 * IEEE 802.11 MAC ACL support.
 *
 * When this module is loaded the sender address of each received
 * frame is passed to the iac_check method and the module indicates
 * if the frame should be accepted or rejected.  If the policy is
 * set to ACL_POLICY_OPEN then all frames are accepted w/o checking
 * the address.  Otherwise, the address is looked up in the database
 * and if found the frame is either accepted (ACL_POLICY_ALLOW)
 * or rejected (ACL_POLICY_DENT).
 */

enum
{
    ACL_POLICY_OPEN             = 0,/* open, don't check ACL's */
    ACL_POLICY_ALLOW	        = 1,/* allow traffic from MAC */
    ACL_POLICY_DENY             = 2,/* deny traffic from MAC */
};

#define ACL_MAC_VALID_CHECK_TIMER       1000
#define ACL_TIMER_MAX_VALUE_LONG        ULONG_MAX

/* Macros to check if the flag passed belong to both ACL lists */
#define IS_PART_OF_BOTH_ACL_LIST (IEEE80211_ACL_FLAG_ACL_LIST_1 | IEEE80211_ACL_FLAG_ACL_LIST_2)

/* 
 * The ACL list is accessed from process context when ioctls are made to add,
 * delete mac entries or set/get policy (read/write operations). It is also 
 * accessed in tasklet context for read purposes only. Hence, we must use
 * spinlocks with DPCs disabled to protect this list. 
 * 
 * It may be noted that ioctls are serialized by the big kernel lock in Linux 
 * and so the process context code does not use mutual exclusion. It may not
 * be true for other OSes. In such cases, this code must be made safe for 
 * ioctl mutual exclusion. 
 */
struct ieee80211_acl_entry
{
    /* 
     * list element for linking on acl_list 
     */
    TAILQ_ENTRY(ieee80211_acl_entry)     ae_list; 
    /*
     * Parameters specific to this entry.
     * Parameters can be used for setting parameters for each peer
     * added to the ACL. This is useful when some more details in addition to
     * flags are needed for policy decisions with respect to the peer
     */
    u_int8_t                             ae_param_values[IEEE80211_ACL_PARAM_MAX];
    /* 
     * list element for linking on acl_hash list 
     */
    LIST_ENTRY(ieee80211_acl_entry)      ae_hash; 

    u_int8_t                             ae_macaddr[QDF_MAC_ADDR_SIZE];

    /*
     * Flags that indicate how the ACL should behave under other conditions
     * as needed by band steering (or potentially other modules in the
     * future).
     */
    u_int8_t                             ae_flags;
#if ATH_ACL_SOFTBLOCKING
#define ACL_SOFTBLOCKING_TIMESTAMP_INIT 0x0
    systime_t                            ae_last_req;
    qdf_timer_t                          ae_softblocking_timer;
#endif
    systime_t                            ae_mac_expiry_time;
};
struct ieee80211_acl
{
    osdev_t                              acl_osdev;
    spinlock_t                           acl_lock;
    int                                  acl_policy;              /* ACL policy for first ACL entry */
    int                                  acl_policy_sec;          /* ACL policy for secondary ACL entry */
    TAILQ_HEAD(, ieee80211_acl_entry)    acl_list;                /* List of all ACL entries */
    ATH_LIST_HEAD(, ieee80211_acl_entry) acl_hash[ACL_HASHSIZE];
    qdf_timer_t                           acl_mac_expiry_timer;
};

/* 
 * simple hash is enough for variation of macaddr 
 */
#define	ACL_HASH(addr)	\
    (((const u_int8_t *)(addr))[QDF_MAC_ADDR_SIZE - 1] % ACL_HASHSIZE)

static void acl_free_all_locked(ieee80211_acl_t acl, u_int8_t acl_list_id);
static int ieee80211_acl_check_list(struct ieee80211_acl_entry *entry, u_int8_t policy,
                                                enum ieee80211_acl_flag acl_list_id);
static void _acl_free(ieee80211_acl_t acl, struct ieee80211_acl_entry *entry);
#if ATH_ACL_SOFTBLOCKING
static int ieee80211_acl_softblocking(wlan_if_t vap, struct ieee80211_acl_entry *entry);
#endif

static OS_TIMER_FUNC(_acl_mac_validity) {
    ieee80211_acl_t acl;
    struct ieee80211_acl_entry *entry, *next_entry;
    struct ieee80211vap *vap = NULL;
    systime_t now;
    OS_GET_TIMER_ARG(vap, struct ieee80211vap *);

    if (vap && !ieee80211_vap_deleted_is_set(vap)) {
        acl = vap->iv_acl;
        if (!acl) return;

        spin_lock_dpc(&acl->acl_lock);
        entry = TAILQ_FIRST(&acl->acl_list);
        do {
            if (!entry) break;
            next_entry = TAILQ_NEXT(entry, ae_list);

            if (entry && (entry->ae_flags & IEEE80211_ACL_FLAG_VALIDITY_TIMER) ==
                             IEEE80211_ACL_FLAG_VALIDITY_TIMER) {
                now = OS_GET_TIMESTAMP();
                if (now - entry->ae_mac_expiry_time < (ACL_TIMER_MAX_VALUE_LONG / 2)) {
                    /* Clear the flag corresponding to that ACL list */
                    entry->ae_flags &= ~IEEE80211_ACL_FLAG_ACL_LIST_1;

                    /* If both the lists don't contain the macaddress, then remove it */
                    if (!(entry->ae_flags & IS_PART_OF_BOTH_ACL_LIST)) {
                        _acl_free(acl, entry);
                    }
                }
            }
            entry = next_entry;
        } while (next_entry != NULL);
        spin_unlock_dpc(&acl->acl_lock);
        qdf_timer_mod(&vap->iv_acl->acl_mac_expiry_timer, ACL_MAC_VALID_CHECK_TIMER);
    }

}

int ieee80211_acl_attach(wlan_if_t vap)
{
    ieee80211_acl_t acl;

    if (vap->iv_acl)
        return EOK; /* already attached */

#if ATH_ACL_SOFTBLOCKING
    vap->iv_softblock_wait_time = SOFTBLOCK_WAIT_TIME_DEFAULT;
    vap->iv_softblock_allow_time = SOFTBLOCK_ALLOW_TIME_DEFAULT;
#endif

    acl = (ieee80211_acl_t) OS_MALLOC(vap->iv_ic->ic_osdev, 
                                sizeof(struct ieee80211_acl), 0);
    if (acl) {
        OS_MEMZERO(acl, sizeof(struct ieee80211_acl));
        acl->acl_osdev  = vap->iv_ic->ic_osdev;
        vap->iv_acl = acl;

        spin_lock_init(&acl->acl_lock);
        TAILQ_INIT(&acl->acl_list);
        acl->acl_policy = ACL_POLICY_OPEN;
        qdf_timer_init(vap->iv_ic->ic_osdev, &vap->iv_acl->acl_mac_expiry_timer, _acl_mac_validity,
                      vap, QDF_TIMER_TYPE_WAKE_APPS);
        qdf_timer_mod(&vap->iv_acl->acl_mac_expiry_timer, ACL_MAC_VALID_CHECK_TIMER);
        return EOK;
    }

    return ENOMEM;
}

int ieee80211_acl_detach(wlan_if_t vap)
{
    ieee80211_acl_t acl;

    if (vap->iv_acl == NULL)
        return EINPROGRESS; /* already detached or never attached */

    qdf_timer_free(&vap->iv_acl->acl_mac_expiry_timer);
    acl = vap->iv_acl;
    acl_free_all_locked(acl, IS_PART_OF_BOTH_ACL_LIST);

    spin_lock_destroy(&acl->acl_lock);

    OS_FREE(acl);

    vap->iv_acl = NULL;

    return EOK;
}

static __inline struct ieee80211_acl_entry * 
_find_acl(ieee80211_acl_t acl, const u_int8_t *macaddr)
{
    struct ieee80211_acl_entry *entry;
    int hash;

    hash = ACL_HASH(macaddr);
    LIST_FOREACH(entry, &acl->acl_hash[hash], ae_hash) {
        if (IEEE80211_ADDR_EQ(entry->ae_macaddr, macaddr))
            return entry;
    }
    return NULL;
}

/* 
 * This function is always called from tasklet context and it may be noted
 * that the same tasklet is not scheduled on more than one CPU at the same 
 * time. The user context functions that modify the ACL use spin_lock_dpc 
 * which disable softIrq on the current CPU. However, a softIrq scheduled 
 * on another CPU could execute the rx tasklet. Hence, protection is needed 
 * here. spinlock is sufficient as it disables kernel preemption and if the 
 * user task is accessing this list, the rx tasklet will wait until the user 
 * task releases the spinlock. The original code didn't use any protection.
 */
int 
ieee80211_acl_check(wlan_if_t vap, const u_int8_t mac[QDF_MAC_ADDR_SIZE])
{
    struct ieee80211_acl_entry *entry;
    ieee80211_acl_t acl = vap->iv_acl;
    int allow_acl = 0, allow_acl_sec = 0;

    if (acl == NULL) return 1;

    /* EV : 89216
     * WPS2.0 : Ignore MAC Address Filtering if WPS Enabled
     * Display the message.
     * return 1 to report success
     */
    if(vap->iv_wps_mode) {
        /* Only disallow ACL while not using band steering
           and if its not a public vap when ssid steering is enabled */
#if (QCA_SUPPORT_SSID_STEERING && QCA_SUPPORT_SON)
        if ((!wlan_son_is_vdev_enabled(vap->vdev_obj)) &&
            (!son_get_ssid_steering_vdev_is_pub(vap->vdev_obj))) {
            IEEE80211_DPRINTF(vap, IEEE80211_MSG_ACL, "WPS Enabled : Ignoring MAC Filtering\n");
            return 1;
        }
#endif
    }

    /* If both ACL policies are open mode, then return 1 */
    if (!acl->acl_policy && !acl->acl_policy_sec)
        return 1;

    spin_lock(&acl->acl_lock);
    entry = _find_acl(acl, mac);
    spin_unlock(&acl->acl_lock);

    /* If the entry is permitted by both ACL lists, then return 1 */
    allow_acl = ieee80211_acl_check_list(entry,
                        acl->acl_policy, IEEE80211_ACL_FLAG_ACL_LIST_1);
    if(allow_acl == 1)
        allow_acl_sec = ieee80211_acl_check_list(entry,
                        acl->acl_policy_sec, IEEE80211_ACL_FLAG_ACL_LIST_2);

#if ATH_ACL_SOFTBLOCKING
    if ((acl->acl_policy == ACL_POLICY_DENY || acl->acl_policy_sec == ACL_POLICY_DENY)
        // softblocking entry can be in either list.
        && entry && (entry->ae_flags & IEEE80211_ACL_FLAG_SOFTBLOCKING)
        && (allow_acl_sec == 0)) {
        allow_acl_sec = ieee80211_acl_softblocking(vap, entry);
    }
#endif
    return allow_acl_sec;

}

#if ATH_ACL_SOFTBLOCKING
static OS_TIMER_FUNC(_acl_softblocking_cleanup)
{
    struct ieee80211_acl_entry *entry;
    OS_GET_TIMER_ARG(entry, struct ieee80211_acl_entry *);

    entry->ae_last_req = ACL_SOFTBLOCKING_TIMESTAMP_INIT;
}

int ieee80211_acl_softblocking(wlan_if_t vap, struct ieee80211_acl_entry *entry)
{
    int ret = 0;
    unsigned int wait_time = vap->iv_softblock_wait_time, allow_time = vap->iv_softblock_allow_time;
    systime_t now = OS_GET_TIMESTAMP();

    if (now == ACL_SOFTBLOCKING_TIMESTAMP_INIT) {
        now++; // Skip the initial value
    }

    if (allow_time == 0) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ACL,
            "No grace period implemented for soft blocking\n");
    }
    else if (entry->ae_last_req == ACL_SOFTBLOCKING_TIMESTAMP_INIT) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ACL,
            "Got the first req. Start timer, now: %lu\n", now);
        entry->ae_last_req = now;
        qdf_timer_stop(&entry->ae_softblocking_timer);
        qdf_timer_mod(&entry->ae_softblocking_timer, (wait_time+allow_time));
    }
    else if (qdf_system_time_before(now,
                (entry->ae_last_req + CONVERT_MS_TO_SYSTEM_TIME(wait_time)))) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ACL,
            "In Waiting Time until timestamp %lu, now: %lu\n",
            entry->ae_last_req+CONVERT_MS_TO_SYSTEM_TIME(wait_time), now);
    }
    else if (qdf_system_time_before(now,
                (entry->ae_last_req + CONVERT_MS_TO_SYSTEM_TIME(wait_time+allow_time)))) {
        IEEE80211_DPRINTF(vap, IEEE80211_MSG_ACL,
            "In Allow Time until timestamp %lu, now: %lu\n",
            entry->ae_last_req+CONVERT_MS_TO_SYSTEM_TIME(wait_time+allow_time), now);
        ret = 1;
    }
    return ret;
}
#endif

int ieee80211_acl_check_list(struct ieee80211_acl_entry *entry, u_int8_t policy,
                                                enum ieee80211_acl_flag acl_list_id) {

    int allow_entry = 0;

   /* -> If the ACL policy is OPEN, then return 1.
    * -> If the ACL policy is ALLOW, then return 1 if the entry is part
    *    of the given ACL list.
    * -> If the ACL policy is DENY, then return 1 if the entry is not part
    *    of the given ACL list.
    */
    switch (policy) {
        case ACL_POLICY_OPEN:
            allow_entry = 1;
            break;
        case ACL_POLICY_ALLOW:
            if(entry != NULL)
                allow_entry = !!(entry->ae_flags & acl_list_id);
            break;
        case ACL_POLICY_DENY:
            if(entry != NULL)
                allow_entry = !(entry->ae_flags & acl_list_id);
            else
                allow_entry = 1;
            break;
    }
    return allow_entry;
}

#if defined(QCA_SUPPORT_SON) || defined(ATH_ACL_SOFTBLOCKING)
/**
 * @brief Determine if the flag is set on the ACL entry or not.
 *
 * @param [in] vap  the interface on which to check
 * @param [in] mac  the MAC address of the entry to check
 * @param [in] flag  the flag that is being checked; multiple flags can
 *                   be checked (with all having to be set)
 *
 * @return 1 if the flag is set; otherwise 0
 */
int
ieee80211_acl_flag_check(wlan_if_t vap, const u_int8_t mac[QDF_MAC_ADDR_SIZE],
                         enum ieee80211_acl_flag flag)
{
    ieee80211_acl_t acl;
    struct ieee80211_acl_entry *entry;
    int retval = 0;

    if (!vap) return 0;

    acl = vap->iv_acl;
    if (acl == NULL) return 0;

    spin_lock(&acl->acl_lock);
    entry = _find_acl(acl, mac);
    if (entry &&
        (entry->ae_flags & flag) == flag) {
        retval = 1;
    }
    spin_unlock(&acl->acl_lock);

    return retval;
}

/**
 * @brief Enable the flag on the ACL entry.
 *
 * @param [in] vap  the interface on which to manipulate an ACL entry
 * @param [in] mac  the MAC address of the entry to change
 * @param [in] flag  the flag (or flags) to set
 *
 * @return EOK on success; ENOENT if the entry cannot be found
 */
int
ieee80211_acl_set_flag(wlan_if_t vap, const u_int8_t mac[QDF_MAC_ADDR_SIZE],
                       enum ieee80211_acl_flag flag)
{
    ieee80211_acl_t acl = vap->iv_acl;
    struct ieee80211_acl_entry *entry;
    int retval = -ENOENT;
    if (acl) {
        spin_lock(&acl->acl_lock);
        entry = _find_acl(acl, mac);
        if (entry) {
            entry->ae_flags |= flag;
            retval = EOK;
        }
        spin_unlock(&acl->acl_lock);
    }
    return retval;
}

/**
 * @brief Set parameter value in an ACL entry
 *
 * @param [in]  vap        the interface on which to manipulate an ACL entry
 * @param [in]  mac        the MAC address of the entry to change
 * @param [in]  flag       the flag (or flags) to set
 * @param [in]  params     the array of parameters for which values are needed
 * @param [in]  vals       values for the above specified parameters
 * @param [in]  num_params number of parameters in the 'params' and 'values'
 *                         array

 * @return EOK on success;
 *         EINVAL if one of the parameters are invalid or no entry is found.
 */
int
ieee80211_acl_set_params_and_flag(wlan_if_t vap,
                                 const u_int8_t mac[QDF_MAC_ADDR_SIZE],
                                 enum ieee80211_acl_flag flag,
                                 u_int8_t params[],
                                 u_int8_t vals[],
                                 u_int8_t num_params)
{
    struct ieee80211_acl_entry  *entry;
    ieee80211_acl_t             acl = vap->iv_acl;
    int                         retval = -EINVAL;

    if (acl) {
        spin_lock(&acl->acl_lock);
        entry = _find_acl(acl, mac);
        if (entry) {
            int i;

            retval = EOK;
            entry->ae_flags |= flag;
            for (i = 0; i < num_params; i++) {
                if (params[i] < 0 || params[i] >= IEEE80211_ACL_PARAM_MAX) {
                    QDF_TRACE(QDF_MODULE_ID_ACL, QDF_TRACE_LEVEL_ERROR,
                              "%s: ERROR! Invalid ACL parameter %d\n",
                              __func__, params[i]);
                    retval = -EINVAL;
                    break;
                }
                entry->ae_param_values[params[i]] = vals[i];
            }
        }
        spin_unlock(&acl->acl_lock);
    }

    return retval;
}

/**
 * @brief Get parameter value in an ACL entry provided a given flag is set
 *
 * @param [in]  vap        the interface on which to get the ACL entry
 * @param [in]  mac        the MAC address for the ACL entry
 * @param [in]  flag       the flag (or flags) to check
 * @param [in]  params     the array of parameters for which values are needed
 * @param [out] vals       placeholder for copying the values of the above
 *                         specified parameters
 * @param [in]  num_params number of parameters in the 'params' and 'values'
 *                         array
 *
 * @return 0 if flag is not set
 *         1 if the flag is set
 *         -ENOENT if the ACL entry was not found or
 *         -EINVAL if one of the parameters are invalid or
 *                 if no ACL databse found
 */
int
ieee80211_acl_get_params_if_flag(wlan_if_t vap,
                                const u_int8_t mac[QDF_MAC_ADDR_SIZE],
                                enum ieee80211_acl_flag flag,
                                u_int8_t params[],
                                u_int8_t *vals[],
                                u_int8_t num_params)
{
    struct ieee80211_acl_entry  *entry;
    ieee80211_acl_t             acl = vap->iv_acl;
    int                         retval;

    if (!acl) {
        QDF_TRACE(QDF_MODULE_ID_ACL, QDF_TRACE_LEVEL_ERROR,
                        "%s: ERROR! No ACL database found\n", __func__);
        return -EINVAL;
    }

    spin_lock(&acl->acl_lock);
    entry = _find_acl(acl, mac);
    if (entry == NULL) {

        QDF_TRACE(QDF_MODULE_ID_ACL,
                  QDF_TRACE_LEVEL_DEBUG,
                  "%s: ERROR! No ACL entry found for mac %pM",
                  __func__, mac);
        retval = -ENOENT;
        goto get_params_error;
    }

    if ((entry->ae_flags & flag) == flag) {
        int i;

        retval = 1;
        for (i = 0; i < num_params; i++) {
            if (params[i] < 0 || params[i] >= IEEE80211_ACL_PARAM_MAX) {
                QDF_TRACE(QDF_MODULE_ID_ACL, QDF_TRACE_LEVEL_ERROR,
                        "%s: ERROR! Invalid ACL parameter %d\n",
                        __func__, params[i]);
                retval = -EINVAL;
                break;
            }
            *(vals[i]) = entry->ae_param_values[params[i]];
        }
    } else {
        retval = 0;
    }
get_params_error:
    spin_unlock(&acl->acl_lock);
    return retval;
}

/**
 * @brief Disable the flag on the ACL entry.
 *
 * @param [in] vap  the interface on which to manipulate an ACL entry
 * @param [in] mac  the MAC address of the entry to change
 * @param [in] flag  the flag (or flags) to clear
 *
 * @return EOK on success; ENOENT if the entry cannot be found
 */
int
ieee80211_acl_clr_flag(wlan_if_t vap, const u_int8_t mac[QDF_MAC_ADDR_SIZE],
                       enum ieee80211_acl_flag flag)
{
    ieee80211_acl_t acl = vap->iv_acl;
    struct ieee80211_acl_entry *entry;
    int retval = -ENOENT;

    if (acl) {
        spin_lock(&acl->acl_lock);
        entry = _find_acl(acl, mac);
        if (entry) {
            entry->ae_flags &= ~flag;
            retval = EOK;
        }
        spin_unlock(&acl->acl_lock);
    }

    return retval;
}
#endif /* QCA_SUPPORT_SON || ATH_ACL_SOFTBLOCKING */

/* 
 * The ACL list is modified when in user context and the list needs to be 
 * protected from rx tasklet. Using spin_lock alone won't be sufficient as
 * that only disables task pre-emption and not irq or softIrq preemption.
 * Hence, effective protection is possible only by disabling softIrq on
 * local CPU and spin_lock_bh needs to be used.
 */
int 
ieee80211_acl_add(wlan_if_t vap, const u_int8_t mac[QDF_MAC_ADDR_SIZE], u_int8_t acl_list_id)
{
    ieee80211_acl_t acl = vap->iv_acl;
    struct ieee80211_acl_entry *entry, *new;
    int hash, rc;

    if (acl == NULL) {
        rc = ieee80211_acl_attach(vap);
        if (rc != EOK) return rc;
        acl = vap->iv_acl;
    }

    hash = ACL_HASH(mac);
    spin_lock_bh(&acl->acl_lock);
    LIST_FOREACH(entry, &acl->acl_hash[hash], ae_hash) {
        if (IEEE80211_ADDR_EQ(entry->ae_macaddr, mac)) {
           /* return EEXIST only if mac is part of the same logical ACL list,
            * that is primary or secondary ACL. Otherwise we want to
            * update the flags field and return OK.
            */
            if (!(entry->ae_flags & acl_list_id)) {
                entry->ae_flags |= acl_list_id;
                spin_unlock_bh(&acl->acl_lock);
                return 0;
            }
            spin_unlock_bh(&acl->acl_lock);
            return EEXIST;
        }
    }
    new = (struct ieee80211_acl_entry *) OS_MALLOC(acl->acl_osdev,
                                              sizeof(struct ieee80211_acl_entry), 0);
    if (new == NULL)
        return ENOMEM;

    IEEE80211_ADDR_COPY(new->ae_macaddr, mac);
    new->ae_flags = acl_list_id;
#if ATH_ACL_SOFTBLOCKING
    new->ae_last_req = ACL_SOFTBLOCKING_TIMESTAMP_INIT;
    qdf_timer_init(vap->iv_ic->ic_osdev, &new->ae_softblocking_timer,
            _acl_softblocking_cleanup, new, QDF_TIMER_TYPE_WAKE_APPS);
#endif
    TAILQ_INSERT_TAIL(&acl->acl_list, new, ae_list);
    LIST_INSERT_HEAD(&acl->acl_hash[hash], new, ae_hash);
    spin_unlock_bh(&acl->acl_lock);

    return 0;
}

static void
_acl_free(ieee80211_acl_t acl, struct ieee80211_acl_entry *entry)
{
    TAILQ_REMOVE(&acl->acl_list, entry, ae_list);
    LIST_REMOVE(entry, ae_hash);
#if ATH_ACL_SOFTBLOCKING
    qdf_timer_stop(&entry->ae_softblocking_timer);
    qdf_timer_free(&entry->ae_softblocking_timer);
#endif
    OS_FREE(entry);
}

int 
ieee80211_acl_remove(wlan_if_t vap, const u_int8_t mac[QDF_MAC_ADDR_SIZE], u_int8_t acl_list_id)
{
    ieee80211_acl_t acl = vap->iv_acl;
    struct ieee80211_acl_entry *entry;

    if (acl == NULL) return EINVAL;

    spin_lock_dpc(&acl->acl_lock);
    entry = _find_acl(acl, mac);
    if (entry != NULL) {
        /* Clear the flag corresponding to that ACL list */
        entry->ae_flags &= ~acl_list_id;

        /* If both the lists don't contain the macaddress, then remove it */
        if (!(entry->ae_flags & IS_PART_OF_BOTH_ACL_LIST))
            _acl_free(acl, entry);
    }
    spin_unlock_dpc(&acl->acl_lock);

    return (entry == NULL ? ENOENT : 0);
}

int 
ieee80211_acl_get(wlan_if_t vap, u_int8_t *mac_list, int len, int *num_mac, u_int8_t acl_list_id)
{
    ieee80211_acl_t acl = vap->iv_acl;
    struct ieee80211_acl_entry *entry;
	int rc;

    if (acl == NULL) {
        rc = ieee80211_acl_attach(vap);
        if (rc != EOK) return rc;
        acl = vap->iv_acl;
    }

    if ((mac_list == NULL) || (!len)) {
        return ENOMEM;
	}

    *num_mac = 0;

    spin_lock_dpc(&acl->acl_lock);
    TAILQ_FOREACH(entry, &acl->acl_list, ae_list) {
        len -= QDF_MAC_ADDR_SIZE;
        if (len < 0) {
            spin_unlock_dpc(&acl->acl_lock);
            return E2BIG;
        }
        if (entry->ae_flags & acl_list_id) {
            IEEE80211_ADDR_COPY(&(mac_list[*num_mac*QDF_MAC_ADDR_SIZE]), entry->ae_macaddr);
            (*num_mac)++;
        }
    }
    spin_unlock_dpc(&acl->acl_lock);

    return 0;
}

static void
acl_free_all_locked(ieee80211_acl_t acl, u_int8_t acl_list_id)
{
    struct ieee80211_acl_entry *entry, *next_entry;

    spin_lock_dpc(&acl->acl_lock); 
    entry = TAILQ_FIRST(&acl->acl_list);
    do {
            if (!entry)
                break;
            /*
             * If mac entry is present in the list, then clear that coresponding ACL
             * list flag. If that mac entry is not used by any ACL lists, free it
             * or iterate to the next mac entry in the list.
             */
            entry->ae_flags &= ~acl_list_id;
            next_entry = TAILQ_NEXT(entry, ae_list);
            if (!(entry->ae_flags & IS_PART_OF_BOTH_ACL_LIST))
               _acl_free(acl, entry);
            entry = next_entry;
    } while (next_entry != NULL);
    spin_unlock_dpc(&acl->acl_lock);
}

int ieee80211_acl_flush(wlan_if_t vap, u_int8_t acl_list_id)
{
    ieee80211_acl_t acl = vap->iv_acl;
    if (acl == NULL) return EINVAL;
    acl_free_all_locked(acl, acl_list_id);
    return 0;
}

int ieee80211_acl_setpolicy(wlan_if_t vap, int policy, u_int8_t acl_list_id)
{
    ieee80211_acl_t acl = vap->iv_acl;
    int rc;

    if (acl == NULL) {
        rc = ieee80211_acl_attach(vap);
        if (rc != EOK) return rc;
        acl = vap->iv_acl;
    }
    switch (policy)
    {
        case IEEE80211_MACCMD_POLICY_OPEN:
            if (acl_list_id == IEEE80211_ACL_FLAG_ACL_LIST_1)
                acl->acl_policy = ACL_POLICY_OPEN;
            else
                acl->acl_policy_sec = ACL_POLICY_OPEN;
            break;
        case IEEE80211_MACCMD_POLICY_ALLOW:
            if (acl_list_id == IEEE80211_ACL_FLAG_ACL_LIST_1)
                acl->acl_policy = ACL_POLICY_ALLOW;
            else
                acl->acl_policy_sec = ACL_POLICY_ALLOW;
            break;
        case IEEE80211_MACCMD_POLICY_DENY:
            if (acl_list_id == IEEE80211_ACL_FLAG_ACL_LIST_1)
                acl->acl_policy = ACL_POLICY_DENY;
            else
                acl->acl_policy_sec = ACL_POLICY_DENY;
            break;
        default:
            return EINVAL;
    }
    return 0;
}

int ieee80211_acl_getpolicy(wlan_if_t vap, u_int8_t acl_list_id)
{
    ieee80211_acl_t acl = vap->iv_acl;
    int rc;
    
    if (acl == NULL) {
        rc = ieee80211_acl_attach(vap);
        if (rc != EOK) return rc;
        acl = vap->iv_acl;
    }

    if (acl == NULL) return EINVAL;
    if (acl_list_id == IEEE80211_ACL_FLAG_ACL_LIST_1)
        return acl->acl_policy;
    else
        return acl->acl_policy_sec;
}

int wlan_set_acl_policy(wlan_if_t vap, int policy, u_int8_t acl_list_id)
{
    switch (policy) {
    case IEEE80211_MACCMD_POLICY_OPEN:
    case IEEE80211_MACCMD_POLICY_ALLOW:
    case IEEE80211_MACCMD_POLICY_DENY:
        ieee80211_acl_setpolicy(vap, policy, acl_list_id);
        break;
    case IEEE80211_MACCMD_FLUSH:
        ieee80211_acl_flush(vap, acl_list_id);
        break;
    case IEEE80211_MACCMD_DETACH:
        ieee80211_acl_detach(vap);
        break;
    }    

    return 0;
}

int wlan_get_acl_policy(wlan_if_t vap, u_int8_t acl_list_id)
{
    return ieee80211_acl_getpolicy(vap, acl_list_id);
}

int wlan_set_acl_add(wlan_if_t vap, const u_int8_t mac[QDF_MAC_ADDR_SIZE], u_int8_t acl_list_id)
{
    return ieee80211_acl_add(vap, mac, acl_list_id);
}

int wlan_set_acl_remove(wlan_if_t vap, const u_int8_t mac[QDF_MAC_ADDR_SIZE], u_int8_t acl_list_id)
{
    return ieee80211_acl_remove(vap, mac, acl_list_id);
}

int wlan_get_acl_list(wlan_if_t vap, u_int8_t *mac_list, int len, int *num_mac, u_int8_t acl_list_id)
{
    return ieee80211_acl_get(vap, mac_list, len, num_mac, acl_list_id);
}

int wlan_set_acl_block_mgmt(wlan_if_t vap,
                            const u_int8_t mac[QDF_MAC_ADDR_SIZE], bool flag)
{
    int ret, i, num_mac;
    u_int8_t *mac_list;

    if (wlan_get_acl_policy(vap, IEEE80211_ACL_FLAG_ACL_LIST_1) !=
                            IEEE80211_MACCMD_POLICY_DENY) {
        qdf_err("Config valid only when ACL policy DENY\n");
        return EFAULT;
    }

    if (IEEE80211_IS_BROADCAST(mac)) {
        /* flag to be set/cleared for all entries in ACL */
        mac_list = (uint8_t *)qdf_mem_malloc(QDF_MAC_ADDR_SIZE * ACL_ENTRY_MAX);

        if (!mac_list)
            return EFAULT;

        wlan_get_acl_list(vap, mac_list, (QDF_MAC_ADDR_SIZE * ACL_ENTRY_MAX),
                          &num_mac, IEEE80211_ACL_FLAG_ACL_LIST_1);
        if (!num_mac) {
            qdf_err("Config valid for non-empty ACL\n");
            qdf_mem_free(mac_list);
            return EFAULT;
        }

        for (i = 0; i < num_mac; i++) {
            if (flag)
                ieee80211_acl_set_flag(vap, &mac_list[i * QDF_MAC_ADDR_SIZE],
                                       IEEE80211_ACL_FLAG_BLOCK_MGMT);
             else
                ieee80211_acl_clr_flag(vap, &mac_list[i * QDF_MAC_ADDR_SIZE],
                                       IEEE80211_ACL_FLAG_BLOCK_MGMT);
        }

        qdf_mem_free(mac_list);
        return EOK;
    }

    if (flag)
        ret = ieee80211_acl_set_flag(vap, mac, IEEE80211_ACL_FLAG_BLOCK_MGMT);
    else
        ret = ieee80211_acl_clr_flag(vap, mac, IEEE80211_ACL_FLAG_BLOCK_MGMT);

    if (ret)
        qdf_err("Config valid only when address already part of ACL\n");

    return ret;
}

bool wlan_acl_is_block_mgmt_set(wlan_if_t vap,
                                const u_int8_t mac_addr[QDF_MAC_ADDR_SIZE])
{
    return ieee80211_acl_flag_check(vap, mac_addr,
                                    IEEE80211_ACL_FLAG_BLOCK_MGMT);
}

int ieee80211_acl_add_with_validity(struct ieee80211vap *vap, const u_int8_t *mac_addr,
                         u_int16_t validity_timer) {
    ieee80211_acl_t acl = vap->iv_acl;
    struct ieee80211_acl_entry *entry;

    if (ieee80211_acl_add(vap, mac_addr, IEEE80211_ACL_FLAG_ACL_LIST_1) == ENOMEM) {
        return ENOMEM;
    }

    spin_lock(&acl->acl_lock);
    entry = _find_acl(acl, mac_addr);
    if (entry) {
        systime_t now = OS_GET_TIMESTAMP();
        entry->ae_flags |= IEEE80211_ACL_FLAG_VALIDITY_TIMER;
        entry->ae_mac_expiry_time = now + CONVERT_SEC_TO_SYSTEM_TIME(validity_timer);
    }
    spin_unlock(&acl->acl_lock);
    return 0;
}

#if ATH_ACL_SOFTBLOCKING
int wlan_acl_set_softblocking(struct ieee80211vap *vap,
                                     const u_int8_t *mac_addr, bool enable)
{
    int ret = 0;

    if (enable) {
        ret = ieee80211_acl_set_flag(vap, mac_addr, IEEE80211_ACL_FLAG_SOFTBLOCKING);
    } else {
        ret = ieee80211_acl_clr_flag(vap, mac_addr, IEEE80211_ACL_FLAG_SOFTBLOCKING);
    }

    if (ret == EOK) {
        ieee80211_acl_t acl = vap->iv_acl;
        struct ieee80211_acl_entry *entry;
        spin_lock(&acl->acl_lock);
        entry = _find_acl(acl, mac_addr);
        if (entry) {
            entry->ae_last_req = ACL_SOFTBLOCKING_TIMESTAMP_INIT;
            qdf_timer_stop(&entry->ae_softblocking_timer);
        }
        spin_unlock(&acl->acl_lock);
    }
    return ret;
}

int wlan_acl_get_softblocking(struct ieee80211vap *vap,
                                     const u_int8_t *mac_addr)
{
    /* Note that a lookup failure (no entry in the ACL) will also indicate
     * that false.
     */
    return ieee80211_acl_flag_check(vap, mac_addr, IEEE80211_ACL_FLAG_SOFTBLOCKING);
}

bool wlan_acl_check_softblocking(struct ieee80211vap *vap,
                                          const u_int8_t *mac_addr)
{
    int ret = 1;
    if (ieee80211_acl_flag_check(vap, mac_addr, IEEE80211_ACL_FLAG_SOFTBLOCKING)) {
        ret = ieee80211_acl_check(vap, mac_addr);
    }
    return ret;
}
#endif

/**
 * @brief Override VAP-Level RSSI thresholds with node level thresholds
 *
 * @param [in] ni  the node structure
 * @param [out] inact_rssi_low_threshold inactive RSSI low threshold
 * @param [out] inact_rssi_high_threshold inactive RSSI high threshold
 * @param [out] low_rssi_threshold Low RSSI threshold
 * @param [out] low_rate_rssi_threshold Low rate RSSI threshold
 * @param [out] high_rate_rssi_threshold High rate RSSI threshold
 *
 * @return none
 */
void wlan_acl_override_rssi_thresholds(struct ieee80211_node *ni,
        u_int32_t *inact_rssi_low_threshold,
        u_int32_t *inact_rssi_high_threshold,
        u_int32_t *low_rssi_threshold,
        u_int32_t *low_rate_rssi_threshold,
        u_int32_t *high_rate_rssi_threshold)
{
    if (ni->ni_inact_rssi_xing) {
        *inact_rssi_low_threshold  = ni->ni_inact_rssi_xing;
        *inact_rssi_high_threshold = ni->ni_inact_rssi_xing;
    }

    if (ni->ni_low_rssi_xing)
        *low_rssi_threshold = ni->ni_low_rssi_xing;

    if (ni->ni_low_rate_rssi_xing)
        *low_rate_rssi_threshold = ni->ni_low_rate_rssi_xing;

    if (ni->ni_high_rate_rssi_xing)
        *high_rate_rssi_threshold = ni->ni_high_rate_rssi_xing;
}

/**
 * @brief Apply the thresholds in ACL to the corresponding node
 *
 * @param [in] ni  the VAP structure
 * @param [in] mac_addr MAC Address of the node where override thresholds
 *             will be applied
 *
 * @return none
 */
void
wlan_acl_apply_node_rssi_thresholds(struct ieee80211vap *vap,
                                    const u_int8_t *mac_addr)
{
    struct ieee80211_node               *ni;
    struct ieee80211com                 *ic = vap->iv_ic;
    int                                 ret = EOK;

    ni = ieee80211_find_node(&ic->ic_sta, mac_addr, WLAN_MLME_SB_ID);
    if (!ni)
        return;

    {
        u_int8_t params[] = {IEEE80211_ACL_PARAM_INACT_RSSI_XING,
                             IEEE80211_ACL_PARAM_LOW_RSSI_XING,
                             IEEE80211_ACL_PARAM_LOW_RATE_RSSI_XING,
                             IEEE80211_ACL_PARAM_HIGH_RATE_RSSI_XING};

        u_int8_t *values[] = {&ni->ni_inact_rssi_xing,
                              &ni->ni_low_rssi_xing,
                              &ni->ni_low_rate_rssi_xing,
                              &ni->ni_high_rate_rssi_xing};

        ret = ieee80211_acl_get_params_if_flag(vap,
                                               mac_addr,
                                               0,
                                               params,
                                               values,
                                               QDF_ARRAY_SIZE(params));
    }
    ieee80211_free_node(ni, WLAN_MLME_SB_ID);
}

static int
get_cli_params_rssi_xing_values(struct ieee80211vap *vap,
                                struct ieee80211req_athdbg *req)
{
    u_int8_t params[] = {IEEE80211_ACL_PARAM_INACT_RSSI_XING,
                         IEEE80211_ACL_PARAM_LOW_RSSI_XING,
                         IEEE80211_ACL_PARAM_LOW_RATE_RSSI_XING,
                         IEEE80211_ACL_PARAM_HIGH_RATE_RSSI_XING};

    u_int8_t *values[] = {&req->data.acl_cli_param.inact_rssi_xing,
                          &req->data.acl_cli_param.low_rssi_xing,
                          &req->data.acl_cli_param.low_rate_rssi_xing,
                          &req->data.acl_cli_param.high_rate_rssi_xing};

    return ieee80211_acl_get_params_if_flag(vap, req->dstmac,
                                            0, params, values,
                                            QDF_ARRAY_SIZE(params));

}

static int
get_cli_params_auth_block_values(struct ieee80211vap *vap,
                                 struct ieee80211req_athdbg *req)
{
    int ret = 0;
    u_int8_t params[] = {IEEE80211_ACL_PARAM_AUTH_RSSI_HWM,
                         IEEE80211_ACL_PARAM_AUTH_RSSI_LWM,
                         IEEE80211_ACL_PARAM_AUTH_REJECT_REASON};

    u_int8_t *values[] = {&req->data.acl_cli_param.auth_rssi_hwm,
                          &req->data.acl_cli_param.auth_rssi_lwm,
                          &req->data.acl_cli_param.auth_reject_reason};

    ret = ieee80211_acl_get_params_if_flag(vap, req->dstmac,
                                           IEEE80211_ACL_FLAG_AUTH_BLOCK,
                                           params,
                                           values,
                                           QDF_ARRAY_SIZE(params));

    if (ret == 0) {
        /*
         * AUTH_BLOCK flag is not set
         * Hence the related parameters are invalid
         */
        req->data.acl_cli_param.auth_block = 0;
        req->data.acl_cli_param.auth_rssi_hwm = 0;
        req->data.acl_cli_param.auth_rssi_lwm = 0;
        req->data.acl_cli_param.auth_reject_reason = 0;
    } else if (ret == 1) {
        req->data.acl_cli_param.auth_block = 1;
    }
    return ret;
}

static int
get_cli_params_probe_resp_values(struct ieee80211vap *vap,
                                 struct ieee80211req_athdbg *req)
{
    int ret = 0;

    u_int8_t params[] = {IEEE80211_ACL_PARAM_PROBE_RSSI_HWM,
                         IEEE80211_ACL_PARAM_PROBE_RSSI_LWM};

    u_int8_t *values[] = {&req->data.acl_cli_param.probe_rssi_hwm,
                          &req->data.acl_cli_param.probe_rssi_lwm};

    ret = ieee80211_acl_get_params_if_flag(vap, req->dstmac,
                                           IEEE80211_ACL_FLAG_PROBE_RESP_WH,
                                           params,
                                           values,
                                           QDF_ARRAY_SIZE(params));
    if (ret == 0) {
        /*
         * PROBE_RESP_WH flag is not set
         */
        req->data.acl_cli_param.probe_rssi_hwm = 0;
        req->data.acl_cli_param.probe_rssi_lwm = 0;

    }
    return ret;
}

/**
 * @brief Get the ACL parameter values for CLI command
 *
 * @param [in] vap  the interface on which to get the ACL entry
 * @param [out] req  ieee80211req_athdbg structure to fill in the ACL parameter
 *                  values
 *
 * @return EOK on success; EINVAL if the entry cannot be found;
 */
int
wlan_acl_get_cli_params(struct ieee80211vap *vap,
                              struct ieee80211req_athdbg *req)
{
    int     ret;

    ret = get_cli_params_probe_resp_values(vap, req);
    if (ret < 0)
        return ret;

    ret = get_cli_params_rssi_xing_values(vap, req);
    if (ret < 0)
        return ret;

    ret = get_cli_params_auth_block_values(vap, req);
    if (ret < 0)
        return ret;

    return EOK;
}

static int
set_cli_params_rssi_xing_values(struct ieee80211vap *vap,
                                const struct ieee80211req_athdbg *req)
{
    u_int8_t params[] = {IEEE80211_ACL_PARAM_INACT_RSSI_XING,
                         IEEE80211_ACL_PARAM_LOW_RSSI_XING,
                         IEEE80211_ACL_PARAM_LOW_RATE_RSSI_XING,
                         IEEE80211_ACL_PARAM_HIGH_RATE_RSSI_XING};

    u_int8_t values[] = {req->data.acl_cli_param.inact_rssi_xing,
                         req->data.acl_cli_param.low_rssi_xing,
                         req->data.acl_cli_param.low_rate_rssi_xing,
                         req->data.acl_cli_param.high_rate_rssi_xing};

    return ieee80211_acl_set_params_and_flag(vap, req->dstmac,
                                             0, params, values,
                                             QDF_ARRAY_SIZE(params));

}

static int
set_cli_params_auth_block_values(struct ieee80211vap *vap,
                                 const struct ieee80211req_athdbg *req)
{
    int ret = 0;


    u_int8_t params[] = {IEEE80211_ACL_PARAM_AUTH_RSSI_HWM,
                         IEEE80211_ACL_PARAM_AUTH_RSSI_LWM,
                         IEEE80211_ACL_PARAM_AUTH_REJECT_REASON};

    u_int8_t values[] = {req->data.acl_cli_param.auth_rssi_hwm,
                         req->data.acl_cli_param.auth_rssi_lwm,
                         req->data.acl_cli_param.auth_reject_reason};

    if (req->data.acl_cli_param.auth_block)
        ret = ieee80211_acl_set_params_and_flag(vap, req->dstmac,
                                                IEEE80211_ACL_FLAG_AUTH_BLOCK,
                                                params,
                                                values,
                                                QDF_ARRAY_SIZE(params));
    else
        ret = ieee80211_acl_clr_flag(vap, req->dstmac,
                                     IEEE80211_ACL_FLAG_AUTH_BLOCK);

    return ret;
}



static int
set_cli_params_probe_resp_values(struct ieee80211vap *vap,
                                 const struct ieee80211req_athdbg *req)
{
    int ret = 0;


    u_int8_t params[] = {IEEE80211_ACL_PARAM_PROBE_RSSI_HWM,
                         IEEE80211_ACL_PARAM_PROBE_RSSI_LWM};

    u_int8_t values[] = {req->data.acl_cli_param.probe_rssi_hwm,
                         req->data.acl_cli_param.probe_rssi_lwm};

    ret = ieee80211_acl_set_params_and_flag(vap, req->dstmac,
                                            0,
                                            params,
                                            values,
                                            QDF_ARRAY_SIZE(params));
    return ret;
}


/**
 * @brief Set the ACL parameter values coming as part of CLI command
 *
 * @param [in] vap  the interface on which to manipulate the ACL entry
 * @param [in] req  ieee80211req_athdbg structure to get the ACL parameter
 *                  values
 *
 * @return EOK on success; ENOENT if the entry cannot be found;
 */
int
wlan_acl_set_cli_params(struct ieee80211vap *vap,
                              const struct ieee80211req_athdbg *req)
{
    int     ret;

    ret = set_cli_params_probe_resp_values(vap, req);
    if (ret < 0)
        return ret;

    ret = set_cli_params_rssi_xing_values(vap, req);
    if (ret < 0)
        return ret;

    ret = set_cli_params_auth_block_values(vap, req);
    if (ret < 0)
        return ret;

    /*
     * Apply now if client is connected
     */
    wlan_acl_apply_node_rssi_thresholds(vap, req->dstmac);
    return EOK;
}

/**
 * @brief Query the band steering module for whether it is blocking
 *        auth requests for the given MAC address on this VAP.
 *
 * @param [in]  vap  The VAP on which the probe request was received
 * @param [in]  mac_addr  The MAC address of the client that sent the probe
 *                        request
 * @param [in]  auth_rssi  The RSSI of the probe request
 * @param [out] rej_reason  The reason code when rejecting (0 = no response)
 *
 * @return true if auth should be blocked; otherwise false
 */
bool ieee80211_acl_is_auth_blocked(struct ieee80211vap *vap,
                                   const u_int8_t *mac_addr,
                                   u_int8_t auth_rssi,
                                   u_int8_t *rej_reason)
{
    u_int8_t rssi_hwm = 0;
    u_int8_t rssi_lwm = 0;

    u_int8_t params[] = {IEEE80211_ACL_PARAM_AUTH_RSSI_HWM,
                         IEEE80211_ACL_PARAM_AUTH_RSSI_LWM,
                         IEEE80211_ACL_PARAM_AUTH_REJECT_REASON};
    u_int8_t *values[] = {&rssi_hwm,
                          &rssi_lwm,
                          rej_reason};

    if (rej_reason == NULL) {
        QDF_TRACE(QDF_MODULE_ID_ACL, QDF_TRACE_LEVEL_ERROR,
                 "%s: ERROR! NULL pointer passed in rej_reason\n", __func__);
        return false;
    }

    if (ieee80211_acl_get_params_if_flag(vap, mac_addr,
                                           IEEE80211_ACL_FLAG_AUTH_BLOCK,
                                           params,
                                           values,
                                           QDF_ARRAY_SIZE(params)) > 0) {

        if (rssi_hwm > 0 && auth_rssi > rssi_hwm)
            return true;
        else if (rssi_lwm > 0 && auth_rssi < rssi_lwm)
            return true;
        else if (rssi_hwm == 0 && rssi_lwm == 0)
            return true;
    }

    return false;
}


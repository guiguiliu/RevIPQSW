/*
 * Copyright (c) 2018-2019 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 */

#include <wlanconfig.h>
#include <inttypes.h>
#ifdef WLAN_CFR_ENABLE
#include <wlan_cfr_public_structs.h>
#endif

#if ATH_SUPPORT_HYFI_ENHANCEMENTS

static int handle_ald(struct socket_context *sock_ctx, const char *ifname, IEEE80211_WLANCONFIG_CMDTYPE cmdtype,
        char *addr1, int enable)
{
    struct ieee80211_wlanconfig config;
    char ni_macaddr[MACSTR_LEN] = {0};
    uint8_t temp[MACSTR_LEN] = {0};

    if (addr1 && strlen(addr1) != 17) {
        printf("Invalid MAC address (format: xx:xx:xx:xx:xx:xx)\n");
        return -1;
    }

    if (addr1) {
        memset(ni_macaddr, '\0', sizeof(ni_macaddr));
        if(strlcpy(ni_macaddr, addr1, sizeof(ni_macaddr)) >= sizeof(ni_macaddr)) {
            printf("Invalid MAC address1\n");
            return -1;
        }
        if (ether_string2mac(temp, ni_macaddr) != 0) {
            printf("Invalid MAC address1\n");
            return -1;
        }
    }

    memset(&config, 0, sizeof config);
    /* fill up configuration */
    config.cmdtype = cmdtype;
    switch (cmdtype) {
        case IEEE80211_WLANCONFIG_ALD_STA_ENABLE:
            memcpy(config.data.ald.data.ald_sta.macaddr, temp, QDF_MAC_ADDR_SIZE);
            config.data.ald.data.ald_sta.enable = (u_int32_t)enable;
            break;
        default:
            printf("%d command not handled yet", cmdtype);
            break;
    }
    send_command(sock_ctx, ifname, &config, sizeof(struct ieee80211_wlanconfig),
            NULL, QCA_NL80211_VENDOR_SUBCMD_ALD_PARAMS, IEEE80211_IOCTL_CONFIG_GENERIC);

    return 0;
}

int handle_command_ald (int argc, char *argv[], const char *ifname,
    struct socket_context *sock_ctx)
{
    if (argc == 6 && streq(argv[3], "sta-enable")) {
        return handle_ald(sock_ctx, ifname, IEEE80211_WLANCONFIG_ALD_STA_ENABLE,
                argv[4], atoi(argv[5]));
    } else {
        errx(1, "invalid ALD command");
    }
    return 0;
}

static int handle_hmmc(struct socket_context *sock_ctx, const char *ifname,
    IEEE80211_WLANCONFIG_CMDTYPE cmdtype, char *ip_str, char *mask_str)
{
    int ip=0, mask=0;
    struct ieee80211_wlanconfig config;
    if (cmdtype == IEEE80211_WLANCONFIG_HMMC_ADD ||
            cmdtype == IEEE80211_WLANCONFIG_HMMC_DEL) {
        if ((ip = inet_addr(ip_str)) == -1 || !ip) {
            printf("Invalid ip string %s\n", ip_str);
            return -1;
        }

        if (!(mask = inet_addr(mask_str))) {
            printf("Invalid ip mask string %s\n", mask_str);
            return -1;
        }
    }

    config.cmdtype = cmdtype;
    switch (cmdtype) {
        case IEEE80211_WLANCONFIG_HMMC_ADD:
        case IEEE80211_WLANCONFIG_HMMC_DEL:
            config.data.hmmc.ip = ip;
            config.data.hmmc.mask = mask;
            break;
        case IEEE80211_WLANCONFIG_HMMC_DUMP:
            break;
        default:
            perror("invalid cmdtype");
            return -1;
    }
    send_command(sock_ctx, ifname, &config, sizeof(struct ieee80211_wlanconfig),
            NULL, QCA_NL80211_VENDOR_SUBCMD_HMMC, IEEE80211_IOCTL_CONFIG_GENERIC);

    return 0;
}

int handle_command_hmmc (int argc, char *argv[], const char *ifname,
    struct socket_context *sock_ctx)
{
    if (argc == 4 && streq(argv[3], "dump")) {
        return handle_hmmc(sock_ctx, ifname, IEEE80211_WLANCONFIG_HMMC_DUMP, NULL, NULL);
    } else if (argc == 6 && streq(argv[3], "add")) {
        return handle_hmmc(sock_ctx, ifname, IEEE80211_WLANCONFIG_HMMC_ADD, argv[4], argv[5]);
    } else if (argc == 6 && streq(argv[3], "del")) {
        return handle_hmmc(sock_ctx, ifname, IEEE80211_WLANCONFIG_HMMC_DEL, argv[4], argv[5]);
        errx(1, "invalid HMMC command");
    }
    return 0;
}
#endif

static int atf_show_stat(const char *ifname, char *macaddr)
{
    int s, i;
    struct iwreq iwr;
    struct ieee80211_wlanconfig config;
    uint8_t len, lbyte, ubyte;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        printf("socket error\n");
        return 1;
    }

    memset(&config, 0, sizeof(config));
    config.cmdtype = IEEE80211_PARAM_STA_ATF_STAT;
    len = strlen(macaddr);
    for (i = 0; i < len; i += 2) {
        lbyte = ubyte = 0;

        if ((macaddr[i] >= '0') && (macaddr[i] <= '9'))  {
            ubyte = macaddr[i] - '0';
        } else if ((macaddr[i] >= 'A') && (macaddr[i] <= 'F')) {
            ubyte = macaddr[i] - 'A' + 10;
        } else if ((macaddr[i] >= 'a') && (macaddr[i] <= 'f')) {
            ubyte = macaddr[i] - 'a' + 10;
        }

        if ((macaddr[i + 1] >= '0') && (macaddr[i + 1] <= '9'))  {
            lbyte = macaddr[i + 1] - '0';
        } else if ((macaddr[i + 1] >= 'A') && (macaddr[i + 1] <= 'F')) {
            lbyte = macaddr[i + 1] - 'A' + 10;
        } else if ((macaddr[i + 1] >= 'a') && (macaddr[i + 1] <= 'f')) {
            lbyte = macaddr[i + 1] - 'a' + 10;
        }

        config.data.atf.macaddr[i / 2] = (ubyte << 4) | lbyte;
    }
    memset(&iwr, 0, sizeof(iwr));

    if (strlcpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name)) >= sizeof(iwr.ifr_name)) {
        fprintf(stderr, "ifname too long: %s\n", ifname);
        close(s);
        return -1;
    }

    iwr.u.data.pointer = (void *)&config;
    iwr.u.data.length = sizeof(config);

    if (ioctl(s, IEEE80211_IOCTL_CONFIG_GENERIC, &iwr) < 0) {
        printf("unable to get stats\n");
        close(s);
        return 1;
    }

    fprintf(stderr, "Short Average %d, Total Used Tokens %"PRIu64"\n",
            config.data.atf.short_avg, config.data.atf.total_used_tokens);

    close(s);
    return 0;
}

int handle_command_atfstat (int argc, char *argv[], const char *ifname,
    struct socket_context *sock_ctx)
{
        if (argc > 3)
            atf_show_stat(ifname, argv[3]);
        else
            fprintf(stderr,"\n\n     Missing parameters!!  \n\n");
        return 0;
}

#if WLAN_CFR_ENABLE
static int handle_cfr_start_command(struct socket_context *sock_ctx, const char *ifname,
        char *peer_mac, int band_width, int periodicity, int capture_type)
{
    struct ieee80211_wlanconfig config;
    char macaddr[MACSTR_LEN];
    uint8_t temp[MACSTR_LEN];

    memset(temp, '\0', sizeof(temp));
    config.cmdtype = IEEE80211_WLANCONFIG_CFR_START;
    if(peer_mac != NULL) {
        memset(macaddr, '\0', sizeof(macaddr));
        if (strlcpy(macaddr, peer_mac, sizeof(macaddr)) >= sizeof(macaddr)) {
            printf("Invalid MAC address (format: xx:xx:xx:xx:xx:xx)\n");
            return -1;
        }

        if (ether_string2mac(temp, macaddr) != 0) {
            printf("Invalid MAC address\n");
            return -1;
        }
        memcpy(config.data.cfr_config.mac, temp, QDF_MAC_ADDR_SIZE);
    }

    config.data.cfr_config.bandwidth = band_width;
    config.data.cfr_config.periodicity = periodicity;
    config.data.cfr_config.capture_method = capture_type;

    send_command(sock_ctx, ifname, &config, sizeof(struct ieee80211_wlanconfig),
            NULL, QCA_NL80211_VENDOR_SUBCMD_CFR_CONFIG, IEEE80211_IOCTL_CONFIG_GENERIC);

    return 0;
}

static int handle_cfr_stop_command(struct socket_context *sock_ctx, const char *ifname,
        char *peer_mac)
{
    struct ieee80211_wlanconfig config;
    char macaddr[MACSTR_LEN];
    uint8_t temp[MACSTR_LEN];

    memset(temp, '\0', sizeof(temp));
    config.cmdtype = IEEE80211_WLANCONFIG_CFR_STOP;
    if(peer_mac != NULL) {
        memset(macaddr, '\0', sizeof(macaddr));
        if (strlcpy(macaddr, peer_mac, sizeof(macaddr)) >= sizeof(macaddr)) {
            printf("Invalid MAC address (format: xx:xx:xx:xx:xx:xx)\n");
            return -1;
        }

        if (ether_string2mac(temp, macaddr) != 0) {
            printf("Invalid MAC address\n");
            return -1;
        }
        memcpy(config.data.cfr_config.mac, temp, QDF_MAC_ADDR_SIZE);
    }

    send_command(sock_ctx, ifname, &config, sizeof(struct ieee80211_wlanconfig),
            NULL, QCA_NL80211_VENDOR_SUBCMD_CFR_CONFIG, IEEE80211_IOCTL_CONFIG_GENERIC);

    return 0;
}

static int handle_cfr_list_command(struct socket_context *sock_ctx, const char *ifname)
{
    /*
     * List all peers that are CFR enabled & Display all CFR capture properties
     */
    struct ieee80211_wlanconfig config;
    config.cmdtype = IEEE80211_WLANCONFIG_CFR_LIST_PEERS;
    send_command(sock_ctx, ifname, &config, sizeof(struct ieee80211_wlanconfig),
            NULL, QCA_NL80211_VENDOR_SUBCMD_CFR_CONFIG, IEEE80211_IOCTL_CONFIG_GENERIC);
    return 0;
}

#ifdef WLAN_ENH_CFR_ENABLE
/*
 * Purpose of this handler is to display the last successfully committed
 * configuration.
 *
 * @sock_ctx: socket context
 * @ifname  : interface name (athX)
 *
 * Return: 0 on success, negative errno on failure
 *
 */

static int
handle_get_rcc(struct socket_context *sock_ctx, const char *ifname, IEEE80211_WLANCONFIG_CMDTYPE cmdtype)
{
    struct ieee80211_wlanconfig config;
    int retv = 0;
    config.cmdtype = cmdtype;

    retv = send_command(sock_ctx, ifname, &config,
            sizeof(struct ieee80211_wlanconfig), NULL,
            QCA_NL80211_VENDOR_SUBCMD_CFR_CONFIG,
            IEEE80211_IOCTL_CONFIG_GENERIC);

    return retv;
}

/*
 * User given input for TA, TA_MASK, RA and RA_MASK will be processed here.
 *
 * @sock_ctx: socket context
 * @ifname  : interface name (athX)
 *
 * grp_id : Group id, can be of any value, in between 0 and 15
 * ta     : Tx address, should be the client MAC address, which will send the
 *         packet to the Root AP.
 * ta_mask: Tx address mask
 * ra     : Rx address, could be the MAC address of AP or the User can provide
 *          00:00:00:00:00:00, based on below logic:
 *
 *         Match = ((addr & expect_mask) == expect_addr)
 *
 * ra_mask: RX address mask
 *
 * Return: 0 on success, negative errno on failure
 */

static int
handle_taraAddr_config(struct socket_context *sock_ctx, const char *ifname,
        int grp_id, char *ta, char *ta_mask, char *ra, char *ra_mask)
{
    struct ieee80211_wlanconfig config;
    char macaddr[MACSTR_LEN];
    uint8_t temp[MACSTR_LEN];
    int retv = 0;

    /* fill up configuration */
    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = IEEE80211_WLANCONFIG_CFR_RCC_TA_RA_ADDR;

    if (grp_id > 15 || grp_id < 0) {
        printf("Invalid Filter config group: GRP should be from 0 to 15.\n");
        return -EINVAL;
    }

    config.data.cfr_config.grp_id = grp_id;

    if (ta != NULL) {
        memset(macaddr, '\0', sizeof(macaddr));
        memset(temp, '\0', sizeof(temp));
        if (strlcpy(macaddr, ta, sizeof(macaddr)) >= sizeof(macaddr)) {
            printf("Invalid TX address (format: xx:xx:xx:xx:xx:xx)\n");
            return -EINVAL;
        }

        if (ether_string2mac(temp, macaddr) != 0) {
            printf("Invalid MAC address for TA\n");
            return -EINVAL;
        }
        memcpy(config.data.cfr_config.ta, temp, QDF_MAC_ADDR_SIZE);
    }

    if (ta_mask != NULL) {
        memset(macaddr, '\0', sizeof(macaddr));
        memset(temp, '\0', sizeof(temp));
        if (strlcpy(macaddr, ta_mask, sizeof(macaddr)) >= sizeof(macaddr)) {
            printf("Invalid TA_MASK (format: xx:xx:xx:xx:xx:xx)\n");
            return -EINVAL;
        }

        if (ether_string2mac(temp, macaddr) != 0) {
            printf("Invalid MAC address\n");
            return -EINVAL;
        }
        memcpy(config.data.cfr_config.ta_mask, temp, QDF_MAC_ADDR_SIZE);
    }

    if (ra != NULL) {
        memset(macaddr, '\0', sizeof(macaddr));
        memset(temp, '\0', sizeof(temp));
        if (strlcpy(macaddr, ra, sizeof(macaddr)) >= sizeof(macaddr)) {
            printf("Invalid MAC address for RA (format: xx:xx:xx:xx:xx:xx)\n");
            return -EINVAL;
        }

        if (ether_string2mac(temp, macaddr) != 0) {
            printf("Invalid MAC address\n");
            return -EINVAL;
        }
        memcpy(config.data.cfr_config.ra, temp, QDF_MAC_ADDR_SIZE);
    }

    if (ra_mask != NULL) {
        memset(macaddr, '\0', sizeof(macaddr));
        memset(temp, '\0', sizeof(temp));
        if (strlcpy(macaddr, ra_mask, sizeof(macaddr)) >= sizeof(macaddr)) {
            printf("Invalid RX Addr Mask (format: xx:xx:xx:xx:xx:xx)\n");
            return -EINVAL;
        }

        if (ether_string2mac(temp, macaddr) != 0) {
            printf("Invalid MAC address for RA_mask\n");
            return -EINVAL;
        }
        memcpy(config.data.cfr_config.ra_mask, temp, QDF_MAC_ADDR_SIZE);
    }

    retv = send_command(sock_ctx, ifname, &config,
            sizeof(struct ieee80211_wlanconfig), NULL,
            QCA_NL80211_VENDOR_SUBCMD_CFR_CONFIG,
            IEEE80211_IOCTL_CONFIG_GENERIC);

    return retv;
}

/*
 * User given inputs for BW, NSS, and type/subtypes will be preocessed here.
 *
 * @sock_ctx: socket context
 * @ifname  : interface name (athX)
 *
 * cmdtype: Specific cmdtypes representing BW & NSS, Type/ subtypes
 * val1: group id
 *
 * While configuring BW and NSS:
 * val2: BW
 * val3: NSS
 * val4: Null
 *
 * While configuring type/subtype:
 * val2: MGMT
 * val3: CTRL
 * val4: Data
 *
 * Return: 0 on success, negative errno on failure
 */

static int
handle_tara_config(struct socket_context *sock_ctx, const char *ifname,
        IEEE80211_WLANCONFIG_CMDTYPE cmdtype,
        int val1, int val2, int val3, int val4)
{
    struct ieee80211_wlanconfig config;
    int retv = 0;


    if (val1 > 15 || val1 < 0) {
        printf("Invalid Filter config group: GRP should be from 0 to 15.\n");
        return -EINVAL;
    }

    /* fill up configuration */
    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = cmdtype;

    config.data.cfr_config.grp_id = val1;

    switch (cmdtype) {
        case IEEE80211_WLANCONFIG_CFR_RCC_BW_NSS:
            {
                if (val2 <= 0xF && val2 >= 0) {
                    config.data.cfr_config.bw = val2;
                } else {
                    printf("Unsupported bandwidth.\n");
                }
                if (val3 >= 0 && val3 <= 255) {
                    config.data.cfr_config.nss = val3;
                } else {
                    printf("Unsupported NSS.\n");
                }
            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_SUBTYPE:
            {
                if (val2 <= 0xFFFF && val2 >= 0) {
                    config.data.cfr_config.expected_mgmt_subtype = val2;
                } else {
                    printf("Unsupported mgmt subtype.\n");
                }
                if (val3 <= 0xFFFF && val3 >= 0) {
                    config.data.cfr_config.expected_ctrl_subtype = val3;
                } else {
                    printf("Unsupported ctrl subtype.\n");
                }
                if (val4 <= 0xFFFF && val4 >= 0) {
                    config.data.cfr_config.expected_data_subtype = val4;
                } else {
                    printf("Unsupported data subtype.\n");
                }
            }
            break;

        default:
            errx(1, "Invalid cfr rcc mode");
            break;
    }

    /* call NL80211 Vendor commnd frame work */
    retv = send_command(sock_ctx, ifname, &config,
            sizeof(struct ieee80211_wlanconfig), NULL,
            QCA_NL80211_VENDOR_SUBCMD_CFR_CONFIG,
            IEEE80211_IOCTL_CONFIG_GENERIC);

    return retv;
}


/*
 * User given inputs specific to fixed params and commit
 * will be preocessed here.
 *
 * @sock_ctx: socket context
 * @ifname  : interface name (athX)
 *
 * cmdtype: Specific cmdtypes representing the fixed parameters used for RCC,
 *          commit handler and handler to disable all capture modes at a time.
 *
 * val1 and val2: Values provided by user based on the cmdtype.
 *
 * Return: 0 on success, negative errno on failure
 *
 */

static int
handle_cfr_rcc_config(struct socket_context *sock_ctx, const char *ifname,
        IEEE80211_WLANCONFIG_CMDTYPE cmdtype, uint32_t val1, uint32_t val2)
{
    struct ieee80211_wlanconfig config;
    int retv = 0;

    /* fill up configuration */
    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = cmdtype;

    switch (cmdtype) {
        case IEEE80211_WLANCONFIG_CFR_RCC_CAPT_DUR:
            {
                if (val1 > 0xffffff) {
                    printf("Invalid entry:\t"
                            "Capture duration should not be greater than\t"
                            "(16.8 sec) 0xFFFFFF\n");
                    return -EINVAL;
                }
                config.data.cfr_config.cap_dur = val1;
            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_CAPT_INTVAL:
            {
                if (val1 > 0xffffff) {
                    printf("Invalid entry:\t"
                            "Capture interval should not be greater than\t"
                            "(16.8 sec) 0xFFFFFF\n");
                    return -EINVAL;
                }
                config.data.cfr_config.cap_intvl = val1;
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_EN_CFG:
            {
                if (val1 > 0xffff) {
                    printf("Invalid entry:\t"
                            "Valid entry should be in between 1 to 0xFFFF\n");
                    return -EINVAL;
                }
                config.data.cfr_config.en_cfg = val1;
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RESET_CFG:
            {
                if (val1 > 0xffff) {
                    printf("Invalid entry:\t"
                            "Valid entry should be in between 1 to 0xFFFF\n");
                    return -EINVAL;
                }
                config.data.cfr_config.reset_cfg = val1;
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_UL_MU_USER_MASK:
            {
                config.data.cfr_config.ul_mu_user_mask_lower = val1;
                config.data.cfr_config.ul_mu_user_mask_upper = val2;
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_FREEZE_TLV_DELAY_CNT:
            {
                if (val2 > 0xff) {
                    printf("Invalid entry:\t"
                            "Valid entry should be in between 1 to 0xFF\n");
                    return -EINVAL;
                }
                config.data.cfr_config.freeze_tlv_delay_cnt_en = val1;
                config.data.cfr_config.freeze_tlv_delay_cnt_thr = val2;
            }
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_DISABLE_ALL:
            break;

        case IEEE80211_WLANCONFIG_CFR_RCC_COMMIT:
            break;

        default:
            errx(1, "Invalid cfr rcc config");
            break;
    }

    retv = send_command(sock_ctx, ifname, &config,
            sizeof(struct ieee80211_wlanconfig), NULL,
            QCA_NL80211_VENDOR_SUBCMD_CFR_CONFIG,
            IEEE80211_IOCTL_CONFIG_GENERIC);

    return retv;
}

/*
 * There are totally 6 new capture modes introduced in RCC.
 * Capture modes:
 *          1) m_directed_ftm
 *          2) m_directed_ndpa_ndp
 *          3) m_ta_ra_filter
 *          4) m_ndpa_ndp_all
 *          5) m_all_ftm_ack  (Not supported in Cypress)
 *          6) m_all_pkt      (Not supported for now)
 *
 *
 * @sock_ctx: socket context
 * @ifname  : interface name (athX)
 *
 * cmdtype: Specific cmdtypes representing different capture modes
 *          has been mentioned below as part of switch cases
 * val: Could be either 0 or 1
 *          0: To disable the capture mode
 *          1: To enable the capture mode
 *
 * Return: 0 on success, negative errno on failure
 * Note: Multiple capture modes can co-exist.
 *
 */

static int
handle_cfr_rcc_mode(struct socket_context *sock_ctx, const char *ifname,
        IEEE80211_WLANCONFIG_CMDTYPE cmdtype, int val)
{
    struct ieee80211_wlanconfig config;
    int retv = 0;


    if ((val < 0)) {
        printf("Valid values: 0:Disable,\t"
                "non-zero:Enable/capture duration/capture interval\n");
        return -EINVAL;
    }

    /* fill up configuration */
    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = cmdtype;

    switch (cmdtype) {
        case IEEE80211_WLANCONFIG_CFR_RCC_DIRECT_FTM:
            {
                if (val) {
                    config.data.cfr_config.en_directed_ftm = 1;
                } else {
                    config.data.cfr_config.dis_directed_ftm = 1;
                }
            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_DIRECT_NDPA_NDP:
            {
                if (val) {
                    config.data.cfr_config.en_directed_ndpa_ndp = 1;
                } else {
                    config.data.cfr_config.dis_directed_ndpa_ndp = 1;
                }
            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_TA_RA_FLITER:
            {
                if (val) {
                    config.data.cfr_config.en_ta_ra_filter = 1;
                } else {
                    config.data.cfr_config.dis_ta_ra_filter = 1;
                }
            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_ALL_FTM_ACK:
            {
                if (val) {
                    config.data.cfr_config.en_all_ftm_ack = 1;
                } else {
                    config.data.cfr_config.dis_all_ftm_ack = 1;
                }
            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_NDPA_NDP_ALL:
            {
                if (val) {
                    config.data.cfr_config.en_ndpa_ndp_all = 1;
                } else {
                    config.data.cfr_config.dis_ndpa_ndp_all = 1;
                }
            }
            break;
        case IEEE80211_WLANCONFIG_CFR_RCC_ALL_PKT:
            {
                if (val) {
                    config.data.cfr_config.en_all_pkt = 1;
                } else {
                    config.data.cfr_config.dis_all_pkt = 1;
                }
            }
            break;

        default:
            errx(1, "Invalid cfr rcc mode");
            break;
    }

    /* call NL80211 Vendor commnd frame work */
    retv = send_command(sock_ctx, ifname, &config,
            sizeof(struct ieee80211_wlanconfig), NULL,
            QCA_NL80211_VENDOR_SUBCMD_CFR_CONFIG,
            IEEE80211_IOCTL_CONFIG_GENERIC);

    return retv;
}

int strtoint(char *arg)
{
    int val;
    if (!strncmp (arg, "0x", 2)) {
        sscanf(arg, "%x", &val);
    } else {
        sscanf(arg, "%d", &val);
    }
    return val;
}
#endif

int handle_command_cfr(int argc, char *argv[], const char *ifname,
        struct socket_context *sock_ctx)
{
    if (argc == 8 && streq(argv[3], "start")) {
        return handle_cfr_start_command(sock_ctx, ifname,
                argv[4], atoi(argv[5]), atoi(argv[6]), atoi(argv[7]));
    } else if (argc == 5 && streq(argv[3], "stop")) {
        return handle_cfr_stop_command(sock_ctx, ifname,
                argv[4]);
#ifdef WLAN_ENH_CFR_ENABLE
    } else if (argc == 5 && streq(argv[3], "m_directed_ftm")) {
        return handle_cfr_rcc_mode(sock_ctx, ifname,
                IEEE80211_WLANCONFIG_CFR_RCC_DIRECT_FTM,
                strtoint(argv[4]));
    } else if (argc == 5 && streq(argv[3], "m_directed_ndpa_ndp")) {
        return handle_cfr_rcc_mode(sock_ctx, ifname,
                IEEE80211_WLANCONFIG_CFR_RCC_DIRECT_NDPA_NDP,
                strtoint(argv[4]));
    } else if (argc == 5 && streq(argv[3], "m_ta_ra_filter")) {
        return handle_cfr_rcc_mode(sock_ctx, ifname,
                IEEE80211_WLANCONFIG_CFR_RCC_TA_RA_FLITER,
                strtoint(argv[4]));
    } else if (argc == 5 && streq(argv[3], "m_all_ftm_ack")) {
        return handle_cfr_rcc_mode(sock_ctx, ifname,
                IEEE80211_WLANCONFIG_CFR_RCC_ALL_FTM_ACK,
                strtoint(argv[4]));
    } else if (argc == 5 && streq(argv[3], "m_ndpa_ndp_all")) {
        return handle_cfr_rcc_mode(sock_ctx, ifname,
                IEEE80211_WLANCONFIG_CFR_RCC_NDPA_NDP_ALL,
                strtoint(argv[4]));
    } else if (argc == 5 && streq(argv[3], "m_all_pkt")) {
        return handle_cfr_rcc_mode(sock_ctx, ifname,
                IEEE80211_WLANCONFIG_CFR_RCC_ALL_PKT,
                strtoint(argv[4]));
    } else if (argc == 9 && streq(argv[3], "ta_ra_addr")) {
        return handle_taraAddr_config(sock_ctx, ifname,
                strtoint(argv[4]), argv[5], argv[6], argv[7], argv[8]);
    } else if (argc == 7 && streq(argv[3], "bw_nss")) {
        return handle_tara_config(sock_ctx, ifname,
                IEEE80211_WLANCONFIG_CFR_RCC_BW_NSS,
                strtoint(argv[4]), strtoint(argv[5]), strtoint(argv[6]), 0);
    } else if (argc == 8 && streq(argv[3], "subtype")) {
        return handle_tara_config(sock_ctx, ifname,
                IEEE80211_WLANCONFIG_CFR_RCC_SUBTYPE, strtoint(argv[4]),
                strtoint(argv[5]), strtoint(argv[6]), strtoint(argv[7]));
    } else if (argc == 5 && streq(argv[3], "capture_dur")) {
        return handle_cfr_rcc_config(sock_ctx, ifname,
                IEEE80211_WLANCONFIG_CFR_RCC_CAPT_DUR,
                strtoint(argv[4]), 0);
    } else if (argc == 5 && streq(argv[3], "capture_intval")) {
        return handle_cfr_rcc_config(sock_ctx, ifname,
                IEEE80211_WLANCONFIG_CFR_RCC_CAPT_INTVAL,
                strtoint(argv[4]), 0);
    } else if (argc == 6 && streq(argv[3], "ul_mu_user_mask")) {
        return handle_cfr_rcc_config(sock_ctx, ifname,
                IEEE80211_WLANCONFIG_CFR_RCC_UL_MU_USER_MASK,
                strtoint(argv[4]), strtoint(argv[5]));
    } else if (argc == 6 && streq(argv[3], "freeze_tlv_delay_cnt")) {
        return handle_cfr_rcc_config(sock_ctx, ifname,
                IEEE80211_WLANCONFIG_CFR_RCC_FREEZE_TLV_DELAY_CNT,
                strtoint(argv[4]), strtoint(argv[5]));
    } else if (argc == 5 && streq(argv[3], "en_cfg")) {
        return handle_cfr_rcc_config(sock_ctx, ifname,
                IEEE80211_WLANCONFIG_CFR_EN_CFG,
                strtoint(argv[4]), 0);
    } else if (argc == 5 && streq(argv[3], "reset_cfg")) {
        return handle_cfr_rcc_config(sock_ctx, ifname,
                IEEE80211_WLANCONFIG_CFR_RESET_CFG,
                strtoint(argv[4]), 0);
    } else if (argc == 4 && streq(argv[3], "disable_all")) {
        return handle_cfr_rcc_config(sock_ctx, ifname,
                IEEE80211_WLANCONFIG_CFR_RCC_DISABLE_ALL,
                0, 0);
    } else if (argc == 4 && streq(argv[3], "commit")) {
        return handle_cfr_rcc_config(sock_ctx, ifname,
                IEEE80211_WLANCONFIG_CFR_RCC_COMMIT,
                0, 0);
    } else if (argc == 4 && streq(argv[3], "rcc_config_details")) {
            return handle_get_rcc(sock_ctx, ifname,
                                  IEEE80211_WLANCONFIG_CFR_GET_CFG);
    } else if (argc == 4 && streq(argv[3], "rcc_debug_counters")) {
            return handle_get_rcc(sock_ctx, ifname,
                                  IEEE80211_WLANCONFIG_CFR_RCC_DBG_COUNTERS);
    } else if (argc == 4 && streq(argv[3], "rcc_dump_lut")) {
            return handle_get_rcc(sock_ctx, ifname,
                                  IEEE80211_WLANCONFIG_CFR_RCC_DUMP_LUT);
    } else if (argc == 4 && streq(argv[3], "clr_dbg_counters")) {
            return handle_get_rcc(sock_ctx, ifname,
                                  IEEE80211_WLANCONFIG_CFR_RCC_CLR_COUNTERS);
#endif
    } else if (argc == 4 && streq(argv[3], "list")) {
        return handle_cfr_list_command(sock_ctx, ifname);
    } else {
        printf("Invalid CFR command format: Usage\n");
        printf("\t wlanconfig athX cfr start <peer MAC> <bw> <periodicity> <capture type>\n") ;
        printf("\t wlanconfig athX cfr stop <peer MAC>\n") ;
        printf("\t wlanconfig athX cfr list\n") ;
#ifdef WLAN_ENH_CFR_ENABLE
        printf("\t wlanconfig athx cfr m_ta_ra_filter <enable/disable>\n");
        printf("\t wlanconfig athx cfr m_directed_ndpa_ndp <enable/disable>\n");
        printf("\t wlanconfig athx cfr m_ndpa_ndp_all <enable/disable>\n");
        printf("\t wlanconfig athx cfr disable_all\n");
        printf("\t wlanconfig athx cfr ta_ra_addr <group_id> <ta> <ta_mask> <ra> <ra_mask>\n");
        printf("\t wlanconfig athx cfr bw_nss <group_id> <bw> <nss>\n");
        printf("\t wlanconfig athx cfr subtype <group_id> <mgmt> <ctrl> <data>\n");
        printf("\t wlanconfig athx cfr en_cfg <bitmask>\n");
        printf("\t wlanconfig athx cfr reset_cfg <bitmask>\n");
        printf("\t wlanconfig athx cfr capture_dur <value>\n");
        printf("\t wlanconfig athx cfr capture_intval <value>\n");
        printf("\t wlanconfig athx cfr ul_mu_user_mask <mask_lower_32> <mask_upper_32>\n");
        printf("\t wlanconfig athx cfr freeze_tlv_delay_cnt <freeze_tlv_delay_cnt_en> <threshold>\n");
        printf("\t wlanconfig athx cfr commit\n");
        printf("\t wlanconfig athx cfr rcc_config_details\n");
        printf("\t wlanconfig athx cfr rcc_debug_counters\n");
        printf("\t wlanconfig athx cfr clr_dbg_counters\n");
        printf("\t wlanconfig athx cfr rcc_dump_lut\n");
#endif
    }

    return 0;
}
#endif

#ifdef WLAN_SUPPORT_RX_FLOW_TAG
static int handle_rx_flow_tag_command(struct socket_context *sock_ctx, const char *ifname,
                                      int opcode, int ipver, char *srcip, char *dstip, int srcport,
                                      int dstport, int protocol, int tag)
{
    struct ieee80211_wlanconfig config;
    uint16_t inet_domain;

    memset(&config, 0, sizeof(struct ieee80211_wlanconfig));
    config.cmdtype = IEEE80211_WLANCONFIG_RX_FLOW_TAG_OP;

    if (ipver != 0 && ipver != 1) {
        printf("Invalid IPversion\n");
        return 1;
    }
    if (protocol != 0 && protocol != 1) {
        printf("Invalid protocol\n");
        return 1;
    }

    config.data.rx_flow_tag_info.op_code = opcode;

    config.data.rx_flow_tag_info.ip_ver = ipver;
    inet_domain = (ipver == 0)? AF_INET: AF_INET6;
    if (AF_INET == inet_domain) {
        if (!inet_pton(inet_domain, srcip, (struct in_addr *) &config.data.rx_flow_tag_info.flow_tuple.source_ip[3])) {
            printf("Could not convert src IP address to network order\n");
            return 1;
        }

        if (!inet_pton(inet_domain, dstip, (struct in_addr *) &config.data.rx_flow_tag_info.flow_tuple.dest_ip[3])) {
            printf("Could not convert dst IP address to network order\n");
            return 1;
        }
    } else {
        if (!inet_pton(inet_domain, srcip, (struct in_addr *) config.data.rx_flow_tag_info.flow_tuple.source_ip)) {
            printf("Could not convert src IP address to network order\n");
            return 1;
        }

        if (!inet_pton(inet_domain, dstip, (struct in_addr *) config.data.rx_flow_tag_info.flow_tuple.dest_ip)) {
            printf("Could not convert dst IP address to network order\n");
            return 1;
        }
    }

    config.data.rx_flow_tag_info.flow_tuple.source_port = srcport;
    config.data.rx_flow_tag_info.flow_tuple.dest_port = dstport;
    config.data.rx_flow_tag_info.flow_tuple.protocol = protocol;
    config.data.rx_flow_tag_info.flow_metadata = tag;

    send_command(sock_ctx, ifname, &config, sizeof(struct ieee80211_wlanconfig),
           NULL, QCA_NL80211_VENDOR_SUBCMD_RX_FLOW_TAG_OP, IEEE80211_IOCTL_CONFIG_GENERIC);
    return 0;
}

int handle_command_flow_tag(int argc, char *argv[], const char *ifname, struct socket_context *sock_ctx)
{
    if (argc != 11 && streq(argv[3], "0"))
        goto flow_tag_usage;

    if (argc < 10 && (streq(argv[3], "1") || streq(argv[3], "2")))
        goto flow_tag_usage;

    if (streq(argv[3], "0")) {
        return handle_rx_flow_tag_command(sock_ctx, ifname,
        atoi(argv[3]), atoi(argv[4]), argv[5], argv[6],
        atoi(argv[7]), atoi(argv[8]), atoi(argv[9]), atoi(argv[10]));
    } else if (streq(argv[3], "1") || streq(argv[3], "2")) {
        return handle_rx_flow_tag_command(sock_ctx, ifname,
                   atoi(argv[3]), atoi(argv[4]), argv[5], argv[6],
                   atoi(argv[7]), atoi(argv[8]), atoi(argv[9]), 0);
    }

flow_tag_usage:
    printf("Invalid command format: Usage \n");
    printf("\t wlanconfig wifiX rx_flow_tag_op <opcode: 0-Add, 1-Del, 2-Dump> <ipversion 0-IPv4, 1-IPv6> <src ip> <dest ip> <src port> <dst port> <protocol: 0-TCP 1-UDP> <16-bit tag>\n") ;
    return 0;
}
#endif /* WLAN_SUPPORT_RX_FLOW_TAG */

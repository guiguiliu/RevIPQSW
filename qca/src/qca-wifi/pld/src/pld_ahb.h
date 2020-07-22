/*
 * Copyright (c) 2017-2020 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 * Copyright (c) 2016-2017 The Linux Foundation. All rights reserved.
 */

#ifndef __PLD_AHB_H__
#define __PLD_AHB_H__

#ifdef CONFIG_PLD_PCIE_CNSS
#include <net/cnss2.h>
#endif
#include "pld_internal.h"

#ifndef CONFIG_PCI
static inline int pld_ahb_register_driver(void)
{
	return 0;
}

static inline void pld_ahb_unregister_driver(void)
{
	return;
}

static inline int pld_ahb_get_ce_id(int irq)
{
	return 0;
}
#else
int pld_ahb_register_driver(void);
void pld_ahb_unregister_driver(void);
int pld_ahb_get_ce_id(int irq);
#endif

#ifndef CONFIG_PLD_PCIE_CNSS
static inline int pld_ahb_wlan_enable(struct device *dev,
				       struct pld_wlan_enable_cfg *config,
				       enum pld_driver_mode mode,
				       const char *host_version)
{
	return 0;
}
static inline int pld_ahb_wlan_disable(struct device *dev,
					enum pld_driver_mode mode)
{
	return 0;
}
static inline void *pld_ahb_subsystem_get(struct device *dev, int device_id)
{
	return NULL;
}
static inline void pld_ahb_subsystem_put(struct device *dev)
{
	return;
}
#else
int pld_ahb_wlan_enable(struct device *dev, struct pld_wlan_enable_cfg *config,
			 enum pld_driver_mode mode, const char *host_version);
int pld_ahb_wlan_disable(struct device *dev, enum pld_driver_mode mode);
void *pld_ahb_subsystem_get(struct device *dev, int device_id);
void pld_ahb_subsystem_put(struct device *dev);
#endif

#if (!defined(CONFIG_PLD_PCIE_CNSS)) || (!defined(QCA_WIFI_3_0_ADRASTEA))
static inline int pld_ahb_set_fw_log_mode(u8 fw_log_mode)
{
	return 0;
}
static inline void pld_ahb_intr_notify_q6(void)
{
	return;
}
#else
static inline int pld_ahb_set_fw_log_mode(u8 fw_log_mode)
{
	return cnss_set_fw_debug_mode(fw_log_mode);
}
static inline void pld_ahb_intr_notify_q6(void)
{
	cnss_intr_notify_q6();
}
#endif

static inline int pld_ahb_get_sha_hash(const u8 *data,
					u32 data_len, u8 *hash_idx, u8 *out)
{
	return 0;
}
static inline void *pld_ahb_get_fw_ptr(void)
{
	return NULL;
}

#if (!defined(CONFIG_PLD_PCIE_CNSS)) || (!defined(CONFIG_PCI_MSM))
static inline int pld_ahb_wlan_pm_control(bool vote)
{
	return 0;
}
#else
static inline int pld_ahb_wlan_pm_control(bool vote)
{
	return cnss_wlan_pm_control(vote);
}
#endif

#ifndef CONFIG_PLD_PCIE_CNSS
static inline int
pld_ahb_get_fw_files_for_target(struct device *dev, struct pld_fw_files *pfw_files,
				 u32 target_type, u32 target_version)
{
	return 0;
}
static inline void pld_ahb_link_down(struct device *dev)
{
	return;
}
static inline int pld_ahb_shadow_control(bool enable)
{
	return 0;
}
static inline int
pld_ahb_set_wlan_unsafe_channel(u16 *unsafe_ch_list, u16 ch_count)
{
	return 0;
}
static inline int
pld_ahb_get_wlan_unsafe_channel(u16 *unsafe_ch_list,
				 u16 *ch_count, u16 buf_len)
{
	return 0;
}
static inline int pld_ahb_wlan_set_dfs_nol(void *info, u16 info_len)
{
	return 0;
}
static inline int pld_ahb_wlan_get_dfs_nol(void *info, u16 info_len)
{
	return 0;
}
static inline void pld_ahb_schedule_recovery_work(struct device *dev,
					   enum pld_recovery_reason reason)
{
	return;
}
static inline void *pld_ahb_get_virt_ramdump_mem(unsigned long *size)
{
	return NULL;
}
static inline void pld_ahb_device_crashed(void)
{
	return;
}
static inline void pld_ahb_device_self_recovery(struct device *dev,
					 enum pld_recovery_reason reason)
{
	return;
}
static inline void pld_ahb_request_pm_qos(u32 qos_val)
{
	return;
}
static inline void pld_ahb_remove_pm_qos(void)
{
	return;
}
static inline int pld_ahb_request_bus_bandwidth(int bandwidth)
{
	return 0;
}
static inline int pld_ahb_get_platform_cap(struct device *dev, struct pld_platform_cap *cap)
{
	return 0;
}
static inline int pld_ahb_get_soc_info(struct device *dev,
					struct pld_soc_info *info)
{
	return 0;
}
static inline void pld_ahb_set_driver_status(enum pld_driver_status status)
{
	return;
}
static inline int pld_ahb_auto_suspend(void)
{
	return 0;
}
static inline int pld_ahb_auto_resume(void)
{
	return 0;
}
static inline void pld_ahb_lock_pm_sem(void)
{
	return;
}
static inline void pld_ahb_release_pm_sem(void)
{
	return;
}
static inline int pld_ahb_power_on(struct device *dev)
{
	return 0;
}
static inline int pld_ahb_power_off(struct device *dev)
{
	return 0;
}

static inline uint8_t *pld_ahb_get_wlan_mac_address(struct device *dev,
						     uint32_t *num)
{
	*num = 0;
	return NULL;
}
static inline int pld_ahb_get_user_msi_assignment(struct device *dev,
						   char *user_name,
						   int *num_vectors,
						   uint32_t *user_base_data,
						   uint32_t *base_vector)
{
	return 0;
}
static inline int pld_ahb_get_msi_irq(struct device *dev, unsigned int vector)
{
	return 0;
}
static inline void pld_ahb_get_msi_address(struct device *dev,
					    uint32_t *msi_addr_low,
					    uint32_t *msi_addr_high)
{
	return;
}
#else
int pld_ahb_get_fw_files_for_target(struct device *dev, struct pld_fw_files *pfw_files,
				     u32 target_type, u32 target_version);
int pld_ahb_get_platform_cap(struct device *dev, struct pld_platform_cap *cap);
int pld_ahb_get_soc_info(struct device *dev, struct pld_soc_info *info);
void pld_ahb_set_driver_status(enum pld_driver_status status);
void pld_ahb_schedule_recovery_work(struct device *dev,
				     enum pld_recovery_reason reason);
void pld_ahb_device_self_recovery(struct device *dev,
				   enum pld_recovery_reason reason);

static inline void pld_ahb_link_down(struct device *dev)
{
	cnss_pci_link_down(dev);
}
static inline int pld_ahb_shadow_control(bool enable)
{
	return 0;
}
static inline int pld_ahb_set_wlan_unsafe_channel(u16 *unsafe_ch_list,
						   u16 ch_count)
{
	return  0;
}
static inline int pld_ahb_get_wlan_unsafe_channel(u16 *unsafe_ch_list,
						   u16 *ch_count, u16 buf_len)
{
	return  0;
}
static inline int pld_ahb_wlan_set_dfs_nol(void *info, u16 info_len)
{
	return  0;
}
static inline int pld_ahb_wlan_get_dfs_nol(void *info, u16 info_len)
{
	return  0;
}
static inline void *pld_ahb_get_virt_ramdump_mem(unsigned long *size)
{
	return  NULL;
}
static inline void pld_ahb_device_crashed(void)
{

}
static inline void pld_ahb_request_pm_qos(u32 qos_val)
{

}
static inline void pld_ahb_remove_pm_qos(void)
{

}
static inline int pld_ahb_request_bus_bandwidth(int bandwidth)
{
	/* TBD */
	/*return cnss_request_bus_bandwidth(bandwidth); */
	return -EINVAL;
}
static inline int pld_ahb_auto_suspend(void)
{
	return -EINVAL;
}
static inline int pld_ahb_auto_resume(void)
{
	return -EINVAL;
}
static inline void pld_ahb_lock_pm_sem(void)
{

}
static inline void pld_ahb_release_pm_sem(void)
{

}
static inline int pld_ahb_power_on(struct device *dev)
{
	return cnss_power_on_device((struct cnss_plat_data *)dev, 0x0);
}
static inline int pld_ahb_power_off(struct device *dev)
{
	return cnss_power_off_device((struct cnss_plat_data *)dev, 0x0);
}

static inline uint8_t *pld_ahb_get_wlan_mac_address(struct device *dev,
						     uint32_t *num)
{
	*num = 0;
	return NULL;
}
static inline int pld_ahb_get_user_msi_assignment(struct device *dev,
						   char *user_name,
						   int *num_vectors,
						   uint32_t *user_base_data,
						   uint32_t *base_vector)
{
	return cnss_get_user_msi_assignment(dev, user_name, num_vectors,
					    user_base_data, base_vector);
}
static inline int pld_ahb_get_msi_irq(struct device *dev, unsigned int vector)
{
	return cnss_get_msi_irq(dev, vector);
}
static inline void pld_ahb_get_msi_address(struct device *dev,
					    uint32_t *msi_addr_low,
					    uint32_t *msi_addr_high)
{
	cnss_get_msi_address(dev, msi_addr_low, msi_addr_high);
}
#endif
#endif

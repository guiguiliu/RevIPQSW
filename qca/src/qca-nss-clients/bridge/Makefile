ccflags-y := -I$(obj)/../exports -I$(obj)/..  -I$(obj)/nss_hal/include
ccflags-y += -DNSS_CLIENT_BUILD_ID="$(BUILD_ID)"
ccflags-$(NSS_BRIDGE_MGR_OVS_ENABLE) += -DNSS_BRIDGE_MGR_OVS_ENABLE

obj-m += qca-nss-bridge-mgr.o
qca-nss-bridge-mgr-objs := nss_bridge_mgr.o
ifeq ($(NSS_BRIDGE_MGR_OVS_ENABLE), y)
qca-nss-bridge-mgr-objs += nss_bridge_mgr_ovs.o
endif

ccflags-y += -DNSS_BRIDGE_MGR_DEBUG_LEVEL=0
ccflags-y += -Werror

ifeq ($(SoC),$(filter $(SoC),ipq807x ipq807x_64 ipq60xx ipq60xx_64))
ccflags-y += -DNSS_BRIDGE_MGR_PPE_SUPPORT
endif

ifeq ($(CONFIG_BONDING), $(filter $(CONFIG_BONDING), y m))
ifneq ($(findstring 4.4, $(KERNELVERSION)),)
ccflags-y += -DBONDING_SUPPORT
endif
endif

#
# Copyright (C) 2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=attrs
PKG_VERSION:=15.2.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://pypi.python.org/packages/source/a/attrs
PKG_MD5SUM:=b3c460eb6482f6e557c0e4025475c007

PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=LICENSE
PKG_MAINTAINER:=Jeffery To <jeffery.to@gmail.com>

include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/package.mk
$(call include_mk, python-package.mk)

define Package/python-attrs
	SECTION:=lang
	CATEGORY:=Languages
	SUBMENU:=Python
	TITLE:=python-attrs
	URL:=https://attrs.readthedocs.org/
	DEPENDS:=+python-light
endef

define Package/python-attrs/description
attrs is an MIT-licensed Python package with class decorators that ease
the chores of implementing the most common attribute-related object
protocols.
endef

define Build/Compile
	$(call Build/Compile/PyMod,,install --prefix="/usr" --root="$(PKG_INSTALL_DIR)")
endef

define Host/Install
	$(INSTALL_DIR) $(HOST_PYTHON_LIB_DIR)
	$(call Build/Compile/PyMod,,install --prefix="host" --root="$(STAGING_DIR)")
endef

define Build/InstallDev
	$(INSTALL_DIR) $(HOST_PYTHON_LIB_DIR)
	$(CP) $(PKG_INSTALL_DIR)/usr/* $(STAGING_DIR_HOST)/
endef

$(eval $(call HostBuild))
$(eval $(call PyPackage,python-attrs))
$(eval $(call BuildPackage,python-attrs))

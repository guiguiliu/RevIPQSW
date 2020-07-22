LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)/include -I$(LK_TOP_DIR)/platform/msm_shared

PLATFORM := ipq807x

MEMBASE := 0x4A900000 # SDRAM
MEMSIZE := 0x00100000 # 1MB

#BASE_ADDR        := 0x41500000	# banana
BASE_ADDR        := 0x40000000	# date

TAGS_ADDR        := 0x44000000
KERNEL_ADDR      := BASE_ADDR+0x00008000
RAMDISK_ADDR     := BASE_ADDR+0x01000000
SCRATCH_ADDR     := 0x50500000

DEFINES += PERIPH_BLK_BLSP=1

MODULES += \
	lib/zlib_inflate \
	lib/ptable \
	lib/libfdt

DEFINES += \
	MEMSIZE=$(MEMSIZE) \
	MEMBASE=$(MEMBASE) \
	BASE_ADDR=$(BASE_ADDR) \
	TAGS_ADDR=$(TAGS_ADDR) \
	KERNEL_ADDR=$(KERNEL_ADDR) \
	RAMDISK_ADDR=$(RAMDISK_ADDR) \
	SCRATCH_ADDR=$(SCRATCH_ADDR)

OBJS += \
	$(LOCAL_DIR)/boards.o \
	$(LOCAL_DIR)/init.o \
	$(LOCAL_DIR)/meminfo.o

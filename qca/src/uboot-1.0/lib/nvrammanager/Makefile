# Makefile for nvrammngr lib.
# This lib is needed for tplink firmware recovery function support.
#

include $(TOPDIR)/config.mk

LIB	= $(obj)libnvrammanager.o

COBJS-$(CONFIG_NVRAMMANAGER) += missing.o nm_api.o nm_fwup.o nm_lib.o sysProductInfo.o bigNumber.o shaAndSha512.o rsaVerify.o #fwrecovery_api.o fwrecovery_gpio.o
COBJS-$(CONFIG_NVRAMMANAGER_SPI) += nm_spi.o

COBJS := $(COBJS-y)
SRCS  := $(COBJS:.o=.c)
OBJS  := $(addprefix $(obj),$(COBJS))

$(LIB):	$(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS))

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################

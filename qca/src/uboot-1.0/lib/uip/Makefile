# Makefile for nvrammngr lib.
# This lib is needed for tplink firmware recovery function support.
#

include $(TOPDIR)/config.mk

LIB	= $(obj)libuip.o

COBJS-$(CONFIG_UIP_HTTPD) += uip.o uip_arp.o uiplib.o psock.o timer.o uip-neighbor.o httpd.o http-strings.o httpd-fs.o httpd-cgi.o

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

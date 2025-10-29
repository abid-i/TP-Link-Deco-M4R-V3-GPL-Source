/******************************************************************************
Copyright (c) 2009-2013 TP-Link Technologies CO.,LTD.  All rights reserved. 

File name	: br_deliver.h
Version		: v1.0, release version
Description	: This file defines the dual wifi repeater connection functions
		  for some dual band repeaters, such as wa3500re(EU) 1.0.
 
Author		: huanglifu <huanglifu@tp-link.net>
Create date	: 2013/5/27

History		:
01, 2013/05/27 huanglifu, Created file.
02, 2013/09/23 tengfei, Modify file name and some descriptions to remove info
	       related special product(wda3150).
03, 2014/07/11 Zhou Guofeng ,add the proc file for naming the devive ,debug  and 
              fix the bug that block in-loop packet
*****************************************************************************/
#ifndef __TP_DELIVER_H
#define __TP_DELIVER_H

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/inetdevice.h>


//#define BRIDGE_DELIVER_DEBUG_ON_OFF
//#ifdef BRIDGE_DELIVER_DEBUG_ON
#define REPEATER_PRINT(args) do{if(br->bridge_deliver_dbg)printk args;}while(0)
//#else
//#define REPEATER_PRINT(args) do{printk args;}while(0)
//#endif

/*RE Repeater 设备名称*/

#define BRIDGE_DELIVER_DEV_NAME_LEN  10
#define BRIDGE_DELIVER_DEV_NUM	      7
#define BRIDGE_DELIVER_SUBDEV_NUM     3
#define BRIDGE_DELIVER_DEV_MAX_SIZE BRIDGE_DELIVER_DEV_NAME_LEN*BRIDGE_DELIVER_DEV_NUM*BRIDGE_DELIVER_SUBDEV_NUM

enum BRIDGE_GROUP_TYPE
{
	BRIDGE_RELAY_HOST_GROUP      = 1,
	BRIDGE_RELAY_BACKHAUL_GROUP  = 2,
	BRIDGE_NORELAY_GROUP         = 3,
	BRIDGE_ETH_GROUP             = 4,
	BRIDGE_PLC_GROUP             = 5		
};

#define BRIDGE_GROUP_MAX_TYPE  8 
#define BRIDGE_GROUP_DEV_NUM   6
#define BRIDGE_DELIVER_GROUP_MAX_SIZE BRIDGE_DELIVER_DEV_NAME_LEN*BRIDGE_GROUP_MAX_TYPE*BRIDGE_GROUP_DEV_NUM

#define BRIDGE_DELIVER_2G_DEV_NAME (br->bridge_name_matrix)[0]
#define BRIDGE_DELIVER_5G_DEV_NAME (br->bridge_name_matrix)[1]
#define BRIDGE_DELIVER_LAN_ETH_NAME (br->bridge_name_matrix)[2]
#define BRIDGE_DELIVER_NAME (br->bridge_name_matrix)[3]
#define BRIDGE_DELIVER_2G_STA_NAME (br->bridge_name_matrix)[4]
#define BRIDGE_DELIVER_5G_STA_NAME (br->bridge_name_matrix)[5]
#define BRIDGE_DELIVER_PLC_STA_NAME (br->bridge_name_matrix)[6]


#define MAX_BACK_DEV_BR_INDEX_VALUE 4

#define DUAL_REPEATER_HAS_NONE_FROMT_DUT 0
#define DUAL_REPEATER_HAS_SAME_FROMT_DUT 1
#define DUAL_REPEATER_HAS_TWO_FROMT_DUT 2


/*最大的设备索引值，用于生成过滤表大小*/
#define MAX_DEV_IFINDEX_VALUE 10

/* tp double frequence forward info table */
struct tp_db_fre_bridge_fdb_entry 
{
    int is_enabled;              //记录该表是否生效，由用户程序配置，如果是双频wisp模式标记为1，否则标记为0；
    int link_state;				 //由无线模块适时修改该状态，记录2/5g链路状态是否正常，br转发设备选择时会用到该状态；
	int eth_goto_2g_flag;		 //记录来自有线口及网桥本身与前端DUT通信的链路是2G还是5G.
	int is_same_front_dut;	     //用于标记双频Repeater是否是同时链接到同一前端路由器。
	
		
    int ap_dev_2g_idx;           //记录ap端2G无线网口设备索引，即ra0索引
    int ap_dev_5g_idx;           //记录ap端5G无线网口设备索引，即ra1索引
    int client_dev_plc_idx;
    int client_dev_2g_idx;       //记录clinent端2G无线网口设备索引，即cli0索引
    int client_dev_5g_idx;       //记录clinent端5G无线网口设备索引，即cli1索引
    int eth_dev_idx;		 	 //记录ap端有线网口设备索引，即eth2.1索引
    int br0_dev_idx;			 //记录ap端网桥设备索引，即br0索引

    struct 	net_device* client_plc_dev;    
    struct 	net_device* client_2g_dev; //记录clinent端2G无线网口设备指针
	struct 	net_device* client_5g_dev; //记录clinent端5G无线网口设备指针
	struct 	net_device* br0_dev;	   //记录网桥设备指针
	/*struct 	net_device* eth_dev;*/	   //记录Eth0 设备指针
	struct 	net_bridge*  br0_bri_dev;	//记录网桥设备指针
		
    char filter_table_2g_5g[MAX_DEV_IFINDEX_VALUE][MAX_DEV_IFINDEX_VALUE]; //双频2G&5G网桥转发过滤表
    char filter_table_only_2g[MAX_DEV_IFINDEX_VALUE][MAX_DEV_IFINDEX_VALUE];//双频2G网桥转发过滤表
    char filter_table_only_5g[MAX_DEV_IFINDEX_VALUE][MAX_DEV_IFINDEX_VALUE];//双频5G网桥转发过滤表
    char filter_table_none[MAX_DEV_IFINDEX_VALUE][MAX_DEV_IFINDEX_VALUE];
    char filter_table_only_plc[MAX_DEV_IFINDEX_VALUE][MAX_DEV_IFINDEX_VALUE];
    char filter_table_5g_plc[MAX_DEV_IFINDEX_VALUE][MAX_DEV_IFINDEX_VALUE];  
    char filter_table_2g_plc[MAX_DEV_IFINDEX_VALUE][MAX_DEV_IFINDEX_VALUE];  
    char filter_table_2g_5g_plc[MAX_DEV_IFINDEX_VALUE][MAX_DEV_IFINDEX_VALUE];     
    
};

#include "br_private.h"

enum TP_LINK_STATE
{
	LINK_STATE_OFF = 0,
	LINK_STATE_ON  = 1,
};

enum TP_LINK_STATE_TYPE
{
	LINK_STATE_NONE = 0,
	LINK_STATE_ONLY_2G = 2,
	LINK_STATE_ONLY_5G = 4,
	LINK_STATE_ONLY_PLC = 8,
    LINK_STATE_WIFI_2G_5G = LINK_STATE_ONLY_2G | LINK_STATE_ONLY_5G,
    LINK_STATE_5G_PLC = LINK_STATE_ONLY_5G | LINK_STATE_ONLY_PLC,
    LINK_STATE_2G_PLC = LINK_STATE_ONLY_2G | LINK_STATE_ONLY_PLC,    
	LINK_STATE_ALL_2G_5G_PLC = LINK_STATE_ONLY_2G | LINK_STATE_ONLY_5G | LINK_STATE_ONLY_PLC,
};

/*获得设备索引*/
#define GET_DEV_INDEX_BY_NAME(tmpDev, devIndex, NAME) \
	do{\
        br_sub_dev_index_count++;\
        devIndex = br_sub_dev_index_count;\
        noDev = 1;\
        for (dev_sub_index = 0; dev_sub_index < BRIDGE_DELIVER_SUBDEV_NUM; dev_sub_index++)\
        {\
            if (NAME[dev_sub_index][0] != '\0')\
            {\
                tmpDev = dev_get_by_name(&init_net, NAME[dev_sub_index]);\
                if (NULL != tmpDev)\
                {\
                    tmpDev->br_port_index = br_sub_dev_index_count;\
			        if (MAX_DEV_IFINDEX_VALUE <= devIndex)\
			        {\
				        printk("%s %d dev name = %s br_port_index to large.\n", __FUNCTION__, __LINE__, tmpDev->name);\
				        dev_put(tmpDev);\
				        goto INIT_FAIL;\
			        }\
			        dev_put(tmpDev);\
			        noDev = 0;\
			    }\
			    else if (br_sub_dev_index_count < MAX_BACK_DEV_BR_INDEX_VALUE)\
			    {\
			        continue;\
			    }\
                else\
		        {\
			        printk("%s %d dev name = %s is not exist.\n", __FUNCTION__, __LINE__, NAME[dev_sub_index]);\
			        continue;\
		        }\
            }\
        }\
        if (noDev && br_sub_dev_index_count >= MAX_BACK_DEV_BR_INDEX_VALUE)\
        {\
                printk("%s %d front dev %d is not exist.\n", __FUNCTION__, __LINE__, br_sub_dev_index_count);\
        }\
	}while(0)

/*获得设备引用*/
#define GET_DEV_BY_NAME(Dev, NAME) \
	do{\
		Dev = dev_get_by_name(&init_net, NAME);\
		if (NULL == Dev)\
		{\
			printk("%s %d dev name = %s is not exist.\n", __FUNCTION__, __LINE__, NAME);\
		}\
		else \
		{\
		    dev_put(Dev);\
        }\
	}while(0)


#define CHECK_AND_INIT_TABLE(br, skb, ret) \
	do{\
		if (LINK_STATE_OFF == br->bridge_deliver_enable) \
		{\
			return ret;\
		}\
		\
		if (LINK_STATE_OFF == p->is_enabled)\
		{\
			if (LINK_STATE_OFF == init_free_bridge_fdb(br))\
			{\
				return ret;\
			}\
		}\
	}while(0)
	



/*根据不同的过滤表，获取当前索引对应过滤规则*/
#define IS_DEV_FILTERED(table, pSrc, pDst) ((NULL == pSrc) || (NULL == pDst) || (table[pSrc->br_port_index][pDst->br_port_index] == 1))

/*设备过滤表项的值*/
#define FILTER_SET_VALUE_ON(table, srcIdx, DstIdx) 	table[srcIdx][DstIdx] = 1;
#define FILTER_SET_VALUE_OFF(table, srcIdx, DstIdx) 	table[srcIdx][DstIdx] = 0;
#define FILTER_SET_WHOLE_VALUE_ON(table, srcIdx) \
	do{\
        for (dev_index = 0; dev_index < MAX_DEV_IFINDEX_VALUE; dev_index++)\
		{\
			table[srcIdx][dev_index] = 1;\
			table[dev_index][srcIdx] = 1;\
		}\
	}while(0)    


/*根据标志位，获得2G or 5G相应的索引*/
#define GET_2G_5G_DEV_INDEX_BY_FLAG(flag, client_dev_5g_idx, client_dev_2g_idx) (flag == 1 ? client_dev_5g_idx : client_dev_2g_idx)

int br_set_group(struct net_bridge *br);
int br_set_link_state(struct net_bridge *br, unsigned long val);
int br_set_eth_to_2g_enable (struct net_bridge *br, unsigned long val);
int br_db_fre_enable(struct net_bridge *br);
int  br_db_fre_should_deliver(struct net_bridge_port *to, struct sk_buff *skb);
int  br_db_fre_fdb_lookup(struct net_bridge_port **to, struct sk_buff *skb);
int  br_should_deliver_packet(struct net_bridge *br, struct sk_buff *skb, u16 vid);
unsigned br_fdb_update_mapped_hwaddr(struct net_bridge *br, struct net_bridge_port *source,
		  struct sk_buff *skb, u16 vid, bool added_by_user);

#endif //__TP_DELIVER_H


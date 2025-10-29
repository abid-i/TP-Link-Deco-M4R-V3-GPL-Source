/******************************************************************************
Copyright (c) 2009-2013 TP-Link Technologies CO.,LTD.  All rights reserved. 

File name	: br_deliver.c
Version		: v1.0, release version
Description	: This file impleaments the dual wifi repeater connection func
		  for some dual band repeaters, such as wa3500re(EU) 1.0.
 
Author		: huanglifu <huanglifu@tp-link.net>
Create date	: 2013/5/27

History		:
01, 2013/05/27 huanglifu, Created file.
02, 2013/09/23 tengfei, Modify file name and some descriptions to remove info
               related special product(wda3150).
03, 2014/07/11 Zhou Guofeng ,add the proc file for naming the devive ,debug  and 
              fix the bug that block in-loop packet, add non-connection filter table 
03, 2014/10/08 Zhou Guofeng ,remove 2g_cli_enable and 5g_cli_enable, using link_state instead.        
*****************************************************************************/
#include "br_deliver.h"
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/list.h>
#include <net/arp.h>

/*========================================================*/
/*================            GLOBAL VARIABLES             ================*/
/*========================================================*/



/*========================================================*/
/*================            FUNCTION             ======================*/
/*========================================================*/

static void init_filter_table(struct tp_db_fre_bridge_fdb_entry *p)
{
	unsigned int dev_index = 0;
    
	if (NULL == p)
	{	
		return;
	}
	
	memset(&p->filter_table_2g_5g, 0, sizeof(p->filter_table_2g_5g));
	memset(&p->filter_table_only_2g, 0, sizeof(p->filter_table_only_2g));
	memset(&p->filter_table_only_5g, 0, sizeof(p->filter_table_only_5g));
	memset(&p->filter_table_none, 0, sizeof(p->filter_table_none));
	memset(&p->filter_table_only_plc, 0, sizeof(p->filter_table_only_plc));  
	memset(&p->filter_table_2g_plc, 0, sizeof(p->filter_table_2g_plc));        
	memset(&p->filter_table_5g_plc, 0, sizeof(p->filter_table_2g_plc)); 
	memset(&p->filter_table_2g_5g_plc, 0, sizeof(p->filter_table_2g_plc));

    /*
     * FILETER RULE TABLE FOR 2g5gPLC. 5g->5g, eth->5g, 2g->plc
     * when value is 1, it mean that from dev should not deliver to dev. else is allowed.
     * from\to| AP2G    | AP5G    | CLI2G    | CLI5G    | BR0   | ETH | PLC
     * AP2G   |  -      |    0    |    1     |   1      |  0    |  0  |  0
     * AP5G   |  0      |  -      |    1     |   0      |  0    |  0  |  1
     * CLI2G  |  1      |  1      |    -     |   1      |  1    |  1  |  1
     * CLI5G  |  1      |  0      |    1     |   -      |  0    |  0  |  1
     * BR0    |  0      |  0      |    1     |   0      |  -    |  0  |  1
     * ETH    |  0      |  0      |    1     |   0      |  0    |  -  |  1
     * PLC    |  0      |  1      |    1     |   1      |  1    |  1  |  -    
    */
    FILTER_SET_WHOLE_VALUE_ON(p->filter_table_2g_5g_plc, p->client_dev_2g_idx);
    FILTER_SET_WHOLE_VALUE_ON(p->filter_table_2g_5g_plc, p->client_dev_plc_idx);
    FILTER_SET_VALUE_OFF(p->filter_table_2g_5g_plc, p->client_dev_plc_idx, p->ap_dev_2g_idx);
    FILTER_SET_VALUE_OFF(p->filter_table_2g_5g_plc, p->ap_dev_2g_idx, p->client_dev_plc_idx);

    FILTER_SET_VALUE_ON(p->filter_table_2g_5g_plc, p->client_dev_5g_idx, p->ap_dev_2g_idx);    
    FILTER_SET_VALUE_ON(p->filter_table_2g_5g_plc, p->ap_dev_2g_idx, p->client_dev_5g_idx);
    
    /*
     * FILETER RULE TABLE FOR 5gPLC. 5g->5g, eth->5g, 2g->plc
     * when value is 1, it mean that from dev should not deliver to dev. else is allowed.
     * from\to| AP2G    | AP5G    | CLI2G    | CLI5G    | BR0   | ETH | PLC
     * AP2G   |  -      |    0    |    1     |   1      |  0    |  0  |  0
     * AP5G   |  0      |  -      |    1     |   0      |  0    |  0  |  1
     * CLI2G  |  1      |  1      |    -     |   1      |  1    |  1  |  1
     * CLI5G  |  1      |  0      |    1     |   -      |  0    |  0  |  1
     * BR0    |  0      |  0      |    1     |   0      |  -    |  0  |  1
     * ETH    |  0      |  0      |    1     |   0      |  0    |  -  |  1
     * PLC    |  0      |  1      |    1     |   1      |  1    |  1  |  -    
    */
    FILTER_SET_WHOLE_VALUE_ON(p->filter_table_5g_plc, p->client_dev_2g_idx);
    FILTER_SET_WHOLE_VALUE_ON(p->filter_table_5g_plc, p->client_dev_plc_idx);
    FILTER_SET_VALUE_OFF(p->filter_table_5g_plc, p->client_dev_plc_idx, p->ap_dev_2g_idx);
    FILTER_SET_VALUE_OFF(p->filter_table_5g_plc, p->ap_dev_2g_idx, p->client_dev_plc_idx);

    FILTER_SET_VALUE_ON(p->filter_table_5g_plc, p->client_dev_5g_idx, p->ap_dev_2g_idx);    
    FILTER_SET_VALUE_ON(p->filter_table_5g_plc, p->ap_dev_2g_idx, p->client_dev_5g_idx);     

    /*
     * FILETER RULE TABLE FOR only 2gPLC. 5g->plc eth->plc 2.4g->2.4g
     * when value is 1, it mean that from dev should not deliver to dev. else is allowed.
     * from\to| AP2G    | AP5G    | CLI2G    | CLI5G    | BR0   | ETH | PLC
     * AP2G   |  -      |    0    |    0     |   1      |  0    |  0  |  1
     * AP5G   |  0      |  -      |    1     |   1      |  0    |  0  |  0
     * CLI2G  |  0      |  1      |    -     |   1      |  1    |  1  |  1
     * CLI5G  |  1      |  1      |    1     |   -      |  1    |  1  |  1
     * BR0    |  0      |  0      |    1     |   1      |  -    |  0  |  0
     * ETH    |  0      |  0      |    1     |   1      |  0    |  -  |  0
     * PLC    |  1      |  0      |    1     |   1      |  0    |  0  |  -    
    */
    FILTER_SET_WHOLE_VALUE_ON(p->filter_table_2g_plc, p->client_dev_2g_idx);
    FILTER_SET_VALUE_OFF(p->filter_table_2g_plc, p->client_dev_2g_idx, p->ap_dev_2g_idx);
    FILTER_SET_VALUE_OFF(p->filter_table_2g_plc, p->ap_dev_2g_idx, p->client_dev_2g_idx);
    
    FILTER_SET_WHOLE_VALUE_ON(p->filter_table_2g_plc, p->client_dev_5g_idx);

    FILTER_SET_VALUE_ON(p->filter_table_2g_plc, p->client_dev_plc_idx, p->ap_dev_2g_idx);    
    FILTER_SET_VALUE_ON(p->filter_table_2g_plc, p->ap_dev_2g_idx, p->client_dev_plc_idx);      

    
    /*
     * FILETER RULE TABLE FOR only PLC.
     * when value is 1, it mean that from dev should not deliver to dev. else is allowed.
     * from\to| AP2G    | AP5G    | CLI2G    | CLI5G    | BR0   | ETH | PLC
     * AP2G   |  -      |    0    |    1     |   1      |  0    |  0  |  0
     * AP5G   |  0      |  -      |    1     |   1      |  0    |  0  |  0
     * CLI2G  |  1      |  1      |    -     |   1      |  1    |  1  |  1
     * CLI5G  |  1      |  1      |    1     |   -      |  1    |  1  |  1
     * BR0    |  0      |  0      |    1     |   1      |  -    |  0  |  0
     * ETH    |  0      |  0      |    1     |   1      |  0    |  -  |  0
     * PLC    |  0      |  0      |    1     |   1      |  0    |  0  |  -    
    */
    FILTER_SET_WHOLE_VALUE_ON(p->filter_table_only_plc, p->client_dev_2g_idx);
    FILTER_SET_WHOLE_VALUE_ON(p->filter_table_only_plc, p->client_dev_5g_idx);
    
	
    /*
     * FILETER RULE TABLE FOR wifi 2G & 5G.
     * when value is 1, it mean that from dev should not deliver to dev. else is allowed.
     * from\to| AP2G    | AP5G    | CLI2G    | CLI5G    | BR0   | ETH | PLC
     * AP2G   |  -      |    0    |     0    |   1      |  0    |  0  |  1
     * AP5G   |  0      |  -      |    1     |   0      |  0    |  0  |  1
     * CLI2G  |  0      |  1      |    -     |   1      |  0    |  0  |  1
     * CLI5G  |  1      |  0      |    1     |   -      |  1    |  1  |  1
     * BR0    |  0      |  0      |    0     |   1      |  -    |  0  |  1
     * ETH    |  0      |  0      |    0     |   1      |  0    |  -  |  1
     * PLC    |  1      |  1      |    1     |   1      |  1    |  1  |  -    
    */
	FILTER_SET_VALUE_ON(p->filter_table_2g_5g, p->client_dev_2g_idx, p->client_dev_5g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_2g_5g, p->client_dev_2g_idx, p->ap_dev_5g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_2g_5g, p->ap_dev_5g_idx, p->client_dev_2g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_2g_5g, p->ap_dev_2g_idx, p->client_dev_5g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_2g_5g, p->client_dev_5g_idx, p->client_dev_2g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_2g_5g, p->client_dev_5g_idx, p->ap_dev_2g_idx);	

	FILTER_SET_VALUE_ON(p->filter_table_2g_5g, p->br0_dev_idx, 
		GET_2G_5G_DEV_INDEX_BY_FLAG(p->eth_goto_2g_flag, p->client_dev_5g_idx, p->client_dev_2g_idx));	
	FILTER_SET_VALUE_ON(p->filter_table_2g_5g, 
		GET_2G_5G_DEV_INDEX_BY_FLAG(p->eth_goto_2g_flag, p->client_dev_5g_idx, p->client_dev_2g_idx), 
		p->br0_dev_idx);	

	FILTER_SET_VALUE_ON(p->filter_table_2g_5g, p->eth_dev_idx, 
		GET_2G_5G_DEV_INDEX_BY_FLAG(p->eth_goto_2g_flag, p->client_dev_5g_idx, p->client_dev_2g_idx));
	FILTER_SET_VALUE_ON(p->filter_table_2g_5g, 
		GET_2G_5G_DEV_INDEX_BY_FLAG(p->eth_goto_2g_flag, p->client_dev_5g_idx, p->client_dev_2g_idx), 
		p->eth_dev_idx);

    FILTER_SET_WHOLE_VALUE_ON(p->filter_table_2g_5g, p->client_dev_plc_idx);

    /*
     * FILETER TABLE FOR 2G ONLY.
     * when value is 1, it mean that from dev should not deliver to dev. else is allowed.
     * from\to| AP2G | AP5G | CLI2G | CLI5G | BR0 | ETH | PLC
     * AP2G   |  -   |  0   |   0   |   1   |  0  |  0  |  1
     * AP5G   |  0   |  -   |   0   |   1   |  0  |  0  |  1
     * CLI2G  |  0   |  0   |   -   |   1   |  0  |  0  |  1
     * CLI5G  |  1   |  1   |   1   |   -   |  1  |  1  |  1
     * BR0    |  0   |  0   |   0   |   1   |  -  |  0  |  1
     * ETH    |  0   |  0   |   0   |   1   |  0  |  -  |  1
     * PLC    |  1   |  1   |   1   |   1   |  1  |  1  |  -      
    */
	FILTER_SET_VALUE_ON(p->filter_table_only_2g, p->client_dev_5g_idx, p->client_dev_2g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_only_2g, p->client_dev_5g_idx, p->ap_dev_2g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_only_2g, p->client_dev_5g_idx, p->ap_dev_5g_idx);	
	FILTER_SET_VALUE_ON(p->filter_table_only_2g, p->client_dev_5g_idx, p->br0_dev_idx);
	FILTER_SET_VALUE_ON(p->filter_table_only_2g, p->client_dev_5g_idx, p->eth_dev_idx);
	
	FILTER_SET_VALUE_ON(p->filter_table_only_2g, p->ap_dev_5g_idx, p->client_dev_5g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_only_2g, p->client_dev_2g_idx, p->client_dev_5g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_only_2g, p->ap_dev_2g_idx, p->client_dev_5g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_only_2g, p->eth_dev_idx, p->client_dev_5g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_only_2g, p->br0_dev_idx, p->client_dev_5g_idx);
    
    FILTER_SET_WHOLE_VALUE_ON(p->filter_table_2g_5g, p->client_dev_plc_idx);

    /*
     * FILETER TABLE FOR 5G ONLY.
     * when value is 1, it mean that from dev should not deliver to dev. else is allowed.
     * from\to| AP2G | AP5G | CLI2G | CLI5G | BR0 | ETH | PLC
     * AP2G   |  -   |  0   |   1   |   0   |  0  |  0  |  1
     * AP5G   |  0   |  -   |   1   |   0   |  0  |  0  |  1
     * CLI2G  |  1   |  1   |   -   |   1   |  1  |  1  |  1
     * CLI5G  |  0   |  0   |   1   |   -   |  0  |  0  |  1
     * BR0    |  0   |  0   |   1   |   0   |  -  |  0  |  1
     * ETH    |  0   |  0   |   1   |   0   |  0  |  -  |  1
     * PLC    |  1   |  1   |   1   |   1   |  1  |  1  |  - 
    */

	FILTER_SET_VALUE_ON(p->filter_table_only_5g, p->client_dev_2g_idx, p->client_dev_5g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_only_5g, p->client_dev_2g_idx, p->ap_dev_2g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_only_5g, p->client_dev_2g_idx, p->ap_dev_5g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_only_5g, p->client_dev_2g_idx, p->br0_dev_idx);
	FILTER_SET_VALUE_ON(p->filter_table_only_5g, p->client_dev_2g_idx, p->eth_dev_idx);
	
	FILTER_SET_VALUE_ON(p->filter_table_only_5g, p->ap_dev_5g_idx, p->client_dev_2g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_only_5g, p->client_dev_5g_idx, p->client_dev_2g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_only_5g, p->ap_dev_2g_idx, p->client_dev_2g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_only_5g, p->eth_dev_idx, p->client_dev_2g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_only_5g, p->br0_dev_idx, p->client_dev_2g_idx);

    FILTER_SET_WHOLE_VALUE_ON(p->filter_table_2g_5g, p->client_dev_plc_idx);
    
/*
 * FILETER TABLE FOR NONE Connection
 * when value is 1, it mean that from dev should not deliver to dev. else is allowed.
 * from\to| AP2G    | AP5G    | CLI2G    | CLI5G    | BR0    | ETH  | PLC
 * AP2G   |    -    |    0    |    1     |   1      |    0   |  0   |  1
 * AP5G   |    0    |    -    |    1     |   1      |    0   |  0   |  1
 * CLI2G  |    1    |    1    |    -     |   1      |    1   |  1   |  1
 * CLI5G  |    1    |    1    |    1     |   -      |    0   |  1   |  1
 * BR0    |     0   |    0    |    1     |   0      |    -   |  0   |  1
 * ETH    |     0   |    0    |    1     |   1      |    0   |  -   |  1
 * PLC    |     1   |    1    |    1     |   1      |    1   |  1   |  - 
*/
	FILTER_SET_VALUE_ON(p->filter_table_none, p->ap_dev_2g_idx, p->client_dev_2g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_none, p->ap_dev_2g_idx, p->client_dev_5g_idx);
	
	FILTER_SET_VALUE_ON(p->filter_table_none, p->ap_dev_5g_idx, p->client_dev_2g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_none, p->ap_dev_5g_idx, p->client_dev_5g_idx)

	FILTER_SET_VALUE_ON(p->filter_table_none, p->client_dev_2g_idx, p->ap_dev_2g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_none, p->client_dev_2g_idx, p->ap_dev_5g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_none, p->client_dev_2g_idx, p->client_dev_5g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_none, p->client_dev_2g_idx, p->eth_dev_idx);
	
	FILTER_SET_VALUE_ON(p->filter_table_none, p->client_dev_5g_idx, p->ap_dev_2g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_none, p->client_dev_5g_idx, p->ap_dev_5g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_none, p->client_dev_5g_idx, p->client_dev_2g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_none, p->client_dev_5g_idx, p->eth_dev_idx);

	FILTER_SET_VALUE_ON(p->filter_table_none, p->br0_dev_idx, 
		GET_2G_5G_DEV_INDEX_BY_FLAG(p->eth_goto_2g_flag, p->client_dev_5g_idx, p->client_dev_2g_idx));	
	FILTER_SET_VALUE_ON(p->filter_table_none, 
		GET_2G_5G_DEV_INDEX_BY_FLAG(p->eth_goto_2g_flag, p->client_dev_5g_idx, p->client_dev_2g_idx), 
		p->br0_dev_idx);	

	FILTER_SET_VALUE_ON(p->filter_table_none, p->eth_dev_idx, p->client_dev_2g_idx);
	FILTER_SET_VALUE_ON(p->filter_table_none, p->eth_dev_idx, p->client_dev_5g_idx);

    FILTER_SET_WHOLE_VALUE_ON(p->filter_table_2g_5g, p->client_dev_plc_idx);
	
	return;
}

static unsigned int tp_get_br_front_port_ip(struct net_bridge *br, unsigned int src_ip)
{
	struct tp_db_fre_bridge_fdb_entry *p = &(br->free_bridage_fdb);
	unsigned int dst_ip = 0;	
	int i;

	
	for (i = 0; i < BR_HASH_SIZE && !dst_ip; i++) {
		struct net_bridge_fdb_entry *f;
		struct hlist_node *n;

		hlist_for_each_entry_safe(f, n, &br->hash[i], hlist) {			
		 	
			if (f->dst->dev->br_port_index == p->client_dev_2g_idx
				|| f->dst->dev->br_port_index == p->client_dev_5g_idx)
			{				
				if(src_ip != f->ip){
					dst_ip = f->ip;
					break;
				}
			}
		}
	}
	
	
	return dst_ip;	
}


static void br_do_br0_send_arp_request_packet_proxy(struct net_bridge *br)
{
	struct tp_db_fre_bridge_fdb_entry *p = &(br->free_bridage_fdb);
	struct net_device* tmp_dev = NULL;
	struct in_device *pdev_ipaddr=NULL;
	unsigned int dst_ip = 0;
	unsigned int src_ip = 0;
	//printk("++++++zgf%s %d \n", __FUNCTION__, __LINE__);
	if (!p->br0_dev || !p->br0_dev->ip_ptr)
	{
		return;
	}
	pdev_ipaddr = (struct in_device *)p->br0_dev->ip_ptr;
	if(!pdev_ipaddr || !pdev_ipaddr->ifa_list)
	{
		return;
	}
	src_ip = pdev_ipaddr->ifa_list->ifa_local;
	
	if (!src_ip)
	{
		return;
	}

	switch (p->link_state)
	{
		case LINK_STATE_WIFI_2G_5G:
			tmp_dev = (p->eth_goto_2g_flag) ? p->client_2g_dev : p->client_5g_dev;
		break;
		
		case LINK_STATE_ONLY_2G:
			tmp_dev = p->client_2g_dev;
		break;
		
		case LINK_STATE_ONLY_5G:
			tmp_dev = p->client_5g_dev;
		break;	
	}

	/*
	**	Sometimes tmp_dev(client_2g_dev or client_5g_dev) would be free, and this may cause kernel panic.
	**	So it is necessary to check existence of "tmp_dev".
	*/
	
	if(!tmp_dev)
		return;
	
	spin_lock_bh(&br->hash_lock);
	dst_ip = tp_get_br_front_port_ip(br, src_ip);
	spin_unlock_bh(&br->hash_lock);	

	if (dst_ip)
	{ 
		arp_send(ARPOP_REQUEST, ETH_P_ARP, dst_ip, tmp_dev, src_ip, NULL, p->br0_dev->dev_addr, NULL);
	}

}


static void br_do_send_arp_request_packet_proxy(struct net_bridge *br)
{
	struct tp_db_fre_bridge_fdb_entry *p = &(br->free_bridage_fdb);
	struct net_device* tmp_dev = NULL;
	unsigned int dst_ip = 0;
	int i;


	br_do_br0_send_arp_request_packet_proxy(br);
	
	spin_lock_bh(&br->hash_lock);
	
	for (i = 0; i < BR_HASH_SIZE; i++) {
		struct net_bridge_fdb_entry *f;
		struct hlist_node *n;

		hlist_for_each_entry_safe(f, n, &br->hash[i], hlist) {
			if (f->dst->dev->br_port_index == p->client_dev_2g_idx
				|| f->dst->dev->br_port_index == p->client_dev_5g_idx
				|| !f->ip)
			{
				continue;
			}
			/*initial tmp_dev each loop*/
			tmp_dev=NULL;
			switch (p->link_state)
			{
				case LINK_STATE_WIFI_2G_5G:
					if (f->dst->dev->br_port_index == p->eth_dev_idx
						|| f->dst->dev->br_port_index == p->br0_dev_idx)
					{
						tmp_dev = (p->eth_goto_2g_flag) ? p->client_2g_dev : p->client_5g_dev;
					}else if (f->dst->dev->br_port_index == p->ap_dev_2g_idx)
					{
						tmp_dev = p->client_2g_dev;
					}else if (f->dst->dev->br_port_index == p->ap_dev_5g_idx)
					{
						tmp_dev = p->client_5g_dev;
					}
					break;
				
				case LINK_STATE_ONLY_2G:
					tmp_dev = p->client_2g_dev;
					break;
				
				case LINK_STATE_ONLY_5G:
					tmp_dev = p->client_5g_dev;
					break;	
				case LINK_STATE_NONE:
					tmp_dev=NULL;
					break;
			}

			/*
			**	Sometimes tmp_dev(client_2g_dev or client_5g_dev) would be free, and this may cause kernel panic.
			**	So it is necerrary to check existence of "tmp_dev".
			*/
			if(!tmp_dev)
				continue;
			
			dst_ip = 0;
			dst_ip = tp_get_br_front_port_ip(br, f->ip);
	
			if (dst_ip&&((f->addr.addr[0]&2)==0))
			{
				arp_send(ARPOP_REQUEST,ETH_P_ARP, dst_ip, tmp_dev, f->ip, NULL, f->addr.addr, NULL);
			}
			
		}
	}
	
	spin_unlock_bh(&br->hash_lock);
    printk("%s: %d \n", __FUNCTION__, __LINE__);
}



int br_set_link_state(struct net_bridge *br, unsigned long val)
{
	int old_link_state;

	br->bridge_deliver_link_state = val;
	
	old_link_state = br->free_bridage_fdb.link_state;
	
	br->free_bridage_fdb.link_state = br->bridge_deliver_link_state;
	
	REPEATER_PRINT(("free_bridage_fdb.link_state = %d\n", br->free_bridage_fdb.link_state));

	if(br->bridge_deliver_wireless_repeater_3addrees
		&& br->free_bridage_fdb.is_enabled 
		&& old_link_state != br->free_bridage_fdb.link_state)
	{
		br_do_send_arp_request_packet_proxy(br);
	}	
	return 0;
}


int br_set_eth_to_2g_enable (struct net_bridge *br, unsigned long val)
{
	int old_eth_goto_2g_flag;

    br->bridge_deliver_eth_to_2g_enable = val;
	old_eth_goto_2g_flag = br->free_bridage_fdb.eth_goto_2g_flag;
	REPEATER_PRINT(("bridge_deliver_eth_to_2g_enable = %d\n", br->bridge_deliver_eth_to_2g_enable));

    br->free_bridage_fdb.eth_goto_2g_flag = br->bridge_deliver_eth_to_2g_enable;

	if(br->bridge_deliver_wireless_repeater_3addrees 
		&& br->free_bridage_fdb.is_enabled 
		&& old_eth_goto_2g_flag != br->free_bridage_fdb.eth_goto_2g_flag)
	{
		init_filter_table(&(br->free_bridage_fdb));
		br_do_send_arp_request_packet_proxy(br);
	}
	return 0;
}

int br_set_group(struct net_bridge *br)
{
	struct net_device* tmpDev = NULL;
	unsigned int dev_sub_index = 0;
    unsigned int group_type_index = 0;

    for (group_type_index = 1; group_type_index <= BRIDGE_GROUP_MAX_TYPE; group_type_index ++)
    {
        for (dev_sub_index = 0; dev_sub_index < BRIDGE_GROUP_DEV_NUM; dev_sub_index++)
            {
                if(br->bridge_group_matrix[group_type_index-1][dev_sub_index][0] != '\0')
                {
                    tmpDev = dev_get_by_name(&init_net, br->bridge_group_matrix[group_type_index-1][dev_sub_index]);
                    if (NULL != tmpDev)
                    {
                        tmpDev->br_port_group = group_type_index;
                        printk("%s %d dev name = %s is in group[%d].\n", __FUNCTION__, __LINE__, 
                        br->bridge_group_matrix[group_type_index-1][dev_sub_index], group_type_index);
                        dev_put(tmpDev);
                    }
                    else
                    {
                        printk("%s %d dev name = %s is not exist.\n", __FUNCTION__, __LINE__, 
                        br->bridge_group_matrix[group_type_index-1][dev_sub_index]);
                    }                   
                }
            }
    } 
}



void br_bridge_deliver_init(struct net_bridge *br)
{
    memset(&(br->free_bridage_fdb), 0, sizeof(struct tp_db_fre_bridge_fdb_entry));
    memset(br->bridge_name_matrix, 0, sizeof(br->bridge_name_matrix));
    memset(br->bridge_group_matrix, 0, sizeof(br->bridge_group_matrix)); 
    br->bridge_hyfi_extend_enable = 0;    
    br->bridge_deliver_enable = 0;
    br->bridge_deliver_link_state = 0;
    br->bridge_deliver_eth_to_2g_enable = 0;
    br->bridge_deliver_wireless_repeater_3addrees = 0;
    br->bridge_deliver_has_same_front_dut = 1;
    br->bridge_deliver_psr_alias_rule = 2;
    br->bridge_deliver_dbg = 0;    
}


int init_free_bridge_fdb(struct net_bridge *br)
{

	struct net_device* tmpDev = NULL;
	struct tp_db_fre_bridge_fdb_entry *p = &(br->free_bridage_fdb);
	struct net_bridge_port* tmp_dev_br_port = NULL;
	unsigned int br_sub_dev_index_count = 0;
    unsigned int dev_sub_index = 0;
    int noDev = 1;
	
	if(LINK_STATE_ON != br->bridge_deliver_enable)
	{
		goto INIT_FAIL;
	}
	
	memset(p, 0, sizeof(struct tp_db_fre_bridge_fdb_entry));	
	
	br_sub_dev_index_count = 0;
	/*back of dual repeater*/

    GET_DEV_INDEX_BY_NAME(tmpDev, p->ap_dev_2g_idx, BRIDGE_DELIVER_2G_DEV_NAME);
	GET_DEV_INDEX_BY_NAME(tmpDev, p->ap_dev_5g_idx, BRIDGE_DELIVER_5G_DEV_NAME);	
	GET_DEV_INDEX_BY_NAME(tmpDev, p->eth_dev_idx, BRIDGE_DELIVER_LAN_ETH_NAME);	
	GET_DEV_INDEX_BY_NAME(tmpDev, p->br0_dev_idx, BRIDGE_DELIVER_NAME);	
	/*front of dual repeater*/
	GET_DEV_INDEX_BY_NAME(tmpDev, p->client_dev_2g_idx, BRIDGE_DELIVER_2G_STA_NAME);
	GET_DEV_INDEX_BY_NAME(tmpDev, p->client_dev_5g_idx, BRIDGE_DELIVER_5G_STA_NAME);
	GET_DEV_INDEX_BY_NAME(tmpDev, p->client_dev_plc_idx, BRIDGE_DELIVER_PLC_STA_NAME);    
	
	GET_DEV_BY_NAME(p->client_2g_dev, BRIDGE_DELIVER_2G_STA_NAME[0]);
	GET_DEV_BY_NAME(p->client_5g_dev, BRIDGE_DELIVER_5G_STA_NAME[0]);
	GET_DEV_BY_NAME(p->client_plc_dev, BRIDGE_DELIVER_PLC_STA_NAME[0]);	    
	GET_DEV_BY_NAME(p->br0_dev, BRIDGE_DELIVER_NAME[0]);	
	/*GET_DEV_BY_NAME(p->eth_dev, BRIDGE_DELIVER_LAN_ETH_NAME);*/	
	
    printk("%s: %s->%d, %s->%d, %s->%d, %s->%d, %s->%d, %s->%d, %s->%d\n", __func__,
            BRIDGE_DELIVER_2G_DEV_NAME[0], p->ap_dev_2g_idx,
            BRIDGE_DELIVER_5G_DEV_NAME[0], p->ap_dev_5g_idx,
            BRIDGE_DELIVER_LAN_ETH_NAME[0], p->eth_dev_idx,
            BRIDGE_DELIVER_NAME[0], p->br0_dev_idx,
            BRIDGE_DELIVER_2G_STA_NAME[0], p->client_dev_2g_idx,
            BRIDGE_DELIVER_5G_STA_NAME[0], p->client_dev_5g_idx,
            BRIDGE_DELIVER_PLC_STA_NAME[0], p->client_dev_plc_idx);

	if (p->client_2g_dev) tmp_dev_br_port = br_port_get_rcu(p->client_2g_dev);
	if (p->client_5g_dev) tmp_dev_br_port = br_port_get_rcu(p->client_5g_dev);
	if (p->client_plc_dev) tmp_dev_br_port = br_port_get_rcu(p->client_plc_dev);
    
	if (tmp_dev_br_port == NULL)
	{
        printk("[%s]ERROR: failed to get BR dev\n", __func__);
		goto INIT_FAIL;
	}
	p->br0_bri_dev = tmp_dev_br_port->br;
	
	p->is_enabled = LINK_STATE_ON;
	p->link_state = br->bridge_deliver_link_state; 
	p->eth_goto_2g_flag = br->bridge_deliver_eth_to_2g_enable;	
	/* set its value as default value. */
	p->is_same_front_dut = br->bridge_deliver_has_same_front_dut;

	/*
	**	Put init_filter_table at last to fix the problem that traffic from eth will go through apclii0 link
	@@	By TengFei.
	*/
	init_filter_table(p);
	
	return LINK_STATE_ON;
	
INIT_FAIL:	
	p->is_enabled = LINK_STATE_OFF;
	br->bridge_deliver_enable = 0;
	return LINK_STATE_OFF;
}


int br_should_deliver_packet(struct net_bridge *br, struct sk_buff *skb, u16 vid)
{
	//const unsigned char *mac_src = NULL;
	struct tp_db_fre_bridge_fdb_entry *p = &(br->free_bridage_fdb);
	//int is_cli_2g_5g_mac = 0;
	int ret=1;
	bool is_ring_check=1;

    
	CHECK_AND_INIT_TABLE(br, skb, 1);

	/*
		only check the psta mapped address ,as the back-end client mac address will be translated to the mapped addrress.

		which mac[0]&2=1;
	*/

    if (br->bridge_deliver_psr_alias_rule <= 1)
       is_ring_check=(eth_hdr(skb)->h_source[0]&2);

	if (is_ring_check &&(skb->dev->br_port_index == p->client_dev_5g_idx
			|| skb->dev->br_port_index == p->client_dev_2g_idx
			|| skb->dev->br_port_index == p->client_dev_plc_idx))
	{
		ret=br_fdb_check_ring_packet(skb, p, vid);
	}

	REPEATER_PRINT(("++++zgf :%s!!,source mac is %x:%x:%x:%x:%x:%x,proto is 0x%x,dev index is %d,is_ring_check %d\n",
		ret ? "DELIVER":"DROP",
		eth_hdr(skb)->h_source[0],
		eth_hdr(skb)->h_source[1],
		eth_hdr(skb)->h_source[2],
		eth_hdr(skb)->h_source[3],
		eth_hdr(skb)->h_source[4],
		eth_hdr(skb)->h_source[5],
		eth_hdr(skb)->h_proto,
		skb->dev->br_port_index,
		is_ring_check));
	REPEATER_PRINT(("++++zgf :%s!!,dest mac is %x:%x:%x:%x:%x:%x,proto is  0x%x,dev index is %d\n",
		ret ? "DELIVER":"DROP",
		eth_hdr(skb)->h_dest[0],
		eth_hdr(skb)->h_dest[1],
		eth_hdr(skb)->h_dest[2],
		eth_hdr(skb)->h_dest[3],
		eth_hdr(skb)->h_dest[4],
		eth_hdr(skb)->h_dest[5],
		eth_hdr(skb)->h_proto,
		skb->dev->br_port_index));
	return ret;

}

static int _cmp_mac(unsigned char *mac1, unsigned char *mac2)
{
	return ((mac1[0]!=mac2[0])||(mac1[1]!=mac2[1])||(mac1[2]!=mac2[2])||
				mac1[3]!=mac2[3]||(mac1[4]!=mac2[4])||(mac1[5]=mac2[5]));

}

int br_db_fre_enable(struct net_bridge *br)
{
    struct tp_db_fre_bridge_fdb_entry *p = &(br->free_bridage_fdb);

    if (LINK_STATE_OFF == br->bridge_deliver_enable || ! p->is_enabled)
    {
        return 0;
    }
    return 1;
}

/*
0 - deliver
1 - drop
*/
int br_db_fre_should_deliver(struct net_bridge_port *prev, struct sk_buff *skb)
{	
	struct net_device *psrc_dev = skb->dev;
	struct net_device *pdst_dev = prev->dev;
	struct tp_db_fre_bridge_fdb_entry *p = NULL;
	struct net_bridge_port* tmp_dev_br_port = NULL;
    struct net_bridge* br = NULL;
    int result = 0;	

	if (psrc_dev == NULL || pdst_dev == NULL)
	{
		return 0;
	}

    tmp_dev_br_port= br_port_get_rcu(psrc_dev);
    if (NULL == tmp_dev_br_port)
    {
        tmp_dev_br_port= br_port_get_rcu(pdst_dev);
        if (NULL == tmp_dev_br_port)
            return 0;
    }

    
    br = tmp_dev_br_port->br;
    if (NULL == br)
    {
        return 0;
    }
    p = &(br->free_bridage_fdb);
	
	CHECK_AND_INIT_TABLE(br, skb, 0);

    if (0 == psrc_dev->br_port_index)
	{
        result = 1;
        goto out;
	}        

	REPEATER_PRINT(("proto: %x, source mac is %02x:%02x:%02x, from index %s[%d] to index %s[%d]\n",
            eth_hdr(skb)->h_proto,
    		eth_hdr(skb)->h_source[3],
    		eth_hdr(skb)->h_source[4],
    		eth_hdr(skb)->h_source[5],
            psrc_dev->name,
    		psrc_dev->br_port_index,
            pdst_dev->name,
    		pdst_dev->br_port_index));
	

	if (DUAL_REPEATER_HAS_SAME_FROMT_DUT != p->is_same_front_dut)
	{
		return 0;
	}
    
	switch (p->link_state)
	{
		case LINK_STATE_ALL_2G_5G_PLC:
			if (IS_DEV_FILTERED(p->filter_table_2g_5g_plc, psrc_dev, pdst_dev))
			{
				result = 1;
			}
			break;
            
		case LINK_STATE_5G_PLC:
			if (IS_DEV_FILTERED(p->filter_table_5g_plc, psrc_dev, pdst_dev))
			{
				result = 1;
			}
			break;
            	
		case LINK_STATE_2G_PLC:
			if (IS_DEV_FILTERED(p->filter_table_2g_plc, psrc_dev, pdst_dev))
			{
				result = 1;
			}
			break;
            
		case LINK_STATE_ONLY_PLC:
			if (IS_DEV_FILTERED(p->filter_table_only_plc, psrc_dev, pdst_dev))
			{
				result = 1;
			}
			break;
	
		case LINK_STATE_WIFI_2G_5G:
			if (IS_DEV_FILTERED(p->filter_table_2g_5g, psrc_dev, pdst_dev))
			{
				result = 1;
			}
			break;
		
		case LINK_STATE_ONLY_2G:
			if (IS_DEV_FILTERED(p->filter_table_only_2g, psrc_dev, pdst_dev))
			{
				result = 1;
			}
			break;
		
		case LINK_STATE_ONLY_5G:
			if (IS_DEV_FILTERED(p->filter_table_only_5g, psrc_dev, pdst_dev))
			{
				result = 1;
			}
			break;
		case LINK_STATE_NONE:
			if (IS_DEV_FILTERED(p->filter_table_none, psrc_dev, pdst_dev))
			{
				result=1;
			}
			break;
	}
	
#ifndef ETHER_TYPE_802_1X
#define	ETHER_TYPE_802_1X	0x888e		/* 802.1x */
#endif


	/*  if br0 send 802_1X packet to rootap. pass this packet .
	*    add by zgf.
	*/

	if(result==1&&psrc_dev->br_port_index==p->br0_dev_idx)
	{
		if(eth_hdr(skb)->h_proto==htons(ETHER_TYPE_802_1X))
		{
			result=0;
		}
	}

out:
	REPEATER_PRINT(("after check:source mac is %02x:%02x:%02x, result is %s, link_state is %d\n",
            eth_hdr(skb)->h_source[3],
            eth_hdr(skb)->h_source[4],
            eth_hdr(skb)->h_source[5],
            result ? "drop" : "deliver",
            p->link_state));


	return result;
}


int br_db_fre_fdb_lookup(struct net_bridge_port **to, struct sk_buff *skb)
{	
	struct net_device *psrc_dev = skb->dev;
	struct net_device *pdst_dev = (*to)->dev;
	struct tp_db_fre_bridge_fdb_entry *p = NULL;
	struct net_device* tmp_dev = NULL;
	struct net_bridge_port* tmp_dev_br_port = NULL;
    struct net_bridge* br = NULL;

	if (psrc_dev == NULL || pdst_dev == NULL)
	{
		return 0;
	}

    tmp_dev_br_port= br_port_get_rcu(psrc_dev);
    if (NULL == tmp_dev_br_port)
    {
        tmp_dev_br_port= br_port_get_rcu(pdst_dev);
        if (NULL == tmp_dev_br_port)
            return 0;
    }

    
    br = tmp_dev_br_port->br;
    if (NULL == br)
    {
        return 0;
    }
    p = &(br->free_bridage_fdb);

    CHECK_AND_INIT_TABLE(br, skb, 0);

	REPEATER_PRINT(("[%s]proto: %x, source mac is %x:%x:%x->%x:%x:%x, from %s to %s\n",
        __func__,
        eth_hdr(skb)->h_proto,
		eth_hdr(skb)->h_source[3],
		eth_hdr(skb)->h_source[4],
		eth_hdr(skb)->h_source[5],
        eth_hdr(skb)->h_dest[3],
		eth_hdr(skb)->h_dest[4],
		eth_hdr(skb)->h_dest[5],
		psrc_dev->name,
		pdst_dev->name));

    if (0 == pdst_dev->br_port_index || 0 == psrc_dev->br_port_index)
	{
		return 0;
	}

	if (DUAL_REPEATER_HAS_SAME_FROMT_DUT != p->is_same_front_dut)
	{
		return 0;
	}	

	tmp_dev = NULL;
	switch(p->link_state)
	{
		case LINK_STATE_ALL_2G_5G_PLC:
			if (IS_DEV_FILTERED(p->filter_table_2g_5g_plc, psrc_dev, pdst_dev)
				&&(pdst_dev->br_port_index == p->client_dev_2g_idx 
				|| pdst_dev->br_port_index == p->client_dev_5g_idx
				|| pdst_dev->br_port_index == p->client_dev_plc_idx))
			{
			    if (psrc_dev->br_port_index <= MAX_BACK_DEV_BR_INDEX_VALUE) 
                {
                    if (! IS_DEV_FILTERED(p->filter_table_2g_5g_plc, psrc_dev, p->client_5g_dev))
                        tmp_dev = p->client_5g_dev;
                    else if (! IS_DEV_FILTERED(p->filter_table_2g_5g_plc, psrc_dev, p->client_2g_dev))
                        tmp_dev = p->client_2g_dev; 
                    else 
                        tmp_dev = p->client_plc_dev;                    
			    }
			}
            break;
            
		case LINK_STATE_5G_PLC:
			if (IS_DEV_FILTERED(p->filter_table_5g_plc, psrc_dev, pdst_dev)
				&&(pdst_dev->br_port_index == p->client_dev_2g_idx 
				|| pdst_dev->br_port_index == p->client_dev_5g_idx
				|| pdst_dev->br_port_index == p->client_dev_plc_idx))
			{
			    if (psrc_dev->br_port_index <= MAX_BACK_DEV_BR_INDEX_VALUE) 
                {
				    tmp_dev = (IS_DEV_FILTERED(p->filter_table_5g_plc, psrc_dev, p->client_5g_dev)) ? p->client_plc_dev : p->client_5g_dev;
			    }
			}
            break;
            
		case LINK_STATE_2G_PLC:	
			if (IS_DEV_FILTERED(p->filter_table_2g_plc, psrc_dev, pdst_dev)
				&&(pdst_dev->br_port_index == p->client_dev_2g_idx 
				|| pdst_dev->br_port_index == p->client_dev_5g_idx
				|| pdst_dev->br_port_index == p->client_dev_plc_idx))
			{
			    if (psrc_dev->br_port_index <= MAX_BACK_DEV_BR_INDEX_VALUE) 
                {
				    tmp_dev = (IS_DEV_FILTERED(p->filter_table_2g_plc, psrc_dev, p->client_2g_dev)) ? p->client_plc_dev : p->client_2g_dev;
			    }
			}
            break;
            
		case LINK_STATE_ONLY_PLC:
			 if (pdst_dev->br_port_index == p->client_dev_5g_idx || pdst_dev->br_port_index == p->client_dev_2g_idx)
			 {
			 	if (psrc_dev->br_port_index <= MAX_BACK_DEV_BR_INDEX_VALUE)
			 		tmp_dev = p->client_plc_dev;
			 }
			break;
            
		case LINK_STATE_WIFI_2G_5G:						
			if (IS_DEV_FILTERED(p->filter_table_2g_5g, psrc_dev, pdst_dev)
				&&(pdst_dev->br_port_index == p->client_dev_2g_idx 
				|| pdst_dev->br_port_index == p->client_dev_5g_idx
				|| pdst_dev->br_port_index == p->client_dev_plc_idx))
			{
			    if (psrc_dev->br_port_index <= MAX_BACK_DEV_BR_INDEX_VALUE) 
                {
				    tmp_dev = (IS_DEV_FILTERED(p->filter_table_2g_5g, psrc_dev, p->client_2g_dev)) ? p->client_5g_dev : p->client_2g_dev;
			    }
			}

#if 0
			if(psrc_dev->br_port_index==p->br0_dev_idx)
			{
				tmp_dev_br_port= br_port_get_rcu(p->eth_dev);
				if(tmp_dev_br_port)
				{
					br_fdb_update_mapped_hwaddr(tmp_dev_br_port->br, tmp_dev_br_port, skb, 0, false);
				}
			}
#endif            
		break;

		case LINK_STATE_ONLY_2G:
			 if (pdst_dev->br_port_index == p->client_dev_5g_idx || pdst_dev->br_port_index == p->client_dev_plc_idx)
			 {
			 	/*if(psrc_dev->br_port_index!=p->br0_dev_idx)*/
			 		tmp_dev = p->client_2g_dev;
			 }
			break;

		case LINK_STATE_ONLY_5G:
			if (pdst_dev->br_port_index == p->client_dev_2g_idx  || pdst_dev->br_port_index == p->client_dev_plc_idx)
			 {
			 	/*if(psrc_dev->br_port_index!=p->br0_dev_idx)*/
			 		tmp_dev = p->client_5g_dev;
			 }
			break;
		case LINK_STATE_NONE:
			if(IS_DEV_FILTERED(p->filter_table_none, psrc_dev, pdst_dev)
				&&(pdst_dev->br_port_index == p->client_dev_2g_idx 
				|| pdst_dev->br_port_index == p->client_dev_5g_idx
				|| pdst_dev->br_port_index == p->client_dev_plc_idx))
			{
				tmp_dev=psrc_dev;
			}			
				
	}

	if (tmp_dev && (tmp_dev_br_port = br_port_get_rcu(tmp_dev)))
	{
		*to = tmp_dev_br_port;
	}


	REPEATER_PRINT(("%s :after check source mac is %x:%x:%x,from %s to %s,link state is %d\n",
		__func__,
        eth_hdr(skb)->h_source[3],
		eth_hdr(skb)->h_source[4],
		eth_hdr(skb)->h_source[5],
		psrc_dev->name,
		(*to)->dev->name,
		p->link_state));

	return 0;
}


/*
**	For MTK platform, we should update mapped mac by apcli0/apclii0 for ra0/rai0/eth2.1.
@@	TengFei, 14/04/10.
*/
unsigned br_fdb_update_mapped_hwaddr(struct net_bridge *br, struct net_bridge_port *source,
		   struct sk_buff *skb, u16 vid, bool added_by_user)
{
	struct tp_db_fre_bridge_fdb_entry *p = &(br->free_bridage_fdb);
	struct net_device *tmp_dev = skb->dev;
	unsigned char mapped_mac[ETH_ALEN];
	unsigned char oui[ETH_ALEN/2];
	unsigned int m,b;

	CHECK_AND_INIT_TABLE(br, skb, 0);
	if(p->link_state==LINK_STATE_WIFI_2G_5G)
	{
		if (tmp_dev)
		{	
			if (tmp_dev->br_port_index == p->ap_dev_2g_idx ||
				tmp_dev->br_port_index == p->ap_dev_5g_idx ||
				tmp_dev->br_port_index == p->eth_dev_idx||
				tmp_dev->br_port_index == p->br0_dev_idx)
			{
				memcpy(mapped_mac, eth_hdr(skb)->h_source, ETH_ALEN);
				
				if(br->bridge_deliver_psr_alias_rule == 1)
				{
					memcpy(oui,mapped_mac, ETH_ALEN/2);
					oui[0]=oui[0]&0xfd;

					if (memcmp(oui, p->client_5g_dev->dev_addr, ETH_ALEN/2)) {
						/* Use the oui from primary interface's hardware address */
						mapped_mac[0] = p->client_5g_dev->dev_addr[0];
						mapped_mac[1] = p->client_5g_dev->dev_addr[1];
						mapped_mac[2] = p->client_5g_dev->dev_addr[2];
		
						/* Set the locally administered bit */
						mapped_mac[0] |= 0x02;
					}
					else
					{
						/* Set the locally administered bit */
						mapped_mac[0] |= 0x02;
						/* Right rotate the octets[1:3] of the mac address. This will make
						 * sure we generate an unique fixed alias for each mac address. If two
	 					* client mac addresses have the same octets[1:3] then we will have
						 * a collision. If this happens then generate a random number for the
						 * mac address.
						 */
						m = mapped_mac[1] << 16 |mapped_mac[2] << 8 | mapped_mac[3];

						b = m & 1;
						m >>= 1;
						m |= (b << 23);

						mapped_mac[1] = m >> 16;
						mapped_mac[2] = (m >> 8) & 0xff;
						mapped_mac[3] = m & 0xff;
					}

				} else if (br->bridge_deliver_psr_alias_rule == 0) {
					mapped_mac[0]|=0x02;
			    } else {
                    return 0;
                } 
				/* MAC Repeater has been improved */			
				br_fdb_update(br, source, mapped_mac, vid, added_by_user);
			}
		}
	}
	return 0;
}


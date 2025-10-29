/*! Copyright(c) 1996-2009 Shenzhen TP-LINK Technologies Co. Ltd.
 * \file    nm_fwup.c
 * \brief   Implements for upgrade firmware to NVRAM.
 * \author  Meng Qing
 * \version 1.0
 * \date    21/05/2009
 */


/**************************************************************************************************/
/*                                      CONFIGURATIONS                                            */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      INCLUDE_FILES                                             */
/**************************************************************************************************/
#if 0
#include "lib_types.h"
#include "lib_string.h"
#include "lib_printf.h"
#include "lib_malloc.h"
#endif
#include <common.h>

#include "nm_lib.h"
#include "nm_fwup.h"
#include "nm_api.h"

#include "sysProductInfo.h"


/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
/* Porting memory managing utils. */
extern void *malloc(unsigned int size);
extern void free(void *src);
#define fflush(stdout) 

/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      EXTERN_PROTOTYPES                                         */
/**************************************************************************************************/
STATUS nm_initFwupPtnStruct(void);
STATUS nm_getDataFromFwupFile(NM_PTN_STRUCT *ptnStruct, char *fwupPtnIndex, char *fwupFileBase);
STATUS nm_getDataFromNvram(NM_PTN_STRUCT *ptnStruct, NM_PTN_STRUCT *runtimePtnStruct);
STATUS nm_updateDataToNvram(NM_PTN_STRUCT *ptnStruct);
STATUS nm_updateRuntimePtnTable(NM_PTN_STRUCT *ptnStruct, NM_PTN_STRUCT *runtimePtnStruct);
static int nm_checkSupportList(char *support_list, int len);
STATUS nm_checkUpdateContent(NM_PTN_STRUCT *ptnStruct, char *pAppBuf, int nFileBytes, int *errorCode);
STATUS nm_cleanupPtnContentCache(void);
int nm_buildUpgradeStruct(char *pAppBuf, int nFileBytes);
STATUS nm_upgradeFwupFile(char *pAppBuf, int nFileBytes);


/**************************************************************************************************/
/*                                      LOCAL_PROTOTYPES                                          */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/

NM_STR_MAP nm_fwupPtnIndexFileParaStrMap[] =
{
    {NM_FWUP_PTN_INDEX_PARA_ID_NAME,    "fwup-ptn"},
    {NM_FWUP_PTN_INDEX_PARA_ID_BASE,    "base"},
    {NM_FWUP_PTN_INDEX_PARA_ID_SIZE,    "size"},

    {-1,                                NULL}
};



NM_PTN_STRUCT *g_nmFwupPtnStruct;
NM_PTN_STRUCT g_nmFwupPtnStructEntity;
int g_nmCountFwupCurrWriteBytes;
int g_nmCountFwupAllWriteBytes;

STATUS g_nmUpgradeResult;


char *ptnContentCache[NM_PTN_NUM_MAX];

/**************************************************************************************************/
/*                                      LOCAL_FUNCTIONS                                           */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      PUBLIC_FUNCTIONS                                          */
/**************************************************************************************************/

/*******************************************************************
 * Name		: nm_initFwupPtnStruct
 * Abstract	: Initialize partition-struct.
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR
 */
STATUS nm_initFwupPtnStruct()
{
    memset(&g_nmFwupPtnStructEntity, 0, sizeof(g_nmFwupPtnStructEntity));
    g_nmFwupPtnStruct = &g_nmFwupPtnStructEntity;
    
    return OK;
}


void nm_getDataForPtnEntryFromFwupFile(NM_PTN_ENTRY *currPtnEntry, char *fwupFileBase)
{
    currPtnEntry->upgradeInfo.dataType = NM_FWUP_UPGRADE_DATA_FROM_FWUP_FILE;
    currPtnEntry->upgradeInfo.dataStart = (unsigned int)fwupFileBase + NM_FWUP_PTN_INDEX_SIZE;
    /* length of partition-table is "probe to os-image"(4 bytes) and ptn-index-file(string) */
    currPtnEntry->upgradeInfo.dataLen = sizeof(int) + strlen((char*)(currPtnEntry->upgradeInfo.dataStart + sizeof(int)));
}

/*******************************************************************
 * Name		: nm_getDataFromFwupFile
 * Abstract	: 
 * Input	: fwupFileBase: start addr of FwupPtnTable
 * Output	: 
 * Return	: OK/ERROR.
 */
STATUS nm_getDataFromFwupFile(NM_PTN_STRUCT *ptnStruct, char *fwupPtnIndex, char *fwupFileBase)
{   
    int index = 0;
    int paraId = -1;
    int argc;
    char *argv[NM_FWUP_PTN_INDEX_ARG_NUM_MAX];
    NM_PTN_ENTRY *currPtnEntry = NULL;

    argc = nm_lib_makeArgs(fwupPtnIndex, argv, NM_FWUP_PTN_INDEX_ARG_NUM_MAX);
    
    while (index < argc)
    {
        if ((paraId = nm_lib_strToKey(nm_fwupPtnIndexFileParaStrMap, argv[index])) < 0)
        {
            NM_ERROR("invalid partition-index-file para id.\r\n");
            goto error;
        }

        index++;

        switch (paraId)
        {
        case NM_FWUP_PTN_INDEX_PARA_ID_NAME:
            /* we only update upgrade-info to partitions exist in partition-table */
            currPtnEntry = nm_lib_ptnNameToEntry(ptnStruct, argv[index]);

            if (currPtnEntry == NULL)
            {
                NM_DEBUG("partition name not found.");
                continue;           
            }

            if (currPtnEntry->upgradeInfo.dataType == NM_FWUP_UPGRADE_DATA_TYPE_BLANK)
            {
                currPtnEntry->upgradeInfo.dataType = NM_FWUP_UPGRADE_DATA_FROM_FWUP_FILE;
            }
            index++;
            break;
            
        case NM_FWUP_PTN_INDEX_PARA_ID_BASE:
            /* get data-offset in fwupFile */
            if (nm_lib_parseU32((NM_UINT32 *)&currPtnEntry->upgradeInfo.dataStart, argv[index]) < 0)
            {
                NM_ERROR("parse upgradeInfo start value failed.");
                goto error;
            }
            
            currPtnEntry->upgradeInfo.dataStart += (unsigned int)fwupFileBase;
            index++;
            break;

        case NM_FWUP_PTN_INDEX_PARA_ID_SIZE:
            if (nm_lib_parseU32((NM_UINT32 *)&currPtnEntry->upgradeInfo.dataLen, argv[index]) < 0)
            {
                NM_ERROR("parse upgradeInfo len value failed.");
                goto error;
            }
            index++;
            break;

        default:
            NM_ERROR("invalid para id.");
            goto error;
            break;
        }
        
    }

#ifdef NM_DUAL_IMAGE
    currPtnEntry = nm_lib_ptnNameToEntry(ptnStruct, NM_PTN_NAME_PTN_TABLE "@0");
    if (currPtnEntry == NULL)
    {
        NM_ERROR("no partition-table@0 in fwup-file.\r\n");
        goto error;
    }
    if (currPtnEntry->upgradeInfo.dataType == NM_FWUP_UPGRADE_DATA_FROM_FWUP_FILE)
    {
        nm_getDataForPtnEntryFromFwupFile(currPtnEntry, fwupFileBase);
    }

    currPtnEntry = nm_lib_ptnNameToEntry(ptnStruct, NM_PTN_NAME_PTN_TABLE "@1");
    if (currPtnEntry == NULL)
    {
        NM_ERROR("no partition-table@1 in fwup-file.\r\n");
        goto error;
    }
    if (currPtnEntry->upgradeInfo.dataType == NM_FWUP_UPGRADE_DATA_FROM_FWUP_FILE)
    {
        nm_getDataForPtnEntryFromFwupFile(currPtnEntry, fwupFileBase);
    }
#else
    /* force get partition-table from fwup-file */
    currPtnEntry = nm_lib_ptnNameToEntry(ptnStruct, NM_PTN_NAME_PTN_TABLE); 
    if (currPtnEntry == NULL)
    {
        NM_ERROR("no partition-table in fwup-file.\r\n");
        goto error; 
    }

    nm_getDataForPtnEntryFromFwupFile(currPtnEntry, fwupFileBase);
#endif
    
    return OK;
error:
    return ERROR;
}



/*******************************************************************
 * Name		: nm_getDataFromNvram
 * Abstract	: 
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR.
 */
STATUS nm_getDataFromNvram(NM_PTN_STRUCT *ptnStruct, NM_PTN_STRUCT *runtimePtnStruct)
{   
    int index = 0;
    NM_PTN_ENTRY *currPtnEntry = NULL;
    NM_UINT32 readSize = 0;
    

    nm_cleanupPtnContentCache();   

    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {       
        currPtnEntry = nm_lib_ptnNameToEntry(ptnStruct, runtimePtnStruct->entries[index].name);

        if (currPtnEntry == NULL)
        {
            continue;           
        }

        if (currPtnEntry->upgradeInfo.dataType == NM_FWUP_UPGRADE_DATA_TYPE_BLANK)
        {
			/* if base not changed, do nothing */
			if (currPtnEntry->base == runtimePtnStruct->entries[index].base)
			{
				currPtnEntry->upgradeInfo.dataType = NM_FWUP_UPGRADE_DATA_TYPE_NO_CHANGE;
				continue;
			}
            /* read content from NVRAM to a memory cache */
            readSize = 0;
            if(currPtnEntry->size <= runtimePtnStruct->entries[index].size)
            {
                readSize = currPtnEntry->size;
            }
            else
            {
                readSize = runtimePtnStruct->entries[index].size;
            }
            //ptnContentCache[index] = malloc(runtimePtnStruct->entries[index].size);
            ptnContentCache[index] = malloc(readSize);

            if (ptnContentCache[index] == NULL)
            {
                NM_ERROR("memory malloc failed.");
                goto error;
            }
            
            //memset(ptnContentCache[index], 0, runtimePtnStruct->entries[index].size);
            memset(ptnContentCache[index], 0, readSize);

            if (nm_lib_readHeadlessPtnFromNvram((char *)runtimePtnStruct->entries[index].base, 
                                                ptnContentCache[index], readSize) < 0)
            {               
                NM_ERROR("get data from NVRAM failed.");
                goto error;
            }

            currPtnEntry->upgradeInfo.dataStart = (unsigned int)ptnContentCache[index];
            currPtnEntry->upgradeInfo.dataLen = readSize;
            currPtnEntry->upgradeInfo.dataType = NM_FWUP_UPGRADE_DATA_FROM_NVRAM;
        }
    }

    return OK;
error:
    return ERROR;
}
    


/*******************************************************************
 * Name		: nm_updateDataToNvram
 * Abstract	: write to NARAM
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR.
 */
STATUS nm_updateDataToNvram(NM_PTN_STRUCT *ptnStruct)
{   
    int index = 0;
	int numBlookUpdate = 0;
    NM_PTN_ENTRY *currPtnEntry = NULL;
    int  firstFragmentSize = 0;
    int firstFragment = TRUE;
    unsigned long int fragmentBase = 0;
    int  fragmentDataStart = 0;
    int  fwupDataLen = 0;

    /* clear write bytes counter first */
    g_nmCountFwupAllWriteBytes = 0;

    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {
        currPtnEntry = (NM_PTN_ENTRY *)&(ptnStruct->entries[index]);

        switch (currPtnEntry->upgradeInfo.dataType)
        {
        case NM_FWUP_UPGRADE_DATA_TYPE_BLANK:
            /* if a partition is "blank", means it's a new partition
             * without content, we set content of this partition to all zero */
            if (ptnContentCache[index] != NULL)
            {
                free(ptnContentCache[index]);
                ptnContentCache[index] = NULL;
            }

            ptnContentCache[index] = malloc(currPtnEntry->size);            

            if (ptnContentCache[index] == NULL)
            {
                NM_ERROR("memory malloc failed.");
                goto error;
            }
            
            memset(ptnContentCache[index], 0, currPtnEntry->size);

            currPtnEntry->upgradeInfo.dataStart = (unsigned int)ptnContentCache[index];
            currPtnEntry->upgradeInfo.dataLen = currPtnEntry->size;
            break;
		case NM_FWUP_UPGRADE_DATA_TYPE_NO_CHANGE:
			NM_DEBUG("PTN %s no need to update.", currPtnEntry->name);
			break;

        case NM_FWUP_UPGRADE_DATA_FROM_FWUP_FILE:
        case NM_FWUP_UPGRADE_DATA_FROM_NVRAM:
            /* Do Nothing */
            break;

        default:
            NM_ERROR("invalid upgradeInfo dataType found.");
            goto error;
            break;  
        }
        
    }


    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {
        currPtnEntry = (NM_PTN_ENTRY *)&(ptnStruct->entries[index]);

        if (currPtnEntry->usedFlag != TRUE)
        {
            continue;
        }
        
        g_nmCountFwupAllWriteBytes += currPtnEntry->upgradeInfo.dataLen;
        
        NM_DEBUG("PTN %02d: dataLen = %08x, g_nmCountFwupAllWriteBytes = %08x", 
                        index+1, currPtnEntry->upgradeInfo.dataLen, g_nmCountFwupAllWriteBytes);
    }

    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {
        currPtnEntry = (NM_PTN_ENTRY *)&(ptnStruct->entries[index]);

        switch (currPtnEntry->upgradeInfo.dataType)
        {
		case NM_FWUP_UPGRADE_DATA_TYPE_NO_CHANGE:
			NM_DEBUG("PTN %s no need to update.\r\n", currPtnEntry->name);
			break;
        case NM_FWUP_UPGRADE_DATA_TYPE_BLANK:       
        case NM_FWUP_UPGRADE_DATA_FROM_FWUP_FILE:   
        case NM_FWUP_UPGRADE_DATA_FROM_NVRAM:
            if (currPtnEntry->usedFlag != TRUE)
            {
                NM_DEBUG("PTN %02d: usedFlag = FALSE", index+1);
                continue;
            }

            NM_DEBUG("PTN %02d: name = %-16s, base = 0x%08x, size = 0x%08x Bytes, upDataType = %d, upDataStart = %08x, upDataLen = %08x",
                index+1, 
                currPtnEntry->name, 
                currPtnEntry->base,
                currPtnEntry->size,
                currPtnEntry->upgradeInfo.dataType,
                currPtnEntry->upgradeInfo.dataStart,
                currPtnEntry->upgradeInfo.dataLen);


            if (currPtnEntry->upgradeInfo.dataLen > NM_FWUP_FRAGMENT_SIZE)
            {
                fwupDataLen = currPtnEntry->upgradeInfo.dataLen;
                firstFragment = TRUE;
                
                firstFragmentSize = NM_FWUP_FRAGMENT_SIZE - (currPtnEntry->base % NM_FWUP_FRAGMENT_SIZE);
                fragmentBase = 0;
                fragmentDataStart = 0;
                
                while (fwupDataLen > 0)
                {
                    if (firstFragment)
                    {
                        fragmentBase = currPtnEntry->base;
                        fragmentDataStart = currPtnEntry->upgradeInfo.dataStart;
                        
                        NM_DEBUG("PTN f %02d: fragmentBase = %08x, FragmentStart = %08x, FragmentLen = %08x, datalen = %08x", 
                            index+1, fragmentBase, fragmentDataStart, firstFragmentSize, fwupDataLen);

                        if (nm_lib_writeHeadlessPtnToNvram((char *)fragmentBase, 
                                                            (char *)fragmentDataStart,
                                                            firstFragmentSize) < 0)
                        {
                            NM_ERROR("WRITE TO NVRAM FAILED!!!!!!!!.");
                            goto error;
                        }

                        fragmentBase += firstFragmentSize;
                        fragmentDataStart += firstFragmentSize;
                        g_nmCountFwupCurrWriteBytes += firstFragmentSize;
                        fwupDataLen -= firstFragmentSize;
                        NM_DEBUG("PTN f %02d: write bytes = %08x", index+1, g_nmCountFwupCurrWriteBytes);
                        firstFragment = FALSE;
                    }
                    /* last block */
                    else if (fwupDataLen < NM_FWUP_FRAGMENT_SIZE)
                    {
                        NM_DEBUG("PTN l %02d: fragmentBase = %08x, FragmentStart = %08x, FragmentLen = %08x, datalen = %08x", 
                            index+1, fragmentBase, fragmentDataStart, fwupDataLen, fwupDataLen);

                        if (nm_lib_writeHeadlessPtnToNvram((char *)fragmentBase, 
                                                            (char *)fragmentDataStart,
                                                            fwupDataLen) < 0)
                        {
                            NM_ERROR("WRITE TO NVRAM FAILED!!!!!!!!.");
                            goto error;
                        }
						
                        fragmentBase += fwupDataLen;
                        fragmentDataStart += fwupDataLen;
                        g_nmCountFwupCurrWriteBytes += fwupDataLen;
                        fwupDataLen -= fwupDataLen;
                        NM_DEBUG("PTN l %02d: write bytes = %08x", index+1, g_nmCountFwupCurrWriteBytes);
                    }
                    else
                    {
                        NM_DEBUG("PTN n %02d: fragmentBase = %08x, FragmentStart = %08x, FragmentLen = %08x, datalen = %08x", 
                            index+1, fragmentBase, fragmentDataStart, NM_FWUP_FRAGMENT_SIZE, fwupDataLen);
                        
                        if (nm_lib_writeHeadlessPtnToNvram((char *)fragmentBase, 
                                                            (char *)fragmentDataStart,
                                                            NM_FWUP_FRAGMENT_SIZE) < 0)
                        {
                            NM_ERROR("WRITE TO NVRAM FAILED!!!!!!!!.");
                            goto error;
                        }
                   
                        fragmentBase += NM_FWUP_FRAGMENT_SIZE;
                        fragmentDataStart += NM_FWUP_FRAGMENT_SIZE;
                        g_nmCountFwupCurrWriteBytes += NM_FWUP_FRAGMENT_SIZE;
                        fwupDataLen -= NM_FWUP_FRAGMENT_SIZE;
                        NM_DEBUG("PTN n %02d: write bytes = %08x", index+1, g_nmCountFwupCurrWriteBytes);
                    }

					if(numBlookUpdate >= 70)
					{
						numBlookUpdate = 0;
						printf("\r\n");
					}
					numBlookUpdate ++;
                    printf("#");
					fflush(stdout);
                }
            }
            else
            {           
                /* we should add head to ptn-table partition */
                if (
#ifdef NM_DUAL_IMAGE
                    strncmp(currPtnEntry->name, NM_PTN_NAME_PTN_TABLE "@0", NM_PTN_NAME_LEN) == 0
                    || strncmp(currPtnEntry->name, NM_PTN_NAME_PTN_TABLE "@1", NM_PTN_NAME_LEN) == 0
#else
                    strncmp(currPtnEntry->name, NM_PTN_NAME_PTN_TABLE, NM_PTN_NAME_LEN) == 0
#endif
                    )
                {
                    if (nm_lib_writePtnToNvram((char *)currPtnEntry->base, 
                                                    (char *)currPtnEntry->upgradeInfo.dataStart,
                                                    currPtnEntry->upgradeInfo.dataLen) < 0)

                    {
                        NM_ERROR("WRITE TO NVRAM FAILED!!!!!!!!.");
                        goto error;
                    }
                }
                /* head of other partitions can be found in fwup-file or NVRAM */
                else
                {
                	if (nm_lib_writeHeadlessPtnToNvram((char *)currPtnEntry->base, 
                                                    (char *)currPtnEntry->upgradeInfo.dataStart,
                                                    currPtnEntry->upgradeInfo.dataLen) < 0)                                 
                	{
                   	 	NM_ERROR("WRITE TO NVRAM FAILED!!!!!!!!.");
                    	goto error;
                	}
				}
             
                g_nmCountFwupCurrWriteBytes += currPtnEntry->upgradeInfo.dataLen;
                NM_DEBUG("PTN %02d: write bytes = %08x", index+1, g_nmCountFwupCurrWriteBytes);

				if(numBlookUpdate >= 70)
				{
					numBlookUpdate = 0;
					printf("\r\n");
				}
				numBlookUpdate ++;
				printf("#");
				fflush(stdout);
            }
            break;

        default:
            NM_ERROR("invalid upgradeInfo dataType found.");
            goto error;
            break;  
        }       
    }
	printf("\r\nDone.\r\n");
    return OK;
error:
    return ERROR;
}


/*******************************************************************
 * Name		: nm_updateRuntimePtnTable
 * Abstract	: update the runtimePtnTable.
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR
 */
STATUS nm_updateRuntimePtnTable(NM_PTN_STRUCT *ptnStruct, NM_PTN_STRUCT *runtimePtnStruct)
{   
    int index = 0;
    NM_PTN_ENTRY *currPtnEntry = NULL;
    NM_PTN_ENTRY *currRuntimePtnEntry = NULL;

    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {
        currPtnEntry = (NM_PTN_ENTRY *)&(ptnStruct->entries[index]);
        currRuntimePtnEntry = (NM_PTN_ENTRY *)&(runtimePtnStruct->entries[index]);

        memcpy(currRuntimePtnEntry->name, currPtnEntry->name, NM_PTN_NAME_LEN);
        currRuntimePtnEntry->base = currPtnEntry->base;
        currRuntimePtnEntry->tail = currPtnEntry->tail;
        currRuntimePtnEntry->size = currPtnEntry->size;
        currRuntimePtnEntry->usedFlag = currPtnEntry->usedFlag;
    }   

    return OK;
}



int nm_checkSupportList(char *support_list, int len)
{
    int ret = 0;
    
    PRODUCT_INFO_STRUCT *pProductInfo = NULL;

    /* skip partition header */
    len -= 8;
    support_list += 8;
 
    /* check list prefix string */
    if (len < 12 || strncmp(support_list, "SupportList:", 12) != 0)
        return 0;

    len -= 12;
    support_list += 12;

    pProductInfo = sysmgr_getProductInfo();
    ret = sysmgr_cfg_checkSupportList(pProductInfo, support_list, len);
    if (0 == ret)
    {
        NM_INFO("Firmwave supports, check OK.\r\n");
        return 1;
    }
    
    NM_INFO("Firmwave not supports, check failed.\r\n");
    return 0;
}

STATUS nm_checkPartition(NM_PTN_STRUCT *ptnStruct, const char *name, int *errorCode)
{
    int ptnFound = FALSE;
    NM_PTN_ENTRY *currPtnEntry = NULL;
    int index;

    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {
        currPtnEntry = (NM_PTN_ENTRY *)&(ptnStruct->entries[index]);

        if (strncmp(currPtnEntry->name, name, NM_PTN_NAME_LEN) == 0)
        {
            ptnFound = TRUE;
            break;
        }
    }
    if (ptnFound == FALSE)
    {
        NM_ERROR("ptn \"%s\" not found whether in fwup-file or NVRAM.", name);
        *errorCode = -NM_FWUP_ERROR_BAD_FILE;
        return ERROR;
    }

    return OK;
}


/*******************************************************************
 * Name		: nm_checkUpdateContent
 * Abstract	: check the updata content.
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR
 */
STATUS nm_checkUpdateContent(NM_PTN_STRUCT *ptnStruct, char *pAppBuf, int nFileBytes, int *errorCode)
{   
    int index = 0;
    NM_PTN_ENTRY *currPtnEntry = NULL;
    int suppportListIndex = 0;
	int softVersionIndex = 0;

    /* check update content */
    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {
        currPtnEntry = (NM_PTN_ENTRY *)&(ptnStruct->entries[index]);

        if (currPtnEntry->upgradeInfo.dataType == NM_FWUP_UPGRADE_DATA_FROM_FWUP_FILE)
        {       
            if ((currPtnEntry->upgradeInfo.dataStart + currPtnEntry->upgradeInfo.dataLen)
                > (unsigned int)(pAppBuf + nFileBytes))
            {
                NM_ERROR("ptn \"%s\": update data end out of fwup-file.", currPtnEntry->name);
                *errorCode = -NM_FWUP_ERROR_BAD_FILE;
                goto error;
            }
        }
    }

    /* check important partitions */
#ifdef NM_DUAL_IMAGE
    if (nm_checkPartition(ptnStruct, NM_PTN_NAME_FS_UBOOT "@0", errorCode) != OK)
    {
        goto error;
    }
    if (nm_checkPartition(ptnStruct, NM_PTN_NAME_FS_UBOOT "@1", errorCode) != OK)
    {
        goto error;
    }
#else
    if (nm_checkPartition(ptnStruct, NM_PTN_NAME_FS_UBOOT, errorCode) != OK)
    {
        goto error;
    }
#endif

#ifdef NM_DUAL_IMAGE
    if (nm_checkPartition(ptnStruct, NM_PTN_NAME_PTN_TABLE "@0", errorCode) != OK)
    {
        goto error;
    }
    if (nm_checkPartition(ptnStruct, NM_PTN_NAME_PTN_TABLE "@1", errorCode) != OK)
    {
        goto error;
    }
#else
    if (nm_checkPartition(ptnStruct, NM_PTN_NAME_PTN_TABLE, errorCode) != OK)
    {
        goto error;
    }
#endif

    if (nm_checkPartition(ptnStruct, NM_PTN_NAME_PRODUCT_INFO, errorCode) != OK)
    {
        goto error;
    }

    if (nm_checkPartition(ptnStruct, NM_PTN_NAME_SUPPORT_LIST, errorCode) != OK)
    {
        goto error;
    }

#ifdef NM_DUAL_IMAGE
    if (nm_checkPartition(ptnStruct, NM_PTN_NAME_SOFT_VERSION "@0", errorCode) != OK)
    {
        goto error;
    }
    if (nm_checkPartition(ptnStruct, NM_PTN_NAME_SOFT_VERSION "@1", errorCode) != OK)
    {
        goto error;
    }
#else
    if (nm_checkPartition(ptnStruct, NM_PTN_NAME_SOFT_VERSION, errorCode) != OK)
    {
        goto error;
    }
#endif

#ifdef NM_DUAL_IMAGE
    if (nm_checkPartition(ptnStruct, NM_PTN_NAME_OS_IMAGE "@0", errorCode) != OK)
    {
        goto error;
    }
    if (nm_checkPartition(ptnStruct, NM_PTN_NAME_OS_IMAGE "@1", errorCode) != OK)
    {
        goto error;
    }
#else
    if (nm_checkPartition(ptnStruct, NM_PTN_NAME_OS_IMAGE, errorCode) != OK)
    {
        goto error;
    }
#endif

#ifdef NM_DUAL_IMAGE
    if (nm_checkPartition(ptnStruct, NM_PTN_NAME_FILE_SYSTEM "@0", errorCode) != OK)
    {
        goto error;
    }
    if (nm_checkPartition(ptnStruct, NM_PTN_NAME_FILE_SYSTEM "@1", errorCode) != OK)
    {
        goto error;
    }
#else
    if (nm_checkPartition(ptnStruct, NM_PTN_NAME_FILE_SYSTEM, errorCode) != OK)
    {
        goto error;
    }
#endif

	//check the hardware version support list
    currPtnEntry = nm_lib_ptnNameToEntry(ptnStruct, NM_PTN_NAME_SUPPORT_LIST);
    if (NM_FWUP_UPGRADE_DATA_FROM_FWUP_FILE != currPtnEntry->upgradeInfo.dataType ||
        !nm_checkSupportList((char*)currPtnEntry->upgradeInfo.dataStart, currPtnEntry->upgradeInfo.dataLen))
	{
		NM_ERROR("the firmware is not for this model");
		*errorCode = -NM_FWUP_ERROR_INCORRECT_MODEL;
		goto error;
	}

    return OK;
error:
    return ERROR;
}



/*******************************************************************
 * Name		: nm_cleanupPtnContentCache
 * Abstract	: free the memmory of ptnContentCache.
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR.
 */

STATUS nm_cleanupPtnContentCache()
{   
    int index = 0;


    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {           
        if (ptnContentCache[index] != NULL)
        {
            free(ptnContentCache[index]);
            ptnContentCache[index] = NULL;
        }   
    }
    
    return OK;
}


/*******************************************************************
 * Name		: nm_buildUpgradeStruct
 * Abstract	: Generate an upgrade file from NVRAM and firmware file.
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR.
 */
int nm_buildUpgradeStruct(char *pAppBuf, int nFileBytes)
{
    char fwupPtnIndex[NM_FWUP_PTN_INDEX_SIZE+1] = {0};
    char *fwupFileBase = NULL;
    int index;
    int ret = 0;

    memset(g_nmFwupPtnStruct, 0, sizeof(NM_PTN_STRUCT));
    for (index=0; index<NM_PTN_NUM_MAX; index++)
    {
        g_nmFwupPtnStruct->entries[index].usedFlag = FALSE;
    }
    g_nmCountFwupAllWriteBytes = 0;
    g_nmCountFwupCurrWriteBytes = 0;
    nm_cleanupPtnContentCache();

    /* backup "fwup-partition-index" */
    fwupFileBase = pAppBuf;
    strncpy(fwupPtnIndex, pAppBuf, NM_FWUP_PTN_INDEX_SIZE+1); 
    pAppBuf += NM_FWUP_PTN_INDEX_SIZE;
    pAppBuf += sizeof(int);

    NM_DEBUG("nFileBytes = %d",  nFileBytes);
    if (nm_lib_parsePtnIndexFile(g_nmFwupPtnStruct, pAppBuf) != OK)
    {
        NM_ERROR("parse new ptn-index failed.");
        ret = -NM_FWUP_ERROR_BAD_FILE;
        goto cleanup;
    }

    if (nm_getDataFromFwupFile(g_nmFwupPtnStruct, (char *)&fwupPtnIndex, fwupFileBase) != OK)
    {
        NM_ERROR("getDataFromFwupFile failed.");
        ret = -NM_FWUP_ERROR_BAD_FILE;
        goto cleanup;
    }

    if (nm_getDataFromNvram(g_nmFwupPtnStruct, g_nmPtnStruct) != OK)
    {
        NM_ERROR("getDataFromNvram failed.");
        ret = -NM_FWUP_ERROR_BAD_FILE;
        goto cleanup;
    }

    if (nm_checkUpdateContent(g_nmFwupPtnStruct, fwupFileBase, nFileBytes, &ret) != OK)
    {
        NM_ERROR("checkUpdateContent failed.");
        goto cleanup;
    }

    return 0;
    
cleanup:
    memset(g_nmFwupPtnStruct, 0, sizeof(NM_PTN_STRUCT));
    g_nmCountFwupAllWriteBytes = 0;
    g_nmCountFwupCurrWriteBytes = 0;
    nm_cleanupPtnContentCache();
    g_nmUpgradeResult = FALSE;
    return ret;
}


/*******************************************************************
 * Name		: nm_upgradeFwupFile
 * Abstract	: upgrade the FwupFile to NVRAM
 * Input	: 
 * Output	: 
 * Return	: OK/ERROR.
 */
STATUS nm_upgradeFwupFile(char *pAppBuf, int nFileBytes)
{   
    g_nmUpgradeResult = FALSE;

    if (nm_updateDataToNvram(g_nmFwupPtnStruct) != OK)
    {
        NM_ERROR("updateDataToNvram failed.");
        goto cleanup;
    }

    /* update run-time partition-table, active new partition-table without restart */
    if (nm_updateRuntimePtnTable(g_nmFwupPtnStruct, g_nmPtnStruct) != OK)
    {
        NM_ERROR("updateDataToNvram failed.");
        goto cleanup;
    }

    memset(g_nmFwupPtnStruct, 0, sizeof(NM_PTN_STRUCT));
    g_nmCountFwupAllWriteBytes = 0;
    g_nmCountFwupCurrWriteBytes = 0;
    nm_cleanupPtnContentCache();
    g_nmUpgradeResult = TRUE;
    return OK;
    
cleanup:
    memset(g_nmFwupPtnStruct, 0, sizeof(NM_PTN_STRUCT));
    g_nmCountFwupAllWriteBytes = 0;
    g_nmCountFwupCurrWriteBytes = 0;
    nm_cleanupPtnContentCache();
    g_nmUpgradeResult = FALSE;
    return ERROR;
}


#define IMAGE_SIZE_LEN  (0x04)
#define IMAGE_SIZE_MD5  (0x10)
#define IMAGE_SIZE_PRODUCT  (0x1000)
#define IMAGE_SIZE_BASE (IMAGE_SIZE_LEN + IMAGE_SIZE_MD5 + IMAGE_SIZE_PRODUCT)

#define IMAGE_SIZE_MAX  (IMAGE_SIZE_BASE + 0x800 + NM_FLASH_SIZE)
#define IMAGE_SIZE_MIN  (IMAGE_SIZE_BASE + 0x800)

#if 1

#define IMAGE_SIZE_FWTYPE	(IMAGE_SIZE_LEN + IMAGE_SIZE_MD5)
#define IMAGE_SIZE_RSA_SIG	(IMAGE_SIZE_FWTYPE + 0x11C)
#define IMAGE_LEN_RSA_SIG	0x80


static unsigned char l_rsaPubKey[] = "BgIAAACkAABSU0ExAAQAAAEAAQD9lxDCQ5DFNSYJBriTmTmZlEMYVgGcZTO+AIwm" \
				"dVjhaeJI6wWtN7DqCaHQlOqJ2xvKNrLB+wA1NxUh7VDViymotq/+9QDf7qEtJHmesji" \
				"rvPN6Hfrf+FO4/hmjbVXgytHORxGta5KW4QHVIwyMSVPOvMC4A5lFIh+D1kJW5GXWtA==";
static int handle_fw_cloud(unsigned char *buf, int buf_len);


static int handle_fw_cloud(unsigned char *buf, int buf_len)
{	
	unsigned char md5_dig[IMAGE_SIZE_MD5] = {0};
	unsigned char sig_buf[IMAGE_LEN_RSA_SIG] = {0};
	unsigned char tmp_rsa_sig[IMAGE_LEN_RSA_SIG] = {0};
	int ret = 0;

	/*backup data*/
	memcpy(tmp_rsa_sig,buf + IMAGE_SIZE_RSA_SIG, IMAGE_LEN_RSA_SIG);
	memcpy(sig_buf, buf + IMAGE_SIZE_RSA_SIG, IMAGE_LEN_RSA_SIG);

	/* fill with 0x0 */
	memset(buf + IMAGE_SIZE_RSA_SIG, 0x0, IMAGE_LEN_RSA_SIG);

	md5(buf + IMAGE_SIZE_FWTYPE, buf_len - IMAGE_SIZE_FWTYPE, md5_dig);

	ret = rsaVerifySignByBase64EncodePublicKeyBlob(l_rsaPubKey, strlen(l_rsaPubKey),
                md5_dig, IMAGE_SIZE_MD5, sig_buf, IMAGE_LEN_RSA_SIG);

	memcpy(buf + IMAGE_SIZE_RSA_SIG, tmp_rsa_sig, IMAGE_LEN_RSA_SIG);
	
	return ret;
}
#endif

int nm_upgradeFirmware(char *buf, int size)
{
    int filesize = 0;

    if (nm_flashOpPortInit() != 0)
    {
        NM_ERROR("Failed to initialize flash");
        return -1;
    }

    if (nm_init() != 0)
    {
        NM_ERROR("Failed to initialize partition table");
        return -1;
    }

    memcpy(&filesize, buf, sizeof(int));
    filesize = ntohl(filesize);

    if (filesize < IMAGE_SIZE_MIN || filesize > IMAGE_SIZE_MAX || filesize > size)
    {
        NM_ERROR("Bad file size: buffer: %d, file: %d", size, filesize);
        return -1;
    }
#if 1
	if (!handle_fw_cloud((unsigned char*)buf, size))
	{
		NM_ERROR("Verify sig error!\n");
		return -1;
	}
#endif

    buf += IMAGE_SIZE_BASE;
    size -= IMAGE_SIZE_BASE;
    if (nm_buildUpgradeStruct(buf, size) != 0)
    {
        NM_ERROR("Firmware is invalid");
        return -1;
    }

    printf("Firmware checking passed\n");

    if (nm_upgradeFwupFile(buf, size) != 0)
    {
        NM_ERROR("Failed to upgrade");
        return -1;
    }

    nm_flashOpPortFree();

    return 0;
}

int nm_upgradeFirmware_pre_check(char *buf, int size)
{
    int filesize = 0;

    if (nm_flashOpPortInit() != 0)
    {
        NM_ERROR("Failed to initialize flash");
        return -1;
    }

    if (nm_init() != 0)
    {
        NM_ERROR("Failed to initialize partition table");
        return -1;
    }

    memcpy(&filesize, buf, sizeof(int));
    filesize = ntohl(filesize);

    if (filesize < IMAGE_SIZE_MIN || filesize > IMAGE_SIZE_MAX || filesize > size)
    {
        NM_ERROR("Bad file size: buffer: %d, file: %d", size, filesize);
        return -1;
    }
#if 1
	if (!handle_fw_cloud((unsigned char*)buf, size))
	{
		NM_ERROR("Verify sig error!\n");
		return -1;
	}
#endif

    buf += IMAGE_SIZE_BASE;
    size -= IMAGE_SIZE_BASE;
    if (nm_buildUpgradeStruct(buf, size) != 0)
    {
        NM_ERROR("Firmware is invalid");
        return -1;
    }

    printf("Firmware checking passed\n");

    return 0;
}

/**************************************************************************************************/
/*                                      GLOBAL_FUNCTIONS                                          */
/**************************************************************************************************/


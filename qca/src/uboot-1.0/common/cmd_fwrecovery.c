#include <common.h>
#include <command.h>
#include <asm/io.h>

#include <asm/arch-ipq40xx/iomap.h>

extern int nm_upgradeFirmware(char *buf, int size);

static int do_fwrecovery(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
    char *s = NULL;
    ulong addr = 0;
    ulong filesize = 0;
    void *buf = NULL;
    int retval = 0;

    s = getenv("serverip");
    if (NULL == s)
    {
        setenv("serverip", CONFIG_FWRECOVERY_SERVERIP);
    }

    s = getenv("ipaddr");
    if (NULL == s)
    {
        setenv("ipaddr", CONFIG_FWRECOVERY_IPADDR);
    }

    retval = do_tftpb(cmdtp, flag, argc, argv);
    if (retval != 0)
    {
        return CMD_RET_FAILURE;
    }

    s = getenv("filesize");
    if (s)
    {
        filesize = simple_strtoul(s, NULL, 16);
    }
    else
    {
        return CMD_RET_FAILURE;
    }

    s = getenv("fileaddr");
    if (s)
    {
        addr = simple_strtoul(s, NULL, 16);
    }
    else
    {
        return CMD_RET_FAILURE;
    }

    buf = map_physmem(addr, filesize, MAP_WRBACK);
    if (NULL == buf)
    {
        puts("Failed to map physical memory\n");
        return CMD_RET_FAILURE;
    }

    retval = nm_upgradeFirmware((char *)buf, (int)filesize);
    if (0 == retval)
    {
        printf("Upgrade succeeded!!\n");
        retval = CMD_RET_SUCCESS;
    }
    else
    {
        printf("Upgraded failed!!\n");
        retval = CMD_RET_FAILURE;
    }

    unmap_physmem(buf, filesize);

    return retval;
}

U_BOOT_CMD(fwrecovery, 3, 0, do_fwrecovery,
           "Firmware recovery",
           "loadAddress firmwareName");

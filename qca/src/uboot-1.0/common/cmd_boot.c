/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Misc boot support
 */
#include <common.h>
#include <command.h>
#include <net.h>

#include <spi_flash.h>

#ifdef CONFIG_CMD_GO

/* Allow ports to override the default behavior */
__attribute__((weak))
unsigned long do_go_exec (ulong (*entry)(int, char * const []), int argc, char * const argv[])
{
	return entry (argc, argv);
}

int do_go (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong	addr, rc;
	int     rcode = 0;

	if (argc < 2)
		return CMD_RET_USAGE;

	addr = simple_strtoul(argv[1], NULL, 16);

	printf ("## Starting application at 0x%08lX ...\n", addr);

	/*
	 * pass address parameter as argv[0] (aka command name),
	 * and all remaining args
	 */
	rc = do_go_exec ((void *)addr, argc - 1, argv + 1);
	if (rc != 0) rcode = 1;

	printf ("## Application terminated, rc = 0x%lX\n", rc);
	return rcode;
}

/* -------------------------------------------------------------------- */

U_BOOT_CMD(
	go, CONFIG_SYS_MAXARGS, 1,	do_go,
	"start application at address 'addr'",
	"addr [arg ...]\n    - start application at address 'addr'\n"
	"      passing 'arg' as arguments"
);

#endif

U_BOOT_CMD(
	reset, 1, 0,	do_reset,
	"Perform RESET of the CPU",
	""
);

/* add by wanghao for debug  */
extern u16 qca8075_phy_reg_read(u32 dev_id, u32 phy_id, u32 reg_id);
extern u16 qca8075_phy_reg_write(u32 dev_id, u32 phy_id, u32 reg_id, u16 reg_val);
extern u8 __medium_is_fiber_100fx(u32 dev_id, u32 phy_id);
extern  u32 __phy_reg_pages_sel_by_active_medium(u32 dev_id, u32 phy_id);

#define QCA8075_PHY_CONTROL			0
#define QCA8075_CTRL_POWER_DOWN		0x0800
#define QCA8075_CTRL_AUTONEGOTIATION_ENABLE	0x1000
#define QCA8075_CTRL_RESTART_AUTONEGOTIATION	0x0200

int do_debugFy (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u16 phy_data = 0;
	ulong enable = 0;

	if (argc < 2)
		return CMD_RET_USAGE;

	enable = simple_strtoul(argv[1], NULL, 10);

	if (enable == 0)
	{
		phy_data = qca8075_phy_reg_read(0, 3, QCA8075_PHY_CONTROL);
		phy_data |= QCA8075_CTRL_POWER_DOWN;
		printf("disable phy 3 val is 0x%x\n", phy_data);
		qca8075_phy_reg_write(0, 3, QCA8075_PHY_CONTROL, phy_data);

		
		
		if (__medium_is_fiber_100fx(0, 4))
			return -1;
		__phy_reg_pages_sel_by_active_medium(0, 4);
		phy_data = qca8075_phy_reg_read(0, 4, QCA8075_PHY_CONTROL);
		phy_data |= QCA8075_CTRL_POWER_DOWN;
		printf("disable phy 4 val is 0x%x\n", phy_data);
		qca8075_phy_reg_write(0, 4, QCA8075_PHY_CONTROL, phy_data);
	}
	else if (enable == 1)
	{
		phy_data = qca8075_phy_reg_read(0, 3, QCA8075_PHY_CONTROL);
		phy_data &= ~QCA8075_CTRL_POWER_DOWN;
		phy_data |= QCA8075_CTRL_AUTONEGOTIATION_ENABLE;
		printf("enable phy 3 val is 0x%x\n", phy_data);
		qca8075_phy_reg_write(0, 3, QCA8075_PHY_CONTROL,
			phy_data | QCA8075_CTRL_RESTART_AUTONEGOTIATION);

		
		
		if (__medium_is_fiber_100fx(0, 4))
			return -1;
		__phy_reg_pages_sel_by_active_medium(0, 4);
		phy_data &= ~QCA8075_CTRL_POWER_DOWN;
		phy_data |= QCA8075_CTRL_AUTONEGOTIATION_ENABLE;
		printf("enable phy 4 val is 0x%x\n", phy_data);
		qca8075_phy_reg_write(0, 4, QCA8075_PHY_CONTROL,
			phy_data | QCA8075_CTRL_RESTART_AUTONEGOTIATION);
	}
	else if (enable == 2)
	{		
		unsigned char buff[2] = {0};
		struct spi_flash *flash = spi_flash_probe(0, 0, 1000000, 0x3);
		spi_flash_read(flash, 0x180000, 0x2, buff);
		printf("MAC is %02X %02X\n", buff[0], buff[1]);
		
		/* Lp5521 init */
		run_command("i2c mw 32 0xd 0xff", 0);
		run_command("i2c mw 32 0x0 0xc0", 0);
		run_command("i2c mw 32 0x1 0x3f", 0);
		run_command("i2c mw 32 0x8 0x79", 0);

		run_command("i2c mw 30 0x9 0x06", 0);
        run_command("i2c mw 30 0x4 0x55", 0);
			
		if (buff[0] == 0xff || buff[1] == 0xff)
		{
			/* test mode, turn on LED in sequence */
			//turn on RED
			run_command("i2c mw 32 0x2 0xff", 0);
            run_command("i2c mw 30 0x8 12", 0);
            run_command("i2c mw 30 0x7 0", 0);
            run_command("i2c mw 30 0x6 0", 0);
			udelay(1000 * 1000);
			//turn on GREEN
			run_command("i2c mw 32 0x2 0x0", 0);
			run_command("i2c mw 32 0x3 0xff", 0);
            run_command("i2c mw 30 0x8 1", 0);
            run_command("i2c mw 30 0x7 15", 0);
            run_command("i2c mw 30 0x6 2", 0);
			udelay(1000 * 1000);
			//turn on BLUE
			run_command("i2c mw 32 0x3 0x0", 0);
			run_command("i2c mw 32 0x4 0xff", 0);
            run_command("i2c mw 30 0x8 0", 0);
            run_command("i2c mw 30 0x7 6", 0);
            run_command("i2c mw 30 0x6 35", 0);
			udelay(1000 * 1000);
			//turn on WHITE
			run_command("i2c mw 32 0x2 0xff", 0);
			run_command("i2c mw 32 0x3 0xff", 0);
			run_command("i2c mw 32 0x4 0xff", 0);
            run_command("i2c mw 30 0x8 35", 0);
            run_command("i2c mw 30 0x7 35", 0);
            run_command("i2c mw 30 0x6 35", 0);
		}
		else
		{
			/* turn on LED  */
			run_command("i2c mw 32 0x2 0x3", 0);
			run_command("i2c mw 32 0x3 0x9b", 0);
			run_command("i2c mw 32 0x4 0xe5", 0);
            run_command("i2c mw 30 0x8 0", 0);
            run_command("i2c mw 30 0x7 6", 0);
            run_command("i2c mw 30 0x6 35", 0);
		}
		
		run_command("i2c mw 32 0x0 0xea", 0);
		run_command("i2c mw 32 0x5 0x2f", 0);
		run_command("i2c mw 32 0x6 0x2f", 0);
		run_command("i2c mw 32 0x7 0x2f", 0);
	}
	else if (enable == 3)
	{
		char *channel1Cmd = "E1004000420842084288428800000000";
		char *channel2Cmd = "E00A4000065F065F06DF06DF00000000";
		char *channel3Cmd = "E10040001A171A171A971A9700000000";
		char i2cCmd[32] = {0};
		unsigned char index = 0;
		ulong channel1Addr = 0x10;
		ulong channel2Addr = 0x30;
		ulong channel3Addr = 0x50;

		/* turn off all light  */
		run_command("i2c mw 32 0x1 0x0", 0);

		/* RED light command  */
		run_command("i2c mw 32 0x1 0x10", 0);
		for (index = 0; index < 32 && *channel1Cmd != '\0'; index++)
		{
			snprintf(i2cCmd, 32, "i2c mw 32 0x%x 0x%c%c", channel1Addr, *channel1Cmd, *(channel1Cmd + 1));
			channel1Addr++;
			channel1Cmd += 2;
			//printf("%s\n", i2cCmd);
			run_command(i2cCmd, 0);
		}

		/* GREEN light command  */
		run_command("i2c mw 32 0x1 0x14", 0);
		for (index = 0; index < 32 && *channel2Cmd != '\0'; index++)
		{
			snprintf(i2cCmd, 32, "i2c mw 32 0x%x 0x%c%c", channel2Addr, *channel2Cmd, *(channel2Cmd + 1));
			channel2Addr++;
			channel2Cmd += 2;
			//printf("%s\n", i2cCmd);
			run_command(i2cCmd, 0);
		}

		/* BLUE light command  */
		run_command("i2c mw 32 0x1 0x15", 0);
		for (index = 0; index < 32 && *channel3Cmd != '\0'; index++)
		{
			snprintf(i2cCmd, 32, "i2c mw 32 0x%x 0x%c%c", channel3Addr, *channel3Cmd, *(channel3Cmd + 1));
			channel3Addr++;
			channel3Cmd += 2;
			//printf("%s\n", i2cCmd);
			run_command(i2cCmd, 0);
		}

		/* run command  */
		run_command("i2c mw 32 0x1 0x2a", 0);
		run_command("i2c mw 32 0x0 0xea", 0);


        run_command("i2c mw 30 0x8 1", 0);
        run_command("i2c mw 30 0x7 11", 0);
        run_command("i2c mw 30 0x6 2", 0);
        run_command("i2c mw 30 0x1 15", 0);
        run_command("i2c mw 30 0x2 138", 0);
        run_command("i2c mw 30 0x5 0x66", 0);
        run_command("i2c mw 30 0x4 0x6a", 0);
	}
	else
	{
		printf("argv[1] %s error...\n", argv[1]);
	}

	return 0;
}

U_BOOT_CMD(
	debugFy, CONFIG_SYS_MAXARGS, 1, do_debugFy,
	"debug program of tp-link.",
	""
);
/* add end  */
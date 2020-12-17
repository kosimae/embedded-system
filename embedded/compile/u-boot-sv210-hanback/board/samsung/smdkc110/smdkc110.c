/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
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

#include <common.h>
#include <regs.h>
#include <asm/io.h>

/* nCS1 - FPGA SROM_BC Resister Value */
#define FPGA_Tacs	(0x0)	// 0clk		address set-up
#define FPGA_Tcos	(0x4)	// 4clk		chip selection set-up
#define FPGA_Tacc	(0xe)	// 14clk	access cycle
#define FPGA_Tcoh	(0x1)	// 1clk		chip selection hold
#define FPGA_Tah	  (0x4)	// 4clk		address holding time
#define FPGA_Tacp	(0x6)	// 6clk		page mode access cycle
#define FPGA_PMC	  (0x0)	// normal(1data)page mode configuration

/* nCS3 - OXU210 SROM_BC Resister Value */
#define OXU210_Tacs	(0x0)	// 0clk		address set-up
#define OXU210_Tcos	(0x4)	// 4clk		chip selection set-up
#define OXU210_Tacc	(0xe)	// 14clk	access cycle
#define OXU210_Tcoh	(0x1)	// 1clk		chip selection hold
#define OXU210_Tah	  (0x4)	// 4clk		address holding time
#define OXU210_Tacp	(0x6)	// 6clk		page mode access cycle
#define OXU210_PMC	  (0x0)	// normal(1data)page mode configuration

/* nCS4 - SMC9115 SROM_BC Resister Value */
#define SMC9115_Tacs	(0x0)	// 0clk		address set-up
#define SMC9115_Tcos	(0x4)	// 4clk		chip selection set-up
#define SMC9115_Tacc	(0xe)	// 14clk	access cycle
#define SMC9115_Tcoh	(0x1)	// 1clk		chip selection hold
#define SMC9115_Tah	  (0x4)	// 4clk		address holding time
#define SMC9115_Tacp	(0x6)	// 6clk		page mode access cycle
#define SMC9115_PMC	  (0x0)	// normal(1data)page mode configuration

/* nCS4 - ST16C554 SROM_BC Resister Value */
#if 0
#define ST16C554_Tacs	(0x0)	// 0clk		address set-up
#define ST16C554_Tcos	(0x4)	// 4clk		chip selection set-up
#define ST16C554_Tacc	(0xe)	// 14clk	access cycle
#define ST16C554_Tcoh	(0x1)	// 1clk		chip selection hold
#define ST16C554_Tah	  (0x4)	// 4clk		address holding time
#define ST16C554_Tacp	(0x6)	// 6clk		page mode access cycle
#define ST16C554_PMC	  (0x0)	// normal(1data)page mode configuration
#else
#define ST16C554_Tacs	(0x7)	// 0clk		address set-up
#define ST16C554_Tcos	(0xb)	// 4clk		chip selection set-up
#define ST16C554_Tacc	(0xf)	// 14clk	access cycle
#define ST16C554_Tcoh	(0x8)	// 1clk		chip selection hold
#define ST16C554_Tah	(0x9)	// 4clk		address holding time
#define ST16C554_Tacp	(0xd)	// 6clk		page mode access cycle
#define ST16C554_PMC	(0x6)	// normal(1data)page mode configuration
#endif


#define SROM_DATA16_WIDTH(x)	(1<<((x*4)+0))
#define SROM_WAIT_ENABLE(x)	(1<<((x*4)+1))
#define SROM_BYTE_ENABLE(x)	(1<<((x*4)+2))

/* ------------------------------------------------------------------------- */
#define DM9000_Tacs	(0x0)	// 0clk		address set-up
#define DM9000_Tcos	(0x4)	// 4clk		chip selection set-up
#define DM9000_Tacc	(0xE)	// 14clk	access cycle
#define DM9000_Tcoh	(0x1)	// 1clk		chip selection hold
#define DM9000_Tah	(0x4)	// 4clk		address holding time
#define DM9000_Tacp	(0x6)	// 6clk		page mode access cycle
#define DM9000_PMC	(0x0)	// normal(1data)page mode configuration


static inline void delay(unsigned long loops)
{
	__asm__ volatile ("1:\n" "subs %0, %1, #1\n" "bne 1b":"=r" (loops):"0"(loops));
}

/*
 * Miscellaneous platform dependent initialisations
 */
static void dm9000_pre_init(void)
{
	unsigned int tmp;

#if defined(DM9000_16BIT_DATA)
	SROM_BW_REG &= ~(0xf << 20);
	SROM_BW_REG |= (0<<23) | (0<<22) | (0<<21) | (1<<20);
#else	
	SROM_BW_REG &= ~(0xf << 20);
	SROM_BW_REG |= (0<<19) | (0<<18) | (0<<16);
#endif
	SROM_BC5_REG = ((0<<28)|(1<<24)|(5<<16)|(1<<12)|(4<<8)|(6<<4)|(0<<0));

	tmp = MP01CON_REG;
	tmp &=~(0xf<<20);
	tmp |=(2<<20);
	MP01CON_REG = tmp;
}

#if defined(CONFIG_EMPOS3SV210) || defined(CONFIG_SM3SV210)
/* nCS1, Resister init.. */
static void fpga_pre_init(void)
{
  unsigned int tmp;

  SROM_BW_REG &= ~(0xf << 4);
  SROM_BW_REG |= (0<<7) | (0<<6) | (1<<5) | (1<<4);
  SROM_BC1_REG = ((FPGA_Tacs<<28)|(FPGA_Tcos<<24)|(FPGA_Tacc<<16)|(FPGA_Tcoh<<12)|(FPGA_Tah<<8)|(FPGA_Tacp<<4)|(FPGA_PMC<<0));

  tmp = MP01CON_REG;
  tmp &=~(0xf<<4);
  tmp |=(2<<4);
  MP01CON_REG = tmp;
}
#endif

#if defined(CONFIG_EMPOS3SV210)
/* nCS3, Resister init.. */
static void oxu210_pre_init(void)
{
  unsigned int tmp;

  SROM_BW_REG &= ~(0xf << 12);
  SROM_BW_REG |= (0<<15) | (0<<14) | (1<<13) | (1<<12);
  SROM_BC3_REG = ((OXU210_Tacs<<28)|(OXU210_Tcos<<24)|(OXU210_Tacc<<16)|(OXU210_Tcoh<<12)|(OXU210_Tah<<8)|(OXU210_Tacp<<4)|(OXU210_PMC<<0));

  tmp = MP01CON_REG;
  tmp &=~(0xf<<12);
  tmp |=(2<<12);
  MP01CON_REG = tmp;
}
#endif

#if defined(CONFIG_EMPOS3SV210)
/* nCS4, Resister init.. */
static void smc9115_pre_init(void)
{
  unsigned int tmp;

  SROM_BW_REG &= ~(0xf << 16);
  SROM_BW_REG |= (0<<19) | (0<<18) | (0<<17) | (1<<16);
  SROM_BC4_REG = ((SMC9115_Tacs<<28)|(SMC9115_Tcos<<24)|(SMC9115_Tacc<<16)|(SMC9115_Tcoh<<12)|(SMC9115_Tah<<8)|(SMC9115_Tacp<<4)|(SMC9115_PMC<<0));

  tmp = MP01CON_REG;
  tmp &=~(0xf<<16);
  tmp |=(2<<16);
  MP01CON_REG = tmp;
}
#elif defined(CONFIG_SM3SV210)
/* nCS5, Resister init.. */
static void smc9115_pre_init(void)
{
	unsigned int tmp;

	SROM_BW_REG &= ~(0xf << 20);
	SROM_BW_REG |= (0<<23) | (0<<22) | (0<<21) | (1<<20);
  SROM_BC5_REG = ((SMC9115_Tacs<<28)|(SMC9115_Tcos<<24)|(SMC9115_Tacc<<16)|(SMC9115_Tcoh<<12)|(SMC9115_Tah<<8)|(SMC9115_Tacp<<4)|(SMC9115_PMC<<0));

	tmp = MP01CON_REG;
	tmp &=~(0xf<<20);
	tmp |=(2<<20);
	MP01CON_REG = tmp;
}
#endif

#if defined(CONFIG_SM3SV210)
/* nCS4, Resister init.. */
static void st16c554_pre_init(void)
{
  unsigned int tmp;

  SROM_BW_REG &= ~(0xf << 16);
  SROM_BW_REG |= (0<<19) | (0<<18) | (0<<17) | (1<<16);
  SROM_BC4_REG = ((ST16C554_Tacs<<28)|(ST16C554_Tcos<<24)|(ST16C554_Tacc<<16)|(ST16C554_Tcoh<<12)|(ST16C554_Tah<<8)|(ST16C554_Tacp<<4)|(ST16C554_PMC<<0));

  tmp = MP01CON_REG;
  tmp &=~(0xf<<16);
  tmp |=(2<<16);
  MP01CON_REG = tmp;
}
#endif

#if defined(CONFIG_EMPOS3SV210)
/* ADC Channel0 Setting */
static void touch_pre_init(void)
{
  unsigned int tmp;

  tmp = __REG(ELFIN_ADC_BASE);
  tmp |= (1<<17);
  __REG(ELFIN_ADC_BASE) = tmp;
}
#endif

int board_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_EMPOS3SV210) || defined(CONFIG_SM3SV210)
  fpga_pre_init();
#endif

#if defined(CONFIG_EMPOS3SV210)
  oxu210_pre_init();
#endif

#ifdef CONFIG_DRIVER_SMC911X
	smc9115_pre_init();
#endif

#if defined(CONFIG_EMPOS3SV210)
  touch_pre_init();
#endif

#if defined(CONFIG_SM3SV210)
  st16c554_pre_init();
#endif

#if defined(CONFIG_SM3SV210)
#if 0
  /* SDKIM :: PS_HOLD_CONTROL */
  GPH0CON_REG |= 0x01;/* EINT0 :: OUT */
  GPH0DAT_REG |= 0x01; /* drive High */
  PS_HOLD_REG = 0x5330;

  /* SDKIM :: DVI, GPS Disable */
  GPH1CON_REG &= ~0x0f;
  GPH1CON_REG |= 0x01;/* EINT8 :: OUT */
  GPH1DAT_REG &= ~0x01; /* drive Low */
#endif
#endif

#ifdef CONFIG_DRIVER_DM9000
	dm9000_pre_init();
#endif

	gd->bd->bi_arch_number = MACH_TYPE;
	gd->bd->bi_boot_params = (PHYS_SDRAM_1+0x100);

	return 0;
}

int dram_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

#if defined(PHYS_SDRAM_2)
	gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
	gd->bd->bi_dram[1].size = PHYS_SDRAM_2_SIZE;
#endif

	return 0;
}

#ifdef BOARD_LATE_INIT
#if defined(CONFIG_BOOT_NAND)
int board_late_init (void)
{
	uint *magic = (uint*)(PHYS_SDRAM_1);
	char boot_cmd[100];

	if ((0x24564236 == magic[0]) && (0x20764316 == magic[1])) {
		sprintf(boot_cmd, "nand erase 0 40000;nand write %08x 0 40000", PHYS_SDRAM_1 + 0x8000);
		magic[0] = 0;
		magic[1] = 0;
		printf("\nready for self-burning U-Boot image\n\n");
		setenv("bootdelay", "0");
		setenv("bootcmd", boot_cmd);
	}

	return 0;
}
#elif defined(CONFIG_BOOT_MOVINAND)
int board_late_init (void)
{
	uint *magic = (uint*)(PHYS_SDRAM_1);
	char boot_cmd[100];
	int hc;

	hc = (magic[2] & 0x1) ? 1 : 0;

	if ((0x24564236 == magic[0]) && (0x20764316 == magic[1])) {
		sprintf(boot_cmd, "movi init %d %d;movi write u-boot %08x", magic[3], hc, PHYS_SDRAM_1 + 0x8000);
		magic[0] = 0;
		magic[1] = 0;
		printf("\nready for self-burning U-Boot image\n\n");
		setenv("bootdelay", "0");
		setenv("bootcmd", boot_cmd);
	}

	return 0;
}
#else
int board_late_init (void)
{
	return 0;
}
#endif
#endif

#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
#ifdef CONFIG_MCP_SINGLE
#if defined(CONFIG_VOGUES)
	printf("\nBoard:   VOGUESV210\n");
#else
	printf("\nBoard:   SMDKV210\n");
#endif //CONFIG_VOGUES
#else
	printf("\nBoard:   SMDKC110\n");
#endif
	return (0);
}
#endif

#ifdef CONFIG_ENABLE_MMU

#ifdef CONFIG_MCP_SINGLE
ulong virt_to_phy_smdkc110(ulong addr)
{
	if ((0xc0000000 <= addr) && (addr < 0xd0000000))
		return (addr - 0xc0000000 + 0x20000000);
	else
		printf("The input address don't need "\
			"a virtual-to-physical translation : %08lx\n", addr);

	return addr;
}
#else
ulong virt_to_phy_smdkc110(ulong addr)
{
	if ((0xc0000000 <= addr) && (addr < 0xd0000000))
		return (addr - 0xc0000000 + 0x30000000);
	else if ((0x30000000 <= addr) && (addr < 0x50000000))
		return addr;
	else
		printf("The input address don't need "\
			"a virtual-to-physical translation : %08lx\n", addr);

	return addr;
}
#endif

#endif

#if defined(CONFIG_CMD_NAND) && defined(CFG_NAND_LEGACY)
#include <linux/mtd/nand.h>
extern struct nand_chip nand_dev_desc[CFG_MAX_NAND_DEVICE];
void nand_init(void)
{
	nand_probe(CFG_NAND_BASE);
        if (nand_dev_desc[0].ChipID != NAND_ChipID_UNKNOWN) {
                print_size(nand_dev_desc[0].totlen, "\n");
        }
}
#endif

#if defined(CONFIG_EMPOS3SV210) || defined(CONFIG_SM3SV210)
#if defined(CONFIG_CMD_BOOTARGS)
int get_env_to_bootarg(void)
{
  int i;
  char *varname[12] = { "root", "rootfstype", "init", "consoledev", "consolerate", "mem", "ethaddr", "ipaddr", "serverip", "gatewayip", "netmask", "bootargs" };
  char varvalue[12][256];
  char temp[6];
  char *s;
  unsigned long bi_ip_addr;

  /* root env value */
  s = getenv("root");
  sprintf(varvalue[0], s);
  //printf("varvalue[0]=%s\n", varvalue[0]);

  /* rootfstype env value */
  s = getenv("rootfstype");
  sprintf(varvalue[1], s);
  //printf("varvalue[1]=%s\n", varvalue[1]);

  /* init env value */
  s = getenv("init");
  sprintf(varvalue[2], s);
  //printf("varvalue[2]=%s\n", varvalue[2]);

  /* consoledev env value */
  s = getenv("consoledev");
  sprintf(varvalue[3], s);
  //printf("varvalue[3]=%s\n", varvalue[3]);

  /* consoledev env value */
  s = getenv("consolerate");
  sprintf(varvalue[4], s);
  //printf("varvalue[4]=%s\n", varvalue[4]);

#ifdef CONFIG_MEM
  /* mem env value */
  s = getenv("mem");
  sprintf(varvalue[5], s);
  //printf("varvalue[5]=%s\n", varvalue[5]);
#endif

  /* board ethaddr env value */
  s = getenv("ethaddr");
  char *e;
  for (i = 0; i < 6; ++i) {
    temp[i] = simple_strtoul(s, &e, 16);
    s = (*e) ? e + 1 : e;
  }
  sprintf(varvalue[6], "%02x:%02x:%02x:%02x:%02x:%02x", temp[0], temp[1], temp[2], temp[3], temp[4], temp[5]);
  //printf("varvalue[6]=%s\n", varvalue[6]);

  /* board ipaddr env value */
  bi_ip_addr = getenv_IPaddr ("ipaddr");
  sprintf(varvalue[7], "%d.%d.%d.%d", ((bi_ip_addr >> 0) & 0xff), (u8) ((bi_ip_addr >> 8) & 0xff), (u8) ((bi_ip_addr >> 16) & 0xff), (u8) ((bi_ip_addr >> 24) & 0xff));
  //printf("varvalue[7]=%s\n", varvalue[7]);

  /* serverip env value */
  bi_ip_addr = getenv_IPaddr ("serverip");
  sprintf(varvalue[8], "%d.%d.%d.%d", ((bi_ip_addr >> 0) & 0xff), (u8) ((bi_ip_addr >> 8) & 0xff), (u8) ((bi_ip_addr >> 16) & 0xff), (u8) ((bi_ip_addr >> 24) & 0xff));
  //printf("varvalue[8]=%s\n", varvalue[8]);

  /* gatewayip env value */
  bi_ip_addr = getenv_IPaddr ("gatewayip");
  sprintf(varvalue[9], "%d.%d.%d.%d", ((bi_ip_addr >> 0) & 0xff), (u8) ((bi_ip_addr >> 8) & 0xff), (u8) ((bi_ip_addr >> 16) & 0xff), (u8) ((bi_ip_addr >> 24) & 0xff));
  //printf("varvalue[9]=%s\n", varvalue[9]);

  /* netmask env value */
  bi_ip_addr = getenv_IPaddr ("netmask");
  sprintf(varvalue[10], "%d.%d.%d.%d", ((bi_ip_addr >> 0) & 0xff), (u8) ((bi_ip_addr >> 8) & 0xff), (u8) ((bi_ip_addr >> 16) & 0xff), (u8) ((bi_ip_addr >> 24) & 0xff));
  //printf("varvalue[10]=%s\n", varvalue[10]);

#ifdef CONFIG_MEM
  /* bootargs env value */
  sprintf(varvalue[11], "root=%s rootfstype=%s init=%s console=%s,%s mem=%s ip=%s:%s:%s:%s::eth0:off ethaddr=%s", varvalue[0], varvalue[1], varvalue[2], varvalue[3], varvalue[4], varvalue[5], varvalue[7], varvalue[8], varvalue[9], varvalue[10], varvalue[6]);
#else
  sprintf(varvalue[11], "root=%s rootfstype=%s init=%s console=%s,%s ip=%s:%s:%s:%s::eth0:off ethaddr=%s", varvalue[0], varvalue[1], varvalue[2], varvalue[3], varvalue[4], varvalue[7], varvalue[8], varvalue[9], varvalue[10], varvalue[6]);
#endif
  //printf("varvalue[11]=%s\n", varvalue[11]);

  /* saving bootargs env value */
  setenv(varname[11], varvalue[11]);

  return 0;
}
#endif
#endif

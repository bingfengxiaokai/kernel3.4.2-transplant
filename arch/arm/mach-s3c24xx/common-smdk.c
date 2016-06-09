/* linux/arch/arm/plat-s3c24xx/common-smdk.c
 *
 * Copyright (c) 2006 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * Common code for SMDK2410 and SMDK2440 boards
 *
 * http://www.fluff.org/ben/smdk2440/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>
#include <linux/io.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <asm/irq.h>

#include <mach/regs-gpio.h>
#include <mach/leds-gpio.h>

#include <plat/nand.h>

#include <plat/common-smdk.h>
#include <plat/gpio-cfg.h>
#include <plat/devs.h>
#include <plat/pm.h>

#include <linux/dm9000.h>

#define MACH_SMDK2440_DM9K_BASE (S3C2410_CS4 + 0x300)

/* LED devices */

static struct s3c24xx_led_platdata smdk_pdata_led4 = {
	.gpio		= S3C2410_GPF(4),
	.flags		= S3C24XX_LEDF_ACTLOW | S3C24XX_LEDF_TRISTATE,
	.name		= "led4",
	.def_trigger	= "timer",
};

static struct s3c24xx_led_platdata smdk_pdata_led5 = {
	.gpio		= S3C2410_GPF(5),
	.flags		= S3C24XX_LEDF_ACTLOW | S3C24XX_LEDF_TRISTATE,
	.name		= "led5",
	.def_trigger	= "nand-disk",
};

static struct s3c24xx_led_platdata smdk_pdata_led6 = {
	.gpio		= S3C2410_GPF(6),
	.flags		= S3C24XX_LEDF_ACTLOW | S3C24XX_LEDF_TRISTATE,
	.name		= "led6",
};

static struct s3c24xx_led_platdata smdk_pdata_led7 = {
	.gpio		= S3C2410_GPF(7),
	.flags		= S3C24XX_LEDF_ACTLOW | S3C24XX_LEDF_TRISTATE,
	.name		= "led7",
};

static struct platform_device smdk_led4 = {
	.name		= "s3c24xx_led",
	.id		= 0,
	.dev		= {
		.platform_data = &smdk_pdata_led4,
	},
};

static struct platform_device smdk_led5 = {
	.name		= "s3c24xx_led",
	.id		= 1,
	.dev		= {
		.platform_data = &smdk_pdata_led5,
	},
};

static struct platform_device smdk_led6 = {
	.name		= "s3c24xx_led",
	.id		= 2,
	.dev		= {
		.platform_data = &smdk_pdata_led6,
	},
};

static struct platform_device smdk_led7 = {
	.name		= "s3c24xx_led",
	.id		= 3,
	.dev		= {
		.platform_data = &smdk_pdata_led7,
	},
};

/* NAND parititon from 2.4.18-swl5 */

static struct mtd_partition smdk_default_nand_part[] = {
	[0] = {
		.name	= "uboot",
		.size	= SZ_256K,
		.offset	= 0,
	},
	[1] = {
		.name	= "envrionment parameter",
		.offset = MTDPART_OFS_APPEND,
		.size	= SZ_128K,
	},
	[2] = {
		.name	= "kernel",
		.offset = MTDPART_OFS_APPEND,
		.size	= SZ_3M,
	},
	[3] = {
		.name	= "rootfs",
		.offset	= MTDPART_OFS_APPEND,
		.size	= MTDPART_SIZ_FULL,
	}
};

static struct s3c2410_nand_set smdk_nand_sets[] = {
	[0] = {
		.name		= "NAND",
		.nr_chips	= 1,
		.nr_partitions	= ARRAY_SIZE(smdk_default_nand_part),
		.partitions	= smdk_default_nand_part,
	},
};

/* choose a set of timings which should suit most 512Mbit
 * chips and beyond.
*/

static struct s3c2410_platform_nand smdk_nand_info = {
	.tacls		= 20,
	.twrph0		= 60,
	.twrph1		= 20,
	.nr_sets	= ARRAY_SIZE(smdk_nand_sets),
	.sets		= smdk_nand_sets,
};

/* DM9000AEP 10/100 ethernet controller */

static struct resource SMDK2440_dm9k_resource[] = {
	[0] = {
		.start = MACH_SMDK2440_DM9K_BASE,
		.end   = MACH_SMDK2440_DM9K_BASE + 3,
		.flags = IORESOURCE_MEM
	},
	[1] = {
		.start = MACH_SMDK2440_DM9K_BASE + 4,
		.end   = MACH_SMDK2440_DM9K_BASE + 7,
		.flags = IORESOURCE_MEM
	},
	[2] = {
		.start = S3C2410_PA_MEMCTRL,
		.end   = S3C2410_PA_MEMCTRL + S3C24XX_SZ_MEMCTRL,
		.flags = IORESOURCE_MEM
	},
	[3] = {
		.start = IRQ_EINT7,
		.end   = IRQ_EINT7,
		.flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE,
	}
};


/*
 * The DM9000 has no eeprom, and it's MAC address is set by
 * the bootloader before starting the kernel.
 */
static struct dm9000_plat_data SMDK2440_dm9k_pdata = {
	.flags		= (DM9000_PLATF_16BITONLY | DM9000_PLATF_NO_EEPROM),
};

static struct platform_device SMDK2440_device_eth = {
	.name		= "dm9000",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(SMDK2440_dm9k_resource),
	.resource	= SMDK2440_dm9k_resource,
	.dev		= {
		.platform_data	= &SMDK2440_dm9k_pdata,
	},
};
/* devices we initialise */

static struct platform_device __initdata *smdk_devs[] = {
	&s3c_device_nand,
	&smdk_led4,
	&smdk_led5,
	&smdk_led6,
	&smdk_led7,
	&SMDK2440_device_eth,
};

void __init smdk_machine_init(void)
{
	/* Configure the LEDs (even if we have no LED support)*/

	s3c_gpio_cfgpin(S3C2410_GPF(4), S3C2410_GPIO_OUTPUT);
	s3c_gpio_cfgpin(S3C2410_GPF(5), S3C2410_GPIO_OUTPUT);
	s3c_gpio_cfgpin(S3C2410_GPF(6), S3C2410_GPIO_OUTPUT);
	s3c_gpio_cfgpin(S3C2410_GPF(7), S3C2410_GPIO_OUTPUT);

	s3c2410_gpio_setpin(S3C2410_GPF(4), 1);
	s3c2410_gpio_setpin(S3C2410_GPF(5), 1);
	s3c2410_gpio_setpin(S3C2410_GPF(6), 1);
	s3c2410_gpio_setpin(S3C2410_GPF(7), 1);

	if (machine_is_smdk2443())
		smdk_nand_info.twrph0 = 50;

	s3c_nand_set_platdata(&smdk_nand_info);

	platform_add_devices(smdk_devs, ARRAY_SIZE(smdk_devs));

	s3c_pm_init();
}

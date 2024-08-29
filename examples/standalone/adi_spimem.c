/*
 * adi_spimem.c - run sc598 soc in spimem mode
 *
 * Copyright 2024 Analog Devices Inc.
 *
 * Licensed under the 2-clause BSD.
 */

#include <common.h>
#include <exports.h>
#include <spi.h>
#include <asm/io.h>

#define CMD_WRITE_STATUS	0x01
#define CMD_READ_STATUS		0x05
#define CMD_ID			0x9f

#define SPI2_REGS ((void *)0x31030000)
#define SPI2_MMAP ((void *)0x60000000)
#define LINUX_SRC (SPI2_MMAP + 0x01a0000)
#define LINUX_SZ (0x1000000)
#define LOADADDR ((void *)0x90001000)

struct adi_spi_regs;
void chip_select_en(struct adi_spi_regs *raw_spi);
void memmap_quad_en(struct adi_spi_regs * raw_spi);

int adi_spimem(int argc, char *const argv[])
{
	struct spi_slave *slave = NULL;
	struct adi_spi_regs *raw_spi = SPI2_REGS;
	uchar buf[4];
	int ret = 0;

	/* Print the ABI version */
	app_startup(argv);
	if (XF_VERSION != get_version()) {
		printf("Expects ABI version %d\n", XF_VERSION);
		printf("Actual U-Boot ABI version %lu\n", get_version());
		printf("Can't run\n\n");
		return 1;
	}

	slave = spi_setup_slave(2, 1, 1000, SPI_MODE_0);
	if (!slave) {
		puts("unable to setup slave\n");
		return -ENODEV;
	}

	ret = spi_claim_bus(slave);
	if (ret) {
		puts("error spi_claim_bus()\n");
		spi_free_slave(slave);
		goto done1;
	}

	buf[0] = CMD_ID;
	ret = spi_xfer(slave, 8 * sizeof(buf), buf, buf, SPI_XFER_BEGIN | SPI_XFER_END);
	if (ret) {
		puts("failed to read jedec id\n");
		goto done2;
	}

	printf("jedec id: %x %x %x\n", buf[1], buf[2], buf[3]);

	buf[0] = CMD_READ_STATUS;
	ret = spi_xfer(slave, 8 * sizeof(buf), buf, buf, SPI_XFER_BEGIN | SPI_XFER_END);
	if (ret) {
		puts("failed to read jedec id\n");
		goto done2;
	}

	printf("status: %x\n", buf[1]);

	//spi_release_bus(slave);
	//spi_free_slave(slave);
	//chip_select_en(raw_spi);
	
	puts("memmap_quad_en()\n");
	memmap_quad_en(raw_spi);

	printf("copying %p to %p (%d bytes)...\n", LINUX_SRC, LOADADDR, LINUX_SZ);
	memcpy(LOADADDR, LINUX_SRC, LINUX_SZ);
	puts("done.\n");

done2:
	spi_release_bus(slave);
done1:
	spi_free_slave(slave);

	return ret;
}

struct adi_spi_regs {
	u32 revid;
	u32 control;
	u32 rx_control;
	u32 tx_control;
	u32 clock;
	u32 delay;
	u32 ssel;
	u32 rwc;
	u32 rwcr;
	u32 twc;
	u32 twcr;
	u32 reserved0;
	u32 emask;
	u32 emaskcl;
	u32 emaskst;
	u32 reserved1;
	u32 status;
	u32 elat;
	u32 elatcl;
	u32 reserved2;
	u32 rfifo;
	u32 reserved3;
	u32 tfifo;
	u32 reserved4;
	u32 mmrdh;
	u32 mmtop;
};

#define BIT_SSEL_VAL(x) ((1 << 8) << (x)) /* Slave Select input value bit */
#define BIT_SSEL_EN(x) (1 << (x))         /* Slave Select enable bit*/
#define CS_NUM 1

void chip_select_en(struct adi_spi_regs *raw_spi) {
	u32 ssel = readl(&raw_spi->ssel);
	ssel |= BIT_SSEL_EN(CS_NUM);
	ssel |= BIT_SSEL_VAL(CS_NUM);
	writel(ssel, &raw_spi->ssel);
}

void memmap_quad_en(struct adi_spi_regs *raw_spi) {
	printf("control 0x%p\n", &raw_spi->control);
	writel(0x00000000, &raw_spi->control);
	printf("clock 0x%p\n", &raw_spi->clock);
	writel(0x00000006, &raw_spi->clock);
	printf("rx_control 0x%p\n", &raw_spi->rx_control);
	writel(0x00000005, &raw_spi->rx_control);  /*  Receive Control Register */
	printf("tx_control 0x%p\n", &raw_spi->tx_control);
	writel(0x00000005, &raw_spi->tx_control);  /*  Transmit Control Register */
	printf("delay 0x%p\n", &raw_spi->delay);
	writel(0x00000301, &raw_spi->delay);    /*  Delay Register */
	printf("ssel 0x%p\n", &raw_spi->ssel);
	writel(0x0000FE02, &raw_spi->ssel); /*  Slave Select Register */
	printf("mmrdh 0x%p\n", &raw_spi->mmrdh);
	writel(0x0500136B, &raw_spi->mmrdh);                    /*  Memory Mapped Read Header */
	printf("mmtop 0x%p\n", &raw_spi->mmtop);
	writel(0x7FFFFFFF, &raw_spi->mmtop);
	printf("control 0x%p\n", &raw_spi->control);
	writel(0x80240073, &raw_spi->control);
}

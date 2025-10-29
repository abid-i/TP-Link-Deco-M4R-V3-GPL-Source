#include <common.h>
#include <spi_flash.h>

static int debug = 0;

static struct spi_flash *flash = NULL;

#ifndef CONFIG_SF_DEFAULT_SPEED
# define CONFIG_SF_DEFAULT_SPEED	1000000
#endif
#ifndef CONFIG_SF_DEFAULT_MODE
# define CONFIG_SF_DEFAULT_MODE		SPI_MODE_3
#endif
#ifndef CONFIG_SF_DEFAULT_CS
# define CONFIG_SF_DEFAULT_CS		0
#endif
#ifndef CONFIG_SF_DEFAULT_BUS
# define CONFIG_SF_DEFAULT_BUS		0
#endif

int
nm_flashOpPortInit(void)
{
    unsigned int bus = CONFIG_SF_DEFAULT_BUS;
    unsigned int cs = CONFIG_SF_DEFAULT_CS;
    unsigned int speed = CONFIG_SF_DEFAULT_SPEED;
    unsigned int mode = CONFIG_SF_DEFAULT_MODE;

    flash = spi_flash_probe(bus, cs, speed, mode);
    if (!flash)
    {
        printf("Failed to initialize SPI flash at %u:%u\n", bus, cs);
        return -1;
    }

    return 0;
}

int
nm_flashOpPortFree(void)
{
    if (flash)
    {
        spi_flash_free(flash);
        flash = NULL;
    }

    return 0;
}

int
nm_flashOpPortErase(unsigned int offset, unsigned int len)
{
    if (debug)
    {
        printf("sf erase 0x%x +0x%x\n", offset, len);
    }

    if (spi_flash_erase(flash, offset, len))
    {
        printf("SPI flash erase failed\n");
        return -1;
    }
    return 0;
}

int
nm_flashOpPortRead(unsigned int offset, unsigned char *buf, unsigned int len)
{
    if (debug)
    {
        printf("sf read 0x%p 0x%x 0x%x\n", buf, offset, len);
    }

    if (spi_flash_read(flash, offset, len, buf))
    {
        printf("SPI flash read failed\n");
        return -1;
    }
    return 0;
}

int
nm_flashOpPortWrite(unsigned int offset, unsigned char *buf, unsigned int len)
{
    if (debug)
    {
        printf("sf write 0x%p 0x%x 0x%x\n", buf, offset, len);
    }

    if (spi_flash_write(flash, offset, len, buf))
    {
        printf("SPI flash write failed\n");
        return -1;
    }
    return 0;
}

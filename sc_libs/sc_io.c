#include "sc_io.h"
#include "stdint.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/version.h>


uint32_t SC_In32(uint64_t addr)
{
    uint32_t data_word = 0x0;
    ssize_t rc = pread(fd, &data_word, 4, addr);
    return data_word;

}

void SC_Out32(uint64_t addr, uint32_t data)
{
    ssize_t rc = pwrite(fd, &data, 4, addr);
}

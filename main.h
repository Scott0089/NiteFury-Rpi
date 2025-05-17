#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "xgpio.h"


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>

#define MAP_SIZE 0x1000
#define MAP_MASK (MAP_SIZE - 1)
#define TARGET_PHYS_ADDR 0x40000000  // Replace with your actual address

#define SDT

extern int fd;

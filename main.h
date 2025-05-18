#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "xgpio.h"
#include "xtmrctr.h"
#include "xsysmon.h"

#include "sc_io.h"

#define TARGET_PHYS_ADDR 0x40000000 

#define SDT

extern int fd;



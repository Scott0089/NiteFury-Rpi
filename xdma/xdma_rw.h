#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>

#include <sys/time.h>


int dev_read (int dev_fd, uint64_t addr, void *buffer, uint64_t size);
int dev_write (int dev_fd, uint64_t addr, void *buffer, uint64_t size);
#include "main.h"

XGpio gpioInst;

#define AXI_Translation 0x40000000

int fd = -1;

int main()
{
    fd = open("/dev/xdma0_user", O_RDWR);
    if(fd < 0)
    {
        perror("open\r\n");
        return -1;
    }
printf("%x\r\n", XPAR_GPIO_1_BASEADDR);
   
 int status;
   // status = XGpio_Initialize(&gpioInst, XPAR_GPIO_1_BASEADDR);
    status = XGpio_Initialize(&gpioInst, 0x40000000 - AXI_Translation);
    if(status !=  XST_SUCCESS)
    {
        printf("Failed to Init GPIO!\r\n");
	printf("Status: %d\r\n", status);
	return status;
    }

//gpioInst.BaseAddress = 0x40010000;

    printf("%x\r\n", XPAR_GPIO_1_BASEADDR - AXI_Translation);
    printf("%p\r\n", gpioInst.BaseAddress);

 uint64_t address = XPAR_GPIO_1_BASEADDR - AXI_Translation;
/*
printf("got here 1\r\n");
    XGpio_DiscreteWrite(&gpioInst, 0x00, 0x00);
printf("got here 2\r\n");
    sleep(1);
printf("got here 3\r\n");
    XGpio_DiscreteWrite(&gpioInst, 0x00, 0x01);
printf("got here 4\r\n");

*/
    XGpio_WriteReg(address, 0x00, 0x00);
    sleep(1);
    XGpio_WriteReg(address, 0x00, 0x01);



    printf("Everything done! \r\n Exiting... \r\n");

    close(fd);

    return 0;
}

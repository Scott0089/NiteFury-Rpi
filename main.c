#include "main.h"

#include "xparameters.h"

#define ON 0x00
#define OFF 0x01

#define AXI_Translation 0x40000000

XGpio gpioInst[4];

uint64_t testarr[4] = {XPAR_AXI_GPIO_0_BASEADDR, XPAR_AXI_GPIO_1_BASEADDR, XPAR_AXI_GPIO_2_BASEADDR, XPAR_AXI_GPIO_3_BASEADDR};

int fd = -1;

int main()
{
    fd = open("/dev/xdma0_user", O_RDWR);
    if(fd < 0)
    {
        perror("open\r\n");
        return -1;
    }

    int status;
    for (size_t i = 0; i < XPAR_XGPIO_NUM_INSTANCES; i++)
    {
        status = XGpio_Initialize(&gpioInst[i], testarr[i]);
        if (status != XST_SUCCESS) 
        {
            printf("GPIO %d failed to Init!\r\n", i);
            return XST_FAILURE;
        }

        gpioInst[i].BaseAddress = gpioInst[i].BaseAddress - AXI_Translation;
    }

    for (size_t i = 0; i < XPAR_XGPIO_NUM_INSTANCES; i++)
    {
        XGpio_DiscreteWrite(&gpioInst[i], 0x01, OFF);
    }

    while (1)
    {
        for (size_t i = 0; i < XPAR_XGPIO_NUM_INSTANCES; i++)
        {
            XGpio_DiscreteWrite(&gpioInst[i], 0x01, ON);
            sleep(1);
        }
        for (size_t i = 0; i < XPAR_XGPIO_NUM_INSTANCES; i++)
        {
            XGpio_DiscreteWrite(&gpioInst[3-i], 0x01, OFF);
            sleep(1);
        }
    }

    printf("Everything done! \r\n Exiting... \r\n");

    close(fd);

    return 0;
}

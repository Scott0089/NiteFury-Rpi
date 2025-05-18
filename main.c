#include "main.h"
#include "xparameters.h"

#define ON 0x00
#define OFF 0x01

#define AXI_Translation 0x40000000

XGpio gpioInst[5];

uint64_t testarr[4] = {XPAR_AXI_GPIO_0_BASEADDR, XPAR_AXI_GPIO_1_BASEADDR, XPAR_AXI_GPIO_2_BASEADDR, XPAR_AXI_GPIO_3_BASEADDR};
#define adj_addr XPAR_AXI_GPIO_1_BASEADDR - AXI_Translation

#define TIMER_COUNTER_0	 0

XTmrCtr tmrInst;

int fd = -1;

int main()
{
    fd = open("/dev/xdma0_user", O_RDWR);
    if(fd < 0)
    {
        printf("Failed to open /dev/xdma0_user!\r\n");
        return XST_FAILURE;
    }

    uint32_t status;
    uint32_t data = NULL;

    status = XGpio_Initialize(&gpioInst[4], XPAR_AXI_GPIO_4_BASEADDR);
    if (status != XST_SUCCESS) 
    {
        printf("GPIO Read failed to Init!\r\n");
        return XST_FAILURE;
    }

    data = XGpio_DiscreteRead(&gpioInst[4], 0x01);

    printf("\n\n\r     ** NiteFury RaspberryPi **\n\r");
    printf("   FPGA Build/Version: 0x%08X\n\n\r", XGpio_DiscreteRead(&gpioInst[4], 1));
    
    for (size_t i = 0; i < 4; i++)
    {
        status = XGpio_Initialize(&gpioInst[i], testarr[i]);
        if (status != XST_SUCCESS) 
        {
            printf("GPIO %d failed to Init!\r\n", i);
            return XST_FAILURE;
        }

    }

    for (size_t i = 0; i < 4; i++)
    {
        XGpio_DiscreteWrite(&gpioInst[i], 0x01, OFF);
    }

    XGpio_DiscreteWrite(&gpioInst[2], 0x01, ON);
    XGpio_DiscreteWrite(&gpioInst[3], 0x01, ON);
    
    status = XTmrCtr_Initialize(&tmrInst, XPAR_AXI_TIMER_0_BASEADDR);
    if (status != XST_SUCCESS) {
        printf("%d \r\n", status);
        return XST_FAILURE;
    }

   	XTmrCtr_SetResetValue(&tmrInst, 0, (u32) -50000000);
   	XTmrCtr_Start(&tmrInst, 0);
    
    XTmrCtr_SetResetValue(&tmrInst, 1, (u32) -37500000);
   	XTmrCtr_Start(&tmrInst, 1);

    uint8_t data2 = 0x00;

    while(1)
    {
        if(XTmrCtr_IsExpired(&tmrInst, 1))
        {

            XTmrCtr_Reset(&tmrInst, 1);
            data2 = XGpio_DiscreteRead(&gpioInst[1], 0x01);
            XGpio_DiscreteWrite(&gpioInst[1], 0x01, !data2);
            data2 = XGpio_DiscreteRead(&gpioInst[2], 0x01);
            XGpio_DiscreteWrite(&gpioInst[2], 0x01, !data2);
        }

        if(XTmrCtr_IsExpired(&tmrInst, 0))
        {

            XTmrCtr_Reset(&tmrInst, 0);
            data2 = XGpio_DiscreteRead(&gpioInst[0], 0x01);
            XGpio_DiscreteWrite(&gpioInst[0], 0x01, !data2);
            data2 = XGpio_DiscreteRead(&gpioInst[3], 0x01);
            XGpio_DiscreteWrite(&gpioInst[3], 0x01, !data2);
        }
    }

    printf("Everything done! \r\n Exiting... \r\n");

    close(fd);

    return 0;
}

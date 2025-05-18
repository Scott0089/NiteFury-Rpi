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
        perror("open\r\n");
        return -1;
    }

    int status;

    
    for (size_t i = 0; i < 4; i++)
    {
        status = XGpio_Initialize(&gpioInst[i], testarr[i]);
        if (status != XST_SUCCESS) 
        {
            printf("GPIO %d failed to Init!\r\n", i);
            return XST_FAILURE;
        }

        //gpioInst[i].BaseAddress = gpioInst[i].BaseAddress - AXI_Translation;
    }

    for (size_t i = 0; i < 4; i++)
    {
        XGpio_DiscreteWrite(&gpioInst[i], 0x01, OFF);
    }

    uint32_t data = NULL;
    status = XGpio_Initialize(&gpioInst[4], XPAR_AXI_GPIO_4_BASEADDR);
    if (status != XST_SUCCESS) 
    {
        printf("GPIO Read failed to Init!\r\n");
        return XST_FAILURE;
    }
    
    //gpioInst[4].BaseAddress = gpioInst[4].BaseAddress - AXI_Translation;

    data = XGpio_DiscreteRead(&gpioInst[4], 0x01);
    printf("Data: %x\r\n", data);
    
    status = XTmrCtr_Initialize(&tmrInst, XPAR_AXI_TIMER_0_BASEADDR);
    if (status != XST_SUCCESS) {
        printf("%d \r\n", status);
        return XST_FAILURE;
    }

    
   	XTmrCtr_SetResetValue(&tmrInst, 0, (u32) -50000000);
   	XTmrCtr_Start(&tmrInst, 0);

    uint8_t data2 = 0x00;

    while(1)
    {
        if(XTmrCtr_IsExpired(&tmrInst, 0))
        {
             //printf("here\r\n");
             XTmrCtr_Reset(&tmrInst, 0);
             data2 = XGpio_DiscreteRead(&gpioInst[0], 0x01);
             XGpio_DiscreteWrite(&gpioInst[0], 0x01, !data2);
        }
    }



    //XGpio_DiscreteWrite(&gpioInst[0], 0x01, ON);
    /*
    while (1)
    {
        for (size_t i = 0; i < 4; i++)
        {
            XGpio_DiscreteWrite(&gpioInst[i], 0x01, ON);
            sleep(1);
        }
        for (size_t i = 0; i < 4; i++)
        {
            XGpio_DiscreteWrite(&gpioInst[3-i], 0x01, OFF);
            sleep(1);
        }
    }


    /*

    status = XTmrCtr_Initialize(&tmrInst, XPAR_AXI_TIMER_0_BASEADDR);
    if (status != XST_SUCCESS) 
    {
        printf("Timer failed to Init!\r\n");
        return XST_FAILURE;
    }
    //tmrInst.BaseAddress = tmrInst.BaseAddress - AXI_Translation;
    
    printf("%x \r\n", tmrInst.BaseAddress);
    //tmrInst.BaseAddress = XPAR_XTMRCTR_0_BASEADDR;
    printf("%x \r\n", tmrInst.BaseAddress);

    /*

    status = XTmrCtr_SelfTest(&tmrInst, 0);
    if (status != XST_SUCCESS) 
    {
        printf("Timer Self Test failed to Init!\r\n");
        return XST_FAILURE;
    }

    */

    //printf("%x \r\n", XPAR_AXI_TIMER_0_BASEADDR);
    //printf("%x \r\n", tmrInst.BaseAddress);
    /*
    uint32_t Value1;
    uint32_t Value2;

    XTmrCtr_SetOptions(&tmrInst, TIMER_COUNTER_0, XTC_AUTO_RELOAD_OPTION);
    Value1 = XTmrCtr_GetValue(&tmrInst, TIMER_COUNTER_0);

    printf("%d \r\n", Value1);
    XTmrCtr_Start(&tmrInst, TIMER_COUNTER_0);

    while(1)
    {
        Value2 = XTmrCtr_GetValue(&tmrInst, TIMER_COUNTER_0);
        //printf("Time: %d\r\n", Value2);
        if (Value1 != Value2)
        {
            break;
        }
        
    }

    while (1)
    {
        for (size_t i = 0; i < 4; i++)
        {
            XGpio_DiscreteWrite(&gpioInst[i], 0x01, ON);
            sleep(1);
        }
        for (size_t i = 0; i < 4; i++)
        {
            XGpio_DiscreteWrite(&gpioInst[3-i], 0x01, OFF);
            sleep(1);
        }
    }

    */

    printf("Everything done! \r\n Exiting... \r\n");

    close(fd);

    return 0;
}

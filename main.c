#include "main.h"
#include "xparameters.h"

#define ON 0x00
#define OFF 0x01

typedef struct 
{
    uint32_t TempRawData;
    uint32_t VCCINTRawData;
    uint32_t VCCAUXRawData;
    uint32_t VCCBRAMRawData;
    float    TempData;
    float    VCCINTData;
    float    VCCAUXData;
    float    VCCBRAMData;
} XADC_Data;

XGpio gpioInst[5];
XTmrCtr tmrInst;
XSysMon sysmonInst;
XSysMon_Config *ConfigPtr;
XBram bramInst;
XBram_Config *BramPtr;

XV_tpg_Config *TpgPtr;
XV_tpg  tpgInst;

XADC_Data xadcInst;

uint64_t testarr[4] = {XPAR_AXI_GPIO_0_BASEADDR, XPAR_AXI_GPIO_1_BASEADDR, XPAR_AXI_GPIO_2_BASEADDR, XPAR_AXI_GPIO_3_BASEADDR};

int fd = -1;
int fd_read = -1;
int hex_fd = -1;

int SysMonFractionToInt(float FloatNum)
{
    float Temp;

    Temp = FloatNum;
    if(FloatNum < 0)
    {
        Temp = -(FloatNum);
    }
    
    return (((int)((Temp - (float)((int)Temp)) * (1000.0f))));
}

int tpg()
{
    uint32_t status;
    ssize_t rc = 0;
    ssize_t bytes_done = 0;
    ssize_t out_offset = 0;

    char* allocated = NULL;
    char* buffer = NULL;

    TpgPtr = XV_tpg_LookupConfig(XPAR_V_TPG_0_BASEADDR);
    if (TpgPtr == NULL)
    {
        printf("TPG failed to get config! \r\n");
        return XST_FAILURE;
    }
    status = XV_tpg_CfgInitialize(&tpgInst, TpgPtr, TpgPtr->BaseAddress);
    if (status != XST_SUCCESS)
    {
        printf("TPG Failed to Init! \r\n");
        return XST_FAILURE;
    }

    printf("TPG Init Success\r\n");

	XV_tpg_Set_height(&tpgInst, 1080);
	XV_tpg_Set_width(&tpgInst, 1920);
	XV_tpg_Set_colorFormat(&tpgInst, XVIDC_CSF_YCRCB_422);
	XV_tpg_Set_maskId(&tpgInst, 0);
	XV_tpg_Set_motionSpeed(&tpgInst, 5);
	XV_tpg_Set_bckgndId(&tpgInst, 0x8);
	XV_tpg_Set_boxSize(&tpgInst, 50);
	XV_tpg_Set_boxColorR(&tpgInst, 0xFF);
	XV_tpg_Set_boxColorG(&tpgInst, 0xFF);
	XV_tpg_Set_boxColorB(&tpgInst, 0xFF);
	XV_tpg_Set_ovrlayId(&tpgInst, 0x01);

    printf("Finished Config\r\n");
    
    XV_tpg_EnableAutoRestart(&tpgInst);
    XV_tpg_Start(&tpgInst);

    printf("Resetting TPG! \r\n");
    XGpio_WriteReg(XPAR_XGPIO_5_BASEADDR, 0x01, 0x00);
    sleep(1);
    XGpio_WriteReg(XPAR_XGPIO_5_BASEADDR, 0x01, 0x01);
    printf("Done resetting TPG\r\n");

    printf("TPG Started\r\n");

    hex_fd = open("VideoOut.bin", O_RDWR | O_CREAT | O_TRUNC | O_SYNC, 0666);
    if(hex_fd < 0)
    {
        printf("Unable to Open File! \r\n");
        return XST_FAILURE;
    }

    posix_memalign((void **)&allocated, 4096 /*alignment */ , 32 + 4096);
	if (!allocated) 
    {
        printf("Failed to allocate memory! \r\n");
        return XST_FAILURE;
	}

    buffer = allocated + 0x00;

    for (size_t i = 0; i < 10000; i++)
    {
        printf("Time: %d \r", i);
        rc = read_to_buffer("/dev/xdma0_c2h_0", fd_read, buffer, 32, 0x00);
        if (rc < 0)
        {
            printf("Failed to read\r\n");
            return XST_FAILURE;
        }
        bytes_done = rc;
        
        rc = write_from_buffer("VideoOut.hex", hex_fd, buffer,
            bytes_done, out_offset);
        if (rc < 0)
        {
            printf("Failed to write to file \r\n");
            return XST_FAILURE;
        }
        out_offset += bytes_done;
    }
    
    XV_tpg_DisableAutoRestart(&tpgInst);

    free(allocated);
    close(hex_fd);

    return XST_SUCCESS;
}

int main()
{
    uint32_t status;

    srand((unsigned int)time(NULL));

    fd = open("/dev/xdma0_user", O_RDWR);
    if(fd < 0)
    {
        printf("Failed to open /dev/xdma0_user!\r\n");
        printf("Check if Driver is installed. \r\n");
        return XST_FAILURE;
    }

    fd_read = open("/dev/xdma0_c2h_0", O_RDWR | O_TRUNC);
    if(fd < 0)
    {
        printf("Failed to open /dev/xdma0_c2h_0!\r\n");
        printf("Check if Driver is installed. \r\n");
        return XST_FAILURE;
    }

    status = XGpio_Initialize(&gpioInst[4], XPAR_AXI_GPIO_4_BASEADDR);
    if (status != XST_SUCCESS) 
    {
        printf("GPIO Read failed to Init!\r\n");
        return XST_FAILURE;
    }

    printf("\n\n\r     ** NiteFury RaspberryPi **\n\r");
    printf("   FPGA Build/Version: 0x%08X\n\n\r", XGpio_DiscreteRead(&gpioInst[4], 0x01));

    tpg();

    for (size_t i = 0; i < 4; i++)
    {
        status = XGpio_Initialize(&gpioInst[i], testarr[i]);
        if (status != XST_SUCCESS) 
        {
            printf("GPIO %ld failed to Init!\r\n", i);
            return XST_FAILURE;
        }
    }

    XGpio_DiscreteWrite(&gpioInst[0], 0x01, rand()%2);
    XGpio_DiscreteWrite(&gpioInst[1], 0x01, rand()%2);
    XGpio_DiscreteWrite(&gpioInst[2], 0x01, rand()%2);
    XGpio_DiscreteWrite(&gpioInst[3], 0x01, rand()%2);

    BramPtr = XBram_LookupConfig(XPAR_AXI_BRAM_CTRL_0_BASEADDR);
    if(BramPtr == NULL)
    {
        printf("Bram failed to get config! \r\n");
        return XST_FAILURE;
    }

    status = XBram_CfgInitialize(&bramInst, BramPtr, BramPtr->BaseAddress);
    if (status != XST_SUCCESS) 
    {
        printf("Bram failed to Init! \r\n");
        return XST_FAILURE;
    }
    
    status = XTmrCtr_Initialize(&tmrInst, XPAR_AXI_TIMER_0_BASEADDR);
    if (status != XST_SUCCESS) 
    {
        printf("Timer failed to Init! \r\n");
        return XST_FAILURE;
    }

   	XTmrCtr_SetResetValue(&tmrInst, 0, (u32) -50000000);
    XTmrCtr_SetResetValue(&tmrInst, 1, (u32) -37500000);

    ConfigPtr = XSysMon_LookupConfig(XPAR_XADC_WIZ_0_BASEADDR);
    if(ConfigPtr == NULL)
    {
        printf("Failed to get XADC Config! \r\n");
        return XST_FAILURE;
    }

    status = XSysMon_CfgInitialize(&sysmonInst, ConfigPtr, ConfigPtr->BaseAddress);
    if(status != XST_SUCCESS)
    {
        printf("Failed to init XADC! \r\n");
        return XST_FAILURE;
    }

    XTmrCtr_Start(&tmrInst, 0);
    XTmrCtr_Start(&tmrInst, 1);

    int x = 0x00;

    while(x != 0x05)
    {   
        xadcInst.TempRawData = XSysMon_GetAdcData(&sysmonInst, XSM_CH_TEMP);
        xadcInst.VCCINTRawData = XSysMon_GetAdcData(&sysmonInst, XSM_CH_VCCINT);
        xadcInst.VCCAUXRawData = XSysMon_GetAdcData(&sysmonInst, XSM_CH_VCCAUX);
        xadcInst.VCCBRAMRawData = XSysMon_GetAdcData(&sysmonInst, XSM_CH_VBRAM);

        xadcInst.TempData = XSysMon_RawToTemperature(xadcInst.TempRawData);
        xadcInst.VCCINTData = XSysMon_RawToVoltage(xadcInst.VCCINTRawData);
        xadcInst.VCCAUXData = XSysMon_RawToVoltage(xadcInst.VCCAUXRawData);
        xadcInst.VCCBRAMData = XSysMon_RawToVoltage(xadcInst.VCCBRAMRawData);
        
        if(XTmrCtr_IsExpired(&tmrInst, 1))
        {
            XTmrCtr_Reset(&tmrInst, 1);
            XGpio_DiscreteWrite(&gpioInst[1], 0x01, !(XGpio_DiscreteRead(&gpioInst[1], 0x01)));
            XGpio_DiscreteWrite(&gpioInst[2], 0x01, !(XGpio_DiscreteRead(&gpioInst[2], 0x01)));
        }

        if(XTmrCtr_IsExpired(&tmrInst, 0))
        {
            XTmrCtr_Reset(&tmrInst, 0);
            XGpio_DiscreteWrite(&gpioInst[0], 0x01, !(XGpio_DiscreteRead(&gpioInst[0], 0x01)));
            XGpio_DiscreteWrite(&gpioInst[3], 0x01, !(XGpio_DiscreteRead(&gpioInst[3], 0x01)));

            printf("\r\nTemperature: %0d.%03d C \r\n", (int)xadcInst.TempData, SysMonFractionToInt(xadcInst.TempData));
            printf("VCCINT: %0d.%03d V \r\n", (int)xadcInst.VCCINTData, SysMonFractionToInt(xadcInst.VCCINTData));
            printf("VCCAUX: %0d.%03d V \r\n", (int)xadcInst.VCCAUXData, SysMonFractionToInt(xadcInst.VCCAUXData));
            printf("VCCBRAM: %0d.%03d V \r\n", (int)xadcInst.VCCBRAMData, SysMonFractionToInt(xadcInst.VCCBRAMData));
            x = x + 1;
        }
    }

    printf("Everything done! \r\n Exiting... \r\n");

    close(fd);
    close(fd_read);
    close(hex_fd);



    return 0;
}

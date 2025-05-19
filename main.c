#include "main.h"
#include "xparameters.h"

#define ON 0x00
#define OFF 0x01

#define FRAME_WIDTH   1920
#define FRAME_HEIGHT  1080
#define BYTES_PER_PIXEL 3  // RGB888
#define FRAME_SIZE (FRAME_WIDTH * FRAME_HEIGHT * BYTES_PER_PIXEL)

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

void write_hex(FILE *out, uint8_t *buffer, ssize_t len) {
    for (ssize_t i = 0; i < len; i++) {
        fprintf(out, "%02x", buffer[i]);
    }
    fprintf(out, "\n");
}

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

    XGpio_WriteReg(XPAR_XGPIO_5_BASEADDR, 0x01, 0x01);

    printf("TPG Init Success\r\n");

	XV_tpg_Set_height(&tpgInst, FRAME_HEIGHT);
	XV_tpg_Set_width(&tpgInst, FRAME_WIDTH);
	XV_tpg_Set_colorFormat(&tpgInst, XVIDC_CSF_RGB);
	XV_tpg_Set_maskId(&tpgInst, 0);
	XV_tpg_Set_motionSpeed(&tpgInst, 0);
    XV_tpg_Set_motionEn(&tpgInst, 0);
	XV_tpg_Set_bckgndId(&tpgInst, XTPG_BKGND_SOLID_RED);

    printf("Finished Config\r\n");
    
    XV_tpg_DisableAutoRestart(&tpgInst);
    XV_tpg_Start(&tpgInst);

    printf("TPG Started\r\n");

    //printf("Data: %x \r\n", XV_tpg_ReadReg(XPAR_V_TPG_0_BASEADDR, XV_TPG_CTRL_ADDR_AP_CTRL));

    
    printf("Resetting TPG! \r\n");
    XGpio_WriteReg(XPAR_XGPIO_5_BASEADDR, 0x01, 0x00);
    sleep(2);
    XGpio_WriteReg(XPAR_XGPIO_5_BASEADDR, 0x01, 0x01);
    printf("Done resetting TPG\r\n");

    //while(!XV_tpg_IsIdle(&tpgInst));

    return XST_SUCCESS;
}

int streaming()
{
    uint8_t* frame_buffer;
    ssize_t bytes_read;
    
    frame_buffer = malloc(FRAME_SIZE);
    if(!frame_buffer)
    {
        printf("Malloc Failed! \r\n");
        return XST_FAILURE;
    }

    fd_read = open("/dev/xdma0_c2h_0", O_RDONLY);
    if(fd_read < 0)
    {
        printf("Opening C2H Failed! \r\n");
        free(frame_buffer);
        return XST_FAILURE;
    }

    bytes_read = pread(fd_read, frame_buffer, FRAME_SIZE, 0x00);
    if(bytes_read < 0)
    {
        printf("Reading Failed! \r\n");
        free(frame_buffer);
        close(fd_read);
        return XST_FAILURE;
    }

    FILE *fout = fopen("VideoOut.rgb", "wb");
    if(!fout)
    {
        printf("Cant open file! \r\n");
        close(fd_read);
        free(frame_buffer);
        return XST_FAILURE;
    }

    fwrite(frame_buffer, 1, bytes_read, fout);
    fclose(fout);

    close(fd_read);
    free(frame_buffer);

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

    streaming();

    while(x != 10000)
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

    XV_tpg_DisableAutoRestart(&tpgInst);

    close(fd);

    return 0;
}

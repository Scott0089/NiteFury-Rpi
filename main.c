#include "main.h"
#include "xparameters.h"

#define ON 0x00
#define OFF 0x01

#define READ_SIZE 4096           // Read 4 KB chunks
#define TOTAL_BYTES 32768        // Total bytes to read


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

    printf("TPG Init Success\r\n");

	XV_tpg_Set_height(&tpgInst, 1080);
	XV_tpg_Set_width(&tpgInst, 1920);
	XV_tpg_Set_colorFormat(&tpgInst, XVIDC_CSF_RGB);
	XV_tpg_Set_maskId(&tpgInst, 0);
	XV_tpg_Set_motionSpeed(&tpgInst, 5);
	XV_tpg_Set_bckgndId(&tpgInst, XTPG_BKGND_COLOR_BARS);
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

    while(1){}

    return XST_SUCCESS;
}

int streaming()
{
    int dev_fd = open("/dev/xdma0_c2h_0", O_RDONLY);
    if (dev_fd < 0) {
        perror("Error opening XDMA device");
        return EXIT_FAILURE;
    }

    FILE *hex_out = fopen("VideoOut.hex", "w");
    if (!hex_out) {
        perror("Error opening output file");
        close(dev_fd);
        return EXIT_FAILURE;
    }

    uint8_t buffer[READ_SIZE];
    ssize_t bytes_read = 0;
    ssize_t total_read = 0;

    while (total_read < TOTAL_BYTES) {
        ssize_t to_read = (TOTAL_BYTES - total_read > READ_SIZE) ? READ_SIZE : TOTAL_BYTES - total_read;
        bytes_read = read(dev_fd, buffer, to_read);

        if (bytes_read < 0) {
            perror("Error reading from XDMA");
            break;
        } else if (bytes_read == 0) {
            printf("End of stream or no data available.\n");
            break;
        }

        write_hex(hex_out, buffer, bytes_read);
        total_read += bytes_read;

        printf("Read %zd / %d bytes...\n", total_read, TOTAL_BYTES);
    }

    close(dev_fd);
    close(hex_out);

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
    /*
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
*/

    printf("Everything done! \r\n Exiting... \r\n");

    XV_tpg_DisableAutoRestart(&tpgInst);

    close(fd);

    return 0;
}

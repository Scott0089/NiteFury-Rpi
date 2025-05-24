#include "main.h"
#include "xparameters.h"

#define ON 0x00
#define OFF 0x01

#define FRAME_WIDTH 1920
#define FRAME_HEIGHT 1080
#define BYTES_PER_PIXEL 3 // RGB888
#define FRAME_SIZE (FRAME_WIDTH * FRAME_HEIGHT * BYTES_PER_PIXEL)

typedef struct
{
    uint32_t TempRawData;
    uint32_t VCCINTRawData;
    uint32_t VCCAUXRawData;
    uint32_t VCCBRAMRawData;
    float TempData;
    float VCCINTData;
    float VCCAUXData;
    float VCCBRAMData;
} XADC_Data;

XGpio gpioInst[5];
XTmrCtr tmrInst;
XSysMon sysmonInst;
XSysMon_Config *ConfigPtr;
XBram bramInst;
XBram_Config *BramPtr;

XV_tpg_Config *TpgPtr;
XV_tpg tpgInst;

XADC_Data xadcInst;

XGpio tpgresetInst;

uint64_t testarr[4] = {XPAR_AXI_GPIO_0_BASEADDR, XPAR_AXI_GPIO_1_BASEADDR, XPAR_AXI_GPIO_2_BASEADDR, XPAR_AXI_GPIO_3_BASEADDR};

static inline uint8_t convert_10bit_to_8bit(uint16_t val10) {
    return (uint8_t)((val10 * 255 + 511) / 1023);
}

int fd = -1;
int fd_read = -1;
int hex_fd = -1;

int kbhit() {
    struct termios oldterm, newterm;
    int ch;
    int oldf;
  
    tcgetattr(STDIN_FILENO, &oldterm);
    newterm = oldterm;
    newterm.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newterm);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
  
    ch = getchar();
  
    tcsetattr(STDIN_FILENO, TCSANOW, &oldterm);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
  
    if (ch != EOF) {
      ungetc(ch, stdin);
      return 1;
    }
  
    return 0;
  }

int SysMonFractionToInt(float FloatNum)
{
    float Temp;

    Temp = FloatNum;
    if (FloatNum < 0)
    {
        Temp = -(FloatNum);
    }

    return (((int)((Temp - (float)((int)Temp)) * (1000.0f))));
}

int tpg_reset()
{
    XGpio_DiscreteWrite(&tpgresetInst, 0x01, 0x00);
    usleep(300000);
    XGpio_DiscreteWrite(&tpgresetInst, 0x01, 0x01);

    return XST_SUCCESS;
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

    tpg_reset();

    // printf("TPG Init Success\r\n");

    XV_tpg_Set_height(&tpgInst, FRAME_HEIGHT);
    XV_tpg_Set_width(&tpgInst, FRAME_WIDTH);
    XV_tpg_Set_colorFormat(&tpgInst, XVIDC_CSF_RGB);
    //XV_tpg_Set_maskId(&tpgInst, 0);
    XV_tpg_Set_motionSpeed(&tpgInst, 50);
    XV_tpg_Set_motionEn(&tpgInst, 1);
    XV_tpg_Set_bckgndId(&tpgInst, XTPG_BKGND_COLOR_BARS);

    XV_tpg_Set_boxColorB(&tpgInst, 0xFF);
    XV_tpg_Set_boxColorR(&tpgInst, 0xFF);
    XV_tpg_Set_boxColorG(&tpgInst, 0xFF);
    XV_tpg_Set_boxSize(&tpgInst, 50);

    XV_tpg_Set_ovrlayId(&tpgInst, 0x01);
    

    // printf("Finished Config\r\n");

    XV_tpg_EnableAutoRestart(&tpgInst);
    XV_tpg_Start(&tpgInst);

    // printf("TPG Started\r\n");

    // printf("Data: %x \r\n", XV_tpg_ReadReg(XPAR_V_TPG_0_BASEADDR, XV_TPG_CTRL_ADDR_AP_CTRL));

    // printf("Resetting TPG! \r\n");
    // printf("Done resetting TPG\r\n");

    // while(!XV_tpg_IsIdle(&tpgInst));

    return XST_SUCCESS;
}

int streaming() {
    int fd_read;
    ssize_t bytes_read, total_read;
    size_t padded_width = ((FRAME_WIDTH + 7) / 8) * 8;
    size_t stride_2pixels = padded_width / 2; // number of 64-bit words per line
    size_t stride_bytes = stride_2pixels * 8; // 8 bytes per 2 pixels
    size_t frame_bytes = stride_bytes * FRAME_HEIGHT;

    uint8_t* input_buffer = malloc(frame_bytes);
    if (!input_buffer) {
        printf("Malloc failed for input_buffer\n");
        return XST_FAILURE;
    }

    uint8_t* output_buffer = malloc(FRAME_WIDTH * FRAME_HEIGHT * 3);
    if (!output_buffer) {
        printf("Malloc failed for output_buffer\n");
        free(input_buffer);
        return XST_FAILURE;
    }

    fd_read = open("/dev/xdma0_c2h_0", O_RDONLY);
    if (fd_read < 0) {
        printf("Failed to open DMA device\n");
        free(input_buffer);
        free(output_buffer);
        return XST_FAILURE;
    }

    total_read = 0;
    while (total_read < frame_bytes) {
        bytes_read = pread(fd_read, input_buffer + total_read, frame_bytes - total_read, total_read);
        if (bytes_read < 0) {
            printf("Failed to read from DMA\n");
            free(input_buffer);
            free(output_buffer);
            close(fd_read);
            return XST_FAILURE;
        }
        if (bytes_read == 0) break; // EOF unexpected here
        total_read += bytes_read;
    }

    if (total_read != frame_bytes) {
        printf("Incomplete frame read: expected %zu, got %zd\n", frame_bytes, total_read);
        free(input_buffer);
        free(output_buffer);
        close(fd_read);
        return XST_FAILURE;
    }

    // I used chatgpt for this. Dont kill me
    // Need to do this is hardware instead. Too CPU intensive
    // Unpack pixels
    // Treat input_buffer as array of 64-bit words (2 pixels each)
    uint64_t* pixels_64 = (uint64_t*)input_buffer;

    for (size_t row = 0; row < FRAME_HEIGHT; row++) {
        size_t out_col = 0;
        for (size_t col = 0; col < stride_2pixels; col++) {
            uint64_t word = pixels_64[row * stride_2pixels + col];

            // Extract first pixel (bits 0-29)
            uint32_t pix1 = (uint32_t)(word & 0x3FFFFFFF);
            uint16_t g10_1 = pix1 & 0x3FF;
            uint16_t b10_1 = (pix1 >> 10) & 0x3FF;
            uint16_t r10_1 = (pix1 >> 20) & 0x3FF;
            if (out_col < FRAME_WIDTH) {
                output_buffer[(row * FRAME_WIDTH + out_col) * 3 + 0] = convert_10bit_to_8bit(g10_1);
                output_buffer[(row * FRAME_WIDTH + out_col) * 3 + 1] = convert_10bit_to_8bit(b10_1);
                output_buffer[(row * FRAME_WIDTH + out_col) * 3 + 2] = convert_10bit_to_8bit(r10_1);
            }
            out_col++;

            // Extract second pixel (bits 30-59)
            uint32_t pix2 = (uint32_t)((word >> 30) & 0x3FFFFFFF);
            uint16_t g10_2 = pix2 & 0x3FF;
            uint16_t b10_2 = (pix2 >> 10) & 0x3FF;
            uint16_t r10_2 = (pix2 >> 20) & 0x3FF;
            if (out_col < FRAME_WIDTH) {
                output_buffer[(row * FRAME_WIDTH + out_col) * 3 + 0] = convert_10bit_to_8bit(g10_2);
                output_buffer[(row * FRAME_WIDTH + out_col) * 3 + 1] = convert_10bit_to_8bit(b10_2);
                output_buffer[(row * FRAME_WIDTH + out_col) * 3 + 2] = convert_10bit_to_8bit(r10_2);
            }
            out_col++;
        }
    }

    FILE* fout = fopen("frame.rgb", "a");
    if (!fout) {
        printf("Failed to open output file\n");
        free(input_buffer);
        free(output_buffer);
        close(fd_read);
        return XST_FAILURE;
    }

    fwrite(output_buffer, 1, FRAME_WIDTH * FRAME_HEIGHT * 3, fout);
    fclose(fout);

    free(input_buffer);
    free(output_buffer);
    close(fd_read);

    return XST_SUCCESS;
}

int main()
{
    uint32_t status;

    srand((unsigned int)time(NULL));

    fd = open("/dev/xdma0_user", O_RDWR);
    if (fd < 0)
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

    status = XGpio_Initialize(&tpgresetInst, XPAR_XGPIO_5_BASEADDR);
    if (status != XST_SUCCESS)
    {
        printf("Failed to Init TPG Reset GPIO! \r\n");
        close(fd);
        return XST_FAILURE;
    }

    XGpio_DiscreteWrite(&tpgresetInst, 0x01, 0x01);

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

    XGpio_DiscreteWrite(&gpioInst[0], 0x01, rand() % 2);
    XGpio_DiscreteWrite(&gpioInst[1], 0x01, rand() % 2);
    XGpio_DiscreteWrite(&gpioInst[2], 0x01, rand() % 2);
    XGpio_DiscreteWrite(&gpioInst[3], 0x01, rand() % 2);

    BramPtr = XBram_LookupConfig(XPAR_AXI_BRAM_CTRL_0_BASEADDR);
    if (BramPtr == NULL)
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

    XTmrCtr_SetResetValue(&tmrInst, 0, (u32)-50000000);
    XTmrCtr_SetResetValue(&tmrInst, 1, (u32)-37500000);

    ConfigPtr = XSysMon_LookupConfig(XPAR_XADC_WIZ_0_BASEADDR);
    if (ConfigPtr == NULL)
    {
        printf("Failed to get XADC Config! \r\n");
        return XST_FAILURE;
    }

    status = XSysMon_CfgInitialize(&sysmonInst, ConfigPtr, ConfigPtr->BaseAddress);
    if (status != XST_SUCCESS)
    {
        printf("Failed to init XADC! \r\n");
        return XST_FAILURE;
    }

    XTmrCtr_Start(&tmrInst, 0);
    XTmrCtr_Start(&tmrInst, 1);

    int x = 0x00;

    while (x != 1)
    {
        xadcInst.TempRawData = XSysMon_GetAdcData(&sysmonInst, XSM_CH_TEMP);
        xadcInst.VCCINTRawData = XSysMon_GetAdcData(&sysmonInst, XSM_CH_VCCINT);
        xadcInst.VCCAUXRawData = XSysMon_GetAdcData(&sysmonInst, XSM_CH_VCCAUX);
        xadcInst.VCCBRAMRawData = XSysMon_GetAdcData(&sysmonInst, XSM_CH_VBRAM);

        xadcInst.TempData = XSysMon_RawToTemperature(xadcInst.TempRawData);
        xadcInst.VCCINTData = XSysMon_RawToVoltage(xadcInst.VCCINTRawData);
        xadcInst.VCCAUXData = XSysMon_RawToVoltage(xadcInst.VCCAUXRawData);
        xadcInst.VCCBRAMData = XSysMon_RawToVoltage(xadcInst.VCCBRAMRawData);

        if (XTmrCtr_IsExpired(&tmrInst, 1))
        {
            XTmrCtr_Reset(&tmrInst, 1);
            XGpio_DiscreteWrite(&gpioInst[1], 0x01, !(XGpio_DiscreteRead(&gpioInst[1], 0x01)));
            XGpio_DiscreteWrite(&gpioInst[2], 0x01, !(XGpio_DiscreteRead(&gpioInst[2], 0x01)));
        }

        if (XTmrCtr_IsExpired(&tmrInst, 0))
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

    printf("\n\nDoing DMA stuff. \r\n\n ");

    while(!kbhit())
    {
        status = streaming();
        if (status != XST_SUCCESS)
        {
            printf("Failed to Capture Frame! \r\n");
            return XST_FAILURE;
        }

        xadcInst.TempRawData = XSysMon_GetAdcData(&sysmonInst, XSM_CH_TEMP);
        xadcInst.TempData = XSysMon_RawToTemperature(xadcInst.TempRawData);
        if (XTmrCtr_IsExpired(&tmrInst, 0))
        {
            XTmrCtr_Reset(&tmrInst, 0);

            printf("Temperature: %0d.%03d C \r", (int)xadcInst.TempData, SysMonFractionToInt(xadcInst.TempData));
        }
    }

    printf("\nEverything done! \r\n Exiting... \r\n");

    XV_tpg_DisableAutoRestart(&tpgInst);
    tpg_reset();

    remove("frame.rgb");

    close(fd);

    return 0;
}

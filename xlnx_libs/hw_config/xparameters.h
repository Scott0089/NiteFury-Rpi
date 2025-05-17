#ifndef XPARAMETERS_H   /* prevent circular inclusions */
#define XPARAMETERS_H   /* by using protection macros */

//Note this file is modified from the Microblaze to work on Linux with our custom build. 
//All configurations save for address are updated, all MCU based defines are removed

/******************************************************************/

// No longer need STDIN and STOUT, trace compile errors from here/
//#define STDIN_BASEADDRESS 0x40600000
//#define STDOUT_BASEADDRESS 0x40600000

/******************************************************************/

/* Platform specific definitions */
//#define PLATFORM_RISCV
 
/* Definitions for sleep timer configuration */
//#define XSLEEP_TIMER_IS_DEFAULT_TIMER

/******************************************************************/

/* Definitions for driver GPIO */
#define XPAR_XGPIO_NUM_INSTANCES 4

/* Definitions for peripheral GPIO_0 */
#define XPAR_GPIO_0_BASEADDR 0x40000000
#define XPAR_GPIO_0_HIGHADDR 0x40001FFF
#define XPAR_GPIO_0_DEVICE_ID 0
#define XPAR_GPIO_0_INTERRUPT_PRESENT 0
#define XPAR_GPIO_0_IS_DUAL 0

/* Definitions for peripheral GPIO_0 */
#define XPAR_GPIO_1_BASEADDR 0x40010000
#define XPAR_GPIO_1_HIGHADDR 0x40011FFF
#define XPAR_GPIO_1_DEVICE_ID 1
#define XPAR_GPIO_1_INTERRUPT_PRESENT 0
#define XPAR_GPIO_1_IS_DUAL 0

#define XPAR_GPIO_2_BASEADDR 0x40020000
#define XPAR_GPIO_2_HIGHADDR 0x40021FFF
#define XPAR_GPIO_2_DEVICE_ID 1
#define XPAR_GPIO_2_INTERRUPT_PRESENT 0
#define XPAR_GPIO_2_IS_DUAL 0

#define XPAR_GPIO_3_BASEADDR 0x40030000
#define XPAR_GPIO_3_HIGHADDR 0x40031FFF
#define XPAR_GPIO_3_DEVICE_ID 1
#define XPAR_GPIO_3_INTERRUPT_PRESENT 0
#define XPAR_GPIO_3_IS_DUAL 0

#endif  /* end of protection macro */

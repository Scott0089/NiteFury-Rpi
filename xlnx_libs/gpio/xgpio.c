/******************************************************************************
* Copyright (C) 2002 - 2021 Xilinx, Inc.  All rights reserved.
* Copyright (c) 2022 - 2024 Advanced Micro Devices, Inc. All Rights Reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

/*****************************************************************************/
/**
* @file xgpio.c
* @addtogroup gpio_api GPIO APIs
* @{
*
* This section explains the implementation of the basic functionality of the XGpio driver.
* This header file contains the software API definition of the Xilinx General Purpose I/O
* device driver.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who  Date     Changes
* ----- ---- -------- -----------------------------------------------
* 1.00a rmm  02/04/02 First release
* 2.00a jhl  12/16/02 Update for dual channel and interrupt support
* 2.01a jvb  12/13/05 Changed Initialize() into CfgInitialize(), and made
*                     CfgInitialize() take a pointer to a config structure
*                     instead of a device id. Moved Initialize() into
*                     xgpio_sinit.c, and had Initialize() call CfgInitialize()
*                     after it retrieved the config structure using the device
*                     id. Removed include of xparameters.h along with any
*                     dependencies on xparameters.h and the _g.c config table.
* 2.11a mta  03/21/07 Updated to new coding style, added GetDataDirection
* 2.12a sv   11/21/07 Updated driver to support access through DCR bus
* 3.00a sv   11/21/09 Updated to use HAL Processor APIs. Renamed the
*		      macros to remove _m from the name.
* 4.1   lks  11/18/15 Clean up of the comments in the code and
*		      removed support for DCR bridge
* 4.2   sk   08/16/16 Used UINTPTR instead of u32 for Baseaddress as part of
*                     adding 64 bit support. CR# 867425.
*                     Changed the prototype of XGpio_CfgInitialize API.
* 4.9   pm   11/30/21 Addressed Doxygen warning
* </pre>
*
*****************************************************************************/

/***************************** Include Files ********************************/

#include "xgpio.h"
#include "xstatus.h"

/************************** Constant Definitions ****************************/

/**************************** Type Definitions ******************************/

/***************** Macros (Inline Functions) Definitions ********************/

/************************** Variable Definitions ****************************/


/************************** Function Prototypes *****************************/


/****************************************************************************/
/**
* Initializes the XGpio instance provided by the caller based on the
* given configuration data.
*
* Nothing is done except to initialize the InstancePtr.
*
* @param	InstancePtr Pointer to an XGpio instance. The memory the
*		pointer references must be pre-allocated by the caller. Further
*		calls to manipulate the driver through the XGpio API must be
*		made with this pointer.
* @param	Config Reference to a structure containing information
*		about a specific GPIO device. This function initializes an
*		InstancePtr object for a specific device specified by the
*		contents of Config. This function can initialize multiple
*		instance objects with the use of multiple calls giving different
*		Config information on each call.
* @param 	EffectiveAddr Device base address in the virtual memory
*		address space. The caller is responsible for keeping the address
*		mapping from EffectiveAddr to the device physical base address
*		unchanged once this function is invoked. Unexpected errors may
*		occur if the address mapping changes after this function is
*		called. If address translation is not used, use
*		Config->BaseAddress for this parameters, passing the physical
*		address instead.
*
* @return
* 		- XST_SUCCESS if the initialization is successful.
*
* @note		None.
*
*****************************************************************************/
int XGpio_CfgInitialize(XGpio * InstancePtr, XGpio_Config * Config,
			UINTPTR EffectiveAddr)
{
	/* Assert arguments */
	Xil_AssertNonvoid(InstancePtr != NULL);

	/* Set some default values. */
	InstancePtr->BaseAddress = EffectiveAddr;

	InstancePtr->InterruptPresent = Config->InterruptPresent;
	InstancePtr->IsDual = Config->IsDual;

	/*
	 * Indicate the instance is now ready to use, initialized without error
	 */
	InstancePtr->IsReady = XIL_COMPONENT_IS_READY;
	return (XST_SUCCESS);
}


/****************************************************************************/
/**
* Sets the input/output direction of all discrete signals for the specified
* GPIO channel.
*
* @param	InstancePtr Pointer to an XGpio instance to be worked on.
* @param	Channel Contains the channel of the GPIO (1 or 2) to operate on.
* @param	DirectionMask Bitmask specifying which discretes are input
*		and which are output. Bits set to 0 are output and bits set to 1
*		are input.
*
* @return	None.
*
* @note		The hardware must be built for dual channels if this function
*		is used with any channel other than 1.  If it is not, this
*		function will assert.
*
*****************************************************************************/
void XGpio_SetDataDirection(XGpio *InstancePtr, unsigned Channel,
			    u32 DirectionMask)
{
	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);
	Xil_AssertVoid((Channel == 1) ||
		     ((Channel == 2) && (InstancePtr->IsDual == TRUE)));

	XGpio_WriteReg(InstancePtr->BaseAddress,
			((Channel - 1) * XGPIO_CHAN_OFFSET) + XGPIO_TRI_OFFSET,
			DirectionMask);
}

/****************************************************************************/
/**
* Gets the input/output direction of all discrete signals for the specified
* GPIO channel.
*
* @param	InstancePtr Pointer to an XGpio instance to be worked on.
* @param	Channel Contains the channel of the GPIO (1 or 2) to operate on.
*
* @return	Bitmask specifying which discretes are input and
*		which are output. Bits set to 0 are output and bits set to 1 are
*		input.
*
* @note
*
* The hardware must be built for dual channels if this function is used
* with any channel other than 1.  If it is not, this function will assert.
*
*****************************************************************************/
u32 XGpio_GetDataDirection(XGpio *InstancePtr, unsigned Channel)
{
	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);
	Xil_AssertNonvoid((Channel == 1)  ||
		((Channel == 2) &&
		(InstancePtr->IsDual == TRUE)));

	return XGpio_ReadReg(InstancePtr->BaseAddress,
		((Channel - 1) * XGPIO_CHAN_OFFSET) + XGPIO_TRI_OFFSET);
}

/****************************************************************************/
/**
* Reads state of discretes for the specified GPIO channel.
*
* @param	InstancePtr Pointer to an XGpio instance to be worked on.
* @param	Channel Contains the channel of the GPIO (1 or 2) to operate on.
*
* @return	Current copy of the discretes register.
*
* @note		The hardware must be built for dual channels if this function
*		is used with any channel other than 1. If it is not, this
*		function will assert.
*
*****************************************************************************/
u32 XGpio_DiscreteRead(XGpio * InstancePtr, unsigned Channel)
{
	Xil_AssertNonvoid(InstancePtr != NULL);
	Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);
	Xil_AssertNonvoid((Channel == 1) ||
			((Channel == 2) && (InstancePtr->IsDual == TRUE)));

	return XGpio_ReadReg(InstancePtr->BaseAddress,
			      ((Channel - 1) * XGPIO_CHAN_OFFSET) +
			      XGPIO_DATA_OFFSET);
}

/****************************************************************************/
/**
* Writes to discretes register for the specified GPIO channel.
*
* @param	InstancePtr Pointer to an XGpio instance to be worked on.
* @param	Channel Contains the channel of the GPIO (1 or 2) to operate on.
* @param	Mask Value to be written to the discretes register.
*
* @return	None.
*
* @note		The hardware must be built for dual channels if this function
*		is  used with any channel other than 1.  If it is not, this
*		function will assert. See also XGpio_DiscreteSet() and
*		XGpio_DiscreteClear().
*
*****************************************************************************/
void XGpio_DiscreteWrite(XGpio * InstancePtr, unsigned Channel, u32 Mask)
{
	Xil_AssertVoid(InstancePtr != NULL);
	Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);
	Xil_AssertVoid((Channel == 1) ||
		     ((Channel == 2) && (InstancePtr->IsDual == TRUE))); 
	
	XGpio_WriteReg(InstancePtr->BaseAddress,
			((Channel - 1) * XGPIO_CHAN_OFFSET) + XGPIO_DATA_OFFSET,
			Mask);
	


}
/** @} */
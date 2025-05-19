#include "tpg.h"

char user_tpg_names[][128] =
					  {
					  "Video Size",
					  "Pattern",
					  "Overlay",
					  "Mask",
					  "Speed",
					  "Boxsize",
					  "Box R",
					  "Box G",
					  "Box B",
					  "Back Motion"
					  };

char tpg_pattern_names[][128] =
		{
        "XTPG_PASS_THROUGH_NO_BCKGND",
		"XTPG_BKGND_H_RAMP",
		"XTPG_BKGND_V_RAMP",
		"XTPG_BKGND_TEMPORAL_RAMP",
		"XTPG_BKGND_SOLID_RED",
		"XTPG_BKGND_SOLID_GREEN",
		"XTPG_BKGND_SOLID_BLUE",
		"XTPG_BKGND_SOLID_BLACK",
		"XTPG_BKGND_SOLID_WHITE",
		"XTPG_BKGND_COLOR_BARS",
		"XTPG_BKGND_ZONE_PLATE",
		"XTPG_BKGND_TARTAN_COLOR_BARS",
		"XTPG_BKGND_CROSS_HATCH",
		"XTPG_BKGND_RAINBOW_COLOR",
		"XTPG_BKGND_HV_RAMP",
		"XTPG_BKGND_CHECKER_BOARD",
		"XTPG_BKGND_PBRS",
		"XTPG_BKGND_DP_COLOR_RAMP",
		"XTPG_BKGND_DP_BW_VERTICAL_LINE",
		"XTPG_BKGND_DP_COLOR_SQUARE",
		"TPG_BKGND_LAST"
		};

/**
 * Setup the TPG
 *
 * @param tpg_inst Pointer to the TPG Inst
 *
 * @return XST_SUCCESS or XST_FAILURE
 *
 * @note This function must be called before tpg_start()
 */
int tpg_init(tpg_setup_t* tpg_inst, int lane_idx) {
	if(tpg_inst == NULL) {
		xil_printf("The TPG Inst must not be null!\r\n");
		return XST_FAILURE;
	}

    if ( (tpg_inst->tpg_config = XV_tpg_LookupConfig(lane_idx)) == NULL) {
		xil_printf("XV_tpg_LookupConfig() failed\r\n");
		return XST_FAILURE;
	}
    if (XV_tpg_CfgInitialize(tpg_inst->tpg, tpg_inst->tpg_config, tpg_inst->tpg_config->BaseAddress) != XST_SUCCESS) {
		xil_printf("XV_tpg_CfgInitialize() failed\r\n");
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/**
 * @brief Set default values for the TPG setup structure
 *
 * @param tpg_inst The TPG setup structure to set defaults for
 * @param tpg The XV_tpg instance to set as the default
 * @param tpg_config The XV_tpg_Config instance to set as the default
 *
 * This function sets the tpg_inst structure to the default values for the TPG.
 *
 * @note This function should be called before calling tpg_start()
 */
void tpg_struct_set_defaults(tpg_setup_t* tpg_inst, XV_tpg *tpg, XV_tpg_Config *tpg_config)
{
	tpg_inst->tpg = tpg;
	tpg_inst->tpg_config = tpg_config;
	tpg_inst->tpg_height  = 1080;
	tpg_inst->tpg_width   = 1920;
	tpg_inst->tpg_pattern = 0x8;		//Solid white (looking to check for pixel offsets in hex dump
	tpg_inst->tpg_overlay = 1;
	tpg_inst->tpg_mask    = 0;
	tpg_inst->tpg_speed   = 5;
	tpg_inst->tpg_boxsize = 50;
	tpg_inst->tpg_box_r   = 0xFF;
	tpg_inst->tpg_box_g   = 0xFF;
	tpg_inst->tpg_box_b   = 0xFF;
	tpg_inst->tpg_back_motion = 0x0;
	tpg_inst->tpg_enabled = 0;
	tpg_inst->tpg_size_change_flag = 0;
}

void tpg_struct_set(tpg_setup_t* tpg_inst, u8 bckgnd_id, XVidC_VideoMode video_mode)
{
	const XVidC_VideoTiming *timing = XVidC_GetTimingInfo(video_mode);
	//Configure the initialized stuct 
	tpg_inst->tpg_pattern 	= bckgnd_id;
	tpg_inst->tpg_height 	= timing->VActive;
	tpg_inst->tpg_width		= timing->HActive;

	xil_printf("------- TPG STRUCT MODIFIED -------\r\n");
	xil_printf("- VIDEO MODE: %s\r\n", XVidC_GetVideoModeStr(video_mode));
	xil_printf("- HEIGHT: %d\r\n", timing->VActive);
	xil_printf("- WIDTH: %d\r\n", timing->HActive);
	xil_printf("- PATTERN: %s\r\n", tpg_pattern_names[tpg_inst->tpg_pattern]);
	xil_printf("-------------------------------------\r\n");
}

void tpg_setup(tpg_setup_t* tpg_inst) 
{
	XV_tpg_Set_height(tpg_inst->tpg, tpg_inst->tpg_height);
	XV_tpg_Set_width(tpg_inst->tpg, tpg_inst->tpg_width);
	//XV_tpg_Set_colorFormat(tpg_inst->tpg, XVIDC_CSF_YCRCB_444);		//This is just always set in our test
	//XV_tpg_Set_colorFormat(tpg_inst->tpg, XVIDC_CSF_RGB);		//This is just always set in our test
	XV_tpg_Set_colorFormat(tpg_inst->tpg, XVIDC_CSF_YCRCB_422);		//Note, this is actually YUV422 from TPG generator. Not sure if makes difference
	XV_tpg_Set_maskId(tpg_inst->tpg, tpg_inst->tpg_mask);
	XV_tpg_Set_motionSpeed(tpg_inst->tpg, tpg_inst->tpg_speed);
	XV_tpg_Set_bckgndId(tpg_inst->tpg, tpg_inst->tpg_pattern);
	XV_tpg_Set_boxSize(tpg_inst->tpg, tpg_inst->tpg_boxsize);
	XV_tpg_Set_boxColorR(tpg_inst->tpg, tpg_inst->tpg_box_r);
	XV_tpg_Set_boxColorG(tpg_inst->tpg, tpg_inst->tpg_box_g);
	XV_tpg_Set_boxColorB(tpg_inst->tpg, tpg_inst->tpg_box_b);
	XV_tpg_Set_ovrlayId(tpg_inst->tpg, tpg_inst->tpg_overlay);

	xil_printf("INFO>> TPG Setup Complete\r\n");
}

int tpg_start(tpg_setup_t* tpg_inst) {
	XV_tpg_EnableAutoRestart(tpg_inst->tpg);
	XV_tpg_Start(tpg_inst->tpg);
	//Same as above
	//XV_tpg_WriteReg(tpg_Config->BaseAddress, XV_TPG_CTRL_ADDR_AP_CTRL, 0x81);

	xil_printf("Test Pattern Updated.\r\n");
	tpg_inst->tpg_enabled = 0x1;

	return XST_SUCCESS;
}

/***
* Stops TPG when we want to display data out of the device via DisplayPort 
* from the exFat on NVMe 
*/ 
int tpg_stop(tpg_setup_t* tpg_inst) {
	if(tpg_inst == NULL) {
		xil_printf("The TPG Inst must not be null!\r\n");
		return XST_FAILURE;
	}

	if(tpg_inst->tpg == NULL) {
		xil_printf("The TPG has to be initialized in tpg_inst.\r\n");
		return XST_FAILURE;
	}

	if(tpg_inst->tpg_enabled == 0x1) {
		tpg_inst->tpg_enabled = 0x0;
	}

    XV_tpg_DisableAutoRestart(tpg_inst->tpg); //Review code, writes 0 to the AP_CTRL register disables the core?
	//XV_tpg_WriteReg(tpg_Config->BaseAddress, XV_TPG_CTRL_ADDR_AP_CTRL, 0x0)	//This is the code for the above
	return XST_SUCCESS;
}

void tpg_print(tpg_setup_t* tpg_inst)
{
	xil_printf("============== TPG SETTINGS ===============\r\n");
	xil_printf("%s: %dx%d\r\n", user_tpg_names[0], tpg_inst->tpg_width, tpg_inst->tpg_height);
	if(tpg_inst->tpg_pattern < 0 || tpg_inst->tpg_pattern >=20) {
		xil_printf("%s: INVALID VALUE (%d)\r\n", user_tpg_names[1], tpg_inst->tpg_pattern);
	}
	else {
		xil_printf("%s: %s (%d)\r\n", user_tpg_names[1], tpg_pattern_names[tpg_inst->tpg_pattern], tpg_inst->tpg_pattern);
	}
	xil_printf("%s: %d\r\n", user_tpg_names[2], tpg_inst->tpg_overlay);
	xil_printf("%s: %d\r\n", user_tpg_names[3], tpg_inst->tpg_mask);
	xil_printf("%s: %d\r\n", user_tpg_names[4], tpg_inst->tpg_speed);
	xil_printf("%s: %d\r\n", user_tpg_names[5], tpg_inst->tpg_boxsize);
	xil_printf("%s: %d\r\n", user_tpg_names[6], tpg_inst->tpg_box_r);
	xil_printf("%s: %d\r\n", user_tpg_names[7], tpg_inst->tpg_box_g);
	xil_printf("%s: %d\r\n", user_tpg_names[8], tpg_inst->tpg_box_b);
	xil_printf("%s: %d\r\n", user_tpg_names[9], tpg_inst->tpg_back_motion);
	xil_printf("===========================================\r\n");
}

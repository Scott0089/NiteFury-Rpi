#ifndef TPG_H_
#define TPG_H_

#include "xparameters.h"
#include "xv_tpg.h"
#include "xvidc.h"

typedef struct tpg_struct
{
	XV_tpg 	      	*tpg;
	XV_tpg_Config 	*tpg_config;
	uint32_t		tpg_height;
	uint32_t		tpg_width;
	uint32_t		tpg_pattern;
	uint32_t		tpg_overlay;
	uint32_t		tpg_mask;
	uint32_t		tpg_speed;
	uint32_t		tpg_boxsize;
	uint32_t		tpg_box_r;
	uint32_t		tpg_box_g;
	uint32_t		tpg_box_b;
	uint32_t		tpg_back_motion;
	uint32_t		tpg_enabled;
	uint32_t 		tpg_size_change_flag;
} tpg_setup_t;

typedef enum
{
	N_TPG_VIDEO_SIZE = 0,
	N_TPG_PATTERN,
	N_TPG_OVERLAY,
	N_TPG_MASK,
	N_TPG_SPEED,
	N_TPG_BOXSIZE,
	N_TPG_BOX_R,
	N_TPG_BOX_G,
	N_TPG_BOX_B,
	N_TPG_BACK_MOTION
} tpg_enumType;

//These will get defined for every time this header is included in c file regardless of circular include protection
//Use extern, define in our associated c file, and these will be grabbed regardless of what c file they are mentioned,
//If we want to private these to our printout, can remove these and only have them defined in tpg.c
extern char user_tpg_names[][128];
extern char tpg_pattern_names[][128];

//Initial setup, calls tpg_update_settings
int  tpg_init(tpg_setup_t* tpg_inst, int lane_idx);
void tpg_setup(tpg_setup_t* tgp_inst);
void tpg_struct_set(tpg_setup_t* tpg_inst, u8 bckgnd_id, XVidC_VideoMode video_mode);
void tpg_struct_set_defaults(tpg_setup_t* tpg_inst, XV_tpg *tpg, XV_tpg_Config *tpg_config);
int  tpg_start(tpg_setup_t* tgp_inst);
//Stops everything
int tpg_stop(tpg_setup_t* tpg_inst);
//Print out settings for sanity
void tpg_print(tpg_setup_t* tpg_inst);

#endif // TPG_H_H circular include guard

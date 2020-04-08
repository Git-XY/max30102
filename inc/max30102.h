/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:   add max30102 function declaration
 * Date           Author       Notes
 * 2020-03-26     xiaoyuan    the first version
 */
 
#ifndef __MAX30102_H__
#define __MAX30102_H__

#include <rthw.h>
#include <rtdevice.h>
#include "max30102_reg.h"

#define MAX30102_REG_SET_MULTBIT(byte,flags)   (byte)|= (flags)
#define MAX30102_REG_CLR_MULTBIT(byte,flags)   (byte)&=~(flags)
#define MAX30102_BYTE_MASK(byte,mask,funbits)  ((byte)&=(mask)),((byte)|=(funbits))
 enum max30102_funset_choose
 {
	 FUN_DISABLE = 0,
	 FUN_ENABLE 
 };

typedef struct max30102_default_setting
 {
    enum max30102_fifo_config smpave;
	enum max30102_mode_config ledmode;
	enum max30102_spo2_config adcscale;
	enum max30102_spo2_config samplerate;
	enum max30102_spo2_config pulsewidth;
	enum max30102_led_pulse_amplitude_config powerlevel;
 }sensor_default_setting;
 
 #define RECORD_SIZE 4
typedef struct record_data
{
	rt_uint32_t red_value[RECORD_SIZE];
	rt_uint32_t ir_value[RECORD_SIZE];
	rt_uint8_t head;
	rt_uint8_t tail;

}sensor_srtuct;


/**************************************************************************************************************************
*                                                Interrupt Register Funcation Statement
***************************************************************************************************************************/
rt_err_t max30102_read_intrrupt(rt_uint8_t intrnum,rt_uint8_t *outdata);
rt_err_t max30102_set_intrrupt(rt_uint8_t intrnum,rt_uint8_t flags,enum max30102_funset_choose choose);
/**************************************************************************************************************************
*                                                FIFO Register Funcation Statement
***************************************************************************************************************************/
rt_err_t max30102_set_fifo_smpave(enum max30102_fifo_config mode);
rt_err_t max30102_set_fifo_rollover(enum max30102_funset_choose choose);
rt_err_t max30102_set_fifo_afull(enum max30102_fifo_config mode);
rt_err_t max30102_read_fifo_pointer(rt_uint8_t reg,rt_uint8_t *outdata);
rt_err_t max30102_read_fifo_allclear(void);
/**************************************************************************************************************************
*                                                Mode Register Funcation Statement
***************************************************************************************************************************/
rt_err_t max30102_mode_soft_reset(void);
rt_err_t max30102_mode_shutdown(void);
rt_err_t max30102_mode_wakeup(void);
rt_err_t max30102_set_mode_ledmode(enum max30102_mode_config mode);
/**************************************************************************************************************************
*                                                Sp02 Register Funcation Statement
***************************************************************************************************************************/
rt_err_t max30102_set_spo2_adcscale(enum max30102_spo2_config mode);
rt_err_t max30102_set_spo2_samplerate(enum max30102_spo2_config mode);
rt_err_t max30102_set_spo2_pulsewidth(enum max30102_spo2_config mode);
/**************************************************************************************************************************
*                                      Led Pulse Amplitude Register Funcation Statement
***************************************************************************************************************************/
rt_err_t max30102_set_led_pa_red(rt_uint8_t data);
rt_err_t max30102_set_led_pa_ir(rt_uint8_t data);
rt_err_t max30102_set_led_pa_proximity (rt_uint8_t data);
rt_err_t max30102_set_proximity_threshold (rt_uint8_t data);
/**************************************************************************************************************************
*                                       Multied Led Mode Register Funcation Statement
***************************************************************************************************************************/
rt_err_t max30102_set_slot_enable(rt_uint8_t slot,rt_uint8_t device);
rt_err_t max30102_set_slot_all_disable(void);
/**************************************************************************************************************************
*                                        Die Temperature Function Statement
***************************************************************************************************************************/
 rt_err_t max30102_read_temperature(float *temp);
 rt_err_t max30102_temp_to_fahrenheit(float *ftemp);
 /**************************************************************************************************************************
*                                              max30102 Setup Function Statement
***************************************************************************************************************************/
rt_err_t max30102_setup(
						enum max30102_fifo_config smpave,
						enum max30102_mode_config ledmode,
						enum max30102_spo2_config adcscale,
						enum max30102_spo2_config samplerate,
						enum max30102_spo2_config pulsewidth,
						enum max30102_led_pulse_amplitude_config powerlevel
                        );
/**************************************************************************************************************************
*                                              max30102 Data Collection Function Statement
***************************************************************************************************************************/
rt_err_t max30102_read_id(rt_uint8_t reg,rt_uint8_t *outid);
rt_uint8_t max30102_get_activeleds(void);	
rt_err_t max30102_read_fifo(rt_uint32_t *red_value,rt_uint32_t *ir_value);
rt_err_t max30102_read_fifo(rt_uint32_t *red_value,rt_uint32_t *ir_value);
rt_size_t max30102_get_data(rt_uint32_t *red_value,rt_uint32_t *ir_value,rt_uint32_t len);	
rt_uint32_t max30102_get_per_sample(rt_uint8_t choose);						
#endif 

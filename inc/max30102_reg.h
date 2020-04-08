/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:   add max30102 register
 * Date           Author       Notes
 * 2020-03-26     xiaoyuan    the first version
 */

#ifndef __MAX30102_REG_H__
#define __MAX30102_REG_H__

/* MAX30102 7-bit I2C Address */
#define MAX30102_IIC_ADDRESS                0x57

/* MAX30102 IIC write and read address */
#define MAX30102_WRITE_ADDR					0xAE
#define MAX30102_READ_ADDR					0xAF

/* MAX30102 status register address */
#define MAX30102_REG_INTR_STATUS_1			0x00      //only B[7:4]&&B0 can be read
#define MAX30102_REG_INTR_STATUS_2			0x01      //only B1 can be read
#define MAX30102_REG_INTR_ENABLE_1			0x02      //B[7:4] can be read and writed   
#define MAX30102_REG_INTR_ENABLE_2			0x03      //B1 can be read and writed  

/* MAX30102 FIFO register address */
#define MAX30102_REG_FIFO_WR_PTR			0x04      //B[4:0] can be read and writed   
#define MAX30102_REG_OVF_COUNTER			0x05      //B[4:0] can be read and writed   
#define MAX30102_REG_FIFO_RD_PTR			0x06      //B[4:0] can be read and writed   
#define MAX30102_REG_FIFO_DATA				0x07      //B[7:0] can be read and writed   

/* MAX30102 configuration register address */
#define MAX30102_REG_FIFO_CONFIG			0x08      //B[7:5]is smp_ave[2:0];B4 is FIFO_ROLL_OVER_EN;B[3:0]is FIFO_A_FULL[3:0];R/W
#define MAX30102_REG_MODE_CONFIG			0x09      //B7 is SHDN;B6 is RESET;B[2:0]is MODE[2:0];R/W
#define MAX30102_REG_SPO2_CONFIG			0x0A      //B[6:5]is SPO2_ADC_RG[1:0];B[4:2]is SPO2_SR[2:0];B[1:0]is LED_PW[1:0];R/W
#define MAX30102_REG_LED1_PA				0x0C      //B[7:0]is LED1_PA[7:0];R/W
#define MAX30102_REG_LED2_PA				0x0D      //B[7:0]is LED2_PA[7:0];R/W
#define MAX30102_REG_PILOT_PA				0x10      //B[7:0]is PILOT_PA[7:0];R/W
#define MAX30102_REG_MULTI_LED_CTRL1		0x11      //B[6:4]is SLOT2[2:0];B[2:0]is SLOT1[2:0];R/W
#define MAX30102_REG_MULTI_LED_CTRL2		0x12      //B[6:4]is SLOT4[2:0];B[2:0]is SLOT3[2:0];R/W

/* MAX30102 die temperature register address */
#define MAX30102_REG_TEMP_INTR				0x1F      //B[7:0]is TINT[7:0];R
#define MAX30102_REG_TEMP_FRAC				0x20      //B[3:0]is TFRAC[3:0];R
#define MAX30102_REG_TEMP_CONFIG			0x21      //B0 is TEMP_EN;R

/* MAX30102 proximity function register address */
#define MAX30102_REG_PROX_INT_THRESH		0x30      //B[7:0]is PROX_INT_THRESH[7:0];R/W

/* MAX30102 part id register address */
#define MAX30102_REG_REV_ID					0xFE      //B[7:0]is REV_ID[7:0];R
#define MAX30102_REG_PART_ID				0xFF      //B[7:0]is PART_ID[7:0];R

/* MAX30102 part id  value */
#define MAX30102_PART_ID_VALUE				0x15

/* max30102 interrupt enable 1 operation bits */
enum max30102_intr_enable_1
{
    PROX_INT = 1<<4,
    ALC_OVF  = 1<<5,
    PPG_RDY  = 1<<6,
    A_FULL   = 1<<7
};

/* max30102 interrupt enable 2 operation bits */
enum max30102_intr_enable_2
{
    DIE_TEMP_RDY  = 1<<1
};

/* max30102 fifo configuration list */
enum max30102_fifo_config
{
    SMP_AVE_MASK		= 0x1F,//(unsigned char)~(0b11100000),
    SMP_AVE_1			= 0x00,
    SMP_AVE_2			= 0x20,
    SMP_AVE_4			= 0x40,
    SMP_AVE_8			= 0x60,
    SMP_AVE_16			= 0x80,
    SMP_AVE_32			= 0xA0,
    FIFO_ROL_LOVER		= 1<<4,
    FIFO_A_FULL_MASK	= 0xF0,//(unsigned char)~(0b00001111),
    FIFO_A_FULL_32		= 0x00,
    FIFO_A_FULL_31,
    FIFO_A_FULL_30,
    FIFO_A_FULL_29,
    FIFO_A_FULL_28,
    FIFO_A_FULL_27,
    FIFO_A_FULL_26,
    FIFO_A_FULL_25,
    FIFO_A_FULL_24,
    FIFO_A_FULL_23,
    FIFO_A_FULL_22,
    FIFO_A_FULL_21,
    FIFO_A_FULL_20,
    FIFO_A_FULL_19,
    FIFO_A_FULL_18,
    FIFO_A_FULL_17
};

/* max30102 mode configuration list*/
enum max30102_mode_config
{
    RESET_MODE			= 1<<6,
    SHDN_MODE			= 1<<7,
    MODE_MASK			= 0xF8,//(unsigned char)~(0b00000111),
    HEARAT_RATE_MODE	= 0x02,
    SPO2_MODE			= 0x03,
    MULTI_LED_MODE		= 0x07
};

/* max30102 spo2 configuration list*/
enum max30102_spo2_config
{
    ADC_FULL_SCALE_MASK		= 0x9F,//(unsigned char)~(0b01100000),
    ADC_FULL_SCALE_2048		= 0x00,
    ADC_FULL_SCALE_4096		= 0x20,
    ADC_FULL_SCALE_8192		= 0x40,
    ADC_FULL_SCALE_16384	= 0x60,
    SAMPLE_RATE_MASK		= 0xE3,//(unsigned char)~(0b00011100),
    SAMPLE_RATE_50			= 0x00,
    SAMPLE_RATE_100			= 0x04,
    SAMPLE_RATE_200			= 0x08,
    SAMPLE_RATE_400			= 0x0C,
    SAMPLE_RATE_800			= 0x10,
    SAMPLE_RATE_1000		= 0x14,
    SAMPLE_RATE_1600		= 0x18,
    SAMPLE_RATE_3200		= 0x1C,
    PULSE_WIDTH_MASK		= 0xFC,//(unsigned char)~(0b00000011),
    PULSE_WIDTH_69			= 0x00,
    PULSE_WIDTH_118			= 0x01,
    PULSE_WIDTH_251			= 0x02,
    PULSE_WIDTH_411			= 0x03
};

/* max30102 led current common configuration list
*  The current value range is 0~50ma
*  The current value range is 0x00~0XFF
*  Note: This list only uses commonly used values
*The correspondence between all currents and registers
*can be found in the table "MAX30102 LED Current and Registers"
*/
enum max30102_led_pulse_amplitude_config
{
    LED_CURRENT_0_MA		= 0x00,
    LED_CURRENT_3_1_MA		= 0x0F,
    LED_CURRENT_6_4_MA		= 0x1F,
    LED_CURRENT_12_5_MA		= 0x3F,
    LED_CURRENT_25_4_MA		= 0x7F
};

/* max30102 multi led mode configuration list */
enum max30102_multi_led_mode_config
{
	SLOT1           = 1<<1,
	SLOT2           = 1<<2,
	SLOT3           = 1<<3,
	SLOT4           = 1<<4,
    SLOT1_MASK 		= 0xF8,//(unsigned char)~(0b00000111),
    SLOT2_MASK 		= 0x8F,//(unsigned char)~(0b01110000),
    SLOT3_MASK 		= 0xF8,//(unsigned char)~(0b00000111),
    SLOT4_MASK 		= 0x8F,//(unsigned char)~(0b01110000),
    SLOT_STOP		= 0x00,
    SLOT_RED_LED	= 0x01,
    SLOT_IR_LED		= 0x02,
    SLOT_RED_PILOT	= 0x05,
    SLOT_IR_PILOT	= 0x06
};

/* max30102 die temperature configuration  */
enum max30102_die_temp_config
{
	TEMP_EN = 1<<0
};

#endif

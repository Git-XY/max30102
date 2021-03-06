/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:   Add max30102 function implementation
 * Date           Author       Notes
 * 2020-03-26     xiaoyuan    the first version
 */
#include "max30102_reg.h"
#include "max30102.h"
#include <string.h>
//#define MAX30102_DEBUG
#ifdef  MAX30102_DEBUG
#define MAX30102DEBUG rt_kprintf
#endif
/* I2C device name must be the same as the I2C device name registered in drv_i2c.c*/
#define MAX30102_I2CBUS_NAME  "i2c1"
static struct rt_i2c_bus_device *max30102_i2c_bus;   /* I2C device handle */

sensor_srtuct max30102;
static const sensor_default_setting ops_default =
{
    .smpave     = SMP_AVE_4,
    .ledmode    = SPO2_MODE,
    .adcscale   = ADC_FULL_SCALE_4096,
    .samplerate = SAMPLE_RATE_200,
    .pulsewidth = PULSE_WIDTH_411,
    .powerlevel = LED_CURRENT_12_5_MA
};

/**
 * This function writes the value for the register of max30102
 *
 * @param reg the register for max30102
 * @param data value to write
 *
 * @return the writing status, RT_EOK reprensents  writing the value of the register successfully.
 */
static rt_err_t max30102_write_reg(rt_uint8_t reg, rt_uint8_t data)
{
    rt_uint8_t buf[2],ret;
    buf[0] = reg;
    buf[1] = data;
    ret = rt_i2c_master_send(
              max30102_i2c_bus,
              MAX30102_IIC_ADDRESS,
              RT_I2C_WR,
              buf,sizeof(buf)
          );
    if(ret == sizeof(buf))//Function returns the number of bytes sent
        return RT_EOK;
    else
        return -RT_ERROR;
}

/**
 * This function reads the value of registers for max30102
 *
 * @param reg the register for max30102
 * @param len number of bytes to read
 * @param buf read data pointer
 *
 * @return the reading status, RT_EOK reprensents reading the value of registers successfully.
 */
static rt_err_t max30102_read_reg(rt_uint8_t reg,rt_uint8_t len,rt_uint8_t *buf)
{
    rt_uint8_t ret;
    ret = rt_i2c_master_send(
              max30102_i2c_bus,
              MAX30102_IIC_ADDRESS,
              RT_I2C_WR,
              &reg,1
          );
    if(ret == 1) //Function returns the number of bytes sent
    {
        ret = rt_i2c_master_recv(
                  max30102_i2c_bus,
                  MAX30102_IIC_ADDRESS,
                  RT_I2C_RD,
                  buf,
                  len
              );
        if(ret == len) //Function returns the number of bytes receive
            return RT_EOK;
        else
            return -RT_ERROR;
    }
    else
        return -RT_ERROR;
}

/**
 * This function request the value of registers for max30102
 *
 * @param reg the register for max30102
 *
 * @return the request status, RT_EOK reprensents request  registers successfully.
 */
static rt_err_t max30102_request_reg(rt_uint8_t reg)
{
    rt_uint8_t ret;
    ret = rt_i2c_master_send(
              max30102_i2c_bus,
              MAX30102_IIC_ADDRESS,
              RT_I2C_WR,
              &reg,1
          );
    if(ret == 1)
        return RT_EOK;
    else
        return -RT_ERROR;
}

/**
 * This function receive the value of registers for max30102
 *
 * @param len number of bytes to read
 * @param buf read data pointer
 *
 * @return the receive status, RT_EOK reprensents receive the value of registers successfully.
 */
static rt_err_t max30102_receive_reg(rt_uint8_t len,rt_uint8_t *buf)
{
    rt_uint8_t ret;
    ret = rt_i2c_master_recv(
              max30102_i2c_bus,
              MAX30102_IIC_ADDRESS,
              RT_I2C_RD,
              buf,
              len
          );
    if(ret == len)
        return RT_EOK;
    else
        return -RT_ERROR;
}

static rt_err_t max30102_set_funbit(rt_uint8_t reg,rt_uint8_t funbit,enum max30102_funset_choose choose)
{
    rt_uint8_t data;
    if(max30102_read_reg(reg,1,&data) == RT_EOK)
    {
        if(choose == FUN_ENABLE)
            MAX30102_REG_SET_MULTBIT(data,funbit);
        else if(choose == FUN_DISABLE)
            MAX30102_REG_CLR_MULTBIT(data,funbit);
        return max30102_write_reg(reg,data);
    }
    else
        return -RT_ERROR;
}

static rt_err_t max30102_funbits_mask(rt_uint8_t reg,rt_uint8_t mask,rt_uint8_t funbits)
{
    rt_uint8_t byte;
    if(max30102_read_reg(reg,1,&byte) == RT_EOK)
    {
        MAX30102_BYTE_MASK(byte,mask,funbits);
        return max30102_write_reg(reg,byte);
    }
    else
        return -RT_ERROR;
}

/**************************************************************************************************************************
*                                                Interrupt Register Funcation Fonfiguration
***************************************************************************************************************************/
rt_err_t max30102_read_intrrupt(rt_uint8_t intrnum,rt_uint8_t *outdata)
{
    if(intrnum == 1)
        return max30102_read_reg(MAX30102_REG_INTR_STATUS_1,1,outdata);
    else if(intrnum == 2)
        return max30102_read_reg(MAX30102_REG_INTR_STATUS_2,1,outdata);
    else return RT_ERROR;
}

rt_err_t max30102_set_intrrupt(rt_uint8_t intrnum,rt_uint8_t flags,enum max30102_funset_choose choose)
{
    if(intrnum == 1)
        return max30102_set_funbit(MAX30102_REG_INTR_ENABLE_1,flags,choose);
    else if(intrnum == 2)
        return max30102_set_funbit(MAX30102_REG_INTR_ENABLE_2,flags,choose);
    else return RT_ERROR;
}

/**************************************************************************************************************************
*                                                FIFO Register Funcation Configuration
***************************************************************************************************************************/
rt_err_t max30102_set_fifo_smpave(enum max30102_fifo_config mode)
{
    return max30102_funbits_mask(MAX30102_REG_FIFO_CONFIG,SMP_AVE_MASK,mode);
}

rt_err_t max30102_set_fifo_rollover(enum max30102_funset_choose choose)
{
    return max30102_set_funbit(MAX30102_REG_FIFO_CONFIG,FIFO_ROL_LOVER,choose);
}

rt_err_t max30102_set_fifo_afull(enum max30102_fifo_config mode)
{
    return max30102_funbits_mask(MAX30102_REG_FIFO_CONFIG,FIFO_A_FULL_MASK,mode);
}

rt_err_t max30102_read_fifo_pointer(rt_uint8_t reg,rt_uint8_t *outdata)
{
    return max30102_read_reg(reg,1,outdata);
}

rt_err_t max30102_read_fifo_allclear(void)
{
    if(max30102_write_reg(MAX30102_REG_FIFO_WR_PTR,0)!= RT_EOK)
        return -RT_ERROR;
    if(max30102_write_reg(MAX30102_REG_OVF_COUNTER,0)!= RT_EOK)
        return -RT_ERROR;
    if(max30102_write_reg(MAX30102_REG_FIFO_RD_PTR,0)!= RT_EOK)
        return -RT_ERROR;
    return RT_EOK;
}

/**************************************************************************************************************************
*                                                Mode Register Funcation Configuration
***************************************************************************************************************************/
rt_err_t max30102_mode_soft_reset(void)
{
    if(max30102_set_funbit(MAX30102_REG_MODE_CONFIG,RESET_MODE,FUN_ENABLE) == RT_EOK)
    {
        rt_uint8_t ret;
        rt_tick_t start_time = rt_tick_get();
        while(rt_tick_get() - start_time < 100) // Timeout after 100ms
        {
            if(max30102_read_reg(MAX30102_REG_MODE_CONFIG,1,&ret)== RT_EOK)
            {
                if((ret & 0x40) == 0) break;
            }
            else
                return -RT_ERROR;
            rt_thread_mdelay(1); //Let's not over burden the I2C bus
        }
        return RT_EOK;
    }
    else
        return -RT_ERROR;
}

rt_err_t max30102_mode_shutdown(void)
{
    return max30102_set_funbit(MAX30102_REG_MODE_CONFIG,SHDN_MODE,FUN_ENABLE);
}

rt_err_t max30102_mode_wakeup(void)
{
    return max30102_set_funbit(MAX30102_REG_MODE_CONFIG,SHDN_MODE,FUN_DISABLE);
}

rt_err_t max30102_set_mode_ledmode(enum max30102_mode_config mode)
{
    return max30102_funbits_mask(MAX30102_REG_MODE_CONFIG,MODE_MASK,mode);
}

/**************************************************************************************************************************
*                                                Sp02 Register Funcation Configuration
***************************************************************************************************************************/
rt_err_t max30102_set_spo2_adcscale(enum max30102_spo2_config mode)
{
    return max30102_funbits_mask(MAX30102_REG_SPO2_CONFIG,ADC_FULL_SCALE_MASK,mode);
}

rt_err_t max30102_set_spo2_samplerate(enum max30102_spo2_config mode)
{
    return max30102_funbits_mask(MAX30102_REG_SPO2_CONFIG,SAMPLE_RATE_MASK,mode);
}

rt_err_t max30102_set_spo2_pulsewidth(enum max30102_spo2_config mode)
{
    return max30102_funbits_mask(MAX30102_REG_SPO2_CONFIG,PULSE_WIDTH_MASK,mode);
}

/**************************************************************************************************************************
*                                             Led Pulse Amplitude Register Funcation Configuration
***************************************************************************************************************************/

rt_err_t max30102_set_led_pa_red(rt_uint8_t data)
{
    return max30102_write_reg(MAX30102_REG_LED1_PA,data);
}

rt_err_t max30102_set_led_pa_ir(rt_uint8_t data)
{
    return max30102_write_reg(MAX30102_REG_LED2_PA,data);
}

rt_err_t max30102_set_led_pa_proximity (rt_uint8_t data)
{
    return max30102_write_reg(MAX30102_REG_PILOT_PA,data);
}

rt_err_t max30102_set_proximity_threshold (rt_uint8_t data)
{
    return max30102_write_reg(MAX30102_REG_PROX_INT_THRESH,data);
}

/**************************************************************************************************************************
*                                             Multied Led Mode Register Funcation Configuration
***************************************************************************************************************************/

rt_err_t max30102_set_slot_enable(rt_uint8_t slot,rt_uint8_t device)
{
    rt_uint8_t ret;
    switch(slot)
    {
    case SLOT1:
        ret = max30102_funbits_mask(MAX30102_REG_MULTI_LED_CTRL1,SLOT1_MASK,device);
        break;
    case SLOT2:
        ret = max30102_funbits_mask(MAX30102_REG_MULTI_LED_CTRL1,SLOT2_MASK,device<<4);
        break;
    case SLOT3:
        ret = max30102_funbits_mask(MAX30102_REG_MULTI_LED_CTRL2,SLOT3_MASK,device);
        break;
    case SLOT4:
        ret = max30102_funbits_mask(MAX30102_REG_MULTI_LED_CTRL2,SLOT4_MASK,device<<4);
        break;
    default:
        /*Shouldn't be here!*/
        break;
    }
    return ret;
}

rt_err_t max30102_set_slot_all_disable(void)
{
    if(max30102_write_reg(MAX30102_REG_MULTI_LED_CTRL1,SLOT_STOP)!= RT_EOK)
        return -RT_ERROR;
    if(max30102_write_reg(MAX30102_REG_MULTI_LED_CTRL2,SLOT_STOP)!= RT_EOK)
        return -RT_ERROR;
    return RT_EOK;
}

/**************************************************************************************************************************
*                                              Die Temperature Function Configuration
***************************************************************************************************************************/
rt_err_t max30102_read_temperature(float *temp)
{
    //First, you need to turn on the interrupt 2 DIE_TEMP_RDY flag, and then call this function
    if(max30102_set_funbit(MAX30102_REG_TEMP_CONFIG,TEMP_EN,FUN_ENABLE)!= RT_EOK)
        return -RT_ERROR;
    rt_uint8_t ret;
    rt_tick_t start_time = rt_tick_get();
    while ( rt_tick_get() - start_time < 100)
    {
        if(max30102_read_intrrupt(2,&ret)== RT_EOK)
        {
            if((ret & 0x02) > 0) break;
        }
        else
            return -RT_ERROR;
        rt_thread_mdelay(1); //Let's not over burden the I2C bus
    }
    if(rt_tick_get() - start_time >= 100) return -RT_ETIMEOUT;
    rt_uint8_t tempint,tempfrac;
    if(max30102_read_reg(MAX30102_REG_TEMP_INTR,1,&tempint)!= RT_EOK)
        return -RT_ERROR;
    if(max30102_read_reg(MAX30102_REG_TEMP_FRAC,1,&tempfrac)!= RT_EOK)
        return -RT_ERROR;
    *temp = (float)tempint + ((float)tempfrac * 0.0625f);
    return RT_EOK;
}

rt_err_t max30102_temp_to_fahrenheit(float *ftemp)
{
    float temp;
    if(max30102_read_temperature(&temp) != RT_EOK)
        return -RT_ERROR;

    *ftemp = temp * 1.8f + 32.0f;
    return RT_EOK;
}
/**************************************************************************************************************************
*                                              max30102 Setup Function
***************************************************************************************************************************/
rt_err_t max30102_setup(
    enum max30102_fifo_config smpave,
    enum max30102_mode_config ledmode,
    enum max30102_spo2_config adcscale,
    enum max30102_spo2_config samplerate,
    enum max30102_spo2_config pulsewidth,
    enum max30102_led_pulse_amplitude_config powerlevel
)
{
    /* After power on, reset max30102 first */
    if(max30102_mode_soft_reset()!= RT_EOK)
        return -RT_ERROR;
    /* Then set the interrupt1 */
    if(max30102_set_intrrupt(1,PPG_RDY|ALC_OVF,FUN_ENABLE) != RT_EOK)
        return -RT_ERROR;
    /* Then set the fifo */
    if(max30102_read_fifo_allclear()!= RT_EOK)
        return -RT_ERROR;
    if(max30102_set_fifo_smpave(smpave)!= RT_EOK)
        return -RT_ERROR;
    //Page 17 of the data sheet, if enabled, it should be used with read and write pointers
    //Because it defaults to zero after reset, it does not need to be set unless this bit is used.
//	if(max30102_set_fifo_rollover(FUN_ENABLE)!= RT_EOK)
//		return -RT_ERROR;
//    //If the A_FULL interrupt flag is not enabled, you do not need to set A_FULL
//	if(max30102_set_fifo_afull(FIFO_A_FULL_17)!= RT_EOK)
//		return -RT_ERROR;

    if(max30102_set_mode_ledmode(ledmode)!= RT_EOK)
        return -RT_ERROR;

    if(max30102_set_spo2_adcscale(adcscale)!= RT_EOK)
        return -RT_ERROR;
    if(max30102_set_spo2_samplerate(samplerate)!= RT_EOK)
        return -RT_ERROR;
    if(max30102_set_spo2_pulsewidth(pulsewidth)!= RT_EOK)
        return -RT_ERROR;

    if(max30102_set_led_pa_red(powerlevel)!= RT_EOK)
        return -RT_ERROR;
    if(max30102_set_led_pa_ir(powerlevel)!= RT_EOK)
        return -RT_ERROR;

//On page 9 of the data sheet, set the proximity mode IR current and IR ADC threshold,
//and you need to turn on the PROX_INT interrupt.
//	if(max30102_set_led_pa_proximity(powerlevel)!= RT_EOK)
//		return -RT_ERROR;
//	if(max30102_set_proximity_threshold(0x4f)!= RT_EOK)
//		return -RT_ERROR;

    if(ledmode == MULTI_LED_MODE)
    {
        if(max30102_set_slot_enable(SLOT1,SLOT_RED_LED)!= RT_EOK)
            return -RT_ERROR;
        if(max30102_set_slot_enable(SLOT2,SLOT_IR_LED)!= RT_EOK)
            return -RT_ERROR;
        if(max30102_set_slot_enable(SLOT3,SLOT_RED_LED)!= RT_EOK)
            return -RT_ERROR;
        if(max30102_set_slot_enable(SLOT4,SLOT_IR_LED)!= RT_EOK)
            return -RT_ERROR;
    }
    if(max30102_mode_shutdown() != RT_EOK)
        return -RT_ERROR;
    return RT_EOK;
}

/**************************************************************************************************************************
*                                              max30102 Data Collection function
***************************************************************************************************************************/
rt_err_t max30102_read_id(rt_uint8_t reg,rt_uint8_t *outid)
{
    return max30102_read_reg(reg,1,outid);
}
rt_uint8_t max30102_get_activeleds(void)
{
    if(ops_default.ledmode == HEARAT_RATE_MODE)
        return 1;
    if((ops_default.ledmode == SPO2_MODE)||(ops_default.ledmode == MULTI_LED_MODE))
        return 2;
    else
        return 0;
}

rt_err_t max30102_read_fifo(rt_uint32_t *red_value,rt_uint32_t *ir_value)
{
    rt_uint8_t buf[max30102_get_activeleds()*3];
    rt_uint32_t temp;
    *red_value = 0;
    *ir_value  = 0;
    if(max30102_request_reg(MAX30102_REG_FIFO_DATA) != RT_EOK)
        return -RT_ERROR;

    if(max30102_receive_reg(max30102_get_activeleds()*3,buf) != RT_EOK)
        return -RT_ERROR;
    temp = buf[0];
    temp <<= 16;
    *red_value+=temp;
    temp = buf[1];
    temp <<= 8;
    *red_value+=temp;
    temp = buf[2];
    *red_value+=temp;
    *red_value&=0x03FFFF;  //Mask MSB [23:18]

    if(max30102_get_activeleds() == 2)
    {
        temp = buf[3];
        temp <<= 16;
        *ir_value+=temp;
        temp = buf[4];
        temp <<= 8;
        *ir_value+=temp;
        temp = buf[5];
        *ir_value+=temp;
        *ir_value &=0x03FFFF;  //Mask MSB [23:18]
    }
    return RT_EOK;
}

rt_size_t max30102_get_data(rt_uint32_t *red_value,rt_uint32_t *ir_value,rt_uint32_t len)
{
    rt_uint8_t ret   = 0;
    rt_size_t sample = 0;
//	rt_uint8_t write_pointer = 0,read_pointer = 0;
//	if(max30102_read_fifo_pointer(MAX30102_REG_FIFO_WR_PTR,&write_pointer) != RT_EOK)
//		 return 0; 
//	if(max30102_read_fifo_pointer(MAX30102_REG_FIFO_RD_PTR,&read_pointer)  != RT_EOK)
//		return 0; 
	
    while(len > 0)
    {
        if(max30102_read_intrrupt(1,&ret) != RT_EOK)
            return 0;

//		if((ret & A_FULL))
//			rt_kprintf("FIFO A FULL!\r\n");

        if((ret & ALC_OVF) != 1)
        {
            if((ret & PPG_RDY))
            {
                if(max30102_read_fifo(red_value,ir_value) != RT_EOK)
                    return 0;
#ifdef MAX30102_DEBUG
//                MAX30102DEBUG("red_value:%d ir_value:%d ret:%x\n",*red_value,*ir_value,ret);
#endif
                len--;
                red_value++;
                ir_value++;
                sample++;
            }
        }
        else
            return 0;// ALC_OVF overflow Data Sheet Page 12
        rt_thread_mdelay(1);//Release i2c resources
    }
    return sample;
}

rt_uint32_t max30102_get_per_sample(rt_uint8_t choose)
{
    rt_uint32_t red_value,ir_value;
    if(max30102_get_data(&red_value,&ir_value,1) != 1)
        return 0;
    else
    {
        if(choose == 1)
            return red_value;
        else
            return ir_value;
    }
}
/**************************************************************************************************************************
*                                              max30102  init  function
***************************************************************************************************************************/

int max30102_hw_init()
{
    rt_uint8_t ret,id;

    max30102_i2c_bus = rt_i2c_bus_device_find(MAX30102_I2CBUS_NAME);

    if (max30102_i2c_bus == RT_NULL)
    {
        rt_kprintf("can't find %s device\r\n",MAX30102_I2CBUS_NAME);
        return -RT_ERROR;
    }

    if((max30102_read_id(MAX30102_REG_PART_ID,&id) != RT_EOK)||(id != MAX30102_PART_ID_VALUE))
    {
        rt_kprintf("can't find max30102 device\r\n");
        return -RT_ERROR;
    }
#ifdef MAX30102_DEBUG
    MAX30102DEBUG("max30102 id is %x\r\n", id);
#endif
    rt_kprintf("max30102 set i2c bus to %s\r\n", MAX30102_I2CBUS_NAME);

    ret = max30102_setup(
              ops_default.smpave,
              ops_default.ledmode,
              ops_default.adcscale,
              ops_default.samplerate,
              ops_default.pulsewidth,
              ops_default.powerlevel
          );
    if(ret != RT_EOK)
    {
        rt_kprintf("max30102 setup fail\r\n");
        return RT_ERROR;
    }
#ifdef MAX30102_DEBUG
    else
        MAX30102DEBUG("max30102 setup success\r\n");
#endif

    return RT_EOK;
}

INIT_DEVICE_EXPORT(max30102_hw_init);
//MSH_CMD_EXPORT(max30102_mode_shutdown,max30102 shutdown);
//MSH_CMD_EXPORT(max30102_mode_wakeup,max30102 wakeup);

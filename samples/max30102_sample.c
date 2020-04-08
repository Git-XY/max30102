#include "max30102.h"
#include "max30102_algorithm.h"
#include "algorithm_by_RF.h"
#include <stdio.h>
int max30102_sample()
{
    const rt_uint8_t RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
    rt_uint8_t rates[RATE_SIZE];     //Array of heart rates
    rt_uint8_t rateSpot = 0;
    rt_tick_t lastBeat  = 0;
    float  beatsPerMinute;
    int beatAvg = 0;
//	rt_uint32_t red_value,ir_value;
//	rt_uint8_t write_pointer = 0,read_pointer = 0,ret = 0;
    if(max30102_mode_wakeup() != RT_EOK)
        return -RT_ERROR;
    rt_tick_t time =  rt_tick_get();
    while(1)
    {
//		max30102_read_fifo_pointer(MAX30102_REG_FIFO_WR_PTR,&write_pointer);
//		max30102_read_fifo_pointer(MAX30102_REG_FIFO_RD_PTR,&read_pointer);
//		rt_kprintf("write_pointer:%d read_pointer:%d\r\n",write_pointer,read_pointer);
//		if(max30102_read_intrrupt(1,&ret) != RT_EOK)
//            return 0;
		
        if(checkForBeat(max30102_get_per_sample(2)) == RT_TRUE)
        {
            rt_tick_t delta = rt_tick_get()- lastBeat;
            lastBeat  = rt_tick_get();
            beatsPerMinute = 60 / (delta / 1000.0);
            if (beatsPerMinute < 255 && beatsPerMinute > 20)
            {
                rates[rateSpot++] = (rt_uint8_t)beatsPerMinute;
                rateSpot %= RATE_SIZE;
                beatAvg = 0;
                for (rt_uint8_t x = 0 ; x < RATE_SIZE ; x++)
                    beatAvg += rates[x];
                beatAvg /= RATE_SIZE;
            }
            printf("BPM=%.1f,Avg BPM=%d\r\n",beatsPerMinute,beatAvg);

        }
		rt_thread_delay(10);
        if(((rt_tick_get() - time)/1000)>= 30)
            break;
    }
    if(max30102_mode_shutdown() != RT_EOK)
        return -RT_ERROR;
    return RT_EOK;
}

int max30102_spo2(void)
{
    uint32_t aun_ir_buffer[100]; 	 //IR LED sensor data
    uint32_t aun_red_buffer[100];    //Red LED sensor data
    float n_sp02,ratio,correl;       //SPO2 value
    int8_t ch_spo2_valid;   		 //indicator to show if the SP02 calculation is valid
    int32_t n_heart_rate;    		 //heart rate value
    int8_t  ch_hr_valid;             //indicator to show if the heart rate calculation is valid
    if(max30102_mode_wakeup() != RT_EOK)
        return -RT_ERROR;
	max30102_get_data(aun_red_buffer,aun_ir_buffer,10);//clear 
    rt_tick_t time =  rt_tick_get();
    while(1)
    {
        max30102_get_data(aun_red_buffer,aun_ir_buffer,100);
        rf_heart_rate_and_oxygen_saturation(aun_ir_buffer, 100, aun_red_buffer, &n_sp02, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid,&ratio,&correl);
        printf("HR=%i HRvalid=%i SpO2=%f SPO2Valid=%i ratio=%f correl=%f\r\n", n_heart_rate,ch_hr_valid,n_sp02,ch_spo2_valid,ratio,correl);
        rt_thread_delay(10);
        if(((rt_tick_get() - time)/1000)>= 16)
            break;
    }
    if(max30102_mode_shutdown() != RT_EOK)
        return -RT_ERROR;
    return RT_EOK;
}

MSH_CMD_EXPORT(max30102_sample, max30102 sample);
MSH_CMD_EXPORT(max30102_spo2, max30102 spo2 );

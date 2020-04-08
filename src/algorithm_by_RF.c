/*
 * Created by Robert Fraczkiewicz, 12/2017
 * New signal processing methodology for obtaining heart rate and SpO2 data
 * from the MAX30102 sensor manufactured by MAXIM Integrated Products, Inc.
 */
/*******************************************************************************
* Copyright (C) 2017 Robert Fraczkiewicz, All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL ROBERT FRACZKIEWICZ BE LIABLE FOR ANY CLAIM, DAMAGES
* OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*
* The mere transfer of this software does not imply any licenses
* of trade secrets, proprietary technology, copyrights, patents,
* trademarks, maskwork rights, or any other form of intellectual
* property whatsoever. Robert Fraczkiewicz retains all
* ownership rights.
*******************************************************************************
*/
#include "algorithm_by_RF.h"
#include <math.h>
/*
*可设置的参数
*如果您的电路和硬件设置符合默认值，请不要理会这些
*在此代码的说明中描述。通常，不同的采样率
*和/或样本长度将需要调整这些参数。
*/
#define ST 2      //采样时间（以s为单位）。警告：如果更改ST，则必须重新计算下面的sum_X2参数！
#define FS 50    //采样频率（单位：Hz）。警告：如果更改FS，则必须重新计算下面的sum_X2参数！
//从-mean_X（见下文）到+ mean_X的ST * FS数字的平方和加1。例如，假设ST = 4和FS = 25，
//总和由100个项组成：（-49.5）^ 2 +（-48.5）^ 2 +（-47.5）^ 2 + ... +（47.5）^ 2 +（48.5）^ 2 +（49.5）^ 2
//总和是symmetrc，因此您可以通过将其正半数乘以2来对其求值。
//性能。
static const float sum_X2 = 83325; //警告：如果您更改了上面的ST或FS，则必须重新计算该和！
#define MAX_HR 125  //最大心率。为了消除错误信号，计算出的HR绝不能大于该数字。
#define MIN_HR 40  //最低心率。为了消除错误信号，计算出的HR绝不能低于该数字。
//典型的心率。将其设置为给定应用程序中预期心率范围的上限值。显然，它一定是
//在MIN_HR和MAX_HR之间。例如，如果隔夜测量的HR在46到65之间变化，但有90％的时间
//保持在50到60之间，然后将其设置为60。
//警告：这是一个CRUCIAL参数！正确的人力资源评估取决于此。
#define TYPICAL_HR 65
//两个自相关序列元素的最小比率：一个滞后与一个滞后0。
//高质量信号的比率必须大于此最小值。
static const float min_autocorrelation_ratio = 0.5f;
//红色和IR信号之间的Pearson相关性。
//高质量信号的相关系数必须大于此最小值。
static const float min_pearson_correlation   = 0.8f;

/*
*衍生参数
*请勿触摸这些！
 *
 */
static const int32_t BUFFER_SIZE    = FS*ST; //单个批次中的样本数量
static const int32_t FS60           = FS*60; //从bps到bpm的心率转换因子
static const int32_t LOWEST_PERIOD  = FS60/MAX_HR; //峰之间的最小距离
static const int32_t HIGHEST_PERIOD = FS60/MIN_HR; //峰之间的最大距离
static const int32_t INIT_INTERVAL  = FS60/TYPICAL_HR; //用于确定心率的例程的种子值
static const float mean_X           = (float)(BUFFER_SIZE-1)/2.0f; //从0到BUFFER_SIZE-1的整数集的平均值。对于ST = 4和FS = 25，等于49.5。



void rf_heart_rate_and_oxygen_saturation(uint32_t *pun_ir_buffer, int32_t n_ir_buffer_length, uint32_t *pun_red_buffer, float *pn_spo2, int8_t *pch_spo2_valid,
        int32_t *pn_heart_rate, int8_t *pch_hr_valid, float *ratio, float *correl)
/**
*计算心率和SpO2水平，Robert Fraczkiewicz版本
* 细节
*通过检测PPG周期的峰值以及红色/红外信号的相应AC / DC，可以计算出SPO2的xy_ratio。
*
* \param[in]    *pun_ir_buffer           - IR sensor data buffer
* \param[in]    n_ir_buffer_length       - IR sensor data buffer length
* \param[in]    *pun_red_buffer          - Red sensor data buffer
* \param[out]    *pn_spo2                - Calculated SpO2 value
* \param[out]    *pch_spo2_valid         - 1 if the calculated SpO2 value is valid
* \param[out]    *pn_heart_rate          - Calculated heart rate value
* \param[out]    *pch_hr_valid           - 1 if the calculated heart rate value is valid
*
* \retval       None
*/
{
    int32_t k;
    static int32_t n_last_peak_interval=INIT_INTERVAL;
    float f_ir_mean,f_red_mean,f_ir_sumsq,f_red_sumsq;
    float f_y_ac, f_x_ac, xy_ratio;
    float beta_ir, beta_red, x;
    float an_x[BUFFER_SIZE], *ptr_x; //ir
    float an_y[BUFFER_SIZE], *ptr_y; //red

    // 计算DC均值并从ir和红色中减去DC
    f_ir_mean  = 0.0f;
    f_red_mean = 0.0f;
    for (k = 0; k < n_ir_buffer_length; ++k) 
	{
        f_ir_mean  += pun_ir_buffer[k];
        f_red_mean += pun_red_buffer[k];
    }
    f_ir_mean  = f_ir_mean  / n_ir_buffer_length ;
    f_red_mean = f_red_mean / n_ir_buffer_length ;

    // 移除直流
    for (k = 0,ptr_x = an_x,ptr_y = an_y; k < n_ir_buffer_length; ++k,++ptr_x,++ptr_y)
	{
        *ptr_x = pun_ir_buffer[k]  - f_ir_mean;
        *ptr_y = pun_red_buffer[k] - f_red_mean;
    }

    //RF，消除线性趋势（基线水平）
    beta_ir  = rf_linear_regression_beta(an_x, mean_X, sum_X2);
    beta_red = rf_linear_regression_beta(an_y, mean_X, sum_X2);
    for(k = 0,x = -mean_X,ptr_x = an_x,ptr_y = an_y; k < n_ir_buffer_length; ++k,++x,++ptr_x,++ptr_y) 
	{
        *ptr_x -= beta_ir*x;
        *ptr_y -= beta_red*x;
    }

    // 对于SpO2，请计算两个交流信号的RMS。另外，脉冲检测器需要IR的原始平方和
    f_y_ac=rf_rms(an_y,n_ir_buffer_length,&f_red_sumsq);
    f_x_ac=rf_rms(an_x,n_ir_buffer_length,&f_ir_sumsq);

    // 计算红色和红外之间的皮尔逊相关性
    *correl = rf_Pcorrelation(an_x, an_y, n_ir_buffer_length) / sqrt(f_red_sumsq*f_ir_sumsq);
    if(*correl >= min_pearson_correlation) 
	{
        // RF，如果相关性良好，则求出IR信号的平均周期。如果为非周期性，则返回周期性为0
        rf_signal_periodicity(an_x, BUFFER_SIZE, &n_last_peak_interval, LOWEST_PERIOD, HIGHEST_PERIOD, min_autocorrelation_ratio, f_ir_sumsq, ratio);
    } 
	else
		n_last_peak_interval = 0;
	
    if(n_last_peak_interval!= 0) 
	{
        *pn_heart_rate = (int32_t)(FS60 / n_last_peak_interval);
        *pch_hr_valid  = 1;
    } 
	else 
	{
        n_last_peak_interval = FS;
        *pn_heart_rate  	 = -999; //无法计算，因为信号看起来是非周期性的
        *pch_hr_valid        = 0;
        *pn_spo2             = -999; //不要从此损坏的信号中使用SPO2
        *pch_spo2_valid      = 0;
        return;
    }

    // 趋势消除后，平均值表示直流水平
    xy_ratio = (f_y_ac*f_ir_mean) / (f_x_ac*f_red_mean); //公式是（f_y_ac * f_x_dc）/（f_x_ac * f_y_dc）;
    if(xy_ratio > 0.02f && xy_ratio < 1.84f)           //检查适用范围
	{ 
        *pn_spo2        = (-45.060f*xy_ratio + 30.354f)*xy_ratio + 94.845f;
        *pch_spo2_valid = 1;
    }
	else 
	{
        *pn_spo2         = -999 ; //不要使用SPO2，因为信号an_ratio超出范围
        *pch_spo2_valid  = 0;
    }
}

float rf_linear_regression_beta(float *pn_x, float xmean, float sum_x2)
/**
* \brief        Coefficient beta of linear regression 线性回归系数β
* \par          Details
*计算pn_x对均值中心的线性回归的方向系数beta
*点索引值（0到BUFFER_SIZE-1）。 xmean必须等于（BUFFER_SIZE-1）/ 2！ sum_x2是均心指数值的平方和。
*
*               Robert Fraczkiewicz, 12/22/2017
* \retval       Beta
*/
{
    float x,beta,*pn_ptr;
    beta = 0.0f;
    for(x = -xmean,pn_ptr = pn_x; x <= xmean; ++x,++pn_ptr)
        beta += x*(*pn_ptr);
    return beta / sum_x2;
}

float rf_autocorrelation(float *pn_x, int32_t n_size, int32_t n_lag)
/**
* \brief        Autocorrelation function 自相关功能
* \par          Details
*               计算给定序列pn_x的自相关序列的n_lag元素
*               Robert Fraczkiewicz, 12/21/2017
* \retval       Autocorrelation sum 自相关和
*/
{
    int16_t i, n_temp = n_size - n_lag;
    float sum = 0.0f,*pn_ptr;
    if(n_temp <= 0) return sum;    
    for (i = 0,pn_ptr = pn_x; i < n_temp; ++i,++pn_ptr) 
	{
        sum += (*pn_ptr)*(*(pn_ptr + n_lag));
    }
    return sum / n_temp;
}

void rf_signal_periodicity(float *pn_x, int32_t n_size, int32_t *p_last_periodicity, int32_t n_min_distance, int32_t n_max_distance, float min_aut_ratio, float aut_lag0, float *ratio)
/**
* \brief        信号周期
* \par          Details
*查找可用于计算心率的IR信号的周期性。
*利用自相关功能。如果峰值自相关较小
*比滞后= 0时的自相关的min_aut_ratio分数，则输入信号的周期性不足，可能表示运动伪影。
*
*               Robert Fraczkiewicz, 01/07/2018
* \retval       峰之间的平均距离
*/
{
    int32_t n_lag;
    float aut,aut_left,aut_right,aut_save;
    rt_bool_t left_limit_reached = RT_FALSE;
    //从最后一个周期开始计算对应的自相关
    n_lag = *p_last_periodicity;
    aut_save = aut = rf_autocorrelation(pn_x, n_size, n_lag);
    //自相关是否比左侧大一阶？
    aut_left = aut;
    do {
        aut = aut_left;
        n_lag--;
        aut_left = rf_autocorrelation(pn_x, n_size, n_lag);
    } while(aut_left > aut && n_lag > n_min_distance);
    // 恢复最高自动延迟
    if(n_lag == n_min_distance) 
	{
        left_limit_reached = RT_TRUE;
        n_lag = *p_last_periodicity;
        aut   = aut_save;
    } 
	else n_lag++;
    if(n_lag == *p_last_periodicity)
	{
        // 向左跳没有任何进展。向右走。
        aut_right = aut;
        do {
            aut=aut_right;
            n_lag++;
            aut_right = rf_autocorrelation(pn_x, n_size, n_lag);
        } while(aut_right>aut && n_lag<n_max_distance);
        // 恢复最高自动延迟
        if(n_lag == n_max_distance) n_lag = 0; // 表示失败
        else n_lag--;
        if(n_lag == *p_last_periodicity && left_limit_reached) n_lag = 0; // 表示失败
    }
    *ratio = aut / aut_lag0;
    if(*ratio < min_aut_ratio) n_lag = 0; // 表示失败
    *p_last_periodicity = n_lag;
}

float rf_rms(float *pn_x, int32_t n_size, float *sumsq)
/**
* \brief       均方根变化
* \par          Details
*              计算给定序列pn_x的均方根变化
*               Robert Fraczkiewicz, 12/25/2017
* \retval       RMS值和原始平方和
*/
{
    int16_t i;
    float r,*pn_ptr;
    (*sumsq) = 0.0f;
    for (i = 0,pn_ptr = pn_x; i < n_size; ++i,++pn_ptr) 
	{
        r = (*pn_ptr);
        (*sumsq) += r*r;
    }
    (*sumsq)/= n_size; //这对应于滞后= 0时的自相关
    return sqrt(*sumsq);
}

float rf_Pcorrelation(float *pn_x, float *pn_y, int32_t n_size)
/**
* \brief       相关乘积
* \par          Details
*               计算* pn_x和* pn_y向量之间的标量积
*               Robert Fraczkiewicz, 12/25/2017
* \retval      相关乘积
*/
{
    int16_t i;
    float r,*x_ptr,*y_ptr;
    r = 0.0f;
    for (i = 0,x_ptr = pn_x,y_ptr = pn_y; i < n_size; ++i,++x_ptr,++y_ptr)
	{
        r += (*x_ptr)*(*y_ptr);
    }
    r /= n_size;
    return r;
}


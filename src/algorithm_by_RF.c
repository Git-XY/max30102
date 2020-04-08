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
*�����õĲ���
*������ĵ�·��Ӳ�����÷���Ĭ��ֵ���벻Ҫ�����Щ
*�ڴ˴����˵����������ͨ������ͬ�Ĳ�����
*��/���������Ƚ���Ҫ������Щ������
*/
#define ST 2      //����ʱ�䣨��sΪ��λ�������棺�������ST����������¼��������sum_X2������
#define FS 50    //����Ƶ�ʣ���λ��Hz�������棺�������FS����������¼��������sum_X2������
//��-mean_X�������ģ���+ mean_X��ST * FS���ֵ�ƽ���ͼ�1�����磬����ST = 4��FS = 25��
//�ܺ���100������ɣ���-49.5��^ 2 +��-48.5��^ 2 +��-47.5��^ 2 + ... +��47.5��^ 2 +��48.5��^ 2 +��49.5��^ 2
//�ܺ���symmetrc�����������ͨ����������������2��������ֵ��
//���ܡ�
static const float sum_X2 = 83325; //���棺����������������ST��FS����������¼���úͣ�
#define MAX_HR 125  //������ʡ�Ϊ�����������źţ��������HR�����ܴ��ڸ����֡�
#define MIN_HR 40  //������ʡ�Ϊ�����������źţ��������HR�����ܵ��ڸ����֡�
//���͵����ʡ���������Ϊ����Ӧ�ó�����Ԥ�����ʷ�Χ������ֵ����Ȼ����һ����
//��MIN_HR��MAX_HR֮�䡣���磬�����ҹ������HR��46��65֮��仯������90����ʱ��
//������50��60֮�䣬Ȼ��������Ϊ60��
//���棺����һ��CRUCIAL��������ȷ��������Դ����ȡ���ڴˡ�
#define TYPICAL_HR 65
//�������������Ԫ�ص���С���ʣ�һ���ͺ���һ���ͺ�0��
//�������źŵı��ʱ�����ڴ���Сֵ��
static const float min_autocorrelation_ratio = 0.5f;
//��ɫ��IR�ź�֮���Pearson����ԡ�
//�������źŵ����ϵ��������ڴ���Сֵ��
static const float min_pearson_correlation   = 0.8f;

/*
*��������
*��������Щ��
 *
 */
static const int32_t BUFFER_SIZE    = FS*ST; //���������е���������
static const int32_t FS60           = FS*60; //��bps��bpm������ת������
static const int32_t LOWEST_PERIOD  = FS60/MAX_HR; //��֮�����С����
static const int32_t HIGHEST_PERIOD = FS60/MIN_HR; //��֮���������
static const int32_t INIT_INTERVAL  = FS60/TYPICAL_HR; //����ȷ�����ʵ����̵�����ֵ
static const float mean_X           = (float)(BUFFER_SIZE-1)/2.0f; //��0��BUFFER_SIZE-1����������ƽ��ֵ������ST = 4��FS = 25������49.5��



void rf_heart_rate_and_oxygen_saturation(uint32_t *pun_ir_buffer, int32_t n_ir_buffer_length, uint32_t *pun_red_buffer, float *pn_spo2, int8_t *pch_spo2_valid,
        int32_t *pn_heart_rate, int8_t *pch_hr_valid, float *ratio, float *correl)
/**
*�������ʺ�SpO2ˮƽ��Robert Fraczkiewicz�汾
* ϸ��
*ͨ�����PPG���ڵķ�ֵ�Լ���ɫ/�����źŵ���ӦAC / DC�����Լ����SPO2��xy_ratio��
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

    // ����DC��ֵ����ir�ͺ�ɫ�м�ȥDC
    f_ir_mean  = 0.0f;
    f_red_mean = 0.0f;
    for (k = 0; k < n_ir_buffer_length; ++k) 
	{
        f_ir_mean  += pun_ir_buffer[k];
        f_red_mean += pun_red_buffer[k];
    }
    f_ir_mean  = f_ir_mean  / n_ir_buffer_length ;
    f_red_mean = f_red_mean / n_ir_buffer_length ;

    // �Ƴ�ֱ��
    for (k = 0,ptr_x = an_x,ptr_y = an_y; k < n_ir_buffer_length; ++k,++ptr_x,++ptr_y)
	{
        *ptr_x = pun_ir_buffer[k]  - f_ir_mean;
        *ptr_y = pun_red_buffer[k] - f_red_mean;
    }

    //RF�������������ƣ�����ˮƽ��
    beta_ir  = rf_linear_regression_beta(an_x, mean_X, sum_X2);
    beta_red = rf_linear_regression_beta(an_y, mean_X, sum_X2);
    for(k = 0,x = -mean_X,ptr_x = an_x,ptr_y = an_y; k < n_ir_buffer_length; ++k,++x,++ptr_x,++ptr_y) 
	{
        *ptr_x -= beta_ir*x;
        *ptr_y -= beta_red*x;
    }

    // ����SpO2����������������źŵ�RMS�����⣬����������ҪIR��ԭʼƽ����
    f_y_ac=rf_rms(an_y,n_ir_buffer_length,&f_red_sumsq);
    f_x_ac=rf_rms(an_x,n_ir_buffer_length,&f_ir_sumsq);

    // �����ɫ�ͺ���֮���Ƥ��ѷ�����
    *correl = rf_Pcorrelation(an_x, an_y, n_ir_buffer_length) / sqrt(f_red_sumsq*f_ir_sumsq);
    if(*correl >= min_pearson_correlation) 
	{
        // RF�������������ã������IR�źŵ�ƽ�����ڡ����Ϊ�������ԣ��򷵻�������Ϊ0
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
        *pn_heart_rate  	 = -999; //�޷����㣬��Ϊ�źſ������Ƿ������Ե�
        *pch_hr_valid        = 0;
        *pn_spo2             = -999; //��Ҫ�Ӵ��𻵵��ź���ʹ��SPO2
        *pch_spo2_valid      = 0;
        return;
    }

    // ����������ƽ��ֵ��ʾֱ��ˮƽ
    xy_ratio = (f_y_ac*f_ir_mean) / (f_x_ac*f_red_mean); //��ʽ�ǣ�f_y_ac * f_x_dc��/��f_x_ac * f_y_dc��;
    if(xy_ratio > 0.02f && xy_ratio < 1.84f)           //������÷�Χ
	{ 
        *pn_spo2        = (-45.060f*xy_ratio + 30.354f)*xy_ratio + 94.845f;
        *pch_spo2_valid = 1;
    }
	else 
	{
        *pn_spo2         = -999 ; //��Ҫʹ��SPO2����Ϊ�ź�an_ratio������Χ
        *pch_spo2_valid  = 0;
    }
}

float rf_linear_regression_beta(float *pn_x, float xmean, float sum_x2)
/**
* \brief        Coefficient beta of linear regression ���Իع�ϵ����
* \par          Details
*����pn_x�Ծ�ֵ���ĵ����Իع�ķ���ϵ��beta
*������ֵ��0��BUFFER_SIZE-1���� xmean������ڣ�BUFFER_SIZE-1��/ 2�� sum_x2�Ǿ���ָ��ֵ��ƽ���͡�
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
* \brief        Autocorrelation function ����ع���
* \par          Details
*               �����������pn_x����������е�n_lagԪ��
*               Robert Fraczkiewicz, 12/21/2017
* \retval       Autocorrelation sum ����غ�
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
* \brief        �ź�����
* \par          Details
*���ҿ����ڼ������ʵ�IR�źŵ������ԡ�
*��������ع��ܡ������ֵ����ؽ�С
*���ͺ�= 0ʱ������ص�min_aut_ratio�������������źŵ������Բ��㣬���ܱ�ʾ�˶�αӰ��
*
*               Robert Fraczkiewicz, 01/07/2018
* \retval       ��֮���ƽ������
*/
{
    int32_t n_lag;
    float aut,aut_left,aut_right,aut_save;
    rt_bool_t left_limit_reached = RT_FALSE;
    //�����һ�����ڿ�ʼ�����Ӧ�������
    n_lag = *p_last_periodicity;
    aut_save = aut = rf_autocorrelation(pn_x, n_size, n_lag);
    //������Ƿ������һ�ף�
    aut_left = aut;
    do {
        aut = aut_left;
        n_lag--;
        aut_left = rf_autocorrelation(pn_x, n_size, n_lag);
    } while(aut_left > aut && n_lag > n_min_distance);
    // �ָ�����Զ��ӳ�
    if(n_lag == n_min_distance) 
	{
        left_limit_reached = RT_TRUE;
        n_lag = *p_last_periodicity;
        aut   = aut_save;
    } 
	else n_lag++;
    if(n_lag == *p_last_periodicity)
	{
        // ������û���κν�չ�������ߡ�
        aut_right = aut;
        do {
            aut=aut_right;
            n_lag++;
            aut_right = rf_autocorrelation(pn_x, n_size, n_lag);
        } while(aut_right>aut && n_lag<n_max_distance);
        // �ָ�����Զ��ӳ�
        if(n_lag == n_max_distance) n_lag = 0; // ��ʾʧ��
        else n_lag--;
        if(n_lag == *p_last_periodicity && left_limit_reached) n_lag = 0; // ��ʾʧ��
    }
    *ratio = aut / aut_lag0;
    if(*ratio < min_aut_ratio) n_lag = 0; // ��ʾʧ��
    *p_last_periodicity = n_lag;
}

float rf_rms(float *pn_x, int32_t n_size, float *sumsq)
/**
* \brief       �������仯
* \par          Details
*              �����������pn_x�ľ������仯
*               Robert Fraczkiewicz, 12/25/2017
* \retval       RMSֵ��ԭʼƽ����
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
    (*sumsq)/= n_size; //���Ӧ���ͺ�= 0ʱ�������
    return sqrt(*sumsq);
}

float rf_Pcorrelation(float *pn_x, float *pn_y, int32_t n_size)
/**
* \brief       ��س˻�
* \par          Details
*               ����* pn_x��* pn_y����֮��ı�����
*               Robert Fraczkiewicz, 12/25/2017
* \retval      ��س˻�
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


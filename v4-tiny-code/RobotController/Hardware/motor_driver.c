#include "tim.h"
#include "gpio.h"
#include "motor_driver.h"
#include "delay.h"
#include "adc.h"

__IO int16_t MotorDirver_Tim4_Update_Count = 0;
__IO int16_t MotorDirver_Tim5_Update_Count = 0;
__IO int16_t MotorDirver_Tim3_Update_Count = 0;
__IO int16_t MotorDirver_Tim2_Update_Count = 0;

#define LedIO_Reset HAL_GPIO_WritePin(FnLEDn_GPIO_Port, FnLEDn_Pin, GPIO_PIN_RESET)
#define LedIO_Set HAL_GPIO_WritePin(FnLEDn_GPIO_Port, FnLEDn_Pin, GPIO_PIN_SET)
// 电机初始出，注意case里面没有break，所以会依次启动下面的

/**
 * @brief  电机驱动初始化函数
 * @note   该函数将根据 MOTOR_COUNT 确定初始化的电机数量
 *         基于 DRV8243HW 芯片的电机驱动初始化函数，可参考 Drv8243 DataSheet P23页
 */
void MotorDriver_Init(void) {

    /* 检查电机数量是否有效 */
    if (MOTOR_COUNT < 1 || MOTOR_COUNT > 4) return;

#if (IS_ENABLE_MOTOR_CURRENT_DETECTION && !IS_ENABLE_MOTOR_CURRENT_FULL_DETECTION)
    /* 若关闭全通道采样 则 检查ADC通道配置是否与电机数量匹配 */
    if (hadc1.Init.NbrOfConversion != MOTOR_COUNT) return; 
#endif

    /* for循环依次唤醒和配置指定数量个电机 */
    for (uint8_t motor = 1; motor <= MOTOR_COUNT; motor++) {
        GPIO_TypeDef* nSLEEP_Port;
        uint16_t nSLEEP_Pin;
        GPIO_TypeDef* OFF_Port;
        uint16_t OFF_Pin;
        uint32_t channel;

        /* 根据电机编号选择相应的引脚和pwm输出通道 */
        switch (motor) {
            case 1:
                nSLEEP_Port = M1_nSLEEP_GPIO_Port;
                nSLEEP_Pin = M1_nSLEEP_Pin;
                OFF_Port = M1_OFF_GPIO_Port;
                OFF_Pin = M1_OFF_Pin;
                channel = TIM_CHANNEL_4;
                break;
            case 2:
                nSLEEP_Port = M2_nSLEEP_GPIO_Port;
                nSLEEP_Pin = M2_nSLEEP_Pin;
                OFF_Port = M2_OFF_GPIO_Port;
                OFF_Pin = M2_OFF_Pin;
                channel = TIM_CHANNEL_1;
                break;
            case 3:
                nSLEEP_Port = M3_nSLEEP_GPIO_Port;
                nSLEEP_Pin = M3_nSLEEP_Pin;
                OFF_Port = M3_OFF_GPIO_Port;
                OFF_Pin = M3_OFF_Pin;
                channel = TIM_CHANNEL_3;
                break;
            case 4:
                nSLEEP_Port = M4_nSLEEP_GPIO_Port;
                nSLEEP_Pin = M4_nSLEEP_Pin;
                OFF_Port = M4_OFF_GPIO_Port;
                OFF_Pin = M4_OFF_Pin;
                channel = TIM_CHANNEL_2;
                break;
        }

        /* 唤醒电机并等待进入待机状态 */
        HAL_GPIO_WritePin(nSLEEP_Port, nSLEEP_Pin, GPIO_PIN_SET);
        HAL_Delay(1); /* 等待器件回应 */

        /* 确认电机已唤醒 */
        HAL_GPIO_WritePin(nSLEEP_Port, nSLEEP_Pin, GPIO_PIN_RESET);
        delay_us(31); /* 等待器件回应 */
        // while(HAL_GPIO_ReadPin(M4_nFAULT_GPIO_Port,M4_nFAULT_Pin)!=1){}//不用while而用delay的写法，是担心卡死。
        HAL_GPIO_WritePin(nSLEEP_Port, nSLEEP_Pin, GPIO_PIN_SET); /* nsleep激活 */

        /* 开启PWM输出和关闭OFF引脚 */
        HAL_TIM_PWM_Start(&htim1, channel);
        HAL_GPIO_WritePin(OFF_Port, OFF_Pin, GPIO_PIN_RESET);

        /* 设定电机处于停转状态，并设置占空比为50% */
        MotorDriver_Stop(motor, PWM_DUTY_LIMIT / 2);
    }
}

/**
 * @brief  电机启动函数
 * @param  nMotor 电机编号
 * @param  nDuty  PWM占空比，0 ~ PWM_DUTY_LIMIT 对应 0 ~ 100%
 */
void MotorDriver_Start(uint8_t nMotor, uint16_t nDuty) {
    
    /* 输入参数检验 */
    uint16_t nDutySet;
    if (nDuty > PWM_DUTY_LIMIT) {
        nDutySet = PWM_DUTY_LIMIT;
    } else {
        nDutySet = nDuty;
    }

    switch (nMotor) {
        case 4:
            TIM1->CCR2 = nDutySet;
            HAL_Delay(1);
            HAL_GPIO_WritePin(M4_OFF_GPIO_Port, M4_OFF_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(M4_IN1_GPIO_Port, M4_IN1_Pin, GPIO_PIN_SET);
            break;
        case 3:
            TIM1->CCR3 = nDutySet;
            HAL_Delay(1);
            HAL_GPIO_WritePin(M3_OFF_GPIO_Port, M3_OFF_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(M3_IN1_GPIO_Port, M3_IN1_Pin, GPIO_PIN_SET);
            break;
        case 2:
            TIM1->CCR1 = nDutySet;
            HAL_Delay(1);
            HAL_GPIO_WritePin(M2_OFF_GPIO_Port, M2_OFF_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(M2_IN1_GPIO_Port, M2_IN1_Pin, GPIO_PIN_SET);
            break;
        case 1:
            TIM1->CCR4 = nDutySet;
            HAL_Delay(1);
            HAL_GPIO_WritePin(M1_OFF_GPIO_Port, M1_OFF_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(M1_IN1_GPIO_Port, M1_IN1_Pin, GPIO_PIN_SET);
            break;
        default:;
    }
}

/**
 * @brief   设置电机驱动的占空比
 * @param   nMotor 电机编号
 * @param   nDuty  PWM占空比，0 ~ PWM_DUTY_LIMIT 对应 0 ~ 100%
 * @details 该函数通过设置PWM的占空比，控制电机驱动H桥的开关周期，从而实现电机的降压控制。
 *          占空比 0 ~ 100%  对应电压 0 ~ VCC。
 */
void MotorDriver_SetPWMDuty(uint8_t nMotor, uint16_t nDuty) {
    
    /* 输入参数检验 */
    uint16_t nDutySet;
    if (nDuty > PWM_DUTY_LIMIT) {
        nDutySet = PWM_DUTY_LIMIT;
    } else {
        nDutySet = nDuty;
    }

    switch (nMotor) {
        case 1:
            TIM1->CCR4 = nDutySet;
            break;
        case 2:
            TIM1->CCR1 = nDutySet;
            break;
        case 3:
            TIM1->CCR3 = nDutySet;
            break;
        case 4:
            TIM1->CCR2 = nDutySet;
            break;
        default:;
    }
}

/* 电机停止 */
void MotorDriver_Stop(uint8_t nMotor, uint16_t nDuty) {
    
    /* 输入参数检验 */
    uint16_t nDutySet;
    if (nDuty > PWM_DUTY_LIMIT) {
        nDutySet = PWM_DUTY_LIMIT;
    } else {
        nDutySet = nDuty;
    }

    switch (nMotor) {
        case 1:
            HAL_GPIO_WritePin(M1_IN1_GPIO_Port, M1_IN1_Pin, GPIO_PIN_RESET);
            TIM1->CCR4 = nDutySet;
            break;
        case 2:
            HAL_GPIO_WritePin(M2_IN1_GPIO_Port, M2_IN1_Pin, GPIO_PIN_RESET);
            TIM1->CCR1 = nDutySet;
            break;
        case 3:
            HAL_GPIO_WritePin(M3_IN1_GPIO_Port, M3_IN1_Pin, GPIO_PIN_RESET);
            TIM1->CCR3 = nDutySet;
            break;
        case 4:
            HAL_GPIO_WritePin(M4_IN1_GPIO_Port, M4_IN1_Pin, GPIO_PIN_RESET);
            TIM1->CCR2 = nDutySet;
            break;
        default:;
    }
}

/**
 * @brief   关闭电机驱动
 * @param   nMotor
 * @details 该函数将关闭电机驱动的H桥输出，此时电机正负极均为高阻态，此使电机可以自由旋转。
 *          该函数需要与Stop函数进行区分，Stop函数为电机停止，且电机处于制动状态，不能自由旋转。
 */
void MotorDriver_OFF(uint8_t nMotor) {
    switch (nMotor) {
        case 1:
            HAL_GPIO_WritePin(M1_OFF_GPIO_Port, M1_OFF_Pin, GPIO_PIN_SET);
            break;
        case 2:
            HAL_GPIO_WritePin(M2_OFF_GPIO_Port, M2_OFF_Pin, GPIO_PIN_SET);
            break;
        case 3:
            HAL_GPIO_WritePin(M3_OFF_GPIO_Port, M3_OFF_Pin, GPIO_PIN_SET);
            break;
        case 4:
            HAL_GPIO_WritePin(M4_OFF_GPIO_Port, M4_OFF_Pin, GPIO_PIN_SET);
            break;
        default:;
    }
}


/**
 * @brief  启用电机驱动
 */
void MotorDriver_ON(uint8_t nMotor) {
    switch (nMotor) {
        case 1:
            HAL_GPIO_WritePin(M1_OFF_GPIO_Port, M1_OFF_Pin, GPIO_PIN_RESET);
            break;
        case 2:
            HAL_GPIO_WritePin(M2_OFF_GPIO_Port, M2_OFF_Pin, GPIO_PIN_RESET);
            break;
        case 3:
            HAL_GPIO_WritePin(M3_OFF_GPIO_Port, M3_OFF_Pin, GPIO_PIN_RESET);
            break;
        case 4:
            HAL_GPIO_WritePin(M4_OFF_GPIO_Port, M4_OFF_Pin, GPIO_PIN_RESET);
            break;
        default:;
    }
}

// 这个还没改完
uint8_t MotorDriver_GetMotorState(uint8_t nMotor) {
    switch (nMotor) {
        case 1:
            return HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_14);
        case 2:
            return HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_10);
        case 3:
            return HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8);
        case 4:
            return HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_8);
        default:
            return 1;
    }
}


/**
 * @brief  获取电机的负载电流
 * @param  motor_currents 存储电机负载电流的数组
 * @note   该函数为阻塞查询函数，消耗的时间一般可以忽略不计（理想情况下ADC查询开销10~几十us），但在最坏的情况下可能发生ADC超时（极少）。
 *         传入的 motor_currents 需要确保 空间大于 4 从而避免溢出问题
 * @retval 1 获取成功
 * @retval 0 获取失败，一般为ADC超时，此时必须进行DEBUG排查。
 */
uint8_t MotorDriver_GetCurrent(uint32_t* motor_currents) {

#if (IS_ENABLE_MOTOR_CURRENT_FULL_DETECTION)
    
    uint8_t motor_current_channel_num = 4;
    uint8_t error = 0;

#if (!IS_ENABLE_MOTOR_CURRENT_FULL_DETECTION)
    motor_current_channel_num = MOTOR_COUNT;
#endif

    for (uint8_t i = 1; i <= motor_current_channel_num; i++)
    {
        HAL_ADC_Start(&hadc1);
        error |= HAL_ADC_PollForConversion(&hadc1,2); /* ADC采样等待 超时2ms */
        uint32_t adc_value = HAL_ADC_GetValue(&hadc1);
        /* 整形运算版本 将 x3075 拆分两次以优化整形运算的舍入误差 */
        // adc_value = ((adc_value * ADC_REF_VOLTAGE * 123) >> 12) * 25 / 1000;
        /* 浮点运算版本 带FPU的情况下，浮点运算可能速度相差无几 */
        adc_value = (uint32_t)(adc_value * ADC_REF_VOLTAGE * 3075.0f * 0.000244140625f * 0.001f);
        *(motor_currents + motor_current_channel_num - 1) = adc_value;
    }

    return (!error);

#endif

    return 0;
}



/**
 * @brief   编码器初始化函数
 * @note    该函数将根据 ENCODER_COUNT 宏定义确定初始化的编码器数量
 * @details 基于 AB 相反馈 的编码器初始化
 */
void Encoder_Init(void) {

    /* 检查编码器数量是否有效 */
    if (ENCODER_COUNT < 1 || ENCODER_COUNT > 4)
        return;

    switch (ENCODER_COUNT) {
        case 4:
            __HAL_TIM_CLEAR_IT(&htim5, TIM_IT_UPDATE);
            HAL_TIM_Base_Start_IT(&htim5);
            TIM5->CNT = 0;
            HAL_TIM_Encoder_Start(&htim5, TIM_CHANNEL_ALL);
        case 3:
            __HAL_TIM_CLEAR_IT(&htim3, TIM_IT_UPDATE);
            HAL_TIM_Base_Start_IT(&htim3);
            TIM3->CNT = 0;
            HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
        case 2:
            __HAL_TIM_CLEAR_IT(&htim4, TIM_IT_UPDATE);
            HAL_TIM_Base_Start_IT(&htim4);
            TIM4->CNT = 0;
            HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);
        case 1:
            __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_UPDATE);
            HAL_TIM_Base_Start_IT(&htim2);
            TIM2->CNT = 0;
            HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
        default:;
    }
}

/**
 * @brief  获取编码器的当前计数值
 * @param  nEncoder 编码器编号，nEncoder=1返回编码器1，以此类推。
 * @retval 编码器的当前计数值
 */
uint16_t Encoder_GetCNT(uint8_t nEncoder) {
    switch (nEncoder) {
        case 1:
            return TIM2->CNT;
        case 2:
            return TIM4->CNT;
        case 3:
            return TIM3->CNT;
        case 4:
            return TIM5->CNT;
        default:
            return 0;
    }
}

/**
 * @brief  获取编码器累计计数值
 * @param  nEncoder 编码器编号，nEncoder=1返回编码器1，以此类推。
 * @return 编码器的累计计数值 
 */
int32_t Encoder_GetEncCount(uint8_t nEncoder) {
    switch (nEncoder) {
        case 1:
            return (int32_t)ENC_TIM_ARR * MotorDirver_Tim2_Update_Count + TIM2->CNT;
        case 2:
            return (int32_t)ENC_TIM_ARR * MotorDirver_Tim4_Update_Count + TIM4->CNT;
        case 3:
            return (int32_t)ENC_TIM_ARR * MotorDirver_Tim3_Update_Count + TIM3->CNT;
        case 4:
            return (int32_t)ENC_TIM_ARR * MotorDirver_Tim5_Update_Count + TIM5->CNT;
        default:
            return 0;
    }
}

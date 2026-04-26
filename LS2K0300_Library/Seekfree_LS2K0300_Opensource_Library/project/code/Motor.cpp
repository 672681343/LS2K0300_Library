// motor.cpp
#include "Motor.h"
#include <csignal>
#include <cstdlib>

namespace Motor {
    // 设备路径定义
    static constexpr const char* MOTOR1_DIR = "/dev/zf_driver_gpio_motor_1";
    static constexpr const char* MOTOR1_PWM = "/dev/zf_device_pwm_motor_1";
    static constexpr const char* MOTOR2_DIR = "/dev/zf_driver_gpio_motor_2";
    static constexpr const char* MOTOR2_PWM = "/dev/zf_device_pwm_motor_2";

    // PWM信息存储
    static pwm_info motor1_pwm;
    static pwm_info motor2_pwm;

    //电机死锁
    uint8_t motor_lock = 0;

    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 电机初始化
    // 参数说明: 无
    // 返回参数: 无
    // 使用示例: Motor::Init();
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    void Init() {
        // 获取PWM设备信息
        pwm_get_dev_info(MOTOR1_PWM, &motor1_pwm);
        pwm_get_dev_info(MOTOR2_PWM, &motor2_pwm);
        
        pwm_set_duty(MOTOR1_PWM, 0);
        pwm_set_duty(MOTOR2_PWM, 0);

        motor_lock = 0;
        
    }
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 电机停止
    // 参数说明: 无
    // 返回参数: 无
    // 使用示例: Motor::deInit();
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    void deinit() {
        // 停止所有电机
        pwm_set_duty(MOTOR1_PWM, 0);
        pwm_set_duty(MOTOR2_PWM, 0);

        motor_lock = 1;
    }
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 设定PWM
    // 参数说明: 左轮PWM，右轮PWM
    // 返回参数: 无
    // 使用示例: Motor::goMotor(1500,1500);
    // 备注信息:
    //-------------------------------------------------------------------------------------------------------------------
    void goMotor(int16_t L_PWM, int16_t R_PWM) {
        if(motor_lock == 0){
            L_PWM = -L_PWM;
            R_PWM = R_PWM;
            // 手动限幅
            // 限制左电机PWM范围
            if(L_PWM > PWM_MAX) {
                L_PWM = PWM_MAX;
            } else if(L_PWM < -PWM_MAX) {
                L_PWM = -PWM_MAX;
            }
            // 限制右电机PWM范围
            if(R_PWM > PWM_MAX) {
                R_PWM = PWM_MAX;
            } else if(R_PWM < -PWM_MAX) {
                R_PWM = -PWM_MAX;
            }
        
            // 控制右电机
            if(R_PWM >= 0) {
                gpio_set_level(MOTOR2_DIR, 0);
                pwm_set_duty(MOTOR2_PWM, R_PWM);
            } else {
                gpio_set_level(MOTOR2_DIR, 1);
                pwm_set_duty(MOTOR2_PWM, -R_PWM);
            }
        
            // 控制左边电机
            if(L_PWM >= 0) {
                gpio_set_level(MOTOR1_DIR, 1);
                pwm_set_duty(MOTOR1_PWM, L_PWM);
            } else {
                gpio_set_level(MOTOR1_DIR, 0);
                pwm_set_duty(MOTOR1_PWM, -L_PWM);

            }
        }
    }
}


//-------------------------------------------------------------------------------------------------------------------
// 函数简介: 
// 参数说明: 
// 返回参数: 
// 使用示例: 
// 备注信息: 
//-------------------------------------------------------------------------------------------------------------------
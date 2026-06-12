#include "BLDC.h"

namespace BLDC{
    //设备路径定义
    static constexpr const char* PWM_1 = "/dev/zf_device_pwm_esc_1";
    static constexpr const char* PWM_2 = "/dev/zf_device_pwm_esc_2";
    //PWM信息存储
    struct pwm_info pwm_1_info;
    struct pwm_info pwm_2_info;

    uint16_t duty = 0;
    //电机死锁
    uint8_t motor_lock = 0;
    void Init() {
        // 获取PWM设备信息
        pwm_get_dev_info(PWM_1, &pwm_1_info);
        pwm_get_dev_info(PWM_2, &pwm_2_info);
        
        pwm_set_duty(PWM_1, 0);
        pwm_set_duty(PWM_2, 0);
        
        motor_lock = 0; // 重置电机锁定状态
    }

    void deinit() {
        // 停止所有电机
        pwm_set_duty(PWM_1, 0);
        pwm_set_duty(PWM_2, 0);

        motor_lock = 1; // 设置电机锁定状态
    }

    void Set_duty(uint16_t L_PWM, uint16_t R_PWM){
        if (motor_lock == 0) {
            pwm_set_duty(PWM_1, L_PWM);
            pwm_set_duty(PWM_2, R_PWM);
        }
    }
}
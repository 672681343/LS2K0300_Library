// motor.h
#pragma once
#include "zf_common_headfile.h"

namespace Motor {
    //限幅
    constexpr int16_t PWM_MAX =  9000;   

    // 初始化电机模块
    void Init();
    
    // 关闭电机模块（停止所有电机）
    void deinit();
    
    // 控制电机运动
    void goMotor(int16_t L_PWM, int16_t R_PWM);
}

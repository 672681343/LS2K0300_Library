#pragma once
#include "zf_common_headfile.h"


namespace BLDC{
    
    extern uint16_t duty;

    void Init();
    void deinit();
    void Set_duty(uint16_t L_PWM, uint16_t R_PWM);
}
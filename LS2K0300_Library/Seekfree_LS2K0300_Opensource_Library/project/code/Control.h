#pragma once
#include "zf_common_headfile.h"

namespace Control{
    
    typedef struct{
        int16_t cur_L;   //当前速度
        int16_t cur_R;   //当前速度
    }Current_Speed;

    typedef struct
    {
        int16_t L;
        int16_t R;
    }Motor_PWM;
    
    typedef struct
    {
        float kp;
        float kp2;
        float ki;
        float gkd;
        float kd;
    }Set_Pid;


    typedef struct
    {
        int16_t set_speed;
        float   dif;   //差速系数
        uint16_t BLDC_duty;
        uint8_t err_row; // 误差行数

        float l_kp;
        float l_ki;

        float r_kp;
        float r_ki;

    }Set_Speed;
    
    enum RoadSegment
    {
        NO    = 0,
        CURVE = 1,
        SHORT = 2,
        LOOG  = 3,
        ISLAND = 4,
        RAMP = 5
    };
    extern Current_Speed aim_speed;
    
    extern Motor_PWM pwm;
    extern Motor_PWM LongStr_pwm;
    extern Motor_PWM ShortStr_pwm;
    extern Motor_PWM Curve_pwm;
    extern Motor_PWM Island_pwm;

    extern Motor_PWM BLDC; // BLDC电机PWM
    extern uint8_t RoadSegment_t; // 0:直道 1:弯道
    extern float dif_rep; // 差速补偿
    
    extern Set_Speed LongStrSpeed;
    extern Set_Speed ShortStrSpeed;
    extern Set_Speed CurveSpeed;
    extern Set_Speed IslandSpeed;
    extern Set_Speed RampSpeed;
    
    extern Set_Pid LongStrPid;
    extern Set_Pid ShortStrPid;
    extern Set_Pid CurvePid;
    extern Set_Pid IslandPid;
    extern Set_Pid RampPid;
    void ControlOutput(uint8_t RoadSegment);
    void stop(uint8_t mode);
    uint8_t classifyRoadSegment (void);
}
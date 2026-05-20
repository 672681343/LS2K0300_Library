#pragma once
#include "zf_common_headfile.h"

namespace PID {
    // PID参数及状态结构体
    typedef struct PIDParams {
        float kp;               // 小弯比例系数
        float kp2;              // 大弯比例系数
        float ki;               // 积分系数
        float gkd;              // 陀螺仪辅助微分系数
        float kd;               // 摄像头预测微分系数
        int32_t p_term;      // 小弯比例项
        int32_t p2_term;     // 大弯比例项
        int32_t i_term;      // 积分项
        int32_t gd_term;     // 陀螺仪辅助微分项
        int32_t d_term;      // 摄像头预测微分项
        int16_t integral;       // 积分累积量
        int16_t prev_error;     // 前次误差
        int16_t I_limit;        // I限幅值 (0表示无限制)
        int16_t out_limit;      // 输出限幅值 (0表示无限制)
        int32_t prev_output;    // 前次输出
        int16_t prev_prev_error;    // 前前次目标值
    }PIDParams;


    extern PIDParams angle_pid;    // 角度环PID
    extern PIDParams turn_pid;     // 转向环PID
    extern PIDParams speed_pid_L;  // 左轮速度环PID
    extern PIDParams speed_pid_R;  // 右轮速度环PID

    void PID_init();                                                    // 初始化PID参数
    int16_t TurnPID(int16_t target, int16_t current, PIDParams *pid);   // 转向环PID
    int16_t AnglePID(int16_t target, int16_t current, PIDParams *pid);  // 角度环PID
    int16_t Speed_loc_PID(int16_t target, int16_t current, PIDParams* pid);  // 速度环PID
    int16_t Speed_inc_PID(int16_t target, int16_t current, PIDParams* pid);  // 速度环增量PID
}
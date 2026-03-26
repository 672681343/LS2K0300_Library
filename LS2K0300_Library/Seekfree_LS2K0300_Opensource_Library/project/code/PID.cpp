#include "PID.h"

namespace PID {
    PIDParams turn_pid = {0};
    PIDParams speed_pid_L = {0};
    PIDParams speed_pid_R = {0};
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 初始化PID参数
    // 参数说明: 无
    // 返回参数: 无
    // 使用示例: PID::PID_init();
    // 备注信息:
    //-------------------------------------------------------------------------------------------------------------------
    void PID_init() {
        turn_pid.kp = 1.6;
        turn_pid.kp2 = 0.0;
        turn_pid.ki = 0.0;
        turn_pid.gkd = 0.0;
        turn_pid.kd = 2.0;
        turn_pid.integral = 0;
        turn_pid.prev_error = 0;
        turn_pid.I_limit = 0;
        turn_pid.out_limit = 0;

        speed_pid_L.kp = 0.0;
        speed_pid_L.ki = 0.0;
        speed_pid_L.kd = 0.0;
        speed_pid_L.integral = 0;
        speed_pid_L.prev_error = 0;
        speed_pid_L.I_limit = 7000;
        speed_pid_L.out_limit = 9000;

        speed_pid_L.prev_output = 0;
        speed_pid_L.prev_error = 0;
        speed_pid_L.prev_prev_error = 0;


        speed_pid_R.kp = 0.0;
        speed_pid_R.ki = 0.0;
        speed_pid_R.kd = 0.0;
        speed_pid_R.integral = 0;
        speed_pid_R.prev_error = 0;
        speed_pid_R.I_limit = 7000;
        speed_pid_R.out_limit = 9000;
    }

    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 转向环PID
    // 参数说明: 目标，实际
    // 返回参数: int16
    // 使用示例: PID::TurnPID(0, Error::Err.current, &PID::turn_pid);
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    int16_t TurnPID(int16_t target, int16_t current, PIDParams *pid)
    {
        int16_t error = target - current;
        pid->p_term = pid->kp * error;
        pid->p2_term = pid->kp2 * error * abs(error);
        pid->gd_term = pid->gkd * imu660ra_gyro_z;
        pid->d_term = pid->kd * (error - pid->prev_error);
        int16_t output = pid->p_term + pid->d_term + pid->p2_term + pid->gd_term;
        // 总输出限幅
        if (pid->out_limit > 0) {
            if (output > pid->out_limit) {
                output = pid->out_limit;
            } else if (output < -pid->out_limit) {
                output = -pid->out_limit;
            }
        }
        pid->prev_error = error;

        // TCP::test[0] = output;
        
        return output;
    }
    
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 速度环位置PID
    // 参数说明: 目标，实际
    // 返回参数: int16
    // 使用示例: PID::SpeedPID(0, 10, &PID::speed_pid_L);
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    int16_t Speed_loc_PID(int16_t target, int16_t current, PIDParams* pid) {
        int16_t error = target - current;
        int32_t output = 0;
        // 积分累积（先累积后限幅）
        pid->integral += error;
        
        // 积分限幅
        if(pid->I_limit > 0) {
            if(pid->integral > pid->I_limit) {
                pid->integral = pid->I_limit;
            } else if(pid->integral < -pid->I_limit) {
                pid->integral = -pid->I_limit;
            }
        }
        
        // 计算各分量
        pid->p_term = pid->kp * error;
        pid->i_term = pid->ki * pid->integral;
        pid->d_term = pid->kd * (error - pid->prev_error);
 
        // 总输出
        output = (pid->p_term + pid->i_term + pid->d_term);
       // printf("error = %d, output = %d\n", error, output);
        // 输出限幅
        if(pid->out_limit > 0) {
            if(output > pid->out_limit) {
                output = pid->out_limit;
                // 抗饱和处理：回退多余积分
                if(pid->ki != 0) {
                    pid->integral -= (output - pid->out_limit) / pid->ki;
                }
            } 
            else if(output < -pid->out_limit) {
                output = -pid->out_limit;
                // 抗饱和处理：回退多余积分
                if(pid->ki != 0) {
                    pid->integral -= (output + pid->out_limit) / pid->ki;
                }
            }
        }
        
        // 保存误差记录
        pid->prev_error = error;
        
        return output;
    }

    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 速度环增量PID
    // 参数说明: 
    // 返回参数: 
    // 使用示例: 
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    #if 0
    int16_t Speed_inc_PID(int16_t target, int16_t current, PIDParams* pid) {
        int16_t error = target - current;

        // 计算各分量
        float p_term = pid->kp * (error - pid->prev_error);
        float i_term = pid->ki * error;
        float d_term = pid->kd * (error - 2*pid->prev_error + pid->prev_prev_error);
        
        // 总输出
        pid->prev_output += (p_term + i_term + d_term);
        
        // 输出限幅
        if(pid->out_limit > 0) {
            if(pid->prev_output > pid->out_limit) {
                pid->prev_output = pid->out_limit;
            } 
            else if(pid->prev_output < -pid->out_limit) {
                pid->prev_output = -pid->out_limit;
            }
        }

        pid->prev_prev_error = pid->prev_error;
        // 保存误差记录
        pid->prev_error = error;

        return pid->prev_output;
    }
    #endif
}


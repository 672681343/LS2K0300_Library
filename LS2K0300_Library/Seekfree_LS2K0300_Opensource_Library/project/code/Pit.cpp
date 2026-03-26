#include "Pit.h"
namespace Pit{
    uint16_t count_50ms = 0;
    uint16_t count_5ms = 0;
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 中断初始化
    // 参数说明: 无
    // 返回参数: 无
    // 使用示例: Pit::Init();
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    void Init(){
        pit_ms_init(5 , callback_5ms);
        pit_ms_init(50, callback_50ms);
    }
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 5ms中断
    // 参数说明: 无
    // 返回参数: 无
    // 使用示例: pit_ms_init(5, callback_5ms);
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    void callback_5ms()
    {
        Encoder::update();
        imu660ra_get_gyro();

        // // 公共停机逻辑
        if (MenuSystem::Car_flag.stop == 1) {
            Motor::deinit();
            return; // 直接退出，避免后续逻辑
        }
        if (Elem::isHandling){
            count_5ms++;
            if (count_5ms >= 20) { // 每50ms处理一次
                Elem::isHandling = 0; // 重置处理状态
                count_5ms = 0; // 重置计数器
            }
        }

        // 测试模式分发
        if (MenuSystem::Car_flag.test && MenuSystem::Car_flag.init && MenuSystem::Car_flag.start) {
            // if (MenuSystem::Car_flag.speed_loop ) {

            //     // PID::speed_pid_R.kp = PID::speed_pid_L.kp;
            //     // PID::speed_pid_R.ki = PID::speed_pid_L.ki;
            //     // PID::speed_pid_R.kd = PID::speed_pid_L.kd;
                
            //     // 闭环测试（允许左右轮独立 PID 参数）
            //     Control::pwm.L = PID::Speed_loc_PID(Control::aim_speed.cur_L, Encoder::count.L, &PID::speed_pid_L);
            //     Control::pwm.R = PID::Speed_loc_PID(Control::aim_speed.cur_R, Encoder::count.R, &PID::speed_pid_R);
            //     // Control::pwm.L = PID::Speed_loc_PID(0, Encoder::count.L, &PID::speed_pid_L);
            //     // Control::pwm.R = PID::Speed_loc_PID(0, Encoder::count.R, &PID::speed_pid_R);

            // } else if(MenuSystem::Car_flag.stop == 0){
            //     // 开环测试（允许左右轮独立 PWM）
            //     Control::pwm.L = MenuSystem::Car_flag.pwm; // 需在 Car_flag 中增加 pwm_L/pwm_R
            //     Control::pwm.R = MenuSystem::Car_flag.pwm;
            // } else {
            //     // 停止测试模式
            //     Control::pwm.L = 0;
            //     Control::pwm.R = 0;
            // }
            // Motor::goMotor(Control::pwm.L, Control::pwm.R);
           Motor::goMotor(1500, 1500);
        } 
        // 非测试模式
        else if (MenuSystem::Car_flag.init && MenuSystem::Car_flag.start) {
            // 速度环控制
            // PID::speed_pid_R.kp = PID::speed_pid_L.kp;
            // PID::speed_pid_R.ki = PID::speed_pid_L.ki;
            // PID::speed_pid_R.kd = PID::speed_pid_L.kd;
            
            Control::pwm.L = PID::Speed_loc_PID(Control::aim_speed.cur_L, Encoder::count.L, &PID::speed_pid_L);
            Control::pwm.R = PID::Speed_loc_PID(Control::aim_speed.cur_R, Encoder::count.R, &PID::speed_pid_R);

            Motor::goMotor(Control::pwm.L, Control::pwm.R);
        }
    }
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 50ms中断
    // 参数说明: 无
    // 返回参数: 无
    // 使用示例: pit_ms_init(50, callback_50ms);
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    void callback_50ms()
    {
        // TCP::data_analysis();
        if (MenuSystem::Car_flag.test && MenuSystem::Car_flag.start && MenuSystem::Car_flag.stop_times > 0) {
            if (MenuSystem::Car_flag.stop_times > count_50ms) {
                count_50ms++;
            } else {
                MenuSystem::Car_flag.stop = 1;
            }
        }

        
    }
}
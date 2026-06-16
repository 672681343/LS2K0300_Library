#include "Pit.h"
namespace Pit{
    uint16_t count_50ms = 0;
    uint16_t count_5ms = 0;
    int16_t Err_out = 0;
    int16_t Angle_out = 0;
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 中断初始化
    // 参数说明: 无
    // 返回参数: 无
    // 使用示例: Pit::Init();
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    void Init(){
        pit_ms_init(5 , callback_5ms);
        pit_ms_init(10 ,callback_10ms);
        // pit_ms_init(20 ,callback_20ms);
        pit_ms_init(50, callback_50ms);
    }
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 5ms中断
    // 参数说明: 无
    // 返回参数: 无
    // 使用示例: pit_ms_init(5, callback_5ms);
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------

//     #define FILTER_A 0.9f    
//     float gyro_filter(float new_val) {
//         static float last = 0;  // 上一次滤波后的值
//         last = last * FILTER_A + new_val * (1 - FILTER_A);
//         return last;
// }

    void callback_5ms()
    {
        Encoder::update();
        imu660ra_get_gyro();


        Err_out = PID::TurnPID(0, Err::Err.current, &PID::turn_pid);
        Angle_out = PID::AnglePID(-Err_out,imu660ra_gyro_z, &PID::angle_pid);  //imu660ra_gyro_z
        Control::pwm.L = PID::Speed_loc_PID((Control::aim_speed.cur_L+Control::aim_speed.cur_R)/2 , (Encoder::count.L+Encoder::count.R)/2, &PID::speed_pid_L);
        // Control::pwm.R = PID::Speed_loc_PID((Control::aim_speed.cur_L+Control::aim_speed.cur_R)/2 + Err_out , (Encoder::count.L+Encoder::count.R)/2, &PID::speed_pid_L);
                

        // // 公共停机逻辑
        if(ImageProc::state.cutoff < 3){
            MenuSystem::Car_flag.stop = 1;
        }
        // if (MenuSystem::Car_flag.stop == 1) {
        //     Motor::deinit();
        //     return; // 直接退出，避免后续逻辑
        // }
        if (Elem::isHandling){
            count_5ms++;
            if (count_5ms >= 20) { // 每50ms处理一次
                Elem::isHandling = 0; // 重置处理状态
                count_5ms = 0; // 重置计数器
            }
        }

        // 测试模式分发
        if (MenuSystem::Car_flag.test && MenuSystem::Car_flag.init && MenuSystem::Car_flag.start) {
            if (MenuSystem::Car_flag.speed_loop == 1) {
                
                // 闭环测试（允许左右轮独立 PID 参数）
                

                if(MenuSystem::Car_flag.turn == 0){
                    Motor::goMotor(Control::pwm.L, Control::pwm.L ); 
                }else{
                    Motor::goMotor(Control::pwm.L + Angle_out*(1 + Control::CurveSpeed.dif) , Control::pwm.L - Angle_out*(1 - Control::CurveSpeed.dif) );
                }


            } else if(MenuSystem::Car_flag.stop == 0){
                // 开环测试（允许左右轮独立 PWM）

                if(MenuSystem::Car_flag.turn == 0){
                    Motor::goMotor(  Angle_out, -Angle_out); //Control::pwm.L
                }else{
                    Motor::goMotor(1300 - Err_out , 1300 + Err_out );
                }

            } else {
                // 停止测试模式s
                Control::pwm.L = 0;
                Control::pwm.R = 0;
                Motor::goMotor(Control::pwm.L, Control::pwm.L );
            }
            
        } 
        // 非测试模式
        else if (MenuSystem::Car_flag.init && MenuSystem::Car_flag.start) {

                if (MenuSystem::Car_flag.speed_loop ) {
                
                // 闭环测试（允许左右轮独立 PID 参数）
 
                if(MenuSystem::Car_flag.turn == 0){
                    Motor::goMotor(Control::pwm.L, Control::pwm.L ); 
                }else{
                    Motor::goMotor(Control::pwm.L + Angle_out , Control::pwm.L - Angle_out );//左减右加
                }

            } else if(MenuSystem::Car_flag.stop == 0){
                // 开环测试（允许左右轮独立 PWM）

                if(MenuSystem::Car_flag.turn == 0){
                    Motor::goMotor(1500, 1500 ); 
                }else{
                    Motor::goMotor(Control::pwm.L  , Control::pwm.L  );
                }
            }

        }
    }

    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 10ms中断
    // 参数说明: 无
    // 返回参数: 无
    // 使用示例: pit_ms_init(10, callback_50ms);
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------

    void callback_10ms()
    {

        // Control::pwm.L = PID::Speed_loc_PID((Control::aim_speed.cur_L+Control::aim_speed.cur_R)/2, (Encoder::count.L+Encoder::count.R)/2, &PID::speed_pid_L);

    }



    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 20ms中断
    // 参数说明: 无
    // 返回参数: 无
    // 使用示例: pit_ms_init(10, callback_50ms);
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    void callback_20ms()
    {
        // Err_out = -PID::TurnPID(0, Error::Err.current, &PID::turn_pid);

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
        TCP::data_analysis();
        if (MenuSystem::Car_flag.test && MenuSystem::Car_flag.start && MenuSystem::Car_flag.stop_times > 0) {
            if (MenuSystem::Car_flag.stop_times > count_50ms) {
                count_50ms++;
            } else {
                MenuSystem::Car_flag.stop = 1;
            }
        }

        
    }
}
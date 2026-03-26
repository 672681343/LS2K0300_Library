#include "Control.h"

namespace Control{


    //目标速度
    Current_Speed aim_speed = {
        .cur_L = 0,
        .cur_R = 0
    };


///////////////开环////////////////


    //电机PWM
    Motor_PWM pwm = {
        .L = 0,
        .R = 0
    };

    Motor_PWM LongStr_pwm = {
        .L = 0,
        .R = 0
    };

    Motor_PWM ShortStr_pwm = {
        .L = 0,
        .R = 0
    };

    Motor_PWM Curve_pwm = {
        .L = 0,
        .R = 0
    };

    Motor_PWM Island_pwm = {
        .L = 0,
        .R = 0
    };


//////////////闭环////////////////////

    //长直道速度
    Set_Speed LongStrSpeed = {
        .set_speed = 200,
        .dif = 0.0,
        .BLDC_duty = 0,
        .err_row = 35,
        .l_kp = 0,
        .l_ki = 0,
        .r_kp = 0,
        .r_ki = 0
    };
    //短直道速度
    Set_Speed ShortStrSpeed = {
        .set_speed = 200,
        .dif = 0.0,
        .BLDC_duty = 0,
        .err_row = 35,
        .l_kp = 0,
        .l_ki = 0,
        .r_kp = 0,
        .r_ki = 0
    };
    //弯道速度
    Set_Speed CurveSpeed = {
        .set_speed = 200,
        .dif = 0.0,
        .BLDC_duty = 0,
        .err_row = 35,
        .l_kp = 0,
        .l_ki = 0,
        .r_kp = 0,
        .r_ki = 0
    };
    //环岛速度
    Set_Speed IslandSpeed = {
        .set_speed = 200,
        .dif = 0.0,
        .BLDC_duty = 0,
        .err_row = 35,
        .l_kp = 0,
        .l_ki = 0,
        .r_kp = 0,
        .r_ki = 0

    };




/////////////转向pid/////////////////

    //长直道PID参数
    Set_Pid LongStrPid = {
        .kp = 0.0,
        .kp2 = 0.0,
        .gkd = 0.0,
        .kd = 0.0
    };
    //短直道PID参数
    Set_Pid ShortStrPid = {
        .kp = 0.0,
        .kp2 = 0.0,
        .gkd = 0.0,
        .kd = 0.0
    };
    //弯道PID参数
    Set_Pid CurvePid = {
        .kp = 0.0,
        .kp2 = 0.0,
        .gkd = 0.0,
        .kd = 0.0
    };
    //环岛PID参数
    Set_Pid IslandPid = {
        .kp = 0.0,
        .kp2 = 0.0,
        .gkd = 0.0,
        .kd = 0.0
    };



    uint8_t RoadSegment_t = SHORT; 
    uint16_t stop_count = 0; // 停车计帧
    float dif_rep = 0.1; // 差速补偿
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 控制输出
    // 参数说明:
    // 返回参数:
    // 使用示例: Control::speed_decision();
    // 备注信息:
    //-------------------------------------------------------------------------------------------------------------------
    void ControlOutput(uint8_t RoadSegment)
    {
        const int16_t MAX_DELTA = 100;
        //RoadSegment = CURVE;
        if (MenuSystem::Car_flag.stop == 0){
            switch (RoadSegment)
            {
            case LOOG: 
                if(MenuSystem::Car_flag.speed_loop){
                    if (Error::Err.current > 0){
                        aim_speed.cur_L = LongStrSpeed.set_speed - my_min(LongStrSpeed.dif * Error::Err.current * (1 - dif_rep), MAX_DELTA);
                        aim_speed.cur_R = LongStrSpeed.set_speed + my_min(LongStrSpeed.dif * Error::Err.current * (1 + dif_rep), MAX_DELTA);
                    }else{
                        aim_speed.cur_L = LongStrSpeed.set_speed - my_min(LongStrSpeed.dif * Error::Err.current * (1 + dif_rep), MAX_DELTA);
                        aim_speed.cur_R = LongStrSpeed.set_speed + my_min(LongStrSpeed.dif * Error::Err.current * (1 - dif_rep), MAX_DELTA);
                    }
                    PID::speed_pid_L.kp = LongStrSpeed.l_kp;
                    PID::speed_pid_L.ki = LongStrSpeed.l_ki;
                    PID::speed_pid_R.kp = LongStrSpeed.r_kp;
                    PID::speed_pid_R.ki = LongStrSpeed.r_ki;

                    PID::turn_pid.kp = LongStrPid.kp;
                    PID::turn_pid.kp2 = LongStrPid.kp2;
                    PID::turn_pid.gkd = LongStrPid.gkd;
                    PID::turn_pid.kd = LongStrPid.kd;



                }else{
                    pwm.L = LongStr_pwm.L;
                    pwm.R = LongStr_pwm.R;

                    PID::turn_pid.kp = LongStrPid.kp;
                    PID::turn_pid.kp2 = LongStrPid.kp2;
                    PID::turn_pid.gkd = LongStrPid.gkd;
                    PID::turn_pid.kd = LongStrPid.kd;

                }


 
                break;
            case SHORT:
                if(MenuSystem::Car_flag.speed_loop){
                    if (Error::Err.current > 0){
                        aim_speed.cur_L = ShortStrSpeed.set_speed - my_min(ShortStrSpeed.dif * Error::Err.current * (1 - dif_rep), MAX_DELTA);
                        aim_speed.cur_R = ShortStrSpeed.set_speed + my_min(ShortStrSpeed.dif * Error::Err.current * (1 + dif_rep), MAX_DELTA);
                    }else{
                        aim_speed.cur_L = ShortStrSpeed.set_speed - my_min(ShortStrSpeed.dif * Error::Err.current * (1 + dif_rep), MAX_DELTA);
                        aim_speed.cur_R = ShortStrSpeed.set_speed + my_min(ShortStrSpeed.dif * Error::Err.current * (1 - dif_rep), MAX_DELTA);
                    }
                    PID::speed_pid_L.kp = ShortStrSpeed.l_kp;
                    PID::speed_pid_L.ki = ShortStrSpeed.l_ki;
                    PID::speed_pid_R.kp = ShortStrSpeed.r_kp;
                    PID::speed_pid_R.ki = ShortStrSpeed.r_ki;

                    PID::turn_pid.kp = ShortStrPid.kp;
                    PID::turn_pid.kp2 = ShortStrPid.kp2;
                    PID::turn_pid.gkd = ShortStrPid.gkd;
                    PID::turn_pid.kd = ShortStrPid.kd;



                }else{
                    pwm.L = ShortStr_pwm.L;
                    pwm.R = ShortStr_pwm.R;

                    PID::turn_pid.kp = ShortStrPid.kp;
                    PID::turn_pid.kp2 = ShortStrPid.kp2;
                    PID::turn_pid.gkd = ShortStrPid.gkd;
                    PID::turn_pid.kd = ShortStrPid.kd;

                }
                

 

                break;
            case CURVE:
                //分段PD控制器
                //SmartPD::SegmentPDController(Error::Err.current, CurvePid.kp, CurvePid.kd);
                if(MenuSystem::Car_flag.speed_loop){
                    if (Error::Err.current > 0){
                        aim_speed.cur_L = CurveSpeed.set_speed - my_min(CurveSpeed.dif * Error::Err.current * (1 - dif_rep), MAX_DELTA);
                        aim_speed.cur_R = CurveSpeed.set_speed + my_min(CurveSpeed.dif * Error::Err.current * (1 + dif_rep), MAX_DELTA);
                    }else{
                        aim_speed.cur_L = CurveSpeed.set_speed - my_min(CurveSpeed.dif * Error::Err.current * (1 + dif_rep), MAX_DELTA);
                        aim_speed.cur_R = CurveSpeed.set_speed + my_min(CurveSpeed.dif * Error::Err.current * (1 - dif_rep), MAX_DELTA);
                    }
                    PID::speed_pid_L.kp = CurveSpeed.l_kp;
                    PID::speed_pid_L.ki = CurveSpeed.l_ki;
                    PID::speed_pid_R.kp = CurveSpeed.r_kp;
                    PID::speed_pid_R.ki = CurveSpeed.r_ki;

                    PID::turn_pid.kp = CurvePid.kp;
                    PID::turn_pid.kp2 = CurvePid.kp2;
                    PID::turn_pid.gkd = CurvePid.gkd;
                    PID::turn_pid.kd = CurvePid.kd;



                }else{
                    pwm.L = Curve_pwm.L;
                    pwm.R = Curve_pwm.R;

                    PID::turn_pid.kp = CurvePid.kp;
                    PID::turn_pid.kp2 = CurvePid.kp2;
                    PID::turn_pid.gkd = CurvePid.gkd;
                    PID::turn_pid.kd = CurvePid.kd;

                }

                break;
            case ISLAND:
                
                if(MenuSystem::Car_flag.speed_loop){
                    if (Error::Err.current > 0){
                        aim_speed.cur_L = IslandSpeed.set_speed - my_min(IslandSpeed.dif * Error::Err.current * (1 - dif_rep), MAX_DELTA);
                        aim_speed.cur_R = IslandSpeed.set_speed + my_min(IslandSpeed.dif * Error::Err.current * (1 + dif_rep), MAX_DELTA);
                    }else{
                        aim_speed.cur_L = IslandSpeed.set_speed - my_min(IslandSpeed.dif * Error::Err.current * (1 + dif_rep), MAX_DELTA);
                        aim_speed.cur_R = IslandSpeed.set_speed + my_min(IslandSpeed.dif * Error::Err.current * (1 - dif_rep), MAX_DELTA);
                    }
                    PID::speed_pid_L.kp = IslandSpeed.l_kp;
                    PID::speed_pid_L.ki = IslandSpeed.l_ki;
                    PID::speed_pid_R.kp = IslandSpeed.r_kp;
                    PID::speed_pid_R.ki = IslandSpeed.r_ki;

                    PID::turn_pid.kp = IslandPid.kp;
                    PID::turn_pid.kp2 = IslandPid.kp2;
                    PID::turn_pid.gkd = IslandPid.gkd;
                    PID::turn_pid.kd = IslandPid.kd;



                }else{
                    pwm.L = Island_pwm.L;
                    pwm.R = Island_pwm.R;

                    PID::turn_pid.kp = IslandPid.kp;
                    PID::turn_pid.kp2 = IslandPid.kp2;
                    PID::turn_pid.gkd = IslandPid.gkd;
                    PID::turn_pid.kd = IslandPid.kd;

                }

                break;

            default:
                break;
            }
        }else{
            PID::turn_pid.kp = ShortStrPid.kp;
            PID::turn_pid.kd = ShortStrPid.kd;
            aim_speed.cur_L = -5; // 停车速度
            aim_speed.cur_R = -5; // 停车速度
        }



        if(MenuSystem::Car_flag.test == 1 && MenuSystem::Car_flag.turn == 0){


        }else{


        }
    }
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 停车检测
    // 参数说明: mode停车0为自动模式1为手动模式
    // 返回参数: 电机停止
    // 使用示例: Control::stop(1);
    // 备注信息: 自动模式(检测出界保护)手动模式(运行立刻停车)
    //-------------------------------------------------------------------------------------------------------------------
    void stop(uint8_t mode){
        if(mode == 1){
                MenuSystem::Car_flag.stop = 1;
        }
        else if(mode == 0){
            if(Camera::outlier_count >= (WIDTH * 3) - 30 && MenuSystem::Car_flag.init == 1 ){
                if(MenuSystem::Car_flag.test == 1 && MenuSystem::Car_flag.protect == 0){
                    MenuSystem::Car_flag.stop = MenuSystem::Car_flag.stop;
                }else{
                    MenuSystem::Car_flag.stop = 1;

                }

                //MenuSystem::sh_down();
            }

            // if(Elem::Zebra.count >= 4 && MenuSystem::Car_flag.init == 1 && MenuSystem::Car_flag.stop ==0){
            //     stop_count ++;
            //     if (stop_count > 2){ // 停止计数
            //         MenuSystem::Car_flag.stop = 1;
            //         stop_count = 0; // 重置停止计数
            //     }
            //     //MenuSystem::sh_down();
            // }
        }

        if(MenuSystem::Car_flag.stop == 1){
            if(Encoder::count.L < 3 &&  Encoder::count.R < 3){
                stop_count ++;
                if (stop_count > 30){ // 停止计数
                    MenuSystem::sh_down();
                }
            }
        }
    }


    /**
     * @brief 长直道判断函数
     * 
     * @return true 
     * @return false 
     */
    bool isLongStraightCondition() {
        return ImageProc::state.cutoff > 60 &&
               ImageProc::Mid_BF.LostParam.sum < 2 &&
               ImageProc::Right_BF.LostParam.sum < 2 &&
               ImageProc::Left_BF.LostParam.sum < 2 &&
               ImageProc::Left_BF.Var < 10 && ImageProc::Right_BF.Var < 10 &&
               ImageProc::Mid_BF.Var < 10 &&
               abs(ImageProc::Mid_BF.Slope) < 0.2 ;
    }  

    /**
     * @brief 进弯道短直道判断函数
     * 
     * @return true 
     * @return false 
     */
    bool isShortStraightCondition1() {
        return  ImageProc::state.cutoff > 60 &&
                ((ImageProc::Mid_BF.LostParam.sum < 10 && ImageProc::Mid_BF.LostParam.start > 67) || 
                 ImageProc::Mid_BF.LostParam.sum < 5) &&
                ((ImageProc::Right_BF.LostParam.sum < 30 && ImageProc::Right_BF.LostParam.start > 60) || 
                 ImageProc::Right_BF.LostParam.sum < 5) &&
                ((ImageProc::Left_BF.LostParam.sum < 30 && ImageProc::Left_BF.LostParam.start > 60) || 
                 ImageProc::Left_BF.LostParam.sum < 5) &&
                (ImageProc::Left_BF.Var < 5 || ImageProc::Right_BF.Var < 5) &&
                (abs(ImageProc::Mid_BF.Slope) < 0.1 && ImageProc::Mid_BF.Var < 60);
    }
    /**
     * @brief 出弯道短直道判断函数
     * 
     * @return true 
     * @return false
     */
    bool isShortStraightCondition2() {
        return ImageProc::state.cutoff > 55 &&
               ((ImageProc::Mid_BF.LostParam.sum < 10 && ImageProc::Mid_BF.LostParam.start > 67) || 
                ImageProc::Mid_BF.LostParam.sum < 2) &&
               ((ImageProc::Right_BF.LostParam.sum < 30 && ImageProc::Right_BF.LostParam.start > 60) || 
                ImageProc::Right_BF.LostParam.sum < 6) &&
               ((ImageProc::Left_BF.LostParam.sum < 30 && ImageProc::Left_BF.LostParam.start > 60) || 
                ImageProc::Left_BF.LostParam.sum < 6) &&
               (ImageProc::Left_BF.Var < 50 || ImageProc::Right_BF.Var < 50) &&
               (abs(ImageProc::Mid_BF.Slope) < 1.4 && ImageProc::Mid_BF.Var < 40);
    }
    /**
     * @brief 缓弯道判断函数
     * 
     * @return true 
     * @return false 
     */

    bool isCurveCondition() {
        const bool left_condition = 
            ((ImageProc::Left_BF.LostParam.sum > 2 && 
            ImageProc::Left_BF.LostParam.start < 50) || 
            (ImageProc::sec_loss_left.LostParam.count > 2 && 
            ImageProc::sec_loss_left.LostParam.start < 50))
             &&
            (ImageProc::Right_BF.LostParam.sum < 5 || 
            (ImageProc::Right_BF.LostParam.sum < 15 && 
            ImageProc::Right_BF.LostParam.start > 65));

        const bool right_condition = 
            ((ImageProc::Right_BF.LostParam.sum > 2 && 
            ImageProc::Right_BF.LostParam.start < 50) || 
            (ImageProc::sec_loss_right.LostParam.count > 2 && 
            ImageProc::sec_loss_right.LostParam.start < 50))
             &&
            (ImageProc::Left_BF.LostParam.sum < 5 || 
            (ImageProc::Left_BF.LostParam.sum < 15 && 
            ImageProc::Left_BF.LostParam.start > 65));

        return 
            (ImageProc::state.cutoff < 60) &&  
            (left_condition || right_condition) && 
            (ImageProc::Mid_BF.Var > 50);
    }
    /**
     * @brief 急弯道判断函数
     * 
     * @return true 
     * @return false 
     */
    bool isCurveCondition2() {

        const bool left_condition = 
            ((ImageProc::Left_BF.LostParam.sum > 3 && 
            ImageProc::Left_BF.LostParam.start < 60) ||
            ImageProc::Left_BF.LostParam.sum > 25)
            &&
            ((ImageProc::Right_BF.LostParam.sum < 15 && 
            ImageProc::Right_BF.LostParam.start > 65) ||
            ImageProc::Right_BF.LostParam.sum < 5) ;


        const bool right_condition = 
            ((ImageProc::Right_BF.LostParam.sum > 3 && 
            ImageProc::Right_BF.LostParam.start < 60) ||
            ImageProc::Right_BF.LostParam.sum > 25) 
             &&
            ((ImageProc::Left_BF.LostParam.sum < 15 &&
            ImageProc::Left_BF.LostParam.start > 65) ||
            ImageProc::Left_BF.LostParam.sum < 5) ;

        return 
            (ImageProc::state.cutoff < 55) && 
            ((left_condition && Error::Err.current > 25)|| (right_condition && Error::Err.current < -25)) &&
            (ImageProc::Mid_BF.Var < 70);
    }
    /**
     * @brief 路段分类
     * 
     * @return uint8_t 当前赛道类型
     * 
     * @note 1.合并元素检测条件 2.修正弯道判断逻辑 3.简化状态返回逻辑
     */
    uint8_t classifyRoadSegment(void)
    {
        static uint8_t lastType = SHORT;

        const int start = HEIGHT - 1;
        const int end = HEIGHT - ImageProc::state.cutoff + 2;


        // 计算中线特征参数
        ImageProc::Mid_BF.Slope = two_segment_fitting_int(start, end, 0, 0, 
                                Error::center_line, Error::fit_center_line);
        ImageProc::Mid_BF.Var = fangcha(start, end, 
                                Error::center_line, Error::fit_center_line);
        
        //return SHORT;
        //return CURVE;
        /*---------- 优先级从高到低判断 ----------*/
        // 特殊元素（立即更新lastType）
        if (Elem::Left_Island.State == 1 || Elem::Right_Island.State == 1 || Elem::Zebra.State != 0) {
            lastType = SHORT;
            return SHORT;
        }
        
        if (((Elem::Left_Island.State == 8 || Elem::Right_Island.State == 8) && abs(ImageProc::Mid_BF.Slope) < 1.5) ||
              Elem::Left_Island.State == 1 || Elem::Right_Island.State == 1 || Elem::Left_Island.State == 2 || Elem::Right_Island.State == 2){
            lastType = SHORT;
            return SHORT;
        }

        if ((Elem::Left_Island.State >= 3 && Elem::Left_Island.State <= 8) ||
            (Elem::Right_Island.State >= 3 && Elem::Right_Island.State <= 8)) {
            lastType = ISLAND;
            return ISLAND;
        }


        if(Elem::Cross.State != 0 && ImageProc::state.cutoff > 50 &&
            (abs(ImageProc::Mid_BF.Slope) < 1.3 && ImageProc::Mid_BF.Var < 60)){
            lastType = SHORT;
            return SHORT;
        }
        // 普通赛道类型（仅更新有效类型）

        if (isLongStraightCondition()) { // 长直道判断函数
            lastType = LOOG;
            return LOOG;
        }
        if (isCurveCondition()) {     // 缓弯道判断函数
            lastType = CURVE;
            return CURVE;
        }
        if (isCurveCondition2()) {    // 急弯道判断函数
            lastType = CURVE;
            return CURVE;
        }
        if (isShortStraightCondition1()) { // 短直道判断函数
            lastType = SHORT;
            return SHORT;
        }
        if (isShortStraightCondition2()) { // 短直道判断函数
            lastType = SHORT;
            return SHORT;
        }


        return lastType;

    }

}
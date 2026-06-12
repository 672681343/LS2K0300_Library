#include "zf_common_headfile.h"
using namespace cv;

void debug_log_printf_callback(const char* s) 
{
    if (s == NULL)  return;
    printf("%s", s);
}



void sigint_handler(int signum) 
{
    printf("收到Ctrl+C，程序即将退出\n");
    exit(0);
}

void cleanup()
{
    printf("程序异常退出，执行清理操作\n");
    Motor::deinit();
    BLDC::deinit();
    // 处理程序退出！！！
    // 这里需要关闭电机，关闭电调等。
}


int main(int, char**) 
{

    // 时间统计结构体--------------------------------
    // auto loop_start_time = std::chrono::high_resolution_clock::now();
    // auto loop_end_time = std::chrono::high_resolution_clock::now();
    // auto image_process_start_time = std::chrono::high_resolution_clock::now();
    // auto image_process_end_time = std::chrono::high_resolution_clock::now();
    // std::chrono::duration<double, std::milli> loop_time_elapsed;
    // std::chrono::duration<double, std::milli> image_process_time_elapsed;
    // 时间统计结构体--------------------------------


    // 注册清理函数
    atexit(cleanup);

    // 注册SIGINT信号的处理函数
    signal(SIGINT, sigint_handler);

    // 初始化UVC摄像头
    if(uvc_camera_init("/dev/video0") < 0)
    {
        return -1;
    }

    TCP::init();                                // 初始化逐飞助手
    Motor::Init();                              // 初始化电机
    Motor::goMotor(0, 0);                       // 电机清空
    BLDC::Init();                               // 初始化电调
    BLDC::Set_duty(0, 0);                       // 电调清空
    Key::Init();                                // 初始化按键
    PID::PID_init();                            // PID初始化
    MenuSystem::init();                         // 菜单初始化
    DataSaving::init();                         // 上位机初始化
    Pit::Init();                                // 中断初始化


    
    // // 日志控制变量
    int frame_count = 0;          // 帧计数器
    

    while(1)
    {

        //统计开始时间
        // loop_start_time = std::chrono::high_resolution_clock::now();

        frame_count++;  // 帧计数+1

        // 阻塞式等待，图像刷新
        if(wait_image_refresh() < 0)
        {
            // 摄像头未采集到图像，这里需要关闭电机，关闭电调等。
            exit(0);
        }

        Camera::basic_image_processing(gray_image);
        ImageProc::image_measurement(Camera::bin_pixels,Camera::lcd_pixels);
        Elem::elem_judge();
        Err::error_get(ImageProc::state, ImageProc::l_black, ImageProc::r_black, ImageProc::left_flags, ImageProc::right_flags);
        
        // ips200_show_gray_image(0, 0, rgay_image, UVC_WIDTH, UVC_HEIGHT);

        if (MenuSystem::Car_flag.init == 1  ){

            Control::BLDC.L = BLDC::duty;
            Control::BLDC.R = BLDC::duty;

            if(MenuSystem::Car_flag.start == 1){
                Control::RoadSegment_t = Control::classifyRoadSegment();
                Control::ControlOutput(Control::RoadSegment_t);
                DataSaving::saveTask(1);
                TCP::send_data();
            }
            Control::stop(0);
            BLDC::Set_duty(Control::BLDC.L, Control::BLDC.R);
        }
        Err::last_error_update();

        MenuSystem::key_shutdown();
        MenuSystem::Menu_display();

        process_image(rgb_image, frame_count);



        if(!boxes.empty()){
            Inference(rgb_image, frame_count);
        }



        //统计结束时间
        // loop_end_time = std::chrono::high_resolution_clock::now();

//---------------------------------时间统计--------------------------------
        // loop_time_elapsed = loop_end_time - loop_start_time;

        // //输出时间信息
        // std::cout << "主循环运行时间: " << loop_time_elapsed.count() << " 毫秒" << std::endl;
        
// ---------------------------------时间统计--------------------------------

        system_delay_ms(10);
        
    }
}

#include "zf_common_headfile.h"


void sigint_handler(int signum) 
{
    printf("收到Ctrl+C，程序即将退出\n");
    exit(0);
}

void cleanup()
{
    printf("程序异常退出，执行清理操作\n");
    // 处理程序退出！！！
    // 这里需要关闭电机，关闭电调等。
}


int main(int, char**) 
{

    // 注册清理函数
    atexit(cleanup);

    // 注册SIGINT信号的处理函数
    signal(SIGINT, sigint_handler);

    // 初始化屏幕
    ips200_init("/dev/fb0");

    // 初始化UVC摄像头
    if(uvc_camera_init("/dev/video0") < 0)
    {
        return -1;
    }



    Motor::Init();                              // 初始化电机
    Motor::goMotor(0, 0);                       // 电机清空
    Key::Init();                                // 初始化按键
    PID::PID_init();                            // PID初始化
    MenuSystem::init();                         // 菜单初始化
    Pit::Init();                                // 中断初始化

    while(1)
    {

        // 阻塞式等待，图像刷新
        if(wait_image_refresh() < 0)
        {
            // 摄像头未采集到图像，这里需要关闭电机，关闭电调等。
            exit(0);
        }

        Camera::basic_image_processing(rgay_image);
        ImageProc::image_measurement(Camera::bin_pixels,Camera::lcd_pixels);
        Elem::elem_judge();
        Error::error_get(ImageProc::state, ImageProc::l_black, ImageProc::r_black, ImageProc::left_flags, ImageProc::right_flags);
        // ips200_show_gray_image(0, 0, Camera::lcd_pixels, WIDTH, HEIGHT);

        if (MenuSystem::Car_flag.init == 1  ){

            if(MenuSystem::Car_flag.start == 1){
                Control::RoadSegment_t = Control::classifyRoadSegment();
                Control::ControlOutput(Control::RoadSegment_t);
                // DataSaving::saveTask(MenuSystem::Car_flag.saving);
                // TCP::send_data();
            }
            Control::stop(0);
        }

        Error::last_error_update();
        MenuSystem::key_shutdown();
        MenuSystem::Menu_display();
        
    }
}
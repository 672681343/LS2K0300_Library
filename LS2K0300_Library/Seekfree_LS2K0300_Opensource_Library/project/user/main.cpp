#include "zf_common_headfile.h"
using namespace cv;

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

    //摄像头初始化
    printf("===== 程序初始化 =====\n");
    if(uvc_camera_init("/dev/video0") < 0)
    {
        printf("❌ 摄像头初始化失败！\n");
        return -1;
    }
    printf("✅ 摄像头初始化成功\n");

    // 张量内存
    uint8_t tensor_arena[TENSOR_ARENA_SIZE];
    
    // 识别初始化（无返回值，通过全局变量访问）
    if (!init_tflite_simple(tensor_arena, TENSOR_ARENA_SIZE)) {
        printf("Failed to initialize TFLite!\n");
        return -1;
    }
  
    // ======================== 基础初始化 ========================
    TCP::init();
    Motor::Init();                              // 初始化电机
    Motor::goMotor(0, 0);                       // 电机清空
    Key::Init();                                // 初始化按键
    PID::PID_init();                            // PID初始化
    // MenuSystem::init();                         // 菜单初始化
    Pit::Init();                                // 中断初始化

    while(1)
    {

        //摄像头采集数据
        if(wait_image_refresh() < 0)
        {
            printf("❌ 第%d帧：摄像头采集异常，程序退出！\n", frame_count);
            exit(0);   // 摄像头采集异常，退出程序，防止程序卡死
        }
        //获取摄像头采集原始数据

        if(frame_rgb.empty())
        {
            printf("❌ 第%d帧：获取图像数据为空！\n", frame_count);
            continue;  // 跳过空帧，不退出程序
        }


        // ======================== 图像和控制处理 ========================

        Camera::basic_image_processing(rgay_image);
        ImageProc::image_measurement(Camera::bin_pixels,Camera::lcd_pixels);
        Elem::elem_judge();
        Err::error_get(ImageProc::state, ImageProc::l_black, ImageProc::r_black, ImageProc::left_flags, ImageProc::right_flags);
        
        // ips200_show_gray_image(0, 0, rgay_image, UVC_WIDTH, UVC_HEIGHT);

        if (MenuSystem::Car_flag.init == 1  ){

            if(MenuSystem::Car_flag.start == 1){
                Control::RoadSegment_t = Control::classifyRoadSegment();
                Control::ControlOutput(Control::RoadSegment_t);
                DataSaving::saveTask(1);
                TCP::send_data();
            }
            Control::stop(0);
        }

        Err::last_error_update();
        // MenuSystem::key_shutdown();
        // MenuSystem::Menu_display();

        process_image(rgb565_image, &frame_count);

        // ips200_show_rgb565_image(0,0,rgb565_image,160,120,60,20,0);


    

        
        // show_rgb565_image(0, 0, scc8660_image[0], SCC8660_W, SCC8660_H, 100, 100, 1);  //
        // ips200_show_rgb565_image(0, 0, rgb565_image, 160, 120, 160, 120, 0);//用这个

        ips200_show_rgb565_image(0, 0, rgb565_image, UVC_WIDTH, UVC_HEIGHT, 240, 240, 0);///显示彩色图像



        // ips200_show_gray_image(0, 0, rgay_image, 186, 70);
        
    }
}
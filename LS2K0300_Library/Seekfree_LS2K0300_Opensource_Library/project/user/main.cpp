
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
        ips200_show_gray_image(0, 0, Camera::lcd_pixels, WIDTH, HEIGHT);
        
    }
}
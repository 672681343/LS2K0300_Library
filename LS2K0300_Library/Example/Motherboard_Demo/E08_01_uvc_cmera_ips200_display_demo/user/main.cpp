/*********************************************************************************************************************
* LS2K0300 Opensourec Library 即（LS2K0300 开源库）是一个基于官方 SDK 接口的第三方开源库
* Copyright (c) 2022 SEEKFREE 逐飞科技
*
* 本文件是LS2K0300 开源库的一部分
*
* LS2K0300 开源库 是免费软件
* 您可以根据自由软件基金会发布的 GPL（GNU General Public License，即 GNU通用公共许可证）的条款
* 即 GPL 的第3版（即 GPL3.0）或（您选择的）任何后来的版本，重新发布和/或修改它
*
* 本开源库的发布是希望它能发挥作用，但并未对其作任何的保证
* 甚至没有隐含的适销性或适合特定用途的保证
* 更多细节请参见 GPL
*
* 您应该在收到本开源库的同时收到一份 GPL 的副本
* 如果没有，请参阅<https://www.gnu.org/licenses/>
*
* 额外注明：
* 本开源库使用 GPL3.0 开源许可证协议 以上许可申明为译文版本
* 许可申明英文版在 libraries/doc 文件夹下的 GPL3_permission_statement.txt 文件中
* 许可证副本在 libraries 文件夹下 即该文件夹下的 LICENSE 文件
* 欢迎各位使用并传播本程序 但修改内容时必须保留逐飞科技的版权声明（即本声明）
*
* 文件名称          main
* 公司名称          成都逐飞科技有限公司
* 适用平台          LS2K0300
* 店铺链接          https://seekfree.taobao.com/
*
* 修改记录
* 日期              作者           备注
* 2025-02-27        大W            first version
********************************************************************************************************************/

#include "zf_common_headfile.h"
#include "red_detecte.h"


#include <opencv2/imgproc/imgproc.hpp>  // for cv::cvtColor
#include <opencv2/highgui/highgui.hpp> // for cv::VideoCapture
#include <opencv2/opencv.hpp>
#include <signal.h>

// *************************** 例程硬件连接说明 ***************************
//  久久派与主板使用54pin排线连接，再将久久派插到主板上面，确保插到底核心板与主板插座间没有缝隙即可
//  久久派与主板使用54pin排线连接，再将久久派插到主板上面，确保插到底核心板与主板插座间没有缝隙即可
//  久久派与主板使用54pin排线连接，再将久久派插到主板上面，确保插到底核心板与主板插座间没有缝隙即可
//  使用本历程，就需要使用我们逐飞科技提供的内核。
// 
//  目前仅支持两寸SPI屏幕
//  SPI 两寸屏 硬件引脚
//  SCL         查看 seekfree_smart_cat_pai_99 文件中 st7789v 节点定义 默认 GPIO60
//  SDA         查看 seekfree_smart_cat_pai_99 文件中 st7789v 节点定义 默认 GPIO62
//  RST         查看 seekfree_smart_cat_pai_99 文件中 st7789v 节点定义 默认 GPIO74
//  DC          查看 seekfree_smart_cat_pai_99 文件中 st7789v 节点定义 默认 GPIO26
//  CS          查看 seekfree_smart_cat_pai_99 文件中 st7789v 节点定义 默认 GPIO62
//  BL          查看 seekfree_smart_cat_pai_99 文件中 st7789v 节点定义 默认 GPIO63
//  GND         核心板电源地 GND
//  3V3         核心板 3V3 电源
//
// 使用USB插入UVC摄像头
// 
// *************************** 例程测试说明 ***************************
// 1.核心板烧录本例程 插在主板上 2寸IPS 显示模块插在主板的屏幕接口排座上 请注意引脚对应 不要插错
// 
// 2.电池供电 上电后 2寸IPS 屏幕亮起 显示数字等信息
// 
// 3.判断屏幕为SPI屏幕或者并口屏幕: 查看屏幕背面的PCB丝印，如果带有SPI字样就为SPI屏幕，否则为并口屏幕
//   例程文件夹下有不同类型2寸屏幕的区别示意图 "SPI和并口2寸屏幕区别.png"
// 
// 4.运行例程，既可看到灰度图像显示到屏幕上
//
// 如果发现现象与说明严重不符 请参照本文件最下方 例程常见问题说明 进行排查
//
// **************************** 代码区域 ****************************

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


//     lq_camera_ex cam(320, 240, 90,LQ_CAMERA_0CPU_MJPG,"/dev/video0");

//     // 检查状态
//     if (!cam.is_cam_opened()) {
//         std::cerr << "摄像头初始化失败，请检查 /dev/video0 是否存在" << std::endl;
//         return -1;
//     }

//     std::cout << "摄像头已启动: " 
//               << cam.get_camera_width() << "x" 
//               << cam.get_camera_height() 
//               << " @ " << cam.get_camera_fps() << "fps" << std::endl;

//     // ========== 循环采集 ==========
//     while (1)
//     {
//         // 获取原始彩色图像
//         cv::Mat raw = cam.get_frame_raw();
//         cv::Mat frame_rgb565;
//         cv::cvtColor(raw, frame_rgb565, cv::COLOR_BGR2BGR565);

//         const uint16_t* ptr = reinterpret_cast<const uint16_t*>(frame_rgb565.data);
        
//         // 或者获取灰度图像
//         // cv::Mat gray = cam.get_frame_gray();
        
//         // 或者同时获取彩色+灰度（效率更高，只读一次硬件）
//         // cv::Mat raw, gray;
//         // cam.get_frame_raw_gray(raw, gray);

//         if (raw.empty()) {
//             std::cerr << "获取图像失败" << std::endl;
//             exit(0);
//         }

//         // ips200_show_gray_image(0, 0, raw.data, UVC_WIDTH, UVC_HEIGHT);
//         // ips200_show_rgb565_image(0,0,rgb565_image,320,240,60,20,0);


//     }

//     // 析构时会自动停止采集、释放资源
//     return 0;
// }


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

        process_image(rgb_image);
        // // 显示图像到屏幕上
        // ips200_show_gray_image(0, 0, gray_image, UVC_WIDTH, UVC_HEIGHT);
        // ips200_show_rgb565_image(0,0,rgb565_image,320,240,60,20,0);




    }
}
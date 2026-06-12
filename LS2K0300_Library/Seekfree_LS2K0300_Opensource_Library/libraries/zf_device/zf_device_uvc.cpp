// #include "zf_device_uvc.h"


// #include <opencv2/imgproc/imgproc.hpp>  // for cv::cvtColor
// #include <opencv2/highgui/highgui.hpp> // for cv::VideoCapture
// #include <opencv2/opencv.hpp>

// #include <iostream> // for std::cerr
// #include <fstream>  // for std::ofstream
// #include <iostream>
// #include <thread>
// #include <chrono>
// #include <atomic>

// using namespace cv;

// cv::Mat frame_rgb;      // 构建opencv对象 彩色
// cv::Mat frame_rgay;     // 构建opencv对象 灰度
// cv::Mat frame_rgb565;     // 构建opencv对象 彩色565

// uint8_t *rgay_image;    // 灰度图像数组指针

// uint16_t *rgb565_image;

// VideoCapture cap;

// int8 uvc_camera_init(const char *path)
// {
//     cap.open(path);

//     if(!cap.isOpened())
//     {
//         printf("find uvc camera error.\r\n");
//         return -1;
//     } 
//     else 
//     {
//         printf("find uvc camera Successfully.\r\n");
//     }

    
//     cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('Y', 'V', 'Y', '2'));  // 设置格式
//     // cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M', 'J', 'P', 'G'));  // 设置格式
//     cap.set(CAP_PROP_FRAME_WIDTH, UVC_WIDTH);                           // 设置摄像头宽度
//     cap.set(CAP_PROP_FRAME_HEIGHT, UVC_HEIGHT);                         // 设置摄像头高度
//     cap.set(CAP_PROP_FPS, UVC_FPS);                                     // 显示屏幕帧率

//     printf("get uvc width = %f.\r\n",  cap.get(CAP_PROP_FRAME_WIDTH));
//     printf("get uvc height = %f.\r\n", cap.get(CAP_PROP_FRAME_HEIGHT));
//     printf("get uvc fps = %f.\r\n",    cap.get(CAP_PROP_FPS));

//     printf("Actual: %.0fx%.0f @ %.0f FPS\n", 
//             cap.get(CAP_PROP_FRAME_WIDTH),
//             cap.get(CAP_PROP_FRAME_HEIGHT),
//             cap.get(CAP_PROP_FPS));

//     return 0;
// }


// int8 wait_image_refresh()
// {
//     try 
//     {
//         // 阻塞式等待图像刷新
//         cap >> frame_rgb;
//         // cap.read(frame_rgb);
//         if (frame_rgb.empty()) 
//         {
//             std::cerr << "未获取到有效图像帧" << std::endl;
//             return -1;
//         }
//     } 
//     catch (const cv::Exception& e) 
//     {
//         std::cerr << "OpenCV 异常: " << e.what() << std::endl;
//         return -1;
//     }

//     // rgb转灰度
//     cv::cvtColor(frame_rgb, frame_rgay, cv::COLOR_BGR2GRAY);

//     cv::cvtColor(frame_rgb, frame_rgb565, cv::COLOR_BGR2BGR565);

//     // cv对象转指针
//     rgay_image = reinterpret_cast<uint8_t *>(frame_rgay.ptr(0));

//     rgb565_image = reinterpret_cast<uint16_t *>(frame_rgb565.ptr(0));


//     return 0;
// }



#include "zf_device_uvc.h"


#include <opencv2/imgproc/imgproc.hpp>  // for cv::cvtColor

#include <opencv2/opencv.hpp>
#include "lq_camera_ex.hpp"

#include <iostream> // for std::cerr
#include <fstream>  // for std::ofstream
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

using namespace cv;

cv::Mat frame_rgb;      // 构建opencv对象 彩色
cv::Mat frame_gray;     // 构建opencv对象 灰度
// cv::Mat frame_rgb565;     // 构建opencv对象 彩色565

uint8_t *gray_image;    // 灰度图像数组指针

uint8_t *rgb_image;



std::unique_ptr<lq_camera_ex> cam;



int8 uvc_camera_init(const char *path)
{

    frame_rgb.create(UVC_HEIGHT, UVC_WIDTH, CV_8UC3);
    frame_gray.create(UVC_HEIGHT, UVC_WIDTH, CV_8UC1);
    cam.reset(new lq_camera_ex(UVC_WIDTH, UVC_HEIGHT, UVC_FPS, LQ_CAMERA_0CPU_MJPG, "/dev/video0"));  //LQ_CAMERA_HIGH_MJPG  LQ_CAMERA_0CPU_MJPG

    // 检查状态
    if (!cam->is_cam_opened()) {
        std::cerr << "摄像头初始化失败，请检查 /dev/video0 是否存在" << std::endl;
        return -1;
    }

    std::cout << "摄像头已启动: " 
              << cam->get_camera_width() << "x" 
              << cam->get_camera_height() 
              << " @ " << cam->get_camera_fps() << "fps" << std::endl;


    return 0;
}


int8 wait_image_refresh()
{


    // frame_rgb = cam.get_frame_raw();

    cam->get_frame_raw_gray(frame_rgb, frame_gray);

    if (frame_rgb.empty()) {
        std::cerr << "获取图像失败" << std::endl;
        return -1;
    }



    // cv::cvtColor(frame_rgb, frame_rgb565, cv::COLOR_BGR2BGR565);

    // cv对象转指针
    gray_image = reinterpret_cast<uint8_t *>(frame_gray.ptr(0));

    rgb_image = reinterpret_cast<uint8_t *>(frame_rgb.ptr(0));


    return 0;
}




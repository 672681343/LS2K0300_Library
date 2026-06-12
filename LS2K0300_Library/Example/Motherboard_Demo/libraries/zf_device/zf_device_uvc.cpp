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
cv::Mat frame_rgb565;     // 构建opencv对象 彩色565

uint8_t *gray_image;    // 灰度图像数组指针

uint8_t *rgb_image;



std::unique_ptr<lq_camera_ex> cam;



int8 uvc_camera_init(const char *path)
{

    frame_rgb.create(UVC_HEIGHT, UVC_WIDTH, CV_8UC3);
    frame_rgb565.create(UVC_HEIGHT, UVC_WIDTH, CV_8UC2);
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




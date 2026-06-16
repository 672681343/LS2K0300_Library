
// #include "zf_device_uvc.h"


// #include <opencv2/imgproc/imgproc.hpp>  // for cv::cvtColor

// #include <opencv2/opencv.hpp>
// #include "lq_camera_ex.hpp"

// #include <iostream> // for std::cerr
// #include <fstream>  // for std::ofstream
// #include <iostream>
// #include <thread>
// #include <chrono>
// #include <atomic>

// using namespace cv;

// cv::Mat frame_rgb;      // 构建opencv对象 彩色
// cv::Mat frame_rgb_120;
// cv::Mat frame_gray;     // 构建opencv对象 灰度

// cv::Mat gray_320; // 320x240 灰度中间结果

// uint8_t *gray_image;    // 灰度图像数组指针

// uint8_t *rgb_image;

// uint8_t *rgb_120_image;

// std::unique_ptr<lq_camera_ex> cam;



// int8 uvc_camera_init(const char *path)
// {


//     cam.reset(new lq_camera_ex(UVC_WIDTH, UVC_HEIGHT, UVC_FPS, LQ_CAMERA_0CPU_MJPG, "/dev/video0"));  //LQ_CAMERA_HIGH_MJPG  LQ_CAMERA_0CPU_MJPG

//     // 检查状态
//     if (!cam->is_cam_opened()) {
//         std::cerr << "摄像头初始化失败，请检查 /dev/video0 是否存在" << std::endl;
//         return -1;
//     }

//     std::cout << "摄像头已启动: " 
//               << cam->get_camera_width() << "x" 
//               << cam->get_camera_height() 
//               << " @ " << cam->get_camera_fps() << "fps" << std::endl;

//     // 预分配内存
//     frame_rgb.create(UVC_HEIGHT, UVC_WIDTH, CV_8UC3); // 240x320 彩色
//     frame_rgb_120.create(120, 160, CV_8UC3); // 120x160 彩色
//     frame_gray.create(120, 160, CV_8UC1);             // 120x160 灰度
//     gray_320.create(UVC_HEIGHT, UVC_WIDTH, CV_8UC1);

//     return 0;
// }


// int8 wait_image_refresh()
// {

//     auto t1 = std::chrono::high_resolution_clock::now();

//     cv::Mat frame_tmp = cam->get_frame_raw();

//     auto t2 = std::chrono::high_resolution_clock::now();


//     if (frame_tmp.empty())
//     {
//         std::cerr << "获取图像失败" << std::endl;
//         return -1;
//     }

//     if (frame_tmp.isContinuous() && frame_rgb.isContinuous())
//     {
//         memcpy(frame_rgb.data, frame_tmp.data, frame_tmp.total() * frame_tmp.elemSize());
//     }
//     else
//     {
//         frame_tmp.copyTo(frame_rgb);
//     }

//     auto t3 = std::chrono::high_resolution_clock::now();

//     // 160x120 灰度
//     for (int y = 0; y < 120; y++)
//     {
//         for (int x = 0; x < 160; x++)
//         {
//             // 取 2x2 块的平均，或只取左上角像素
//             cv::Vec3b pixel = frame_rgb.at < cv::Vec3b > (y * 2, x * 2);
//             // 灰度 = 0.299*R + 0.587*G + 0.114*B（简化版）
//             uint8_t gray = (pixel[2] * 77 + pixel[1] * 150 + pixel[0] * 29) >> 8;
//             frame_gray.at<uint8_t>(y, x) = gray;
//         }
//     }

//     auto t4 = std::chrono::high_resolution_clock::now();

//     cv::resize(frame_rgb, frame_rgb_120, cv::Size(160, 120));

//     auto t5 = std::chrono::high_resolution_clock::now();

// #define MS(ta, tb) std::chrono::duration<double, std::milli>(tb - ta).count()
//     printf("get_raw: %.2f, memcpy: %.2f, gray: %.2f, resize: %.2f, total: %.2f\n",
//            MS(t1, t2), MS(t2, t3), MS(t3, t4), MS(t4, t5), MS(t1, t5));

//     rgb_image = frame_rgb.ptr<uint8_t>(0);
//     rgb_120_image = frame_rgb_120.ptr<uint8_t>(0);
//     gray_image = frame_gray.ptr<uint8_t>(0);

//     return 0;
// }

// #include <opencv2/highgui/highgui.hpp>
// #include <opencv2/imgproc/imgproc.hpp>
// #include <opencv2/opencv.hpp>
// #include <iostream>
// #include <stdint.h>
// #include <chrono>

// using namespace cv;

// cv::Mat frame_rgb;     // 320x240 BGR（直接从摄像头取）
// cv::Mat frame_rgb_160; // 160x120 BGR
// cv::Mat frame_gray;    // 160x120 灰度

// uint8_t *rgb_image;
// uint8_t *rgb_120_image;
// uint8_t *gray_image;

// cv::VideoCapture cap;

// int8_t uvc_camera_init(const char *path)
// {
//     cap.open("/dev/video0");
//     if (!cap.isOpened())
//     {
//         std::cerr << "摄像头打开失败" << std::endl;
//         return -1;
//     }

//     // 尝试设置 YUYV，但 OpenCV 可能自动转 BGR
//     cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('Y', 'U', 'Y', 'V'));
//     cap.set(cv::CAP_PROP_FRAME_WIDTH, 320);
//     cap.set(cv::CAP_PROP_FRAME_HEIGHT, 240);
//     cap.set(cv::CAP_PROP_FPS, 60);

//     // 打印实际参数
//     double actual_w = cap.get(cv::CAP_PROP_FRAME_WIDTH);
//     double actual_h = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
//     double actual_fps = cap.get(cv::CAP_PROP_FPS);
//     int actual_fourcc = cap.get(cv::CAP_PROP_FOURCC);
//     int actual_type = -1;

//     // 取一帧看实际格式
//     cv::Mat test_frame;
//     cap >> test_frame;
//     if (!test_frame.empty())
//     {
//         actual_type = test_frame.type();
//     }

//     printf("设置: YUYV 320x240@60\n");
//     printf("实际: %.0fx%.0f @ %.0f FPS, FourCC=%c%c%c%c, type=%d (CV_8UC2=%d, CV_8UC3=%d)\n",
//            actual_w, actual_h, actual_fps,
//            actual_fourcc & 0xFF, (actual_fourcc >> 8) & 0xFF,
//            (actual_fourcc >> 16) & 0xFF, (actual_fourcc >> 24) & 0xFF,
//            actual_type, CV_8UC2, CV_8UC3);

//     // 预分配
//     frame_rgb.create(240, 320, CV_8UC3);
//     frame_rgb_160.create(120, 160, CV_8UC3);
//     frame_gray.create(120, 160, CV_8UC1);

//     return 0;
// }

// int8_t wait_image_refresh()
// {
//     auto t1 = std::chrono::high_resolution_clock::now();

//     // 直接取 BGR（OpenCV 已转换）
//     cv::Mat frame_tmp;
//     cap >> frame_tmp;

//     auto t2 = std::chrono::high_resolution_clock::now();

//     if (frame_tmp.empty())
//     {
//         std::cerr << "获取图像失败" << std::endl;
//         return -1;
//     }

//     // 快速 memcpy（BGR 320x240，3通道，连续内存）
//     if (frame_tmp.isContinuous())
//     {
//         memcpy(frame_rgb.data, frame_tmp.data, 320 * 240 * 3);
//     }
//     else
//     {
//         // 逐行复制
//         for (int y = 0; y < 240; y++)
//         {
//             memcpy(frame_rgb.ptr(y), frame_tmp.ptr(y), 320 * 3);
//         }
//     }

//     auto t3 = std::chrono::high_resolution_clock::now();

//     // 快速灰度：直接从 BGR 下采样（指针版，无 at<>）
//     for (int y = 0; y < 120; y++)
//     {
//         uint8_t *gray_row = frame_gray.ptr<uint8_t>(y);
//         int src_y = y * 2;

//         for (int x = 0; x < 160; x++)
//         {
//             int src_x = x * 2;
//             uint8_t *rgb_ptr = frame_rgb.ptr<uint8_t>(src_y, src_x * 3);
//             // BGR 顺序：rgb_ptr[0]=B, [1]=G, [2]=R
//             gray_row[x] = (rgb_ptr[2] * 77 + rgb_ptr[1] * 150 + rgb_ptr[0] * 29) >> 8;
//         }
//     }

//     auto t4 = std::chrono::high_resolution_clock::now();

//     // BGR 320x240 → BGR 160x120
//     cv::resize(frame_rgb, frame_rgb_160, cv::Size(160, 120));

//     auto t5 = std::chrono::high_resolution_clock::now();

// // 计时
// #define MS(ta, tb) std::chrono::duration<double, std::milli>(tb - ta).count()
//     printf("capture: %.2f, memcpy: %.2f, gray: %.2f, resize: %.2f, total: %.2f\n",
//            MS(t1, t2), MS(t2, t3), MS(t3, t4), MS(t4, t5), MS(t1, t5));

//     rgb_image = frame_rgb.ptr<uint8_t>(0);
//     rgb_120_image = frame_rgb_160.ptr<uint8_t>(0);
//     gray_image = frame_gray.ptr<uint8_t>(0);


//     return 0;
// }

#include "zf_device_uvc.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include "lq_camera_ex.hpp"
#include <iostream>
#include <chrono>

// #define MS(ta, tb) std::chrono::duration<double, std::milli>(tb - ta).count()

using namespace cv;

cv::Mat frame_rgb;
cv::Mat frame_rgb_120;
cv::Mat frame_gray;

uint8_t *gray_image;
uint8_t *rgb_image;
uint8_t *rgb_120_image;

std::unique_ptr < lq_camera_ex > cam;

int8 uvc_camera_init(const char *path)
{
    cam.reset(new lq_camera_ex(UVC_WIDTH, UVC_HEIGHT, UVC_FPS, // LQ_CAMERA_HIGH_MJPG  //LQ_CAMERA_0CPU_MJPG
                               LQ_CAMERA_0CPU_MJPG, "/dev/video0"));

    if (!cam->is_cam_opened())
    {
        std::cerr << "摄像头初始化失败" << std::endl;
        return -1;
    }

    std::cout << "摄像头: " << cam->get_camera_width() << "x"
              << cam->get_camera_height() << " @ "
              << cam->get_camera_fps() << "fps" << std::endl;

    frame_rgb.create(UVC_HEIGHT, UVC_WIDTH, CV_8UC3);
    frame_rgb_120.create(120, 160, CV_8UC3);
    frame_gray.create(120, 160, CV_8UC1);

    return 0;
}

void fast_downsample_bgr(const uint8_t *src, uint8_t *dst, int src_w, int src_h)
{
    int dst_w = src_w / 2;
    int dst_h = src_h / 2;

    for (int y = 0; y < dst_h; y++)
    {
        const uint8_t *src_row = src + (y * 2) * src_w * 3;
        uint8_t *dst_row = dst + y * dst_w * 3;

        for (int x = 0; x < dst_w; x++)
        {
            int src_x = x * 2 * 3;
            dst_row[x * 3] = src_row[src_x];         // B
            dst_row[x * 3 + 1] = src_row[src_x + 1]; // G
            dst_row[x * 3 + 2] = src_row[src_x + 2]; // R
        }
    }
}

int8 wait_image_refresh()
{
    // auto t1 = std::chrono::high_resolution_clock::now();

    // 阻塞等待，直到拿到帧（即使 15-20ms）
    cv::Mat frame_tmp = cam->get_frame_raw();
    // auto t2 = std::chrono::high_resolution_clock::now();
    // double get_ms = MS(t1, t2);

    if (frame_tmp.empty())
    {
        std::cerr << "获取图像失败" << std::endl;
        return -1;
    }

    // memcpy
    if (frame_tmp.isContinuous() && frame_rgb.isContinuous())
    {
        memcpy(frame_rgb.data, frame_tmp.data, 320 * 240 * 3);
    }
    else
    {
        for (int y = 0; y < 240; y++)
        {
            memcpy(frame_rgb.ptr(y), frame_tmp.ptr(y), 320 * 3);
        }
    }
    // auto t3 = std::chrono::high_resolution_clock::now();

    // 灰度
    for (int y = 0; y < 120; y++)
    {
        uint8_t *gray_row = frame_gray.ptr<uint8_t>(y);
        int src_y = y * 2;
        uint8_t *rgb_row = frame_rgb.ptr<uint8_t>(src_y);

        for (int x = 0; x < 160; x++)
        {
            int src_x = x * 2 * 3;
            uint8_t b = rgb_row[src_x];
            uint8_t g = rgb_row[src_x + 1];
            uint8_t r = rgb_row[src_x + 2];
            gray_row[x] = (r * 77 + g * 150 + b * 29) >> 8;
        }
    }
    // auto t4 = std::chrono::high_resolution_clock::now();

    // resize（手动下采样，稳定）
    fast_downsample_bgr(frame_rgb.ptr<uint8_t>(0), frame_rgb_120.ptr<uint8_t>(0), 320, 240);
    // auto t5 = std::chrono::high_resolution_clock::now();

    // // 计时
    // printf("get_raw: %.2f, memcpy: %.2f, gray: %.2f, resize: %.2f, total: %.2f\n",
    //        get_ms, MS(t2, t3), MS(t3, t4), MS(t4, t5), MS(t1, t5));

    rgb_image = frame_rgb.ptr<uint8_t>(0);
    rgb_120_image = frame_rgb_120.ptr<uint8_t>(0);
    gray_image = frame_gray.ptr<uint8_t>(0);


    

    return 0;
}
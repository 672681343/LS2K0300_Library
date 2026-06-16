#ifndef _zf_driver_uvc_h
#define _zf_driver_uvc_h


#include "zf_common_typedef.h"
#include <opencv2/opencv.hpp>


#define UVC_WIDTH   320
#define UVC_HEIGHT  240
#define UVC_FPS     60

int8_t uvc_camera_init(const char *path);
int8_t wait_image_refresh();


extern uint8_t *gray_image;
extern uint8_t *rgb_image;
extern uint8_t *rgb_120_image;

extern cv::Mat frame_rgb;      
extern cv::Mat frame_gray;   
extern cv::Mat frame_120;


#endif

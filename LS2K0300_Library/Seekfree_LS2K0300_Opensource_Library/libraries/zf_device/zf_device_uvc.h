#ifndef _zf_driver_uvc_h
#define _zf_driver_uvc_h


#include "zf_common_typedef.h"
#include <opencv2/opencv.hpp>


#define UVC_WIDTH   160
#define UVC_HEIGHT  120
#define UVC_FPS     60

#define UVC_WIDTH_H   320
#define UVC_HEIGHT_H  240

int8 uvc_camera_init(const char *path);
int8 wait_image_refresh();


extern uint8_t *gray_image;
extern uint8_t *rgb_image;

extern cv::Mat frame_rgb;      
extern cv::Mat frame_gray;    


#endif

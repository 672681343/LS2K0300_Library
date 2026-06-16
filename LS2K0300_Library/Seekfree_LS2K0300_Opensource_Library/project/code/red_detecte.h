
#pragma once  // 防止重复包含
#include "zf_common_headfile.h"
#include <opencv2/opencv.hpp>
#include "lq_ntp.hpp"
#include <cstdint>
#include <vector>

// 检测结果结构体
struct RedBox {
    int x, y;       // 左上角坐标
    int w, h;       // 宽度和高度
    int area;       // 像素面积
    float density;  // 填充密度
};

// 图像尺寸（根据你的IPS200屏幕修改）
#define IMG_WIDTH 320
#define IMG_HEIGHT 240

#define MODEL_INPUT_WIDTH          32 //  定义模型的输入宽度为48像素
#define MODEL_INPUT_HEIGHT         32 //  定义模型的输入高度为48像素

bool is_red_pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t min_red = 120, uint8_t min_diff = 8);

void detect_red_regions(uint8_t* image, int width, int height,
                       std::vector<RedBox>& boxes,
                       int min_area, int max_area);

void detect_red_regions_H(uint8_t *image, int width, int height,
                        std::vector<RedBox> &boxes,
                        int min_area, int max_area);

void draw_rect_bgr888(uint8_t* image, int width, int height,
                     int x, int y, int w, int h,
                     uint32_t color, int thickness);

void draw_cross_bgr888(uint8_t* image, int width, int height,
                      int cx, int cy, uint32_t color, int size);

uint8_t* crop_image(uint8_t* src_image, int src_width, int src_height,
                    int x, int y, int w, int h);

cv::Mat crop_image_to_bgr_mat(uint8_t* src_image, int src_width, int src_height,
                               int x, int y, int w, int h);

void save_bgr888_as_bmp(const char* filename, uint8_t* bgr, int w, int h, int stride);

void process_image(uint8_t* bgr888_image, int frame_count);

extern std::vector<RedBox> boxes;

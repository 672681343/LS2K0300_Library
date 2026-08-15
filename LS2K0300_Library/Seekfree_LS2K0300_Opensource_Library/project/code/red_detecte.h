#pragma once  // 防止重复包含

#include <cstdint>
#include <vector>



struct RedPoint2f {
    float x;
    float y;
};

// 检测结果结构体
struct RedBox {
    int x, y;       // 左上角坐标
    int w, h;       // 宽度和高度
    int area;       // 像素面积
    float density;  // 填充密度

        // === 新增字段（旋转矩形 OBB）===
    float cx, cy;       // 旋转中心
    float obb_w, obb_h; // 旋转矩形宽高
    float angle;        // 弧度，范围 [-π/2, π/2]
    int max_y;
};

bool is_red_pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t min_red = 120, uint8_t min_diff = 8);

void detect_red_regions(uint8_t* image, int width, int height,
                       std::vector<RedBox>& boxes,
                       int min_area, int max_area);

void draw_rect_bgr888(uint8_t* image, int width, int height,
                     int x, int y, int w, int h,
                     uint32_t color, int thickness);

void draw_cross_bgr888(uint8_t* image, int width, int height,
                      int cx, int cy, uint32_t color, int size);

uint8_t* crop_image(uint8_t* src_image, int src_width, int src_height,
                    int x, int y, int w, int h);

void save_bgr888_as_bmp(const char* filename, uint8_t* bgr, int w, int h, int stride);

void get_rotated_box_corners(const RedBox& box, RedPoint2f corners[4]);

void draw_rotated_rect_bgr888(uint8_t* image, int width, int height,
                              const RedPoint2f corners[4],
                              uint32_t color, int thickness);

void process_image(uint8_t* bgr888_image, int* frame_count);

extern std::vector<RedBox> boxes;

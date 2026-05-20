#pragma once  // 防止重复包含

#include <cstdint>
#include <vector>

// 检测结果结构体
struct RedBox {
    int x, y;       // 左上角坐标
    int w, h;       // 宽度和高度
    int area;       // 像素面积
    float density;  // 填充密度
};

// 函数声明（默认参数只在这里写一次）
bool is_red_pixel(uint16_t pixel, uint8_t min_red = 18, uint8_t min_diff = 4);
void detect_red_regions(uint16_t* image, int width, int height,
                       std::vector<RedBox>& boxes,
                       int min_area = 30, int max_area = 10000);
void draw_rect_rgb565(uint16_t* image, int width, int height,
                     int x, int y, int w, int h, 
                     uint16_t color, int thickness = 1);
void draw_cross_rgb565(uint16_t* image, int width, int height,
                      int cx, int cy, uint16_t color, int size = 5);
void process_image(uint16_t* rgb565_image);

extern uint16_t* cropped;

extern std::vector<RedBox> boxes;


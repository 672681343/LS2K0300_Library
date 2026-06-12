#ifndef RED_DETECTE_H
#define RED_DETECTE_H

#include <vector>
#include <cstdint>

struct RedBox {
    int x, y, w, h;
    int area;
    float density;
};

bool is_red_pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t min_red = 20, uint8_t min_diff = 8);

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

// 纯C实现，保存为BMP（无OpenCV依赖）
void save_bgr888_as_bmp(const char* filename, uint8_t* bgr, int w, int h);

void process_image(uint8_t* bgr888_image);

#endif
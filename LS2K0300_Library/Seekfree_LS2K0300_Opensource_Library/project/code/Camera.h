#pragma once
#include "zf_common_headfile.h"


namespace Camera {
    // 外部可见的全局状态
    extern uint8_t threshold;
    extern int8_t threshold_camp;          // 补偿值         
    extern uint16_t outlier_count;     
    
    extern uint8_t bin_pixels[WIDTH*HEIGHT];
    extern uint8_t lcd_pixels[WIDTH*HEIGHT];
    // 核心功能接口
    #if CAMERA_MODE
        void basic_image_processing(uint8_t* image);
        uint8_t calculate_otsu_threshold(uint8_t* image);
        void binarize_image(uint8_t* image, uint8_t threshold);
        void stretching_image(uint8_t* original_image);
    #else
        void basic_image_processing(uint8_t* image);
    #endif
    uint8_t adjust_threshold(uint8_t base, int8_t comp, uint8_t min, uint8_t max);
}
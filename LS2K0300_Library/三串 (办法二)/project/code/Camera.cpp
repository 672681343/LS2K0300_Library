#include "Camera.h"
#include <opencv2/opencv.hpp>
#include <array>
#include <algorithm>
#include <cstdint>

namespace Camera {
    // 全局变量定义
    uint8_t threshold = 128;                // 默认阈值(补偿后的值)
    int8_t threshold_camp = 0;             // 阈值补偿值
    uint16_t outlier_count = 0;             // 出界点计数
    
    uint8_t bin_pixels[WIDTH * HEIGHT]{};   // 受处理图像
    uint8_t lcd_pixels[WIDTH * HEIGHT]{};   // 可视化图像
    uint8_t huidu[WIDTH * HEIGHT]{};

        uint8_t original_pixels[UVC_WIDTH * UVC_HEIGHT]{};
        void basic_image_processing(uint8_t* image) {
            // 阈值计算流程
            threshold = calculate_otsu_threshold(image);
            // 阈值调整
            threshold = adjust_threshold(threshold, threshold_camp, 0, 255);
            // 二值化处理
            binarize_image(image, threshold);

        }
        // Otsu算法实现
        uint8_t calculate_otsu_threshold(uint8_t* image) {
            constexpr size_t PIXEL_SAMPLE_STRIDE = 2;  // 下采样步长
            constexpr size_t GRAYSCALE_LEVELS = 256;
            
            std::array<uint32_t, GRAYSCALE_LEVELS> histogram{};  
            uint8_t min_val = 255, max_val = 0;
            uint32_t pixel_sum = 0;

            // 带下采样的直方图统计
            for (size_t y = 0; y < HEIGHT; y += PIXEL_SAMPLE_STRIDE) {
                for (size_t x = 0; x < UVC_WIDTH; x += PIXEL_SAMPLE_STRIDE) {
                    const auto val = image[y * UVC_WIDTH + x];
                    ++histogram[val];
                    min_val = std::min(min_val, val);
                    max_val = std::max(max_val, val);
                    pixel_sum += val;
                }
            }

            const float total_pixels = (UVC_WIDTH * HEIGHT) / 
                                    (PIXEL_SAMPLE_STRIDE * PIXEL_SAMPLE_STRIDE);
            float max_variance = 0.0f;
            uint8_t optimal_thresh = min_val;
            
            float w0 = 0.0f, u0 = 0.0f;
            const float mean = pixel_sum / total_pixels;

            // Otsu算法优化实现
            for (size_t t = min_val; t <= max_val; ++t) {
                const float prob = histogram[t] / total_pixels;
                w0 += prob;
                const float u0_tmp = u0 + t * prob;

                if (w0 < 1e-6f || w0 > 1.0f - 1e-6f) continue;

                const float w1 = 1.0f - w0;
                const float u1_tmp = (mean - u0_tmp) / w1;
                const float variance = w0 * w1 * (u0_tmp/w0 - u1_tmp) * (u0_tmp/w0 - u1_tmp);

                if (variance > max_variance) {
                    max_variance = variance;
                    optimal_thresh = t;
                }
                u0 = u0_tmp;
            }

            return optimal_thresh;
        }

    void binarize_image(uint8_t* image, uint8_t threshold) {
        const uint32_t sx = (UVC_WIDTH  << 16) / WIDTH;   // 列方向
        for (int i = 0; i < HEIGHT; ++i) {
            for (int j = 0; j < WIDTH; ++j) {
                uint32_t xs = (j * sx) >> 16;            // 源列坐标
                if (xs >= UVC_WIDTH)  xs = UVC_WIDTH - 1;
                uint8_t pv = image[i * UVC_WIDTH + xs];
                bin_pixels[i * WIDTH + j] = (pv > threshold) ? 255 : 0;
                lcd_pixels[i * WIDTH + j] = (pv > threshold) ? 255 : 0;
                huidu[i * WIDTH + j] = pv;
            }
        }
    }

    uint8_t adjust_threshold(uint8_t base, int8_t comp, 
        uint8_t min_val, uint8_t max_val) {
        const int16_t adjusted = static_cast<int16_t>(base) + comp; 
        return static_cast<uint8_t>(
            std::clamp(adjusted, 
            static_cast<int16_t>(min_val), 
            static_cast<int16_t>(max_val))
        );
    }
} 
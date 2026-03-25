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

    #if CAMERA_MODE
        uint8_t original_pixels[UVC_WIDTH * UVC_HEIGHT]{};
        void basic_image_processing(uint8_t* image) {
            // 阈值计算流程
            threshold = calculate_otsu_threshold(image);
            // 阈值调整
            threshold = adjust_threshold(threshold, 14, 0, 255);
            // 二值化处理
            binarize_image(image, threshold);
            // 图像拉伸
            stretching_image(image);
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
            // 带边界检查的并行化处理
            constexpr uint8_t OUTLIER_CHECK_ROWS = 3;
            outlier_count = 0;
            for (size_t y = 0; y < HEIGHT; ++y) {
                uint8_t* original_row = &original_pixels[y * UVC_WIDTH];
                const uint8_t* img_row = &image[y * UVC_WIDTH];
                for (size_t x = 0; x < UVC_WIDTH; ++x) {
                    const uint8_t val = (img_row[x] > threshold) ? 255 : 0;
                    original_row[x] = val;
                    // 统计前三行的异常点
                    if (y > HEIGHT - OUTLIER_CHECK_ROWS - 1&& val == 0) {
                        ++outlier_count;
                    }
                }
            }
        }

        void stretching_image(uint8_t* original_image)
        {
            // 转换为OpenCV的Mat格式
            cv::Mat src(70, 160, CV_8UC1, original_image); // 高70，宽160
            cv::Mat dst;
            // 缩放到目标尺寸 WIDTH x HEIGHT
            cv::resize(src, dst, cv::Size(WIDTH, HEIGHT), 0, 0, cv::INTER_LINEAR);

            // 验证目标尺寸与数组容量是否匹配
            static_assert(WIDTH * HEIGHT == 13020, "Size mismatch!");

            // 将 OpenCV 处理后的数据拷贝到全局数组
            std::memcpy(bin_pixels, dst.data, WIDTH * HEIGHT * sizeof(uint8_t));
            std::memcpy(lcd_pixels, dst.data, WIDTH * HEIGHT * sizeof(uint8_t));
        }
    #else
        //-------------------------------------------------------------------------------------------------------------------
        // 函数简介: 使用OpenCV进行简单的图像二值化，拉伸，存储
        // 参数说明: 传入长度WIDTH*HEIGHT 的uint8_t 一维数组
        // 返回参数: 直接赋值给全局变量    uint8_t bin_pixels[WIDTH * HEIGHT]{}; uint8_t lcd_pixels[WIDTH * HEIGHT]{}; 
        // 使用示例: Camera::basic_image_processing(rgay_image);
        // 备注信息: outlier_count为出界异常点统计
        //-------------------------------------------------------------------------------------------------------------------
        void basic_image_processing(uint8_t* image) {
            // 原始图像转为Mat（确保HEIGHT=70, UVC_WIDTH=160与实际数据一致）
            cv::Mat src(UVC_HEIGHT, UVC_WIDTH, CV_8UC1, image);
        
            //第1次Otsu自动阈值计算与二值化
            cv::Mat otsu_result;  // 必须显式定义输出Mat
            uint8_t computed_thresh = cv::threshold(src, otsu_result, 0, 255, cv::THRESH_OTSU | cv::THRESH_BINARY );
            //动态调整计算得到的阈值
            threshold = adjust_threshold( static_cast<uint8_t>(computed_thresh), threshold_camp,  80,  115 );
            //第2次应用调整后的阈值进行精确二值化
            cv::Mat bin_mat;
            cv::threshold(src, bin_mat, threshold, 255, cv::THRESH_BINARY);
        
            // 图像尺寸标准化处理
            cv::Mat stretched_mat;
            cv::resize(bin_mat, stretched_mat, cv::Size(WIDTH, HEIGHT), 0, 0, cv::INTER_NEAREST);
        
            // 底部区域异常点检测
            cv::Mat bottom_roi = stretched_mat.rowRange(HEIGHT-3, HEIGHT);
            outlier_count = cv::countNonZero(bottom_roi == 0);
        
            // 处理结果输出（确保目标缓冲区足够大）
            assert(WIDTH * HEIGHT <= sizeof(bin_pixels));
            std::memcpy(bin_pixels, stretched_mat.data, WIDTH * HEIGHT);
            std::memcpy(lcd_pixels, stretched_mat.data, WIDTH * HEIGHT);
        }
    #endif

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
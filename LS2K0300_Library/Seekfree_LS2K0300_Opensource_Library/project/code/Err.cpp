#include "Err.h"

namespace Error {
    // 变量定义

    Error_parm Err = {
        .current = 0,
        .previous = 0,
        .mode = 0,          
        .limit = HEIGHT - 1,
        .row = 37,
        .tem_row = 37,
        .rep = 1.00
    };
    alignas(8) std::array<uint16_t, HEIGHT> center_line;    //中线数组
    alignas(8) std::array<uint16_t, HEIGHT> fit_center_line;//拟合中线数组
    //alignas(8) std::array<uint16_t, HEIGHT> fit_Bessel_line;//拟合中线数组

    const std::array<uint8_t, HEIGHT> ROAD_WIDTH = {
              9,  11,  15,  20,  23,  26,  29,  32,  34,  36,  // 0-9
             39,  41,  43,  45,  46,  47,  49,  51,  54,  56,  // 10-19
             58,  61,  63,  65,  68,  70,  73,  75,  77,  79,  // 20-29
             82,  84,  86,  89,  91,  93,  96,  98,  100,  102, // 30-39
            105, 108, 110, 113, 115, 118, 120, 122, 124, 127, // 40-49
            129, 132, 134, 136, 138, 141, 143, 146, 148, 150, // 50-59
            152, 155, 157, 159, 161, 163, 165, 167, 169, 171  // 60-69
    };
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 误差获取
    // 参数说明: 截止行，左右边线，左右边线丢线标志位
    // 返回参数: 直接赋值center_line，Err
    // 使用示例: Error::error_get(ImageProc::state, ImageProc::l_black, ImageProc::r_black, ImageProc::left_flags, ImageProc::right_flags);
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    void error_get(const ImageProc::State& state,
                 const std::array<uint16_t, HEIGHT>& l_black,
                 const std::array<uint16_t, HEIGHT>& r_black,
                 const std::array<bool, HEIGHT>& left_flags,
                 const std::array<bool, HEIGHT>& right_flags) {
        
        center_line.fill(WIDTH/2); // 初始化为0
        midline_calculate(state, l_black, r_black, left_flags, right_flags);

        Err.current=error_calculate(Err.row, state, Err.limit, center_line);
        //printf("row=%d\ntem_row=%d\n",Err.row, Err.tem_row);
        //printf("Err=%d\n",Err.current);

    }
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 中线计算
    // 参数说明: 截止行，左右边线，左右边线丢线标志位
    // 返回参数: 直接赋值center_line
    // 使用示例: midline_calculate(state, l_black, r_black, left_flags, right_flags);
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    void midline_calculate(const ImageProc::State& state,
        const std::array<uint16_t, HEIGHT>& l_black,
        const std::array<uint16_t, HEIGHT>& r_black,
        const std::array<bool, HEIGHT>& left_flags,
        const std::array<bool, HEIGHT>& right_flags) 
    {
        const int16_t cut_off = static_cast<int16_t>(state.cutoff);
        const int16_t bottom_row = HEIGHT - 1;
        const int16_t top_limit = std::max(0, bottom_row - cut_off + 1);

        // 预计算指针参数
        const auto* l_black_ptr = l_black.data();
        const auto* r_black_ptr = r_black.data();
        auto* center_ptr = center_line.data();
        uint8_t* lcd_row_ptr = &Camera::lcd_pixels[bottom_row * WIDTH];
        const int16_t row_step = -static_cast<int16_t>(WIDTH); // 行指针步长

        // 统一指针运算框架
        auto process_row = [&](int16_t row_idx, auto calc_logic) {
            const bool l_flag = left_flags[row_idx];
            const bool r_flag = right_flags[row_idx];
            center_ptr[row_idx] = calc_logic(l_flag, r_flag, 
                                    l_black_ptr[row_idx], 
                                    r_black_ptr[row_idx]);
                //printf("center_line[%d]=%d\n",row_idx,center_ptr[row_idx]);
            // 同步绘制
            const uint16_t x = center_ptr[row_idx];
            if (x < WIDTH) { 
                *(lcd_row_ptr + x) = 0; // 直接指针操作
            }
        };

        switch (Err.mode) {
            case 0: {
                // constexpr float lose_weight = 0.0f;
                for (int16_t i = bottom_row; i >= top_limit; --i, lcd_row_ptr += row_step) {
                    process_row(i, [&](bool lf, bool rf, uint16_t lv, uint16_t rv) -> uint16_t { // 显式指定返回类型
                        const int sum = lv + rv; // 使用int中间值防止溢出
                        // if (lf && rf) {
                            // return static_cast<uint16_t>(sum / 2);
                        // } else if (!rf) {
                        //     return static_cast<uint16_t>(sum / 2 * (1 - lose_weight));
                        //     //return static_cast<uint16_t>(lv + ROAD_WIDTH[i]/2);
                        // } else if (!lf) {
                        //     return static_cast<uint16_t>(sum / 2 * (1 + lose_weight));
                        //     //return static_cast<uint16_t>(rv - ROAD_WIDTH[i]/2);
                        // }
                        return static_cast<uint16_t>(sum / 2);
                    });
                }
                break;
            }

            // case -1: {
            //     for (int16_t i = bottom_row; i >= top_limit; --i, lcd_row_ptr += row_step) {
            //     process_row(i, [&](bool, bool, uint16_t lv, uint16_t) {
            //         return lv + ROAD_WIDTH[i]/2;
            //         });
            //     }
            //     break;
            // }

            // case 1: {
            //     for (int16_t i = bottom_row; i >= top_limit; --i, lcd_row_ptr += row_step) {
            //         process_row(i, [&](bool, bool, uint16_t, uint16_t rv) {
            //             return rv - ROAD_WIDTH[i]/2;
            //         });
            //     }
            //     break;
            // }
        }
    }
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 上次误差更新
    // 参数说明: 
    // 返回参数: 
    // 使用示例: 
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    void last_error_update (){
        Err.last = Err.previous; // 更新last为previous
    }

    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 误差计算
    // 参数说明: 设定行数，截止行，中线
    // 返回参数: Err.current
    // 使用示例: error_value
    // 备注信息:
    //-------------------------------------------------------------------------------------------------------------------
    int16_t error_calculate(uint8_t row, const ImageProc::State& state, uint8_t limit,
                            const std::array<uint16_t, HEIGHT>& center_line){
        constexpr int16_t MAX_ERROR = 75;
        constexpr uint8_t MIN_VALID_CUTOFF = 10;
        constexpr uint8_t ROW_OFFSET = 5;
        int16_t error_value = 0;
        Err.tem_row = row; // 使用参数传入的row
        const uint8_t effective_row = std::min(state.cutoff, limit);

        bool use_override = false;
        float temp = 0.0f;
        //判断是否需要截断后补偿
        if (effective_row < MIN_VALID_CUTOFF){
            Err.tem_row = 0;
            return Err.previous;
        }
        else if (Err.tem_row > effective_row - ROW_OFFSET){
            Err.tem_row = std::clamp(effective_row  - ROW_OFFSET, 0, HEIGHT - 1);
            use_override = true;
        }

        //分发处理
        if (use_override){ temp = Err.rep;}
        else {temp = 1.0f;}
            
        error_value = static_cast<int16_t>(temp * (WIDTH / 2 - center_line[HEIGHT - Err.tem_row - 1]));
        error_value = std::clamp(error_value, static_cast<int16_t>(-MAX_ERROR), MAX_ERROR);
        Err.previous = error_value;
        //std::clamp(error_value, static_cast<int16_t>(-MAX_ERROR), MAX_ERROR);

        // LCD显示处理
        if (Err.tem_row < HEIGHT) {
            const size_t display_row = HEIGHT - Err.tem_row - 1;
            if (display_row < HEIGHT) {
                uint8_t* row_start = &Camera::lcd_pixels[display_row * WIDTH];
                std::fill(row_start, row_start + WIDTH, static_cast<uint8_t>(0));
            }
        }
        
        return error_value;
    }


    //拟合误差
    void fitting_error(
        const std::array<uint16_t, HEIGHT>& raw_line,
        std::array<uint16_t, HEIGHT>& fit_line,
        uint8_t start,
        uint8_t end
    ) {
        // 1. 打印调试信息
        // printf("Fitting between rows %d and %d\n", start, end);
        // printf("Raw values: [%d]=%d, [%d]=%d\n", start, raw_line[start], end, raw_line[end]);
    
        // 2. 自动交换顺序确保 start <= end
        if (start > end) {
            std::swap(start, end);
        }
    
        // 3. 边界检查
        if (start >= HEIGHT || end >= HEIGHT) {
            //printf("Error: Invalid range [%d, %d]\n", start, end);
            return;
        }
    
        // 4. 计算斜率和截距
        float k = 0.0f, b = 0.0f;
        if (start != end) {
            k = static_cast<float>(raw_line[end] - raw_line[start]) / (end - start);  // 注意分母是 end - start
            b = raw_line[start] - k * start;
        } else {
            b = raw_line[start];  // 单点情况
        }
    
        // 5. 拟合并存储结果
        for (uint8_t i = start; i <= end; ++i) {
            float fitted_value = k * i + b;
            fit_line[i] = static_cast<uint16_t>(std::round(fitted_value));
            
            // 6. 绘制到屏幕（可选）
            if (fit_line[i] < WIDTH) {
                Camera::lcd_pixels[i * WIDTH + fit_line[i]] = 0;
            }
        }
    }      

}
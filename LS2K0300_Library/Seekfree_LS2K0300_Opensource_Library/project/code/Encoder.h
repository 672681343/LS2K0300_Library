#pragma once
#include "zf_common_headfile.h"
#include <stdint.h>

namespace Encoder {
    // 设备路径定义
    constexpr const char* LEFT_DEVICE = "/dev/zf_encoder_1";
    constexpr const char* RIGHT_DEVICE = "/dev/zf_encoder_2";

    typedef struct {
        int16_t L;
        int16_t R;
        int16_t L_real;
        int16_t R_real;
    }CountPram;
    extern CountPram count;
    // 更新编码器数值（需周期性调用）
    void update();

    int16_t LowPassFilter(uint8_t ch, int16_t currentValue, float alpha, int16_t maxDelta) ;

    int16_t MovingAverageFilter(uint8_t ch, int16_t currentValue, uint8_t windowSize);
}
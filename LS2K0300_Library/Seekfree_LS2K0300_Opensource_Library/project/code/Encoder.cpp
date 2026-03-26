#include "Encoder.h"
namespace Encoder {
    CountPram count = {
        .L = 0,
        .R = 0,
        .L_real = 0,
        .R_real = 0
    };
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 更新编码器
    // 参数说明: 编码器
    // 返回参数: 编码器值
    // 使用示例: Encoder::update()
    // 备注信息: 中断定时调用
    //-------------------------------------------------------------------------------------------------------------------
    void update() {

        // count.L =  encoder_get_count(LEFT_DEVICE);
        // count.R = -encoder_get_count(RIGHT_DEVICE);

        count.R = -encoder_get_count(LEFT_DEVICE);
        count.L = encoder_get_count(RIGHT_DEVICE);
    }
    /**
     * @brief 
     * 
     * @param ch 
     * @param currentValue 
     * @param alpha 
     * @param maxDelta 
     * @return int16_t 
     */
    int16_t LowPassFilter(uint8_t ch, int16_t currentValue, float alpha, int16_t maxDelta) {
        // 为左右轮分别保存状态
        static struct {
            int16_t lastOutput;
            float filtered;
            uint8_t initialized;
        } state[2] = {{0, 0.0f, 0}, {0, 0.0f, 0}}; // ch0:左轮, ch1:右轮
    
        // 参数校验
        ch = ch > 1 ? 1 : ch;
    
        if(!state[ch].initialized) {
            state[ch].filtered = (float)currentValue;
            state[ch].lastOutput = currentValue;
            state[ch].initialized = 1;
            return currentValue;
        }
    
        if(maxDelta != 0 && abs(currentValue - state[ch].lastOutput) > maxDelta) {
            return state[ch].lastOutput;
        }
    
        state[ch].filtered = alpha * currentValue + (1.0f - alpha) * state[ch].filtered;
        state[ch].lastOutput = (int16_t)state[ch].filtered;
        
        return state[ch].lastOutput;
    }
    
}
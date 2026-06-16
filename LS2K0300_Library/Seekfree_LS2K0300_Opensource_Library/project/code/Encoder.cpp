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

        // count.R = -encoder_get_count(LEFT_DEVICE);
        // count.L = encoder_get_count(RIGHT_DEVICE);


        // count.R = LowPassFilter(1, -encoder_get_count(LEFT_DEVICE),0.8, 580);
        // count.L = LowPassFilter(0, encoder_get_count(RIGHT_DEVICE), 0.8, 580);

        count.R = MovingAverageFilter(1, -encoder_get_count(LEFT_DEVICE), 10);
        count.L = MovingAverageFilter(0, encoder_get_count(RIGHT_DEVICE), 10);
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
    
        // if(!state[ch].initialized) {
        //     state[ch].filtered = (float)currentValue;
        //     state[ch].lastOutput = currentValue;
        //     state[ch].initialized = 1;
        //     return currentValue;
        // }
    
        // if(maxDelta != 0 && abs(currentValue - state[ch].lastOutput) > maxDelta) {
        //     return state[ch].lastOutput;
        // }
    
        state[ch].filtered = alpha * currentValue + (1.0f - alpha) * state[ch].filtered;
        state[ch].lastOutput = (int16_t)state[ch].filtered;
        
        return state[ch].lastOutput;
    }

    int16_t MovingAverageFilter(uint8_t ch, int16_t currentValue, uint8_t windowSize)
    {
        // 为左右轮分别保存状态
        static struct
        {
            int16_t buffer[10]; // 窗口缓冲区，最大支持10点
            uint8_t index;      // 当前写入位置
            uint8_t count;      // 已填充点数
            int32_t sum;        // 累加和（用int32防溢出）
            int16_t lastOutput;
            uint8_t initialized;
        } state[2] = {{{0}, 0, 0, 0, 0, 0}, {{0}, 0, 0, 0, 0, 0}}; // ch0:左轮, ch1:右轮

        // 参数校验
        ch = ch > 1 ? 1 : ch;
        if (windowSize == 0 || windowSize > 10)
            windowSize = 5; // 默认5点，限制最大10

        // 初始化：首次直接赋值
        if (!state[ch].initialized)
        {
            for (uint8_t i = 0; i < windowSize; i++)
            {
                state[ch].buffer[i] = currentValue;
            }
            state[ch].sum = (int32_t)currentValue * windowSize;
            state[ch].index = 0;
            state[ch].count = windowSize;
            state[ch].lastOutput = currentValue;
            state[ch].initialized = 1;
            return currentValue;
        }


        // 递推平均：减去最老的，加上最新的
        state[ch].sum -= state[ch].buffer[state[ch].index]; // 减去即将被覆盖的值
        state[ch].buffer[state[ch].index] = currentValue;   // 写入新值
        state[ch].sum += currentValue;                      // 加上新值

        state[ch].index++;
        if (state[ch].index >= windowSize)
            state[ch].index = 0; // 循环回绕

        if (state[ch].count < windowSize)
            state[ch].count++; // 首次填充期

        // 计算平均
        int16_t output = (int16_t)(state[ch].sum / windowSize);
        state[ch].lastOutput = output;

        return output;
    }
}
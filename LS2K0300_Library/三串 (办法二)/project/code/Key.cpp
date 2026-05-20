#include "Key.h"

namespace Key {
    using Clock = std::chrono::steady_clock;
    
    struct DeviceState {
        Clock::time_point last_change;
        bool current_state;
    };

    static std::map<std::string, DeviceState> states;
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 按键初始化
    // 参数说明: 按键
    // 返回参数: 无
    // 使用示例: Key::Init();
    // 备注信息:
    //-------------------------------------------------------------------------------------------------------------------
    void Init() {
        const auto now = Clock::now();
        states = {
            {KEY_0,    {now, gpio_get_level(KEY_0)}}, // 初始化为当前实际电平
            {KEY_1,    {now, gpio_get_level(KEY_1)}},
            {KEY_2,    {now, gpio_get_level(KEY_2)}},
            {KEY_3,    {now, gpio_get_level(KEY_3)}},
            {SWITCH_0, {now, gpio_get_level(SWITCH_0)}},
            {SWITCH_1, {now, gpio_get_level(SWITCH_1)}}
        };
    }
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 获取按键消抖
    // 参数说明: 按键
    // 返回参数: bool
    // 使用示例: Key::GetDebouncedState(Key::KEY_3)
    // 备注信息:
    //-------------------------------------------------------------------------------------------------------------------
    bool GetDebouncedState(const std::string& device, std::chrono::milliseconds debounce_time) {
        const bool raw_state = gpio_get_level(device.c_str()); // 按下时 raw_state = 0（低电平）
        auto& state = states.at(device);
        const auto now = Clock::now();
    
        if (raw_state != state.current_state) {
            state.last_change = now;
            state.current_state = raw_state;
        }
    
        // 消抖逻辑修正
        if ((now - state.last_change) > debounce_time) {
            return raw_state; // 消抖时间后返回实际状态
        } else {
            return state.current_state; // 抖动期间保持上一次稳定状态
        }
    }


    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 按键测试
    // 参数说明: 
    // 返回参数: 
    // 使用示例: 
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------

    void test(){
        while(1){
            if(gpio_get_level(KEY_0) == 0)printf("key_0 = %d\r\n", gpio_get_level(KEY_0));
            if(gpio_get_level(KEY_1) == 0)printf("key_1 = %d\r\n", gpio_get_level(KEY_1));
            if(gpio_get_level(KEY_2) == 0)printf("key_2 = %d\r\n", gpio_get_level(KEY_2));
            if(gpio_get_level(KEY_3) == 0)printf("key_3 = %d\r\n", gpio_get_level(KEY_3));

            if(gpio_get_level(SWITCH_0) == 0)printf("switch_0 = %d\r\n", gpio_get_level(SWITCH_0));
            if(gpio_get_level(SWITCH_1) == 0)printf("switch_1 = %d\r\n", gpio_get_level(SWITCH_1));
        }
    }
}
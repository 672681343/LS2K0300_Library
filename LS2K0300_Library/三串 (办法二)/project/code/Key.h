#pragma once
#include <chrono>
#include <map>
#include <string>
#include "zf_common_headfile.h"
namespace Key {
    // 设备路径常量
    constexpr const char* KEY_0 = "/dev/zf_driver_gpio_key_0";
    constexpr const char* KEY_1 = "/dev/zf_driver_gpio_key_1";
    constexpr const char* KEY_2 = "/dev/zf_driver_gpio_key_2";
    constexpr const char* KEY_3 = "/dev/zf_driver_gpio_key_3";
    constexpr const char* SWITCH_0 = "/dev/zf_driver_gpio_switch_0";
    constexpr const char* SWITCH_1 = "/dev/zf_driver_gpio_switch_1";

    // 初始化函数声明
    void Init();
    
    // 带消抖的状态检测
    bool GetDebouncedState(const std::string& device, std::chrono::milliseconds debounce_time = std::chrono::milliseconds(20));

    void test();
}
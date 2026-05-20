#pragma once
#include "zf_common_headfile.h"

namespace TCP {
    extern int16_t test[8];
    // 接口函数声明
    void init();
    void send_data();
    void data_analysis();
}
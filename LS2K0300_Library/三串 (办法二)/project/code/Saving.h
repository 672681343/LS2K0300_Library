#pragma once
#include "zf_common_headfile.h"
#include <opencv2/core.hpp>

namespace DataSaving {
    void init();

    // 清空所有存储目录
    void clear_folders(uint8_t mode);

    // 主保存任务入口
    void saveTask(uint8_t mode);
    
}
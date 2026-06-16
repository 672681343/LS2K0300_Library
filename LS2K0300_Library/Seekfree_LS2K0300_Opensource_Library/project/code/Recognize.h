
#pragma once // 防止重复包含
#include "zf_common_headfile.h"
#include "lq_ntp.hpp"
#include <cstdint>
#include <vector>

extern uint8_t ahclass;

// 初始化模型（只调用一次）
bool InitModel();

void Inference(uint8_t *bgr888_image);

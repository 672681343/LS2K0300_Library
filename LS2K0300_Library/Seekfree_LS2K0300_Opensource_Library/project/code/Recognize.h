
#pragma once // 防止重复包含
#include "zf_common_headfile.h"
#include "lq_ntp.hpp"
#include <cstdint>
#include <vector>

void Inference(uint8_t *bgr888_image, int frame_count);

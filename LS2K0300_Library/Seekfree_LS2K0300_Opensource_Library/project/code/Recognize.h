#pragma once
// #ifndef RECOGNIZE_H
// #define RECOGNIZE_H

#include "zf_common_headfile.h"


// 现有的包含和定义...

// 添加推理结果结构体（如果还没有的话）
typedef struct {
    int class_index;          // 预测的类别索引
    float confidence;         // 置信度
    long inference_time_us;   // 推理耗时（微秒）
} InferenceResult;

// 添加函数声明
InferenceResult perform_inference(
    cv::Mat input_image,
    int frame_count,
    bool is_first_frame_debug,
    tflite::MicroInterpreter& interpreter
);

// #endif // RECOGNIZE_H
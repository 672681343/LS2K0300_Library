#pragma once
// #ifndef RECOGNIZE_H
// #define RECOGNIZE_H

#include "zf_common_headfile.h"
// #include <opencv2/opencv.hpp>
using namespace cv;


    //cv图像矩阵初始化
extern Mat float_img,resized_img;

// 日志控制变量
extern int frame_count;          // 帧计数器
extern bool first_frame_logged;  // 第一帧日志是否已打印

extern tflite::MicroInterpreter* interpreter;
// extern tflite::MicroInterpreter* init_tflite_simple(uint8_t* tensor_arena, size_t arena_size);
bool init_tflite_simple(uint8_t* tensor_arena, size_t arena_size);

void debug_log_printf_callback(const char* s); // 日志回调函数

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
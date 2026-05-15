#pragma once
// #ifndef RECOGNIZE_H
// #define RECOGNIZE_H

#include "zf_common_headfile.h"
// #include <opencv2/opencv.hpp>
using namespace cv;

    //模型处理相关的宏定义
    // 1. 模型相关配置
    #define MODEL_INPUT_WIDTH         45      // 模型输入图像宽度（根据实际模型调整） --> 模型期待的标准尺寸
    #define MODEL_INPUT_HEIGHT        45      // 模型输入图像高度（根据实际模型调整）
    #define MODEL_INPUT_CHANNEL       3       // 模型输入图像通道数（RGB=3）
    #define MODEL_OUTPUT_CLASS_NUM    3       // 模型输出类别数（根据实际任务调整）
    #define MODEL_INPUT_SIZE          (MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT * MODEL_INPUT_CHANNEL)  // 输入总元素数
    #define TFLITE_OP_RESOLVER_MAX_NUM 20     // 算子解析器最大支持算子数
    #define TENSOR_ARENA_SIZE         (324 * 1024)  // 张量空间大小（单位：字节，根据模型大小调整）

    // 2. 日志控制配置
    #define LOG_PRINT_FREQUENCY       10      // 每N帧打印一次推理结果（降低刷屏频率）
    #define ENABLE_FIRST_FRAME_DEBUG  true    // 打印第一帧的完整预处理日志（调试用）
    
    //cv图像矩阵初始化
    extern Mat float_img,resized_img;

    extern const char* class_labels[];

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
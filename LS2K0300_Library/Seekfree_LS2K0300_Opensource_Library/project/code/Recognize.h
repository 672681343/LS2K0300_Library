#pragma once

#include "zf_common_headfile.h"
using namespace cv;

// ========== 量化开关 ==========
// 0 = FP32 (浮点模型，精度高，体积大)
// 1 = INT8 (整型量化，速度快，体积小)
#define USE_INT8_MODEL 1 //  定义是否使用INT8模型的宏，1表示使用INT8模型

// ========== 模型配置 ==========
#define MODEL_INPUT_WIDTH          64 //  定义模型的输入宽度为48像素
#define MODEL_INPUT_HEIGHT         64 //  定义模型的输入高度为48像素
#define MODEL_INPUT_CHANNEL        3 //  定义模型的输入通道数为3（RGB三通道）
#define MODEL_OUTPUT_CLASS_NUM     3 //  定义模型的输出类别数量为6
#define MODEL_INPUT_SIZE           (MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT * MODEL_INPUT_CHANNEL)
#define TFLITE_OP_RESOLVER_MAX_NUM 25 //  定义TensorFlow Lite操作解析器支持的最大操作符数量为25
#define TENSOR_ARENA_SIZE          (324 * 1024) //  定义TensorFlow Lite的内存池大小为324KB

// ========== 日志配置 ==========
#define LOG_PRINT_FREQUENCY        1
#define ENABLE_FIRST_FRAME_DEBUG   true

// ========== 6小类标签定义 ==========
extern const char* class_labels[];

// cv图像矩阵
extern Mat float_img, resized_img;

// 日志控制变量
extern int frame_count;
extern bool first_frame_logged;

// TFLite解释器
extern tflite::MicroInterpreter* interpreter;

// ========== 函数声明 ==========

// 日志回调
void debug_log_printf_callback(const char* s);

// 初始化TFLite模型 (use_int8: 0=FP32, 1=INT8)
bool init_tflite_simple(uint8_t* tensor_arena, size_t arena_size, bool use_int8);

// 清理资源
void cleanup_tflite();

// 推理结果结构体
typedef struct {
    int class_index;
    float confidence;
    long inference_time_us;
    float all_scores[MODEL_OUTPUT_CLASS_NUM];
} InferenceResult;

// 执行推理 (use_int8: 0=FP32, 1=INT8)
InferenceResult perform_inference(
    cv::Mat input_image,
    int frame_count,
    bool is_first_frame_debug,
    tflite::MicroInterpreter& interpreter,
    bool use_int8
);

// 获取类别名称
inline const char* get_class_name(int index) {
    if (index >= 0 && index < MODEL_OUTPUT_CLASS_NUM) {
        return class_labels[index];
    }
    return "unknown";
}
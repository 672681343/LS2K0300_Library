#pragma once

#define WIDTH           (186)
#define HEIGHT          (70)
//上位机保存模式 1：保存拟图  0：保存lcd图
#define Saving_mode     (1)
//上位机保存参数 1：图像参数 0：控制参数
#define Saving_param    (1)
//TCP传参  1：启动  0关闭
#define TCP_MODE        (0)
// 配置参数（类型与SDK保持一致）
#define SERVER_IP "192.168.1.105"
// 端口号
#define PORT            (8888)
//图像处理模式 0：opencv图像处理 1：使用自己的图像处理 
#define CAMERA_MODE     (1)
//反转
#define _Reverse        (0)

//交换函数
#define SWAP(a, b) { uint8_t t = a; a = b; b = t; }
//取大值函数
#define my_max(a, b) ((a) > (b) ? (a) : (b))
//取小值函数    
#define my_min(a, b) ((a) < (b) ? (a) : (b))
//取绝对值函数
#define my_abs(a) ((a) > 0 ? (a) : -(a))



// //模型处理相关的宏定义
// // 1. 模型相关配置
// #define MODEL_INPUT_WIDTH         68      // 模型输入图像宽度（根据实际模型调整） --> 模型期待的标准尺寸
// #define MODEL_INPUT_HEIGHT        68      // 模型输入图像高度（根据实际模型调整）
// #define MODEL_INPUT_CHANNEL       3       // 模型输入图像通道数（RGB=3）
// #define MODEL_OUTPUT_CLASS_NUM    3       // 模型输出类别数（根据实际任务调整）
// #define MODEL_INPUT_SIZE          (MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT * MODEL_INPUT_CHANNEL)  // 输入总元素数
// #define TFLITE_OP_RESOLVER_MAX_NUM 20     // 算子解析器最大支持算子数
// #define TENSOR_ARENA_SIZE         (128 * 1024)  // 张量空间大小（单位：字节，根据模型大小调整）


// // 2. 日志控制配置
// #define LOG_PRINT_FREQUENCY       10      // 每N帧打印一次推理结果（降低刷屏频率）
// #define ENABLE_FIRST_FRAME_DEBUG  true    // 打印第一帧的完整预处理日志（调试用）
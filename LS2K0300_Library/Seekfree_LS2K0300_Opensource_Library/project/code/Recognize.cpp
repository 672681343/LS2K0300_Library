#include "Recognize.h"



































// using namespace cv;


// // ======================== 可配置参数宏定义 ========================
// // 1. 模型相关配置
// #define MODEL_INPUT_WIDTH         40      // 模型输入图像宽度（根据实际模型调整）
// #define MODEL_INPUT_HEIGHT        40      // 模型输入图像高度（根据实际模型调整）
// #define MODEL_INPUT_CHANNEL       3       // 模型输入图像通道数（RGB=3）
// #define MODEL_OUTPUT_CLASS_NUM    3       // 模型输出类别数（根据实际任务调整）
// #define MODEL_INPUT_SIZE          (MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT * MODEL_INPUT_CHANNEL)  // 输入总元素数
// #define TFLITE_OP_RESOLVER_MAX_NUM 20     // 算子解析器最大支持算子数
// #define TENSOR_ARENA_SIZE         (128 * 1024)  // 张量空间大小（单位：字节，根据模型大小调整）
// const char* class_labels[] = {"materials","traffic","weapon"}; //需要与train.py提示顺序一致

// // 2. 日志控制配置
// #define LOG_PRINT_FREQUENCY       10      // 每N帧打印一次推理结果（降低刷屏频率）
// #define ENABLE_FIRST_FRAME_DEBUG  true    // 打印第一帧的完整预处理日志（调试用）


//    // 创建模型解析器并分配张量空间（使用宏定义指定张量空间大小）
//     uint8_t tensor_arena[TENSOR_ARENA_SIZE];
//     tflite::MicroInterpreter interpreter(model, resolver, tensor_arena, TENSOR_ARENA_SIZE);
//     interpreter.AllocateTensors();


// // ======================== 日志回调函数 ========================
// void debug_log_printf_callback(const char* s) 
// {
//     if (s == NULL)  return;
//     printf("%s", s);
// }


// void recognize_init() {
//     // 重定向TFLM的MicroPrintf日志输出
//     RegisterDebugLogCallback(debug_log_printf_callback);

//     // tflite库初始化
//     tflite::InitializeTarget();

//     // 导入模型
//     // 传入模型文件生成的.h文件当中的数组名
//     const tflite::Model* model = ::tflite::GetModel(loong_cnn_model_simple_tflite);
//     TFLITE_CHECK_EQ(model->version(), TFLITE_SCHEMA_VERSION);

//     // 构建算子解析器（使用宏定义指定最大算子数）
//     tflite::MicroMutableOpResolver<TFLITE_OP_RESOLVER_MAX_NUM> resolver;

//     /* 添加所有可能会用到的算子 */
// 	resolver.AddConv2D();
// 	resolver.AddDepthwiseConv2D();
// 	resolver.AddMaxPool2D();
// 	resolver.AddFullyConnected();
// 	resolver.AddRelu6();
// 	resolver.AddSoftmax();
// 	resolver.AddReshape();
// 	resolver.AddShape();
// 	resolver.AddSlice();
// 	resolver.AddQuantize();
// 	resolver.AddDequantize();
// 	resolver.AddCast();
// 	resolver.AddSqueeze();
// 	resolver.AddExpandDims();
// 	resolver.AddConcatenation();
// 	resolver.AddTranspose();
// 	resolver.AddStridedSlice();
// 	resolver.AddPack();

 

//     //摄像头初始化
//     printf("===== 程序初始化 =====\n");
//     if(uvc_camera_init("/dev/video0") < 0)
//     {
//         printf("❌ 摄像头初始化失败！\n");
//         exit(0);   // 摄像头初始化失败，退出程序，防止程序卡死
//     }
//     printf("✅ 摄像头初始化成功\n");

 

//     // 日志控制变量
//     int frame_count = 0;          // 帧计数器
//     bool first_frame_logged = false;  // 第一帧日志是否已打印

//     printf("===== 开始实时推理 =====\n");
    
// }

// void recognize_detacion() {
//        //cv图像矩阵初始化
//     Mat float_img,resized_img;
//     frame_count++;  // 帧计数+1



//         // ======================== 图像预处理 ========================
//         // 1. 先缩放图像到模型要求的输入尺寸
//         resize(frame_rgb, resized_img, Size(MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT), INTER_LINEAR);

//         // 2. 类型转换（转为32位浮点型，可选归一化）
//         resized_img.convertTo(float_img, CV_32FC3, 1.0);  // 训练时归一化则用1.0/255.0，否则用1.0

//         // 3. 色彩空间转换（BGR→RGB，适配模型输入）
//         cvtColor(float_img, float_img, cv::COLOR_BGR2RGB);

//         // 4. 确保内存连续，避免拷贝错误（可选）
//         Mat continuous_float_img = float_img.isContinuous() ? float_img : float_img.clone();
//         float* model_input_ptr = continuous_float_img.ptr<float>(0);

//         // ===== 第一帧打印完整预处理日志（仅打印一次）=====
//         if(ENABLE_FIRST_FRAME_DEBUG && !first_frame_logged)
//         {
//             printf("\n===== 第1帧 完整预处理日志 =====\n");
//             // 原始图像信息
//             printf("1. 原始图像：%d x %d (宽x高)，通道数：%d\n", frame_rgb.cols, frame_rgb.rows, frame_rgb.channels());
//             // 缩放后图像信息
//             printf("2. 缩放后图像：%d x %d (宽x高)，通道数：%d\n", resized_img.cols, resized_img.rows, resized_img.channels());
//             // 内存连续性检查
//             printf("3. 浮点图像内存连续：%s\n", float_img.isContinuous() ? "是" : "否");
//             printf("4. 模型输入尺寸匹配：%s\n", 
//                    (continuous_float_img.total() * continuous_float_img.elemSize()) == (MODEL_INPUT_SIZE * sizeof(float)) 
//                    ? "✅ 匹配" : "❌ 不匹配");
//             printf("=================================\n");
//             first_frame_logged = true;  // 标记第一帧日志已打印
//         }

//         // ======================== 模型输入 ========================
//         float* input_data = interpreter.input(0)->data.f;
//         memcpy(input_data, model_input_ptr, MODEL_INPUT_SIZE * sizeof(float));

//         // ======================== 模型推理（计时） ========================
//         auto start_time = std::chrono::high_resolution_clock::now();
//         interpreter.Invoke();
//         auto end_time = std::chrono::high_resolution_clock::now();

//         // 计算推理耗时
//         auto time_cnt = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

//         // ======================== 模型输出解析 ========================
//         float* output_data = interpreter.output(0)->data.f;
//         float max_v = 0;
//         int max_i = 0;
        
//         // 查找置信度最高的类及其索引
//         for (int n = 0; n < MODEL_OUTPUT_CLASS_NUM; n++) 
//         {
//             float v = output_data[n];
//             if (v > max_v) {
//                 max_v = v;
//                 max_i = n;
//             }
//         }

//         // ===== 可控频率打印推理结果 =====
//         if(frame_count % LOG_PRINT_FREQUENCY == 0)
//         {
//             // 打印简洁的推理结果（每N帧一次）
//             printf("[第%d帧] 预测类别：%s | 置信度：%.4f | 推理耗时：%ld us\n",
//                    frame_count, class_labels[max_i], max_v, time_cnt);
            
//             // 可选：打印所有类别的置信度（调试用，按需开启）
//             // printf("        详细置信度：%s=%.4f, %s=%.4f, %s=%.4f\n",
//             //        class_labels[0], output_data[0],
//             //        class_labels[1], output_data[1],
//             //        class_labels[2], output_data[2]);
//         }

    
// }
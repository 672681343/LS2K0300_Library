// #include "zf_common_headfile.h"

// /********************************************************************************
//  * @file    lq_ncnn_demo.cpp
//  * @brief   NCNN 测试.
//  * @author  龙邱科技-006
//  * @date    2026-01-10
//  * @version V2.1.0
//  * @note    适用与龙芯 2K0300/0301 平台.
//  *          本 demo 实现 NCNN 功能，用于测试 NCNN 模型的基本功能.
//  ********************************************************************************/

// // 获取当前时间字符串（格式：YYYY-MM-DD HH:MM:SS）
// static std::string get_time_str(void)
// {
//     // 时间获取对象
//     static lq_ntp g_ntp;
//     return g_ntp.get_local_time_str();
// }

// void lq_ncnn_demo(void)
// {
//     try {
//         std::cout << "========== NCNN 环境自检(无模型) ==========" << std::endl;

//         // 创建 ncnn::Net
//         ncnn::Net net;
//         net.opt.num_threads = 2;
//         net.opt.use_vulkan_compute = false;  // 基础检查

//         // 创建测试输入张量，验证 Mat 内存与基础操作
//         ncnn::Mat input(8, 8, 3); // w, h, c
//         for (int c = 0; c < input.c; ++c) {
//             float* ptr = input.channel(c);
//             for (int i = 0; i < input.w * input.h; ++i) {
//                 ptr[i] = static_cast<float>(c * 100 + i);
//             }
//         }

//         ncnn::Mat input_clone = input.clone();
//         float checksum = 0.0f;
//         for (int c = 0; c < input_clone.c; ++c) {
//             const float* ptr = input_clone.channel(c);
//             for (int i = 0; i < input_clone.w * input_clone.h; ++i) {
//                 checksum += ptr[i];
//             }
//         }

//         // 创建提取器
//         ncnn::Extractor ex = net.create_extractor();

//         // 未加载模型时，输入层不存在，返回非 0 属于预期行为
//         int ret = ex.input("data", input_clone);

//         std::cout << "Mat 维度: w=" << input_clone.w
//                   << ", h=" << input_clone.h
//                   << ", c=" << input_clone.c << std::endl;
//         std::cout << "Mat 校验和: " << checksum << std::endl;
//         std::cout << "Extractor input 返回值(无模型预期非0): " << ret << std::endl;

//         if (ret != 0) {
//             std::cout << "[PASS] ncnn 基础环境可用（头文件/链接/运行时对象均正常）。" << std::endl;
//             return;
//         }

//         std::cout << "[WARN] 未加载模型却返回0，建议进一步检查 ncnn 版本与调用逻辑。" << std::endl;
//         return;
//     } catch (const std::exception& e) {
//         std::cerr << "[FAIL] ncnn 环境自检异常: " << e.what() << std::endl;
//         return;
//     } catch (...) {
//         std::cerr << "[FAIL] ncnn 环境自检出现未知异常" << std::endl;
//         return;
//     }
// }
// void lq_ncnn_photo_demo(void)
// {
//     printf("========================================\n");
//     printf("       NCNN 图像分类推理示例\n");
//     printf("========================================\n\n");

//     // ==================== 配置区域 ====================
//     // 测试图片路径（修改为你的图片路径）
//     std::string test_image_path = "test.jpg";
    
//     // 模型配置
//     std::string model_param = "tiny_classifier_fp32.ncnn.param";
//     std::string model_bin   = "tiny_classifier_fp32.ncnn.bin";
//     int input_width    = 96;
//     int input_height   = 96;
    
//     // 类别标签（顺序必须与训练时一致）
//     std::vector<std::string> labels = {"supplies", "vehicle", "weapon"};
    
//     // 归一化参数（ImageNet标准）
//     float mean_vals[3] = {123.675f, 116.28f, 103.53f};
//     float norm_vals[3] = {0.01712475f, 0.017507f, 0.01742919f};
//     // =================================================

//     // 创建NCNN对象并配置
//     LQ_NCNN ncnn;
//     ncnn.SetModelPath(model_param, model_bin);
//     ncnn.SetInputSize(input_width, input_height);
//     ncnn.SetLabels(labels);
//     ncnn.SetNormalize(mean_vals, norm_vals);

//     // 初始化模型
//     printf("[%s] 正在加载模型...\n", get_time_str().c_str());
//     if (!ncnn.Init()) {
//         printf("[%s] 模型加载失败!\n", get_time_str().c_str());
//         return ;
//     }
//     printf("[%s] 模型加载成功!\n\n", get_time_str().c_str());

//     // 读取测试图片
//     printf("[%s] 读取图片: %s\n", get_time_str().c_str(), test_image_path.c_str());
//     cv::Mat image = cv::imread(test_image_path);

//     if (image.empty()) {
//         printf("无法读取图片: %s\n", test_image_path.c_str());
//         printf("请确保图片文件存在\n");
//         return ;
//     }
//     printf("[%s] 图片尺寸: %d x %d\n\n", get_time_str().c_str(), image.cols, image.rows);

//     // 注意: OpenCV读取的是BGR格式，但在推理时会自动转换为RGB格式以匹配训练时的输入
//     // 训练使用PIL读取的RGB格式，因此需要色彩空间转换

//     // 推理
//     printf("[%s] 开始推理...\n", get_time_str().c_str());
//     auto start = std::chrono::high_resolution_clock::now();
//     std::string result = ncnn.Infer(image);
//     auto end = std::chrono::high_resolution_clock::now();
//     auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

//     // 输出结果
//     printf("\n========================================\n");
//     printf("推理结果: %s\n", result.c_str());
//     printf("推理耗时: %ld ms\n", duration.count());
//     printf("========================================\n");

//     return ;
// }



// int main(int, char**) 
// {
//     lq_ncnn_photo_demo();
//     return 0; 
// }



























/*********************************************************************************************************************
* LS2K0300 Opensourec Library 即（LS2K0300 开源库）是一个基于官方 SDK 接口的第三方开源库
* Copyright (c) 2022 SEEKFREE 逐飞科技
*
* 本文件是LS2K0300 开源库的一部分
*
* LS2K0300 开源库 是免费软件
* 您可以根据自由软件基金会发布的 GPL（GNU General Public License，即 GNU通用公共许可证）的条款
* 即 GPL 的第3版（即 GPL3.0）或（您选择的）任何后来的版本，重新发布和/或修改它
*
* 本开源库的发布是希望它能发挥作用，但并未对其作任何的保证
* 甚至没有隐含的适销性或适合特定用途的保证
* 更多细节请参见 GPL
*
* 您应该在收到本开源库的同时收到一份 GPL 的副本
* 如果没有，请参阅<https://www.gnu.org/licenses/>
*
* 额外注明：
* 本开源库使用 GPL3.0 开源许可证协议 以上许可申明为译文版本
* 许可申明英文版在 libraries/doc 文件夹下的 GPL3_permission_statement.txt 文件中
* 许可证副本在 libraries 文件夹下 即该文件夹下的 LICENSE 文件
* 欢迎各位使用并传播本程序 但修改内容时必须保留逐飞科技的版权声明（即本声明）
*
* 文件名称          main
* 公司名称          成都逐飞科技有限公司
* 适用平台          LS2K0300
* 店铺链接          https://seekfree.taobao.com/
*
* 修改记录
* 日期              作者           备注
* 2025-12-27        大W            first version
* 2026-03-06        补充           添加图像预处理日志输出
* 2026-03-06        优化           统一推理结果输出格式（与摄像头例程一致）
********************************************************************************************************************/
#include "zf_common_headfile.h"
using namespace cv;

// *************************** 例程硬件连接说明 ***************************
//  将2K300核心板插到主板上面，确保插到底核心板与主板插座间没有缝隙即可
//  将2K300核心板插到主板上面，确保插到底核心板与主板插座间没有缝隙即可
//  将2K300核心板插到主板上面，确保插到底核心板与主板插座间没有缝隙即可
//  使用本历程，就需要使用我们逐飞科技提供的内核。
//  使用本历程，就需要使用我们逐飞科技提供的内核。
//  使用本历程，就需要使用我们逐飞科技提供的内核。
// 
//
// *************************** 例程使用步骤说明 ***************************
// 1.使用前需要自行将测试使用的test.jpg文件传到2k300核心板当中
//
// 2.可以选择训练使用的数据集来作为test.jpg 测试结果符合训练结果说明模型传入没有问题
//
// *************************** 例程测试说明 ***************************
// 1.将2K300核心板插到主板上面 主板使用电源供电 下载本例程
//
// 2.运行后可以看到模型输出返回的类别信息和置信度以及模型推理所使用的时间
//
// 如果发现现象与说明严重不符 请参照本文件最下方 例程常见问题说明 进行排查
//
// ======================== 可配置参数宏定义 ========================
// 1. 模型相关配置
#define MODEL_INPUT_WIDTH         45      // 模型输入图像宽度（根据实际模型调整）
#define MODEL_INPUT_HEIGHT        45      // 模型输入图像高度（根据实际模型调整）
#define MODEL_INPUT_CHANNEL       3       // 模型输入图像通道数（RGB=3）
#define MODEL_OUTPUT_CLASS_NUM    3       // 模型输出类别数（根据实际任务调整）
#define MODEL_INPUT_SIZE          (MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT * MODEL_INPUT_CHANNEL)  // 输入总元素数
#define TFLITE_OP_RESOLVER_MAX_NUM 20     // 算子解析器最大支持算子数
#define TENSOR_ARENA_SIZE         (324 * 1024)  // 张量空间大小（单位：字节，根据模型大小调整）
const char* class_labels[] = {"materials","traffic","weapon"}; //需要与train.py提示顺序一致


// ======================== 日志回调函数 ========================
void debug_log_printf_callback(const char* s) 
{
    if (s == NULL)  return;
    printf("%s", s);
}

// ======================== 主函数 ========================
int main(int, char**) 
{
    // 重定向TFLM的MicroPrintf日志输出
    RegisterDebugLogCallback(debug_log_printf_callback);

    // tflite库初始化
    tflite::InitializeTarget();

    // 导入模型
    // 传入模型文件生成的.h文件当中的数组名
    const tflite::Model* model = ::tflite::GetModel(loong_cnn_model_simple_tflite);
    TFLITE_CHECK_EQ(model->version(), TFLITE_SCHEMA_VERSION);

    // 构建算子解析器（使用宏定义指定最大算子数）
    tflite::MicroMutableOpResolver<TFLITE_OP_RESOLVER_MAX_NUM> resolver;

    /* 添加所有可能会用到的算子 */
	resolver.AddConv2D();
	resolver.AddDepthwiseConv2D();
	resolver.AddMaxPool2D();
	resolver.AddFullyConnected();
	resolver.AddRelu6();
	resolver.AddSoftmax();
	resolver.AddReshape();
	resolver.AddShape();
	resolver.AddSlice();
	resolver.AddQuantize();
	resolver.AddDequantize();
	resolver.AddCast();
	resolver.AddSqueeze();
	resolver.AddExpandDims();
	resolver.AddConcatenation();
	resolver.AddTranspose();
	resolver.AddStridedSlice();
	resolver.AddPack();

    //后来添加的算子
    resolver.AddAdd();     // 缺少的ADD算子
    resolver.AddMul();     // 之前缺少的MUL算子
    // resolver.AddMean(); // 之前缺少的MEAN算子

    // 创建模型解析器并分配张量空间（使用宏定义指定张量空间大小）
    uint8_t tensor_arena[TENSOR_ARENA_SIZE];
    tflite::MicroInterpreter interpreter(model, resolver, tensor_arena, TENSOR_ARENA_SIZE);
    interpreter.AllocateTensors();

    // ======================== 图像预处理 ========================

    Mat src_img,float_img,resized_img;
    src_img = imread("test.jpg", 1);  // 读取图片
    if (src_img.empty()) 
    {
        printf("❌ 图片读取失败！\n");
        printf("❌ 未添加test.jpg测试图片\n");
        return -1;
    }
    
    // ========== 打印原始图像信息 ==========
    printf("===== 图像预处理日志 =====\n");
    printf("1. 原始图像信息：\n");
    printf("   - 尺寸：%d x %d (宽x高)\n", src_img.cols, src_img.rows);
    printf("   - 通道数：%d\n", src_img.channels());
    printf("   - 数据类型：%s\n", src_img.type() == CV_8UC3 ? "CV_8UC3 (8位BGR)" : "未知类型");

    // 1. 先缩放图像到模型要求的输入尺寸
    resize(src_img, resized_img, Size(MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT), INTER_LINEAR);
    
    // ========== 打印缩放后图像信息 ==========
    printf("2. 缩放后图像信息：\n");
    printf("   - 尺寸：%d x %d (宽x高)\n", resized_img.cols, resized_img.rows);
    printf("   - 通道数：%d\n", resized_img.channels());
    printf("   - 数据类型：%s\n", resized_img.type() == CV_8UC3 ? "CV_8UC3 (8位BGR)" : "未知类型");

    // 2. 类型转换（转为32位浮点型，可选归一化）
    resized_img.convertTo(float_img, CV_32FC3, 1.0);  // 训练时归一化则用1.0/255.0，否则用1.0
    
    // ========== 打印类型转换后图像信息 ==========
    printf("3. 类型转换后图像信息：\n");
    printf("   - 数据类型：%s\n", float_img.type() == CV_32FC3 ? "CV_32FC3 (32位浮点BGR)" : "未知类型");
    // 打印前几个像素值用于验证（BGR格式）
    printf("   - 前3个像素值（BGR）：[%.1f,%.1f,%.1f], [%.1f,%.1f,%.1f], [%.1f,%.1f,%.1f]\n",
           float_img.at<Vec3f>(0,0)[0], float_img.at<Vec3f>(0,0)[1], float_img.at<Vec3f>(0,0)[2],
           float_img.at<Vec3f>(0,1)[0], float_img.at<Vec3f>(0,1)[1], float_img.at<Vec3f>(0,1)[2],
           float_img.at<Vec3f>(0,2)[0], float_img.at<Vec3f>(0,2)[1], float_img.at<Vec3f>(0,2)[2]);

    // 3. 色彩空间转换（BGR→RGB，适配模型输入）
    cvtColor(float_img, float_img, cv::COLOR_BGR2RGB);
    
    // ========== 打印色彩转换后图像信息 ==========
    printf("4. 色彩空间转换后（BGR→RGB）：\n");
    printf("   - 前3个像素值（RGB）：[%.1f,%.1f,%.1f], [%.1f,%.1f,%.1f], [%.1f,%.1f,%.1f]\n",
           float_img.at<Vec3f>(0,0)[0], float_img.at<Vec3f>(0,0)[1], float_img.at<Vec3f>(0,0)[2],
           float_img.at<Vec3f>(0,1)[0], float_img.at<Vec3f>(0,1)[1], float_img.at<Vec3f>(0,1)[2],
           float_img.at<Vec3f>(0,2)[0], float_img.at<Vec3f>(0,2)[1], float_img.at<Vec3f>(0,2)[2]);

    // 4. 确保内存连续，避免拷贝错误(可选)
    Mat continuous_float_img = float_img.isContinuous() ? float_img : float_img.clone();
    float* model_input_ptr = continuous_float_img.ptr<float>(0);

    // ======================== 模型输入 ========================
    float* input_data = interpreter.input(0)->data.f;

    memcpy(input_data, model_input_ptr, MODEL_INPUT_SIZE * sizeof(float));
    
    // ========== 打印模型输入数据验证 ==========
    printf("5. 模型输入数据验证（前5个浮点值）：\n");  // 修正序号：原来是6，改为5更连贯
    for(int i=0; i<5; i++) {
        printf("     input_data[%d] = %.1f\n", i, input_data[i]);
    }
    printf("=================================\n");
    printf("\n");

    // ======================== 模型推理（计时） ========================
    auto start_time = std::chrono::high_resolution_clock::now();
    interpreter.Invoke();
    auto end_time = std::chrono::high_resolution_clock::now();

    // 计算推理耗时
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    // ======================== 模型输出解析 ========================
    float* output_data = interpreter.output(0)->data.f;
    float max_v = 0;
    int max_i = 0;
    
    // 查找置信度最高的类及其索引
    printf("===== 模型推理结果 =====\n");  // 统一标题名称
    for (int n = 0; n < MODEL_OUTPUT_CLASS_NUM; n++) 
    {
        float v = output_data[n];
        // 改为摄像头例程的简洁格式，置信度保留4位小数（更易读）
        printf("   类别 %d (%s)：置信度 = %.4f\n", n, class_labels[n], v);
        if (v > max_v) {
            max_v = v;
            max_i = n;
        }
    }
    
    // 输出最终结果（和摄像头例程格式完全一致）
    printf("\n✅ [静态图片推理] 预测类别：%s | 置信度：%.4f | 推理耗时：%ld us\n",
           class_labels[max_i], max_v, duration);   
    printf("=========================\n");

    return 0; 
}


// *************************** 例程常见问题说明 ***************************
// 遇到问题时请按照以下问题检查列表检查
// 
// 问题1：终端提示未添加test.jpg测试图片
//      重新检查test.jpg测试图片是否添加在和工程运行文件相同路径下 即用户根目录下
//      
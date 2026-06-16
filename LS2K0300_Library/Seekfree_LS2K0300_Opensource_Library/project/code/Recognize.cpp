#include "Recognize.h"

#include <time.h>
#include <lq_ntp.hpp>
#include <lq_ncnn.hpp>


uint8_t ahclass =-1; // 0: supplies 1: vehicle 2: weapon

static bool g_model_loaded = false;
static LQ_NCNN g_ncnn;

static std::string get_time_str(void)
{
    // 时间获取对象
    static lq_ntp g_ntp;
    return g_ntp.get_local_time_str();
}

// 初始化模型（程序启动时调用一次）
bool InitModel()
{
    if (g_model_loaded)
        return true;

    std::string model_param = "tiny_classifier_fp32.ncnn.param";
    std::string model_bin = "tiny_classifier_fp32.ncnn.bin";
    int input_width = 96;
    int input_height = 96;

    std::vector<std::string> labels = {"supplies", "vehicle", "weapon"};
    float mean_vals[3] = {123.675f, 116.28f, 103.53f};
    float norm_vals[3] = {0.01712475f, 0.017507f, 0.01742919f};

    g_ncnn.SetModelPath(model_param, model_bin);
    g_ncnn.SetInputSize(input_width, input_height);
    g_ncnn.SetLabels(labels);
    g_ncnn.SetNormalize(mean_vals, norm_vals);

    printf("[%s] 正在加载模型...\n", get_time_str().c_str());
    if (!g_ncnn.Init())
    {
        printf("[%s] 模型加载失败!\n", get_time_str().c_str());
        return false;
    }
    printf("[%s] 模型加载成功!\n\n", get_time_str().c_str());

    g_model_loaded = true;
    return true;
}

void Inference(uint8_t *bgr888_image)
{

    // 确保模型已加载
    if (!g_model_loaded)
    {
        printf("错误: 模型未初始化，请先调用 InitModel()\n");
        return;
    }
    printf("正在检测推理红框...\n");

    // boxes.clear();
    // detect_red_regions_H(bgr888_image, UVC_WIDTH, UVC_HEIGHT, boxes, 100, 5200);
    // printf("DEBUG: box=(%d,%d,%d,%d), cx=%d, cy=%d, crop=(%d,%d,%d)\n",
    //    box.x, box.y, box.w, box.h, cx, cy, crop_x, crop_y, crop_size);
    

    if (!boxes.empty() )
    { // 每10帧自动保存一张, 并且只保存第一个检测到的红色区域（如果有的话）
        auto &box = boxes[0]; //  获取检测到的第一个边界框

        int process_height = MODEL_INPUT_HEIGHT; // 裁剪高度固定为48像素，宽度根据检测到的红色区域和模型输入大小调整（适配68*68的）
        int process_width = box.w*2 + 15;          // 裁剪宽度box.w + 8（适配40*40）
        if (process_width < MODEL_INPUT_WIDTH)
        {
            process_width = MODEL_INPUT_WIDTH; // 保持宽度不变，增加一些边距
        }
        process_height = process_width; // 保持宽高一致，适配模型输入要求

        int crop_x = box.x*2 - 8; // 水平偏左一点

        // 框底边 + 偏移 - 裁剪高度 = 裁剪区域顶部
        // 这样框底边在裁剪区域底部附近
        int crop_y = box.y*2 + box.h*2 + 15 - process_height; // 算的是裁剪区域的左上角坐标

        // === 裁剪（用同样的坐标）===
        cv::Mat cropped_mat = crop_image_to_bgr_mat(
            bgr888_image, IMG_WIDTH, IMG_HEIGHT,
            crop_x, crop_y, process_width, process_height);

        // save_bgr888_as_bmp("/tmp/debug.bmp", cropped_mat.data, process_width, process_height, process_width * 3);

        if (cropped_mat.data)
        {

            printf("========================================\n");
            printf("       NCNN 图像分类推理示例\n");
            printf("========================================\n\n");


            if (cropped_mat.empty())
            {
                printf("请确保图片文件存在\n");
                return;
            }
            printf("[%s] 图片尺寸: %d x %d\n\n", get_time_str().c_str(), cropped_mat.cols, cropped_mat.rows);

            // 注意: OpenCV读取的是BGR格式，但在推理时会自动转换为RGB格式以匹配训练时的输入
            // 训练使用PIL读取的RGB格式，因此需要色彩空间转换

            // 推理
            printf("[%s] 开始推理...\n", get_time_str().c_str());
            auto start = std::chrono::high_resolution_clock::now();
            std::string result = g_ncnn.Infer(cropped_mat);
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            // 输出结果
            printf("\n========================================\n");
            printf("推理结果: %s\n", result.c_str());
            printf("推理耗时: %ld ms\n", duration.count());
            printf("========================================\n");
            

            if (result == "supplies"){
                ahclass = 0;
            }else if (result == "vehicle"){
                ahclass = 1;
            }else if (result == "weapon"){
                ahclass = 2;
            }
            printf("ahclass: %d\n", ahclass);
        }
    }
    else{
        printf("检测推理红框失败...\n");
    }
}
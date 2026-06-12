#include "Recognize.h"

#include <time.h>
#include <lq_ntp.hpp>
#include <lq_ncnn.hpp>

static std::string get_time_str(void)
{
    // 时间获取对象
    static lq_ntp g_ntp;
    return g_ntp.get_local_time_str();
}

void Inference(uint8_t *bgr888_image, int frame_count)
{

    boxes.clear();
    detect_red_regions(bgr888_image, IMG_WIDTH, IMG_HEIGHT, boxes, 200, 5200);
    // printf("DEBUG: box=(%d,%d,%d,%d), cx=%d, cy=%d, crop=(%d,%d,%d)\n",
    //    box.x, box.y, box.w, box.h, cx, cy, crop_x, crop_y, crop_size);
    frame_count++; // 帧计数+1

    if (!boxes.empty() && (frame_count % 1 == 0))
    { // 每10帧自动保存一张, 并且只保存第一个检测到的红色区域（如果有的话）
        auto &box = boxes[0]; //  获取检测到的第一个边界框

        int process_height = MODEL_INPUT_HEIGHT; // 裁剪高度固定为48像素，宽度根据检测到的红色区域和模型输入大小调整（适配68*68的）
        int process_width = box.w + 15;          // 裁剪宽度box.w + 8（适配40*40）
        if (process_width < MODEL_INPUT_WIDTH)
        {
            process_width = MODEL_INPUT_WIDTH; // 保持宽度不变，增加一些边距
        }
        process_height = process_width; // 保持宽高一致，适配模型输入要求

        int crop_x = box.x - 8; // 水平偏左一点

        // 框底边 + 偏移 - 裁剪高度 = 裁剪区域顶部
        // 这样框底边在裁剪区域底部附近
        int crop_y = box.y + box.h + 15 - process_height; // 算的是裁剪区域的左上角坐标

        // === 裁剪（用同样的坐标）===
        cv::Mat cropped_mat = crop_image_to_bgr_mat(
            bgr888_image, IMG_WIDTH, IMG_HEIGHT,
            crop_x, crop_y, process_width, process_height);

        if (cropped_mat.data)
        {

            printf("========================================\n");
            printf("       NCNN 图像分类推理示例\n");
            printf("========================================\n\n");

            // ==================== 配置区域 ====================

            // 模型配置
            std::string model_param = "tiny_classifier_fp32.ncnn.param";
            std::string model_bin = "tiny_classifier_fp32.ncnn.bin";
            int input_width = 96;
            int input_height = 96;

            // 类别标签（顺序必须与训练时一致）
            std::vector<std::string> labels = {"supplies", "vehicle", "weapon"};

            // 归一化参数（ImageNet标准）
            float mean_vals[3] = {123.675f, 116.28f, 103.53f};
            float norm_vals[3] = {0.01712475f, 0.017507f, 0.01742919f};
            // =================================================

            // 创建NCNN对象并配置
            LQ_NCNN ncnn;
            ncnn.SetModelPath(model_param, model_bin);
            ncnn.SetInputSize(input_width, input_height);
            ncnn.SetLabels(labels);
            ncnn.SetNormalize(mean_vals, norm_vals);

            // 初始化模型
            printf("[%s] 正在加载模型...\n", get_time_str().c_str());
            if (!ncnn.Init())
            {
                printf("[%s] 模型加载失败!\n", get_time_str().c_str());
                return;
            }
            printf("[%s] 模型加载成功!\n\n", get_time_str().c_str());

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
            std::string result = ncnn.Infer(cropped_mat);
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            // 输出结果
            printf("\n========================================\n");
            printf("推理结果: %s\n", result.c_str());
            printf("推理耗时: %ld ms\n", duration.count());
            printf("========================================\n");
        }
    }
}
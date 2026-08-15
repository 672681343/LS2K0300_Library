#include "lq_Recognize.h"
#include "red_detecte.h"
#include "many_define.h"
#include "Recognize.h"

#include <queue>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <opencv2/opencv.hpp>
#include <time.h>

extern LQ_NCNN g_ncnn;  // ← 声明：这个对象在 lq_Recognize.cpp 里定义
// 图像尺寸（根据你的IPS200屏幕修改）
#define IMG_WIDTH  320  
#define IMG_HEIGHT 240

// RGB565 颜色定义
#define RGB565_RED     0xF800
#define RGB565_GREEN   0x07E0
#define RGB565_BLUE    0x001F
#define RGB565_WHITE   0xFFFF
#define RGB565_BLACK   0x0000
#define RGB565_YELLOW  0xFFE0


// ========== 在文件顶部或合适位置定义颜色表 ==========
static const char* CLASS_COLORS[] = {
    "\033[1;31m",  // 0: 红
    "\033[1;32m",  // 1: 绿
    "\033[1;33m",  // 2: 黄
    "\033[1;34m",  // 3: 蓝
    "\033[1;35m",  // 4: 紫
    "\033[1;36m",  // 5: 青
    "\033[1;37m",  // 6: 白
    "\033[0;91m",  // 7: 亮红
    "\033[0;92m",  // 8: 亮绿
    "\033[0;93m",  // 9: 亮黄
};
#define NUM_CLASS_COLORS (sizeof(CLASS_COLORS) / sizeof(CLASS_COLORS[0]))
#define RESET_COLOR "\033[0m"



std::vector<RedBox> boxes; //  存储红色框对象的容器



RedPoint2f corners[4];

static uint cnt = 0;
cv::Mat crop_image_to_bgr_mat(uint8_t* src_image, int src_width, int src_height,
                               int x, int y, int w, int h) ;


// ========== 运行模式配置（改这一行即可切换） ==========
// 0 = 拍数据集（只保存图片，不做推理）
// 1 = 测试模型（打印耗时、置信度、保存调试图）
// 2 = 实时 NCNN 推理（简单打印结果）
// 3 = 统计模式（测试模型 + 近50帧一致性/准确率统计）
#define RUN_MODE  0

// 拍数据集配置
#define DATASET_MAX_SAVE   3000   // 最多保存多少张
#define DATASET_PREFIX     "weapon_p1_1"  // 文件名前缀，拍什么改什么

// 测试模型配置
#define TEST_SAVE_DEBUG    500     // 测试模式下保存前 N 张调试图，0=不保存

// 统计模式配置（RUN_MODE 3 专用）
#define ENABLE_GROUND_TRUTH  0    // 1=开启准确率统计，0=只显示一致性
#define TEST_GROUND_TRUTH    "weapon"   // ← 测什么改什么
// =========================================================

// 文件名命名规则参考（拍数据集时按这个改 DATASET_PREFIX）：
// 增补时，前面加上一个add就好了add_，比如add_weapon_p2_1_*.bmp，这样就不会和之前的数据集重名了
// 新的文件名格式：
//   traffic_p1_1_*_*_*_*_*.bmp --> 装甲车(白色背景) --      √
//   traffic_p1_2_*_*_*_*_*.bmp --> 装甲车(杂乱背景)       √
//   traffic_p2_1_*_*_*_*_*.bmp --> 救护车(白色背景)       √
//   traffic_p2_2_*_*_*_*_*.bmp --> 救护车(杂乱背景)       √
//
//   weapon_p1_1_*_*_*_*_*.bmp --> 步枪(白色背景)        √
//   weapon_p1_2_*_*_*_*_*.bmp --> 手枪(白色背景)        √
//   weapon_p2_1_*_*_*_*_*.bmp --> 手榴弹(白色背景)      
//   weapon_p2_2_*_*_*_*_*.bmp --> 手榴弹(杂乱背景)      
//   weapon_p2_3_*_*_*_*_*.bmp --> C4(杂乱和白天背景)     ×××不拍这个
//
//   materials_p1_1_*_*_*_*_*.bmp --> 望远镜(白色背景)    
//   materials_p2_1_*_*_*_*_*.bmp --> 急救包(白色背景)
//   materials_p2_2_*_*_*_*_*.bmp --> 急救包(杂乱背景)  
//
// 旧文件格式（已废弃，仅供参考）：
//   traffic_*_*_1.jpg --> 装甲车
//   traffic_*_*_2.jpg --> 救护车
//   weapon_*_*_1.jpg --> 枪
//   weapon_*_*_2.jpg --> 爆炸物
//   materials_*_*_1.jpg --> 望远镜
//   materials_*_*_2.jpg --> 急救包

void process_image(uint8_t* bgr888_image, int* frame_count)
{
    // 检测红色区域（公共部分）
    detect_red_regions(bgr888_image, IMG_WIDTH, IMG_HEIGHT, boxes, 20, 2600);
    printf("第 %d 帧: 检测到 %zu 个红色区域\n", *frame_count, boxes.size());
    (*frame_count)++;

    if (!boxes.empty() && (*frame_count % 1 == 0)) {
        cnt++;

        // 按 max_y 排序，取最底部的红框（max_y 最大的）
        std::sort(boxes.begin(), boxes.end(), [](const RedBox& a, const RedBox& b) {
            return a.max_y > b.max_y;  // 降序：底部优先
        });
        auto& box = boxes[0];

        printf("  [选中] 最底部红框: max_y=%d ，x轴坐标 = %d,面积 = %d , 旋转框x轴坐标 = %f,旋转角度=%.2f(所有框底部: ", box.max_y, box.x, box.area, box.cx, box.angle);
        for (size_t i = 0; i < boxes.size() && i < 5; i++) {
            printf("%d ", boxes[i].max_y);
        }
        printf(")\n");

        // ========== 裁剪参数计算（公共部分，不变）==========
        get_rotated_box_corners(box, corners);

        float angle_factor = 1.0f + 0.6f * fabsf(sinf(box.angle));
        int process_width = (int)(box.obb_w * 1.5f * angle_factor);
        if (process_width < MODEL_INPUT_WIDTH) process_width = MODEL_INPUT_WIDTH;
        int process_height = process_width;

        int crop_bottom = box.max_y + 16;
        if (crop_bottom > IMG_HEIGHT) crop_bottom = IMG_HEIGHT;

        float angle_offset = sinf(box.angle) * (box.obb_h * 1.8f);
        int crop_y = crop_bottom - process_height;
        int crop_x = (int)(box.cx - process_width / 2.0f) + angle_offset;

        // 边界保护（左右）
        if (crop_x < 0) {  
            crop_x = 0;
            if (process_width > IMG_WIDTH) {
                process_width = IMG_WIDTH;
                process_height = process_width;
            }
        } else if (crop_x + process_width > IMG_WIDTH) {
            crop_x = IMG_WIDTH - (int)(box.obb_w * 1.5f);
            if (crop_x < 0) {
                crop_x = 0;
                process_width = (int)(box.obb_w * 1.5f);
                process_height = process_width;
            }
            process_width = (int)(box.obb_w * 1.5f);
            process_height = process_width;
            crop_y = crop_bottom - process_height;
        }
        //左转弯道边界特殊处理
        if ((box.x < 80 && box.area < 380 && box.max_y < 120 && box.angle < -0.2f) || (box.angle < -0.38f && box.max_y < 120)) {
            
            process_width += (box.obb_h/box.obb_w)*process_width;
            crop_x -= (box.obb_h/box.obb_w)*process_width/2;
            printf("旋转矩形框高宽比 = %f", box.obb_h/box.obb_w);
            if  (crop_bottom - process_width < 0) {
                crop_y = 0;
            } else {
                crop_y = crop_bottom - process_width;
            }

            if (crop_x < 0) {
                crop_x = 0;
            }

            process_height = process_width;
            printf("  [弯道边界] 调整裁剪参数: crop_x=%d, crop_y=%d, process_width=%d, process_height=%d\n",
                   crop_x, crop_y, process_width, process_height);
        }

         //右转弯道边界特殊处理
        if ((box.x > IMG_WIDTH-80 && box.area < 380 && box.max_y < 120 && box.angle > 0.2f) || (box.angle > 0.38f && box.max_y < 120)) {
            
            process_width += (box.obb_h/box.obb_w)*process_width;
            crop_x += (box.obb_h/box.obb_w)*process_width/2;
            printf("旋转矩形框高宽比 = %f", box.obb_h/box.obb_w);
            if  (crop_bottom - process_width < 0) {
                crop_y = 0;
            } else {
                crop_y = crop_bottom - process_width;
            }

            if (crop_x < 0) {
                crop_x = 0;
            }

            process_height = process_width;
            printf("  [弯道边界] 调整裁剪参数: crop_x=%d, crop_y=%d, process_width=%d, process_height=%d\n",
                   crop_x, crop_y, process_width, process_height);
        }




        


        // 边界保护（上下）
        if (crop_y < 0) {
            crop_y = 0;
            process_height = crop_bottom;
            if (process_height < MODEL_INPUT_WIDTH) process_height = MODEL_INPUT_WIDTH;
            process_width = process_height;
        }
        if (crop_y + process_height > IMG_HEIGHT) {
            crop_y = IMG_HEIGHT - process_height;
            if (crop_y < 0) {
                crop_y = 0;
                process_height = IMG_HEIGHT;
                process_width = process_height;
            }
        }

        // ========== 画框（所有框红色，最底部绿色高亮）==========
        uint8_t* debug_image = new uint8_t[IMG_WIDTH * IMG_HEIGHT * 3];
        memcpy(debug_image, bgr888_image, IMG_WIDTH * IMG_HEIGHT * 3);

        // // 画所有红框（红色细框）
        // for (size_t i = 0; i < boxes.size(); i++) {
        //     RedPoint2f temp_corners[4];
        //     get_rotated_box_corners(boxes[i], temp_corners);
        //     // 红色 = 0x0000FF (BGR: B=0, G=0, R=255)
        //     draw_rotated_rect_bgr888(debug_image, IMG_WIDTH, IMG_HEIGHT, temp_corners, 0x0000FF, 1);
        //     // AABB 青色细框
        //     draw_rect_bgr888(debug_image, IMG_WIDTH, IMG_HEIGHT, 
        //                     boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h, 
        //                     0xFFFF00, 1);  // 黄色
        // }

        // 最底部框高亮（绿色粗框）
        // {
        //     RedPoint2f highlight_corners[4];
        //     get_rotated_box_corners(boxes[0], highlight_corners);
        //     // 绿色 = 0x00FF00 (BGR: B=0, G=255, R=0)
        //     draw_rotated_rect_bgr888(debug_image, IMG_WIDTH, IMG_HEIGHT, highlight_corners, 0x00FF00, 3);
        //     draw_rect_bgr888(debug_image, IMG_WIDTH, IMG_HEIGHT,
        //                     boxes[0].x, boxes[0].y, boxes[0].w, boxes[0].h,
        //                     0x00FF00, 2);  // 绿色粗框
        // }
        // save_bgr888_as_bmp("/tmp/debug.bmp", debug_image, IMG_WIDTH, IMG_HEIGHT, IMG_WIDTH * 3);

        delete[] debug_image;

        // ========== 裁剪（公共部分）==========
        cv::Mat cropped_mat = crop_image_to_bgr_mat(
            bgr888_image, UVC_WIDTH, UVC_HEIGHT,
            crop_x, crop_y, process_width, process_height
        );

        if (!cropped_mat.data) {
            printf("裁剪失败\n");
            return;
        }

        // 生成时间戳（公共部分，两种模式都可能用到）
        struct timeval tv;
        gettimeofday(&tv, NULL);
        time_t now = tv.tv_sec;
        struct tm *tm_info = localtime(&now);
        char timestamp[30];
        strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);
        char full_timestamp[40];
        long ms = tv.tv_usec / 1000;
        if (ms < 0) ms = 0;
        if (ms > 999) ms = 999;
        snprintf(full_timestamp, sizeof(full_timestamp), "%s_%03ld", timestamp, ms);

        // ========== 模式分支 ==========
#if RUN_MODE == 0
        // ==================== 模式 0：拍数据集 ====================
        if (cnt > DATASET_MAX_SAVE) {
            printf("已达到最大保存数量 %d，停止保存\n", DATASET_MAX_SAVE);
            return;
        }

        char path[128];
        snprintf(path, sizeof(path), "/tmp/%s_%d_%d_%d_%d_%d___%s.bmp",
                DATASET_PREFIX, cnt, *frame_count, box.area, box.y + box.h, process_height, full_timestamp);

        // snprintf(path, sizeof(path), "/tmp/测试%s__%s_%d_%d_%d_%d_%d.bmp",
        //         full_timestamp, DATASET_PREFIX, cnt, *frame_count, box.area, box.y + box.h, process_height);  //时间戳在前，方便观察拍摄效果，但是拍数据集别用



        save_bgr888_as_bmp(path, cropped_mat.data,
                          cropped_mat.cols, cropped_mat.rows,
                          cropped_mat.step);

        printf("[数据集模式] 已保存第 %d 张: %s\n", cnt, full_timestamp);

#elif RUN_MODE == 1
        // ==================== 模式 1：测试模型（带耗时和各类置信度）====================
        auto t0 = std::chrono::high_resolution_clock::now();
        auto result_pair = g_ncnn.InferWithProbs(cropped_mat);
        auto t1 = std::chrono::high_resolution_clock::now();

        std::string pred_label = result_pair.first;
        std::vector<float>& probs = result_pair.second;
        long infer_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

        // 找最大概率索引
        size_t pid = 0;
        float max_prob = probs[0];
        for (size_t i = 1; i < probs.size(); ++i) {
            if (probs[i] > max_prob) {
                max_prob = probs[i];
                pid = i;
            }
        }

        // 打印结果
        printf("\n[测试模式] 第 %d 帧 | 耗时: %ldms | 预测: \033[1;32m%s (%.1f%%)\033[0m\n",
               *frame_count, infer_ms, pred_label.c_str(), max_prob * 100);

        // 打印各类别置信度
        printf("  置信度: ");
        for (size_t i = 0; i < CLASS_NAMES.size() && i < probs.size(); ++i) {
            const char* marker = (i == pid) ? "←" : "";
            printf("%s=%.1f%%%s ", CLASS_NAMES[i].c_str(), probs[i] * 100, marker);
        }
        printf("\n");






        // 可选：保存调试图
        if (TEST_SAVE_DEBUG > 0 && cnt <= TEST_SAVE_DEBUG) {
            char debug_path[128];
            snprintf(debug_path, sizeof(debug_path), "/tmp/debug_%d_%s.bmp", cnt, full_timestamp);
            save_bgr888_as_bmp(debug_path, cropped_mat.data,
                              cropped_mat.cols, cropped_mat.rows,
                              cropped_mat.step);
        }

#elif RUN_MODE == 2
        // ==================== 模式 2：实时 NCNN 推理（简单模式）====================
        std::string result = g_ncnn.Infer(cropped_mat);
        printf("[NCNN] 第 %d 帧 | 裁剪尺寸 %dx%d | 推理结果: \033[1;32m%s\033[0m\n",
               *frame_count, cropped_mat.cols, cropped_mat.rows, result.c_str());

#elif RUN_MODE == 3
        // ==================== 模式 3：统计模式（测试 + 近N帧统计 + 普通打印）====================
        auto t0 = std::chrono::high_resolution_clock::now();
        auto result_pair = g_ncnn.InferWithProbs(cropped_mat);
        auto t1 = std::chrono::high_resolution_clock::now();

        std::string pred_label = result_pair.first;
        std::vector<float>& probs = result_pair.second;
        long infer_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

        // 找最大概率索引
        size_t pid = 0;
        float max_prob = probs[0];
        for (size_t i = 1; i < probs.size(); ++i) {
            if (probs[i] > max_prob) {
                max_prob = probs[i];
                pid = i;
            }
        }

        // 可选：保存调试图
        if (TEST_SAVE_DEBUG > 0 && cnt <= TEST_SAVE_DEBUG) {
            char debug_path[128];
            snprintf(debug_path, sizeof(debug_path), "/tmp/debug_%s_%d_%s.bmp", pred_label.c_str(), cnt, full_timestamp);
            save_bgr888_as_bmp(debug_path, cropped_mat.data,
                              cropped_mat.cols, cropped_mat.rows,
                              cropped_mat.step);
        }

        // ========== 近N帧统计（方案A + 方案B）==========
        #define STAT_WINDOW  1

        // --- 方案B：预测一致性（始终运行）---
        static std::vector<int> class_counts(CLASS_NAMES.size(), 0);
        static std::vector<int> frame_history(STAT_WINDOW, 0);
        static int hist_idx = 0;
        static int hist_count = 0;

        if (hist_count >= STAT_WINDOW) {
            class_counts[frame_history[hist_idx]]--;
        }
        frame_history[hist_idx] = static_cast<int>(pid);
        class_counts[pid]++;
        hist_idx = (hist_idx + 1) % STAT_WINDOW;
        if (hist_count < STAT_WINDOW) hist_count++;

        int dominant_id = 0;
        int dominant_cnt = class_counts[0];
        for (size_t i = 1; i < CLASS_NAMES.size(); i++) {
            if (class_counts[i] > dominant_cnt) {
                dominant_cnt = class_counts[i];
                dominant_id = i;
            }
        }
        float consistency = (float)dominant_cnt / hist_count;

        // --- 方案A：平均准确率（可开关）---
        #if ENABLE_GROUND_TRUTH == 1
        float avg_acc = 0.0f;
        int correct_sum = 0;
        int acc_count = 0;
        bool is_correct = false;
       
        static std::vector<int> correct_history(STAT_WINDOW, 0);
        static int acc_idx = 0;
        static int acc_count_static = 0;

        is_correct = (pred_label == TEST_GROUND_TRUTH);
        correct_history[acc_idx] = is_correct ? 1 : 0;
        acc_idx = (acc_idx + 1) % STAT_WINDOW;
        if (acc_count_static < STAT_WINDOW) acc_count_static++;

        correct_sum = 0;
        for (int i = 0; i < acc_count_static; i++) {
            correct_sum += correct_history[i];
        }
        avg_acc = (float)correct_sum / acc_count_static;
        acc_count = acc_count_static;
        #endif

        // ========== 普通打印输出（每5帧一行，无ANSI）==========
        static int print_cnt = 0;
        if (++print_cnt % STAT_WINDOW == 0) {
            const char* frame_label = (hist_count < STAT_WINDOW) ? "累计" : "近";
            
            // ========== 替换原来的 printf 部分 ==========
        const char* pred_color = CLASS_COLORS[pid % NUM_CLASS_COLORS];
        const char* dom_color  = CLASS_COLORS[dominant_id % NUM_CLASS_COLORS];

        printf("置信度:%d", static_cast<int>(max_prob * 100));
        if (max_prob > 0.6f) {
            printf("帧:%d 耗时:%ldms 预测:%s%s %.1f%%%s %s%d帧一致:%.1f%% 多数:%s%s%s",
               *frame_count, infer_ms,
               pred_color, pred_label.c_str(), max_prob * 100, RESET_COLOR,
               frame_label, hist_count, consistency * 100,
               dom_color, CLASS_NAMES[dominant_id].c_str(), RESET_COLOR);
        } else{
            printf("----------------------------低置信度跳过----------------------------");
        }
        



            #if ENABLE_GROUND_TRUTH == 1
            printf(" 准确率:%.1f%%(%d/%d)%s",
                   avg_acc * 100, correct_sum, acc_count,
                   is_correct ? "✓" : "✗");
            #endif
            
            printf("\n");
            fflush(stdout);
        }

#endif
    }

    // if (boxes.empty()) {
    //     *frame_count = 0; // 没有检测到红色区域，重置帧计数器
    // }
}




/*
 *概述：该函数用于判断一个像素点是否为红色像素。通过比较像素的红色分量与给定的最小红色值，以及红色分量与绿色、蓝色分量的差值，来确定该像素是否满足红色的条件。
 *
 *参数：
 *      pixel：一个16位无符号整数，表示一个像素点的RGB565格式颜色值。
 *      min_red：一个8位无符号整数，表示红色分量的最小阈值。
 *      min_diff：一个8位无符号整数，表示红色分量与绿色、蓝色分量的最小差值。
 *返回值：
 *
 *返回一个布尔值，如果像素的红色分量大于等于min_red，并且大于绿色分量和蓝色分量至少min_diff，则返回true，表示该像素是红色像素；否则返回false。
 */
// 函数实现（不写默认参数，只写参数类型）
// bool is_red_pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t min_red, uint8_t min_diff) {
//     return (r >= min_red) && (r > g + min_diff) && (r > b + min_diff);
// }

bool is_red_pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t min_red, uint8_t min_diff) {
    // 条件1: R 不能太低
    if (r < min_red) return false;
    
    // 条件2: R 必须大于 G 和 B 的绝对差值
    if (r <= g + min_diff) return false;
    if (r <= b + min_diff) return false;
    
    // 条件3（新增）: R 必须是 G 和 B 的至少 2 倍
    // 用整数除法，无浮点开销
    if (r / 2 < g) return false;
    if (r / 2 < b) return false;
    
    return true;
}





void detect_red_regions(uint8_t* image, int width, int height,
                       std::vector<RedBox>& boxes,
                       int min_area, int max_area) {
    
    const int total_pixels = width * height;
    uint8_t* mask = new uint8_t[total_pixels];
    uint8_t* visited = new uint8_t[total_pixels];

    // 边界过滤阈值（根据你的 320×240 调整）
    int min_y_threshold = 20;
    int max_y_threshold = height - 20;
    int min_x_threshold = 40;
    int max_x_threshold = width - 20;
    float min_density = 0.2f;
    
    // 生成二值掩码
    for (int i = 0; i < total_pixels; i++) {
        uint8_t b = image[i * 3];
        uint8_t g = image[i * 3 + 1];
        uint8_t r = image[i * 3 + 2];
        mask[i] = is_red_pixel(r, g, b, 30, 15) ? 255 : 0;
    }
    
    memset(visited, 0, total_pixels);
    boxes.clear();
    
    const int dx[4] = {0, 0, -1, 1};
    const int dy[4] = {-1, 1, 0, 0};
    
    // 临时存储连通域像素坐标（PCA 用）
    std::vector<std::pair<int, int>> points;
    points.reserve(2048);
    
    // BFS 查找连通域（步长 1，避免漏检）
    for (int y = 0; y < height; y+=4) {
        for (int x = 0; x < width; x+=4) {
            int idx = y * width + x;
            if (mask[idx] == 0 || visited[idx]) continue;
            
            points.clear();
            int min_x = x, max_x = x;
            int min_y = y, max_y = y;
            long long sum_x = 0, sum_y = 0;
            long long sum_x2 = 0, sum_y2 = 0, sum_xy = 0;
            
            std::queue<std::pair<int, int>> q;
            q.push({x, y});
            visited[idx] = 1;
            
            while (!q.empty()) {
                auto [cx, cy] = q.front();
                q.pop();
                
                points.push_back({cx, cy});
                sum_x += cx;  sum_y += cy;
                sum_x2 += 1LL * cx * cx;
                sum_y2 += 1LL * cy * cy;
                sum_xy += 1LL * cx * cy;
                
                if (cx < min_x) min_x = cx;
                if (cx > max_x) max_x = cx;
                if (cy < min_y) min_y = cy;
                if (cy > max_y) max_y = cy;
                
                for (int i = 0; i < 4; i++) {
                    int nx = cx + dx[i];
                    int ny = cy + dy[i];
                    if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
                    
                    int nidx = ny * width + nx;
                    if (mask[nidx] && !visited[nidx]) {
                        visited[nidx] = 1;
                        q.push({nx, ny});
                    }
                }
            }
            
            int pixel_count = (int)points.size();
            int aabb_w = max_x - min_x + 1;
            int aabb_h = max_y - min_y + 1;
            
            // 快速过滤（AABB 层面）
            if (pixel_count < min_area || pixel_count > max_area) continue;
            if (min_y < min_y_threshold || max_y > max_y_threshold) continue;
            if (min_x < min_x_threshold || max_x > max_x_threshold) continue;
            if (aabb_w < 15) continue;
            
            // ===== PCA 计算旋转矩形 =====
            float n = pixel_count;
            float cx_f = sum_x / n;
            float cy_f = sum_y / n;
            float mu20 = sum_x2 / n - cx_f * cx_f;
            float mu02 = sum_y2 / n - cy_f * cy_f;
            float mu11 = sum_xy / n - cx_f * cy_f;
            
            float angle = 0.5f * atan2f(2.0f * mu11, mu20 - mu02);
            
            float cos_a = cosf(angle);
            float sin_a = sinf(angle);
            float min_u = 1e9f, max_u = -1e9f;
            float min_v = 1e9f, max_v = -1e9f;
            
            for (auto& p : points) {
                float dx_ = p.first - cx_f;
                float dy_ = p.second - cy_f;
                float u =  dx_ * cos_a + dy_ * sin_a;   // 主轴投影
                float v = -dx_ * sin_a + dy_ * cos_a;   // 垂直轴投影
                min_u = std::min(min_u, u); max_u = std::max(max_u, u);
                min_v = std::min(min_v, v); max_v = std::max(max_v, v);
            }
            
            float obb_w = max_u - min_u;
            float obb_h = max_v - min_v;
            if (obb_w < 1.0f) obb_w = 1.0f;
            if (obb_h < 1.0f) obb_h = 1.0f;
            
            // 规范化：w 始终为长边
            if (obb_w < obb_h) {
                std::swap(obb_w, obb_h);
                angle += 3.14159265f / 2.0f;
            }
            while (angle > 3.14159265f / 2.0f) angle -= 3.14159265f;
            while (angle < -3.14159265f / 2.0f) angle += 3.14159265f;
            
            float obb_area = obb_w * obb_h;
            float density = pixel_count / obb_area;
            
            // 旋转矩形过滤（更鲁棒，不受倾斜影响）
            bool aspect_ok = (std::max(obb_w, obb_h) / std::min(obb_w, obb_h)) < 15.0f;
            bool size_ok   = obb_w > 8.0f && obb_h > 4.0f;
            bool angle_ok  = fabsf(angle) < 3.14159265f / 4.0f;  // 限制旋转角度在 ±45 度内
            
            if (density >= min_density && aspect_ok && size_ok && angle_ok) {
                RedBox box;
                // AABB 字段（兼容旧代码：裁剪、画黄框都用这些）
                box.x = min_x;
                box.y = min_y;
                box.w = aabb_w;
                box.h = aabb_h;
                box.area = pixel_count;
                box.density = density;
                
                // OBB 字段（新增：旋转矩形参数）
                box.cx = cx_f;
                box.cy = cy_f;
                box.obb_w = obb_w;
                box.obb_h = obb_h;
                box.angle = angle;
                box.max_y = max_y;  
                
                boxes.push_back(box);
            }
        }
    }
    
    delete[] mask;
    delete[] visited;
}










// void detect_red_regions(uint8_t* image, int width, int height,
//                        std::vector<RedBox>& boxes,
//                        int min_area, int max_area) {
    
//     const int total_pixels = width * height;
//     uint8_t* mask = new uint8_t[total_pixels];
//     uint8_t* visited = new uint8_t[total_pixels];

//     // 边界过滤阈值（根据你的 320×240 调整）
//     int min_y_threshold = 20;
//     int max_y_threshold = height - 20;
//     int min_x_threshold = 20;
//     int max_x_threshold = width - 20;
//     float min_density = 0.2f;
    
//     printf("=== 红框检测开始 ===\n");
//     printf("图像尺寸: %dx%d, 最小面积=%d, 最大面积=%d\n", width, height, min_area, max_area);
//     printf("边界阈值: 上边界=%d, 下边界=%d, 左边界=%d, 右边界=%d, 最小密度=%.2f\n",
//            min_y_threshold, max_y_threshold, min_x_threshold, max_x_threshold, min_density);
    
//     // 生成二值掩码
//     int red_pixels = 0;
//     for (int i = 0; i < total_pixels; i++) {
//         uint8_t b = image[i * 3];
//         uint8_t g = image[i * 3 + 1];
//         uint8_t r = image[i * 3 + 2];
//         mask[i] = is_red_pixel(r, g, b, 100, 80) ? 255 : 0;
//         if (mask[i]) red_pixels++;
//     }
//     printf("红色像素总数: %d (占比 %.2f%%)\n", red_pixels, 100.0f * red_pixels / total_pixels);
    
//     memset(visited, 0, total_pixels);
//     boxes.clear();
    
//     const int dx[4] = {0, 0, -1, 1};
//     const int dy[4] = {-1, 1, 0, 0};
    
//     std::vector<std::pair<int, int>> points;
//     points.reserve(2048);
    
//     int candidate_count = 0;
//     int pass_count = 0;
    
//     // BFS 查找连通域（步长 1，避免漏检）
//     for (int y = 0; y < height; y+=4) {
//         for (int x = 0; x < width; x+=4) {
//             int idx = y * width + x;
//             if (mask[idx] == 0 || visited[idx]) continue;
            
//             candidate_count++;
//             printf("\n--- 候选区域 #%d 起始点=(%d,%d) ---\n", candidate_count, x, y);
            
//             points.clear();
//             int min_x = x, max_x = x;
//             int min_y = y, max_y = y;
//             long long sum_x = 0, sum_y = 0;
//             long long sum_x2 = 0, sum_y2 = 0, sum_xy = 0;
            
//             std::queue<std::pair<int, int>> q;
//             q.push({x, y});
//             visited[idx] = 1;
            
//             while (!q.empty()) {
//                 auto [cx, cy] = q.front();
//                 q.pop();
                
//                 points.push_back({cx, cy});
//                 sum_x += cx;  sum_y += cy;
//                 sum_x2 += 1LL * cx * cx;
//                 sum_y2 += 1LL * cy * cy;
//                 sum_xy += 1LL * cx * cy;
                
//                 if (cx < min_x) min_x = cx;
//                 if (cx > max_x) max_x = cx;
//                 if (cy < min_y) min_y = cy;
//                 if (cy > max_y) max_y = cy;
                
//                 for (int i = 0; i < 4; i++) {
//                     int nx = cx + dx[i];
//                     int ny = cy + dy[i];
//                     if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
                    
//                     int nidx = ny * width + nx;
//                     if (mask[nidx] && !visited[nidx]) {
//                         visited[nidx] = 1;
//                         q.push({nx, ny});
//                     }
//                 }
//             }
            
//             int pixel_count = (int)points.size();
//             int aabb_w = max_x - min_x + 1;
//             int aabb_h = max_y - min_y + 1;
            
//             printf("  像素数=%d, 外接矩形=(%d,%d,%d,%d) 宽=%d 高=%d\n",
//                    pixel_count, min_x, min_y, max_x, max_y, aabb_w, aabb_h);
            
//             // 快速过滤（AABB 层面）
//             if (pixel_count < min_area || pixel_count > max_area) {
//                 printf("  [被过滤] 面积过滤: %d 不在范围 [%d,%d] 内\n", pixel_count, min_area, max_area);
//                 continue;
//             }
//             if (min_y < min_y_threshold) {
//                 printf("  [被过滤] 上边界过滤: 最小y=%d < %d\n", min_y, min_y_threshold);
//                 continue;
//             }
//             if (max_y > max_y_threshold) {
//                 printf("  [被过滤] 下边界过滤: 最大y=%d > %d\n", max_y, max_y_threshold);
//                 continue;
//             }
//             if (min_x < min_x_threshold) {
//                 printf("  [被过滤] 左边界过滤: 最小x=%d < %d\n", min_x, min_x_threshold);
//                 continue;
//             }
//             if (max_x > max_x_threshold) {
//                 printf("  [被过滤] 右边界过滤: 最大x=%d > %d\n", max_x, max_x_threshold);
//                 continue;
//             }
//             if (aabb_w < 15) {
//                 printf("  [被过滤] 宽度过滤: 外接矩形宽=%d < 15\n", aabb_w);
//                 continue;
//             }
            
//             // ===== PCA 计算旋转矩形 =====
//             float n = pixel_count;
//             float cx_f = sum_x / n;
//             float cy_f = sum_y / n;
//             float mu20 = sum_x2 / n - cx_f * cx_f;
//             float mu02 = sum_y2 / n - cy_f * cy_f;
//             float mu11 = sum_xy / n - cx_f * cy_f;
            
//             float angle = 0.5f * atan2f(2.0f * mu11, mu20 - mu02);
            
//             float cos_a = cosf(angle);
//             float sin_a = sinf(angle);
//             float min_u = 1e9f, max_u = -1e9f;
//             float min_v = 1e9f, max_v = -1e9f;
            
//             for (auto& p : points) {
//                 float dx_ = p.first - cx_f;
//                 float dy_ = p.second - cy_f;
//                 float u =  dx_ * cos_a + dy_ * sin_a;
//                 float v = -dx_ * sin_a + dy_ * cos_a;
//                 min_u = std::min(min_u, u); max_u = std::max(max_u, u);
//                 min_v = std::min(min_v, v); max_v = std::max(max_v, v);
//             }
            
//             float obb_w = max_u - min_u;
//             float obb_h = max_v - min_v;
//             if (obb_w < 1.0f) obb_w = 1.0f;
//             if (obb_h < 1.0f) obb_h = 1.0f;
            
//             if (obb_w < obb_h) {
//                 std::swap(obb_w, obb_h);
//                 angle += 3.14159265f / 2.0f;
//             }
//             while (angle > 3.14159265f / 2.0f) angle -= 3.14159265f;
//             while (angle < -3.14159265f / 2.0f) angle += 3.14159265f;
            
//             float obb_area = obb_w * obb_h;
//             float density = pixel_count / obb_area;
            
//             printf("  旋转矩形: 宽=%.1f 高=%.1f 角度=%.1f° 密度=%.3f\n",
//                    obb_w, obb_h, angle * 180.0f / 3.14159265f, density);
            
//             // 旋转矩形过滤
//             float aspect = std::max(obb_w, obb_h) / std::min(obb_w, obb_h);
//             bool aspect_ok = aspect < 15.0f;
//             bool size_ok = obb_w > 8.0f && obb_h > 4.0f;
//             bool angle_ok = fabsf(angle) < 3.14159265f / 4.0f;
            
//             if (density < min_density) {
//                 printf("  [被过滤] 密度过滤: 密度=%.3f < %.2f\n", density, min_density);
//                 continue;
//             }
//             if (!aspect_ok) {
//                 printf("  [被过滤] 宽高比过滤: 宽高比=%.2f >= 15.0\n", aspect);
//                 continue;
//             }
//             if (!size_ok) {
//                 printf("  [被过滤] 尺寸过滤: 旋转矩形宽=%.1f <= 8.0 或 高=%.1f <= 4.0\n", obb_w, obb_h);
//                 continue;
//             }
//             if (!angle_ok) {
//                 printf("  [被过滤] 角度过滤: 角度=%.1f° >= 45°\n", angle * 180.0f / 3.14159265f);
//                 continue;
//             }
            
//             printf("  [通过] 外接矩形=(%d,%d,%d,%d)\n", min_x, min_y, aabb_w, aabb_h);
//             pass_count++;
            
//             RedBox box;
//             box.x = min_x;
//             box.y = min_y;
//             box.w = aabb_w;
//             box.h = aabb_h;
//             box.area = pixel_count;
//             box.density = density;
//             box.cx = cx_f;
//             box.cy = cy_f;
//             box.obb_w = obb_w;
//             box.obb_h = obb_h;
//             box.angle = angle;
//             box.max_y = max_y;  
//             boxes.push_back(box);
//         }
//     }
    
//     printf("\n=== 检测结束: 候选区域=%d, 通过=%d, 最终框数=%zu ===\n",
//            candidate_count, pass_count, boxes.size());
    
//     delete[] mask;
//     delete[] visited;
// }







//该函数用于检测图像中的红色区域，并根据面积和密度条件筛选出符合条件的红色矩形框。
// void detect_red_regions(uint8_t* image, int width, int height,
//                        std::vector<RedBox>& boxes,
//                        int min_area, int max_area) {
    
//     const int total_pixels = width * height;
//     uint8_t* mask = new uint8_t[total_pixels];
//     uint8_t* visited = new uint8_t[total_pixels];

//     // 边界过滤阈值（根据你的 320×240 调整）
//     int min_y_threshold = 20;
//     int max_y_threshold = height - 20;
//     int min_x_threshold = 20;
//     int max_x_threshold = width - 20;
//     float min_density = 0.2f;
    
//     // 生成二值掩码
//     for (int i = 0; i < total_pixels; i++) {
//         uint8_t b = image[i * 3];
//         uint8_t g = image[i * 3 + 1];
//         uint8_t r = image[i * 3 + 2];
//         mask[i] = is_red_pixel(r, g, b, 100, 80) ? 255 : 0;
//     }
    
//     memset(visited, 0, total_pixels);
//     boxes.clear();
    
//     const int dx[4] = {0, 0, -1, 1};
//     const int dy[4] = {-1, 1, 0, 0};
    
//     // 临时存储连通域像素坐标（PCA 用）
//     std::vector<std::pair<int, int>> points;
//     points.reserve(2048);
    
//     // BFS 查找连通域（步长 1，避免漏检）
//     for (int y = 0; y < height; y+=4) {
//         for (int x = 0; x < width; x+=4) {
//             int idx = y * width + x;
//             if (mask[idx] == 0 || visited[idx]) continue;
            
//             points.clear();
//             int min_x = x, max_x = x;
//             int min_y = y, max_y = y;
//             long long sum_x = 0, sum_y = 0;
//             long long sum_x2 = 0, sum_y2 = 0, sum_xy = 0;
            
//             std::queue<std::pair<int, int>> q;
//             q.push({x, y});
//             visited[idx] = 1;
            
//             while (!q.empty()) {
//                 auto [cx, cy] = q.front();
//                 q.pop();
                
//                 points.push_back({cx, cy});
//                 sum_x += cx;  sum_y += cy;
//                 sum_x2 += 1LL * cx * cx;
//                 sum_y2 += 1LL * cy * cy;
//                 sum_xy += 1LL * cx * cy;
                
//                 if (cx < min_x) min_x = cx;
//                 if (cx > max_x) max_x = cx;
//                 if (cy < min_y) min_y = cy;
//                 if (cy > max_y) max_y = cy;
                
//                 for (int i = 0; i < 4; i++) {
//                     int nx = cx + dx[i];
//                     int ny = cy + dy[i];
//                     if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
                    
//                     int nidx = ny * width + nx;
//                     if (mask[nidx] && !visited[nidx]) {
//                         visited[nidx] = 1;
//                         q.push({nx, ny});
//                     }
//                 }
//             }
            
//             int pixel_count = (int)points.size();
//             int aabb_w = max_x - min_x + 1;
//             int aabb_h = max_y - min_y + 1;
            
//             // 快速过滤（AABB 层面）
//             if (pixel_count < min_area || pixel_count > max_area) continue;
//             if (min_y < min_y_threshold || max_y > max_y_threshold) continue;
//             if (min_x < min_x_threshold || max_x > max_x_threshold) continue;
//             if (aabb_w < 15) continue;
            
//             // ===== PCA 计算旋转矩形 =====
//             float n = pixel_count;
//             float cx_f = sum_x / n;
//             float cy_f = sum_y / n;
//             float mu20 = sum_x2 / n - cx_f * cx_f;
//             float mu02 = sum_y2 / n - cy_f * cy_f;
//             float mu11 = sum_xy / n - cx_f * cy_f;
            
//             float angle = 0.5f * atan2f(2.0f * mu11, mu20 - mu02);
            
//             float cos_a = cosf(angle);
//             float sin_a = sinf(angle);
//             float min_u = 1e9f, max_u = -1e9f;
//             float min_v = 1e9f, max_v = -1e9f;
            
//             for (auto& p : points) {
//                 float dx_ = p.first - cx_f;
//                 float dy_ = p.second - cy_f;
//                 float u =  dx_ * cos_a + dy_ * sin_a;   // 主轴投影
//                 float v = -dx_ * sin_a + dy_ * cos_a;   // 垂直轴投影
//                 min_u = std::min(min_u, u); max_u = std::max(max_u, u);
//                 min_v = std::min(min_v, v); max_v = std::max(max_v, v);
//             }
            
//             float obb_w = max_u - min_u;
//             float obb_h = max_v - min_v;
//             if (obb_w < 1.0f) obb_w = 1.0f;
//             if (obb_h < 1.0f) obb_h = 1.0f;
            
//             // 规范化：w 始终为长边
//             if (obb_w < obb_h) {
//                 std::swap(obb_w, obb_h);
//                 angle += 3.14159265f / 2.0f;
//             }
//             while (angle > 3.14159265f / 2.0f) angle -= 3.14159265f;
//             while (angle < -3.14159265f / 2.0f) angle += 3.14159265f;
            
//             float obb_area = obb_w * obb_h;
//             float density = pixel_count / obb_area;
            
//             // 旋转矩形过滤（更鲁棒，不受倾斜影响）
//             bool aspect_ok = (std::max(obb_w, obb_h) / std::min(obb_w, obb_h)) < 15.0f;
//             bool size_ok   = obb_w > 8.0f && obb_h > 4.0f;
//             bool angle_ok  = fabsf(angle) < 3.14159265f / 4.0f;  // 限制旋转角度在 ±45 度内
            
//             if (density >= min_density && aspect_ok && size_ok && angle_ok) {
//                 RedBox box;
//                 // AABB 字段（兼容旧代码：裁剪、画黄框都用这些）
//                 box.x = min_x;
//                 box.y = min_y;
//                 box.w = aabb_w;
//                 box.h = aabb_h;
//                 box.area = pixel_count;
//                 box.density = density;
                
//                 // OBB 字段（新增：旋转矩形参数）
//                 box.cx = cx_f;
//                 box.cy = cy_f;
//                 box.obb_w = obb_w;
//                 box.obb_h = obb_h;
//                 box.angle = angle;
//                 box.max_y = max_y;  
                
//                 boxes.push_back(box);
//             }
//         }
//     }
    
//     delete[] mask;
//     delete[] visited;
// }



static inline void set_pixel_bgr888(uint8_t* image, int width, int x, int y, uint32_t color) {
    int idx = (y * width + x) * 3;
    image[idx]     = (color >> 16) & 0xFF;  // B
    image[idx + 1] = (color >> 8)  & 0xFF;  // G
    image[idx + 2] =  color        & 0xFF;  // R
}


// ============================================================
// 旋转矩形辅助函数（新增）
// ============================================================


// 安全画点（带边界保护）
static inline void set_pixel_bgr888_safe(uint8_t* image, int width, int height,
                                         int x, int y, uint32_t color) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    set_pixel_bgr888(image, width, x, y, color);
}

// 获取旋转矩形四个角点
void get_rotated_box_corners(const RedBox& box, RedPoint2f corners[4]) {
    float cos_a = cosf(box.angle);
    float sin_a = sinf(box.angle);
    float hw = box.obb_w * 0.5f;
    float hh = box.obb_h * 0.5f;

    float lx[4] = {-hw, hw, hw, -hw};
    float ly[4] = {-hh, -hh, hh, hh};

    for (int i = 0; i < 4; i++) {
        corners[i].x = box.cx + (lx[i] * cos_a - ly[i] * sin_a);
        corners[i].y = box.cy + (lx[i] * sin_a + ly[i] * cos_a);
    }
}

// Bresenham 画线（支持 brush 线宽）
static void draw_line_bgr888(uint8_t* image, int width, int height,
                             int x0, int y0, int x1, int y1,
                             uint32_t color, int thickness) {
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    int r = thickness / 2;

    while (1) {
        for (int by = -r; by <= r; by++) {
            for (int bx = -r; bx <= r; bx++) {
                set_pixel_bgr888_safe(image, width, height, x0 + bx, y0 + by, color);
            }
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

// 旋转矩形画框
void draw_rotated_rect_bgr888(uint8_t* image, int width, int height,
                              const RedPoint2f corners[4],
                              uint32_t color, int thickness) {
    for (int i = 0; i < 4; i++) {
        int j = (i + 1) % 4;
        int x0 = (int)(corners[i].x + 0.5f);
        int y0 = (int)(corners[i].y + 0.5f);
        int x1 = (int)(corners[j].x + 0.5f);
        int y1 = (int)(corners[j].y + 0.5f);
        draw_line_bgr888(image, width, height, x0, y0, x1, y1, color, thickness);
    }
}




// ============================================================
// 标准坐标画框：x,y 为左上角，原点在左上
// ============================================================
void draw_rect_bgr888(uint8_t* image, int width, int height,
                     int x, int y, int w, int h, 
                     uint32_t color, int thickness) {
    // 边界保护
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (w <= 0 || h <= 0) return;
    
    int x1 = x + w - 1;
    int y1 = y + h - 1;
    
    if (x1 >= width)  x1 = width - 1;
    if (y1 >= height) y1 = height - 1;
    if (x > x1 || y > y1) return;
    
    for (int t = 0; t < thickness; t++) {
        int tx = x + t, ty = y + t;
        int tx1 = x1 - t, ty1 = y1 - t;
        if (tx > tx1 || ty > ty1) break;
        
        for (int i = tx; i <= tx1; i++) {
            set_pixel_bgr888(image, width, i, ty, color);
            set_pixel_bgr888(image, width, i, ty1, color);
        }
        for (int i = ty; i <= ty1; i++) {
            set_pixel_bgr888(image, width, tx, i, color);
            set_pixel_bgr888(image, width, tx1, i, color);
        }
    }
}

void draw_cross_bgr888(uint8_t* image, int width, int height,
                      int cx, int cy, uint32_t color, int size) {
    for (int i = -size; i <= size; i++) {
        if (cy + i >= 0 && cy + i < height)
            set_pixel_bgr888(image, width, cx, cy + i, color);
        if (cx + i >= 0 && cx + i < width)
            set_pixel_bgr888(image, width, cx + i, cy, color);
    }
}






// ============================================================
// 标准坐标裁剪：x,y 为左上角
// ============================================================
cv::Mat crop_image_to_bgr_mat(uint8_t* src_image, int src_width, int src_height,
                               int x, int y, int w, int h) {
    // 输入检查
    if (!src_image) {
        std::cerr << "Error: src_image is null!" << std::endl;
        return cv::Mat();
    }
    
    if (src_width <= 0 || src_height <= 0) {
        std::cerr << "Error: Invalid image dimensions!" << std::endl;
        return cv::Mat();
    }
    
    // 计算有效裁剪区域
    int crop_x = std::max(0, x);
    int crop_y = std::max(0, y);
    int crop_w = std::min(w, src_width - crop_x);
    int crop_h = std::min(h, src_height - crop_y);
    
    if (crop_w <= 0 || crop_h <= 0) {
        std::cerr << "Error: Invalid crop dimensions!" << std::endl;
        return cv::Mat();
    }
    
    // 创建
    cv::Mat dst_mat(crop_h, crop_w, CV_8UC3);
    
    // 使用指针逐行复制
    for (int row = 0; row < crop_h; row++) {
        int src_row = crop_y + row;
        
        // 源数据指针（BGR888格式）
        const uint8_t* src_row_ptr = src_image + (src_row * src_width + crop_x) * 3;
        
        // 目标数据指针（cv::Mat行）
        uint8_t* dst_row_ptr = dst_mat.ptr<uint8_t>(row);
        
        // 复制一行
        memcpy(dst_row_ptr, src_row_ptr, crop_w * 3);
    }
    
    return dst_mat;
}

void save_bgr888_as_bmp(const char* filename, uint8_t* bgr, int w, int h, int stride) {

    int row_padded1 = ((w * 3) + 3) & ~3;
    printf("BMP: w=%d stride=%d w*3=%d row_padded=%d\n", w, stride, w*3, row_padded1);

    if (stride <= 0) stride = w * 3;  // 默认连续内存
    
    #pragma pack(push, 1)
    struct {
        uint16_t type = 0x4D42;
        uint32_t size;
        uint16_t reserved1 = 0;
        uint16_t reserved2 = 0;
        uint32_t offset = 54;
        uint32_t dib_size = 40;
        int32_t width;
        int32_t height;
        uint16_t planes = 1;
        uint16_t bpp = 24;
        uint32_t compression = 0;
        uint32_t image_size;
        int32_t x_ppm = 2835;
        int32_t y_ppm = 2835;
        uint32_t colors_used = 0;
        uint32_t important_colors = 0;
    } hdr = {};
    #pragma pack(pop)
    
    int row_padded = ((w * 3) + 3) & ~3;
    hdr.size = 54 + row_padded * h;
    hdr.width = w;
    hdr.height = h;
    hdr.image_size = row_padded * h;
    
    FILE* fp = fopen(filename, "wb");
    if (!fp) return;
    
    fwrite(&hdr, 1, sizeof(hdr), fp);
    
    uint8_t* row_buf = (uint8_t*)calloc(1, row_padded);
    
    for (int row = h - 1; row >= 0; row--) {
        // ✅ 使用 stride 计算源行偏移
        memcpy(row_buf, bgr + row * stride, w * 3);
        fwrite(row_buf, 1, row_padded, fp);
    }
    
    free(row_buf);
    fclose(fp);
}
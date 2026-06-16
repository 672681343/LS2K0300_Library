#include "red_detecte.h"
#include "many_define.h"
#include "Recognize.h"

#include <queue>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <opencv2/opencv.hpp>
#include <time.h>
#include <lq_ntp.hpp>
#include <lq_ncnn.hpp>






// RGB565 颜色定义
#define RGB565_RED     0xF800
#define RGB565_GREEN   0x07E0
#define RGB565_BLUE    0x001F
#define RGB565_WHITE   0xFFFF
#define RGB565_BLACK   0x0000
#define RGB565_YELLOW  0xFFE0


// ========== 模型配置 ==========

// #define MODEL_INPUT_CHANNEL        3 //  定义模型的输入通道数为3（RGB三通道）
// #define MODEL_OUTPUT_CLASS_NUM     6 //  定义模型的输出类别数量为6
// #define MODEL_INPUT_SIZE           (MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT * MODEL_INPUT_CHANNEL)
// #define TFLITE_OP_RESOLVER_MAX_NUM 25 //  定义TensorFlow Lite操作解析器支持的最大操作符数量为25
// #define TENSOR_ARENA_SIZE          (324 * 1024) //  定义TensorFlow Lite的内存池大小为324KB

// // ========== 日志配置 ==========
// #define LOG_PRINT_FREQUENCY        1
// #define ENABLE_FIRST_FRAME_DEBUG   true


std::vector<RedBox> boxes; //  存储红色框对象的容器

// static uint cnt = 0;

// static std::string get_time_str(void)
// {
//     // 时间获取对象
//     static lq_ntp g_ntp;
//     return g_ntp.get_local_time_str();
// }


cv::Mat crop_image_to_bgr_mat(uint8_t* src_image, int src_width, int src_height,
                               int x, int y, int w, int h) ;





// ============================================================
// 主处理：坐标全部按标准图像坐标（左上角原点）
// ============================================================
void process_image(uint8_t* bgr888_image, int frame_count) {

    // detect_red_regions(bgr888_image, UVC_WIDTH, UVC_HEIGHT, boxes, 20, 2600);
    // // printf("DEBUG: box=(%d,%d,%d,%d), cx=%d, cy=%d, crop=(%d,%d,%d)\n",
    // //    box.x, box.y, box.w, box.h, cx, cy, crop_x, crop_y, crop_size);
    // frame_count++;  // 帧计数+1


    // if (!boxes.empty() && (frame_count % 1 == 0)) {  // 每10帧自动保存一张, 并且只保存第一个检测到的红色区域（如果有的话）
    //     cnt++;
    //     auto& box = boxes[0]; //  获取检测到的第一个边界框
    //     // ========== 新增：先保存原始图像（带黄色裁剪框） ==========
    //     // // 创建原图的副本，避免画框影响后续裁剪
    //     uint8_t *debug_image = new uint8_t[UVC_WIDTH * UVC_HEIGHT * 3];
    //     memcpy(debug_image, bgr888_image, UVC_WIDTH * UVC_HEIGHT * 3);

    //     int process_height = MODEL_INPUT_HEIGHT; // 裁剪高度固定为48像素，宽度根据检测到的红色区域和模型输入大小调整（适配68*68的）
    //     int process_width = box.w + 15;   // 裁剪宽度box.w + 8（适配40*40）
    //     if(process_width < MODEL_INPUT_WIDTH){
    //         process_width = MODEL_INPUT_WIDTH; // 保持宽度不变，增加一些边距
    //     }
        // process_height = process_width;// 保持宽高一致，适配模型输入要求

        // int crop_x = box.x - 8;  // 水平偏左一点
            
        // 框底边 + 偏移 - 裁剪高度 = 裁剪区域顶部
        // 这样框底边在裁剪区域底部附近
        // int crop_y = box.y + box.h + 15 - process_height; //算的是裁剪区域的左上角坐标


            // // 边界限幅
            // if (crop_x < 0) crop_x = 0;
            // if (crop_y < 0) crop_y = 0;
            // if (crop_x + process_width > UVC_WIDTH) crop_x = UVC_WIDTH - process_width;
            // if (crop_y + process_height > UVC_HEIGHT) crop_y = UVC_HEIGHT - process_height;

            
            // printf("裁剪前: box=(%d,%d,%d,%d), crop=(%d,%d,%d,%d)\n",
            //     box.x, box.y, box.w, box.h, crop_x, crop_y, process_width, process_height);


        // // === 画框（用限幅后的坐标）=== //这个调试的时候会用到
        // draw_rect_bgr888(bgr888_image, UVC_WIDTH, UVC_HEIGHT,
        //                 box.x, box.y, box.w, box.h,
        //                 RGB565_YELLOW, 2);


        // // // 保存原始调试图
        // char debug_path[128];
        // snprintf(debug_path, sizeof(debug_path), "/tmp/debug_original_%d_%d.bmp", cnt, frame_count);
        // save_bgr888_as_bmp(debug_path, debug_image, UVC_WIDTH, UVC_HEIGHT, UVC_WIDTH * 3);

        // delete[] debug_image;
        // ========== 新增结束 ==========


        // // === 裁剪（用同样的坐标）===
        // cv::Mat cropped_mat = crop_image_to_bgr_mat(
        //     bgr888_image, IMG_WIDTH, IMG_HEIGHT,
        //     crop_x, crop_y, process_width, process_height);

        // if (cropped_mat.data) {
        
        //     printf("========================================\n");
        //     printf("       NCNN 图像分类推理示例\n");
        //     printf("========================================\n\n");

        //     // ==================== 配置区域 ====================
    
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


        //     if (cropped_mat.empty()) {
        //         printf("请确保图片文件存在\n");
        //         return ;
        //     }
        //     printf("[%s] 图片尺寸: %d x %d\n\n", get_time_str().c_str(), cropped_mat.cols, cropped_mat.rows);

        //     // 注意: OpenCV读取的是BGR格式，但在推理时会自动转换为RGB格式以匹配训练时的输入
        //     // 训练使用PIL读取的RGB格式，因此需要色彩空间转换

        //     // 推理
        //     printf("[%s] 开始推理...\n", get_time_str().c_str());
        //     auto start = std::chrono::high_resolution_clock::now();
        //     std::string result = ncnn.Infer(cropped_mat);
        //     auto end = std::chrono::high_resolution_clock::now();
        //     auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        //     // 输出结果
        //     printf("\n========================================\n");
        //     printf("推理结果: %s\n", result.c_str());
        //     printf("推理耗时: %ld ms\n", duration.count());
        //     printf("========================================\n");

        // }

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
bool is_red_pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t min_red, uint8_t min_diff) {
    return (r >= min_red) && (r > g + min_diff) && (r > b + min_diff);
}

//该函数用于检测图像中的红色区域，并根据面积和密度条件筛选出符合条件的红色矩形框。
void detect_red_regions(uint8_t* image, int width, int height,
                       std::vector<RedBox>& boxes,
                       int min_area, int max_area) {        // 最小填充密度
    
    const int total_pixels = width * height;
    uint8_t* mask = new uint8_t[total_pixels];
    uint8_t* visited = new uint8_t[total_pixels];




    int min_y_threshold = 20;          // min_y 必须大于此值（上侧边界裁剪值）
    int max_y_threshold = 95;           // min_y 必须小于 height - 此值（下侧边界裁剪值）
    int min_x_threshold = 45;            // min_x 必须大于此值  （左侧边界裁剪值）
    int max_x_threshold = 115;           // max_x 必须小于 width - 此值（右侧边界裁剪值）
    float min_density = 0.2f;
    
    // 生成二值掩码
    for (int i = 0; i < total_pixels; i++) {
        uint8_t b = image[i * 3];
        uint8_t g = image[i * 3 + 1];
        uint8_t r = image[i * 3 + 2];
        mask[i] = is_red_pixel(r, g, b, 100, 30) ? 255 : 0;

    }
    
    memset(visited, 0, total_pixels);
    boxes.clear();
    
    const int dx[4] = {0, 0, -1, 1};
    const int dy[4] = {-1, 1, 0, 0};
    
    // BFS 查找连通域
    for (int y = 0; y < height; y = y + 4) {
        for (int x = 0; x < width; x = x + 4) {
            int idx = y * width + x;
            if (mask[idx] == 0 || visited[idx]) continue;
            
            int min_x = x, max_x = x;
            int min_y = y, max_y = y;
            int pixel_count = 0;
            
            std::queue<std::pair<int, int>> q;
            q.push({x, y});
            visited[idx] = 1;
            
            while (!q.empty()) {
                auto [cx, cy] = q.front();
                q.pop();
                pixel_count++;
                
                if (cx < min_x) min_x = cx; //  更新当前连通区域的最小和最大x坐标
                if (cx > max_x) max_x = cx;
                if (cy < min_y) min_y = cy; //  更新当前连通区域的最小和最大y坐标
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
            
            // 过滤并记录（使用参数化的阈值）
            if (pixel_count >= min_area && 
                pixel_count <= max_area && 
                min_y < max_y_threshold &&   
                min_y > min_y_threshold &&
                min_x > min_x_threshold && 
                max_x < max_x_threshold &&
                (max_x - min_x) > 15 ){ 
                
                RedBox box;
                box.x = min_x; //  设置框的左上角x坐标为最小x值
                box.y = min_y; //  设置框的左上角y坐标为最小y值
                box.w = max_x - min_x + 1; //  计算框的宽度，最大x值减去最小x值加1（包含边界）
                box.h = max_y - min_y + 1; //  计算框的高度，最大y值减去最小y值加1（包含边界）
                box.area = pixel_count; //  设置框的像素数量
                
                int bbox_area = box.w * box.h;
                box.density = bbox_area > 0 ? (float)pixel_count / bbox_area : 0;
                
                if (box.density > min_density) {
                    boxes.push_back(box);
                }
            }
        }


    }
    
    // 释放内存
    delete[] mask; //  释放动态分配的内存
    delete[] visited;
}

void detect_red_regions_H(uint8_t *image, int width, int height,
                        std::vector<RedBox> &boxes,
                        int min_area, int max_area)
{ // 最小填充密度


        // 保存 320x240 BGR
        save_bgr888_as_bmp("/tmp/debug_320x240.bmp", rgb_image, 320, 240, 320 * 3);

        // 保存 160x120 BGR
        save_bgr888_as_bmp("/tmp/debug_160x120.bmp", rgb_120_image, 160, 120, 160 * 3);


    const int total_pixels = width * height;
    uint8_t *mask = new uint8_t[total_pixels];
    uint8_t *visited = new uint8_t[total_pixels];

    int min_y_threshold = 20;  // min_y 必须大于此值（上侧边界裁剪值）
    int max_y_threshold = 200;  // min_y 必须小于 height - 此值（下侧边界裁剪值）
    int min_x_threshold = 80;  // min_x 必须大于此值  （左侧边界裁剪值）
    int max_x_threshold = 240; // max_x 必须小于 width - 此值（右侧边界裁剪值）
    float min_density = 0.2f;

    // 生成二值掩码
    for (int i = 0; i < total_pixels; i++)
    {
        uint8_t b = image[i * 3];
        uint8_t g = image[i * 3 + 1];
        uint8_t r = image[i * 3 + 2];
        mask[i] = is_red_pixel(r, g, b, 100, 30) ? 255 : 0;
    }

    memset(visited, 0, total_pixels);
    boxes.clear();

    const int dx[4] = {0, 0, -1, 1};
    const int dy[4] = {-1, 1, 0, 0};

    // BFS 查找连通域
    for (int y = 0; y < height; y = y + 4)
    {
        for (int x = 0; x < width; x = x + 4)
        {
            int idx = y * width + x;
            if (mask[idx] == 0 || visited[idx])
                continue;

            int min_x = x, max_x = x;
            int min_y = y, max_y = y;
            int pixel_count = 0;

            std::queue<std::pair<int, int>> q;
            q.push({x, y});
            visited[idx] = 1;

            while (!q.empty())
            {
                auto [cx, cy] = q.front();
                q.pop();
                pixel_count++;

                if (cx < min_x)
                    min_x = cx; //  更新当前连通区域的最小和最大x坐标
                if (cx > max_x)
                    max_x = cx;
                if (cy < min_y)
                    min_y = cy; //  更新当前连通区域的最小和最大y坐标
                if (cy > max_y)
                    max_y = cy;

                for (int i = 0; i < 4; i++)
                {
                    int nx = cx + dx[i];
                    int ny = cy + dy[i];
                    if (nx < 0 || nx >= width || ny < 0 || ny >= height)
                        continue;

                    int nidx = ny * width + nx;
                    if (mask[nidx] && !visited[nidx])
                    {
                        visited[nidx] = 1;
                        q.push({nx, ny});
                    }
                }
            }

            // 过滤并记录（使用参数化的阈值）
            if (pixel_count >= min_area &&
                pixel_count <= max_area &&
                min_y < max_y_threshold &&
                min_y > min_y_threshold &&
                min_x > min_x_threshold &&
                max_x < max_x_threshold &&
                (max_x - min_x) > 30)
            {

                RedBox box;
                box.x = min_x;             //  设置框的左上角x坐标为最小x值
                box.y = min_y;             //  设置框的左上角y坐标为最小y值
                box.w = max_x - min_x + 1; //  计算框的宽度，最大x值减去最小x值加1（包含边界）
                box.h = max_y - min_y + 1; //  计算框的高度，最大y值减去最小y值加1（包含边界）
                box.area = pixel_count;    //  设置框的像素数量

                int bbox_area = box.w * box.h;
                box.density = bbox_area > 0 ? (float)pixel_count / bbox_area : 0;

                if (box.density > min_density)
                {
                    boxes.push_back(box);
                }
            }
        }
    }

    // 释放内存
    delete[] mask; //  释放动态分配的内存
    delete[] visited;
}

static inline void set_pixel_bgr888(uint8_t* image, int width, int x, int y, uint32_t color) {
    int idx = (y * width + x) * 3;
    image[idx]     = (color >> 16) & 0xFF;  // B
    image[idx + 1] = (color >> 8)  & 0xFF;  // G
    image[idx + 2] =  color        & 0xFF;  // R
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
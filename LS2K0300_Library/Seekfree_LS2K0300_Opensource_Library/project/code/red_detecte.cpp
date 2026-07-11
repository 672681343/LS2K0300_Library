#include "red_detecte.h"
#include "many_define.h"
#include "Recognize.h"

#include <queue>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <opencv2/opencv.hpp>
#include <time.h>

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


std::vector<RedBox> boxes; //  存储红色框对象的容器



RedPoint2f corners[4];

static uint cnt = 0;
cv::Mat crop_image_to_bgr_mat(uint8_t* src_image, int src_width, int src_height,
                               int x, int y, int w, int h) ;

// ============================================================
// 主处理：坐标全部按标准图像坐标（左上角原点）
// ============================================================
void process_image(uint8_t* bgr888_image, int* frame_count) {
    // // 打印左上角像素信息（已注释）
    // printf("Pixel(0,0): B=%d, G=%d, R=%d\n", 
    //        bgr888_image[0], bgr888_image[1], bgr888_image[2]);
    
    // // 打印图像中心像素信息（已注释）
    // int center = (IMG_HEIGHT/2 * IMG_WIDTH + IMG_WIDTH/2) * 3;
    // printf("Pixel(center): B=%d, G=%d, R=%d\n", 
    //        bgr888_image[center], bgr888_image[center+1], bgr888_image[center+2]);
    

    // 检测图像中的红色区域
    detect_red_regions(bgr888_image, IMG_WIDTH, IMG_HEIGHT, boxes, 400, 2200);
        printf("第 %d 帧: 检测到 %zu 个红色区域\n", *frame_count, boxes.size());
    // printf("DEBUG: box=(%d,%d,%d,%d), cx=%d, cy=%d, crop=(%d,%d,%d)\n",
    //    box.x, box.y, box.w, box.h, cx, cy, crop_x, crop_y, crop_size);
    (*frame_count)++;  // 帧计数+1

    if (!boxes.empty() && (*frame_count % 1 == 0)) {
        cnt++;
        auto& box = boxes[0];

        // ========== 基于旋转红框的裁剪参数计算 ==========
        // 获取旋转角点（用于画框和计算极值）
        get_rotated_box_corners(box, corners);

        // 裁剪尺寸：基于旋转框实际宽度，保持正方形
        float angle_factor = 1.0f + 0.6f * fabsf(sinf(box.angle));

        int process_width = (int)(box.obb_w * 1.5f* angle_factor);
        if (process_width < MODEL_INPUT_WIDTH) {
            process_width = MODEL_INPUT_WIDTH;
        }
        int process_height = process_width;

        // 红框最下端 + 边距 作为裁剪底部，向上裁剪
        int crop_bottom = box.max_y + 16;
        if (crop_bottom > IMG_HEIGHT) crop_bottom = IMG_HEIGHT;

        float angle_offset = sinf(box.angle) * (box.obb_h * 1.8f);
        
        int crop_y = crop_bottom - process_height;
        int crop_x = (int)(box.cx - process_width / 2.0f) + angle_offset;

        // --- 边界保护（左右）---
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

        // --- 边界保护（上下：红框必须在裁剪底部附近）---
        if (crop_y < 0) {
            // 上方空间不足，压缩高度但保持底部对齐
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

        // ========== 画框与保存（原有逻辑保留）==========
        uint8_t* debug_image = new uint8_t[IMG_WIDTH * IMG_HEIGHT * 3];
        memcpy(debug_image, bgr888_image, IMG_WIDTH * IMG_HEIGHT * 3);

        // // 黄色 AABB 框（保留，调试用）
        // draw_rect_bgr888(debug_image, UVC_WIDTH, UVC_HEIGHT,
        //                 box.x, box.y, box.w, box.h,
        //                 RGB565_YELLOW, 2);
        // draw_rect_bgr888(bgr888_image, UVC_WIDTH, UVC_HEIGHT,
        //                 box.x, box.y, box.w, box.h,
        //                 RGB565_YELLOW, 2);

        // // 绿色旋转框
        // draw_rotated_rect_bgr888(bgr888_image, IMG_WIDTH, IMG_HEIGHT, corners, 0x00FF00, 2);

        // // 红色裁剪框（调试用，显示实际裁剪区域）
        // draw_rect_bgr888(debug_image, IMG_WIDTH, IMG_HEIGHT,
        //                 crop_x, crop_y, process_width, process_height,
        //                 RGB565_RED, 1);

        // // 保存原始调试图
        // char debug_path[128];
        // snprintf(debug_path, sizeof(debug_path), "/tmp/debug_original_%d_%d_%d_%.2f.bmp", cnt, *frame_count, box.area, box.angle);
        // save_bgr888_as_bmp(debug_path, debug_image, IMG_WIDTH, IMG_HEIGHT, IMG_WIDTH * 3);
        // delete[] debug_image;
        // ========== 新增结束 ==========

        // === 裁剪（用旋转框计算出的新坐标）===
        cv::Mat cropped_mat = crop_image_to_bgr_mat(
            bgr888_image, UVC_WIDTH, UVC_HEIGHT,
            crop_x, crop_y, process_width, process_height
        );

        if (cropped_mat.data) {
            // 获取带毫秒的时间戳
            struct timeval tv;
            gettimeofday(&tv, NULL);
            time_t now = tv.tv_sec;
            struct tm *tm_info = localtime(&now);
            
            char timestamp[30];
            strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);
            
            // 添加毫秒部分
            char full_timestamp[40];  // 增加缓冲区大小
            long ms = tv.tv_usec / 1000;  // 使用不同的变量名，避免冲突
            
            // 确保毫秒在有效范围内
            if (ms < 0) ms = 0;
            if (ms > 999) ms = 999;
            
            snprintf(full_timestamp, sizeof(full_timestamp), "%s_%03ld", timestamp, ms);
            
            // printf("已经保存了 %d 张图片\n", cnt); 
            // printf("裁剪成功，裁剪图像尺寸: %dx%d ", cropped_mat.cols, cropped_mat.rows);   
                                                                         
            char path[128];
            // InferenceResult inference_result = perform_inference(
            //     cropped_mat, 
            //     *frame_count, 
            //     (ENABLE_FIRST_FRAME_DEBUG && !first_frame_logged), 
            //     *interpreter,
            //     USE_INT8_MODEL          
            // );
            // // 更新first_frame_logged标志（如果需要）
            // if (ENABLE_FIRST_FRAME_DEBUG && !first_frame_logged) {
            //     first_frame_logged = true;
            // }
            
            // //测试模型效果时使用//
            // snprintf(path, sizeof(path), "/tmp/%s_%d_%d_%d_%d_%d___%s.bmp",
            //         class_labels[inference_result.class_index], cnt, *frame_count, box.area, box.y + box.h, process_height, full_timestamp);

            // //拍数据集的时候用// 把时间戳加入进去，就不会有重名覆盖的风险了（拍数据集的时候用）
            snprintf(path, sizeof(path), "/tmp/weapon_p1_1_%d_%d_%d_%d_%d___%s.bmp",
                    cnt, *frame_count, box.area, box.y + box.h, process_height, full_timestamp);

            if (cnt <= 3000) {  //这个决定单次保存的图片数量，拍数据集的时候用
                save_bgr888_as_bmp(path, cropped_mat.data,  //调试和取数据集用
                                cropped_mat.cols, cropped_mat.rows, 
                                cropped_mat.step);
            }

            //增补时，前面加上一个add就好了add_，比如add_weapon_p2_1_*.bmp，这样就不会和之前的数据集重名了
            //新的文件名格式
            //traffic_p1_1_*_*_*_*_*.bmp --> 是装甲车(白色背景)√
            //traffic_p1_2_*_*_*_*_*.bmp --> 是装甲车(杂乱背景)√
            //traffic_p2_1_*_*_*_*_*.bmp --> 是救护车(白色背景)√
            //traffic_p2_2_*_*_*_*_*.bmp --> 是救护车(杂乱背景)√

            //weapon_p1_1_*_*_*_*_*.bmp --> 是步枪(白色背景)--（还没拍--2026-7-11-5:52）（）
            //weapon_p1_2_*_*_*_*_*.bmp --> 是手枪(白色背景)
            //weapon_p2_1_*_*_*_*_*.bmp --> 是手榴弹(白色背景)
            //weapon_p2_2_*_*_*_*_*.bmp --> 是手榴弹(杂乱背景) 
            //weapon_p2_3_*_*_*_*_*.bmp --> 是C4(杂乱和白天背景) ×××不拍这个

            //materials_p1_1_*_*_*_*_*.bmp --> 是望远镜(白色背景) 
            //materials_p2_1_*_*_*_*_*.bmp --> 是急救包(白色背景)
            //materials_p2_2_*_*_*_*_*.bmp --> 是急救包(杂乱背景) 


            //旧文件格式
            //traffic_*_*_1.jpg --> 是装甲车
            //traffic_*_*_2.jpg --> 是救护车

            //weapon_*_*_1.jpg --> 是枪
            //weapon_*_*_2.jpg --> 是爆炸物

            //materials_*_*_1.jpg --> 是望远镜
            //materials_*_*_2.jpg --> 是急救包
            

            // printf("[保存的第 %d 个图片]红框: x=%d, y=%d, w=%d, h=%d | 裁剪: x=%d, y=%d, size=%dx%d | 时间戳: %s\n", //拍数据集时一起用
            //     cnt, box.x, box.y, box.w, box.h, crop_x, crop_y, process_width, process_height, full_timestamp);

                printf("[保存的第 %d 个图片]时间戳: %s\n", //拍数据集时一起用
                    cnt, full_timestamp);

            // printf("Box: Area=%d at (%d,%d) size %dx%d, 文件名: %s\n", //之前用的这个打印
            //    box.area, box.x, box.y, box.w, box.h, path);
        
        }
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
bool is_red_pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t min_red, uint8_t min_diff) {
    return (r >= min_red) && (r > g + min_diff) && (r > b + min_diff);
}

//该函数用于检测图像中的红色区域，并根据面积和密度条件筛选出符合条件的红色矩形框。
void detect_red_regions(uint8_t* image, int width, int height,
                       std::vector<RedBox>& boxes,
                       int min_area, int max_area) {
    
    const int total_pixels = width * height;
    uint8_t* mask = new uint8_t[total_pixels];
    uint8_t* visited = new uint8_t[total_pixels];

    // 边界过滤阈值（根据你的 320×240 调整）
    int min_y_threshold = 20;
    int max_y_threshold = height - 20;
    int min_x_threshold = 20;
    int max_x_threshold = width - 20;
    float min_density = 0.2f;
    
    // 生成二值掩码
    for (int i = 0; i < total_pixels; i++) {
        uint8_t b = image[i * 3];
        uint8_t g = image[i * 3 + 1];
        uint8_t r = image[i * 3 + 2];
        mask[i] = is_red_pixel(r, g, b, 100, 80) ? 255 : 0;
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
            bool aspect_ok = (std::max(obb_w, obb_h) / std::min(obb_w, obb_h)) < 6.0f;
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

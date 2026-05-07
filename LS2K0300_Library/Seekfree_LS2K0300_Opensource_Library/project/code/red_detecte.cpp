#include "red_detecte.h"
#include "many_define.h"
#include "Recognize.h"

#include <queue>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <opencv2/opencv.hpp>

//后续裁剪功能可以关掉，要看具体识别的速度和准确率

// 图像尺寸（根据你的IPS200屏幕修改）
#define IMG_WIDTH  160  
#define IMG_HEIGHT 120



// RGB565 颜色定义
#define RGB565_RED     0xF800
#define RGB565_GREEN   0x07E0
#define RGB565_BLUE    0x001F
#define RGB565_WHITE   0xFFFF
#define RGB565_BLACK   0x0000
#define RGB565_YELLOW  0xFFE0


std::vector<RedBox> boxes; //  存储红色框对象的容器


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
bool is_red_pixel(uint16_t pixel, uint8_t min_red, uint8_t min_diff) {
    uint8_t r = (pixel >> 11) & 0x1F; //  从16位像素值中提取RGB分量 这里的像素格式通常是RGB565，其中R占5位，G占6位，B占5位 提取红色分量(R)：将像素值右移11位，然后与0x1F(二进制00011111)进行与操作，获取低5位作为红色值
    uint8_t g = (pixel >> 5) & 0x3F;
    uint8_t b = pixel & 0x1F;
    
    return (r >= min_red) && (r > g + min_diff) && (r > b + min_diff);
}

//该函数用于检测图像中的红色区域，并根据面积和密度条件筛选出符合条件的红色矩形框。
void detect_red_regions(uint16_t* image, int width, int height,
                       std::vector<RedBox>& boxes,
                       int min_area, int max_area) {         // 最小填充密度
    
    const int total_pixels = width * height;
    uint8_t* mask = new uint8_t[total_pixels];
    uint8_t* visited = new uint8_t[total_pixels];

        // 边界过滤阈值（新增参数，方便外部配置）
    int min_y_threshold = 85;          // min_y 必须大于此值（上侧边界裁剪值）
    int max_y_threshold = 5;           // min_y 必须小于 height - 此值（下侧边界裁剪值）
    int min_x_threshold = 5;            // min_x 必须大于此值  （左侧边界裁剪值）
    int max_x_threshold = 85;           // max_x 必须小于 width - 此值（右侧边界裁剪值）
    float min_density = 0.2f;
    
    // 生成二值掩码
    for (int i = 0; i < total_pixels; i++) {
        mask[i] = is_red_pixel(image[i]) ? 255 : 0;
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
            
            // 过滤并记录（使用参数化的阈值）
            if (pixel_count >= min_area && 
                pixel_count <= max_area && 
                min_y < height - max_y_threshold &&   // 原：UVC_HEIGHT - 5，改为通用 height
                min_y > min_y_threshold &&
                min_x > min_x_threshold && 
                max_x < width - max_x_threshold) {    // 原：UVC_WIDTH - 85，改为通用 width
                
                RedBox box;
                box.x = min_x;
                box.y = min_y;
                box.w = max_x - min_x + 1;
                box.h = max_y - min_y + 1;
                box.area = pixel_count;
                
                int bbox_area = box.w * box.h;
                box.density = bbox_area > 0 ? (float)pixel_count / bbox_area : 0;
                
                if (box.density > min_density) {
                    boxes.push_back(box);
                }
            }
        }
    }
    
    delete[] mask;
    delete[] visited;
}

//在指定图像上绘制一个指定颜色和粗细的矩形
void draw_rect_rgb565(uint16_t* image, int width, int height,
                     int x, int y, int w, int h, 
                     uint16_t color, int thickness) {
    // y 是底边坐标（越大越靠上），转换为左上角
    int top_y = y - h + 1;
    
    // 边界检查
    if (x < 0) x = 0;
    if (top_y < 0) top_y = 0;
    if (y >= height) y = height - 1;
    
    int x1 = x + w - 1;
    int y1 = y;  // 底边就是 y
    
    if (x1 >= width) x1 = width - 1;
    if (y1 >= height) y1 = height - 1;
    
    // 画线（和原来一样，只是 y 变成了 top_y）
    for (int t = 0; t < thickness; t++) {
        int tx = x + t, ty = top_y + t;      // 从顶边开始
        int tx1 = x1 - t, ty1 = y1 - t;      // 底边内缩
        if (tx > tx1 || ty > ty1) break;
        
        // 上下横线
        for (int i = tx; i <= tx1; i++) {
            if (ty < height) image[ty * width + i] = color;     // 上边
            if (ty1 < height) image[ty1 * width + i] = color;   // 下边
        }
        // 左右竖线
        for (int i = ty; i <= ty1; i++) {
            if (tx < width) image[i * width + tx] = color;      // 左边
            if (tx1 < width) image[i * width + tx1] = color;    // 右边
        }
    }
}



//在指定图像上绘制一个十字，十字的中心位置和颜色可自定义。
void draw_cross_rgb565(uint16_t* image, int width, int height,
                      int cx, int cy, uint16_t color, int size) {
    for (int i = -size; i <= size; i++) {
        if (cy + i >= 0 && cy + i < height)
            image[(cy + i) * width + cx] = color;
        if (cx + i >= 0 && cx + i < width)
            image[cy * width + (cx + i)] = color;
    }
}





/*
src_image：指向源图像数据的指针，图像数据为uint16_t类型
src_width：源图像的宽度（像素数）
src_height：源图像的高度（像素数）
x：裁剪区域左上角的x坐标
y：裁剪区域右下角的y坐标
w：裁剪区域的宽度
h：裁剪区域的高度
*/
//该函数用于从源图像中裁剪出一个指定矩形区域的图像，并返回裁剪后的图像数据
uint16_t* crop_image(uint16_t* src_image, int src_width, int src_height,
                    int x, int y, int w, int h) {
    // 边界检查
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    
    // y 是底边坐标，转换为顶边
    int top_y = y - h + 1;
    if (top_y < 0) {
        h += top_y;  // 调整高度
        top_y = 0;
    }
    if (y >= src_height) y = src_height - 1;
    if (x + w > src_width) w = src_width - x;
    if (w <= 0 || h <= 0) return nullptr;
    
    // 分配内存
    uint16_t* dst_image = (uint16_t*)malloc(w * h * sizeof(uint16_t));
    if (!dst_image) return nullptr;
    
    // 逐行复制（从顶边开始，向下拷）
    for (int row = 0; row < h; row++) {
        int src_y = top_y + row;  // 源图像行（从上往下）
        int dst_offset = row * w;
        int src_offset = src_y * src_width + x;
        
        memcpy(&dst_image[dst_offset], &src_image[src_offset], w * sizeof(uint16_t));
    }
    
    return dst_image; //  返回处理后的图像数据
}

//该函数将RGB565格式的图像数据转换为JPG格式并保存到指定文件。
void save_rgb565_as_jpg(const char* filename, uint16_t* rgb565, int w, int h) {
    // RGB565 -> BGR (OpenCV 格式)
    cv::Mat bgr(h, w, CV_8UC3); //  将RGB565格式的图像转换为BGR格式的OpenCV Mat对象 参数说明： h: 图像的高度（行数） w: 图像的宽度（列数） CV_8UC3: 表示每个像素由3个8位无符号字符（即BGR三个通道）组成
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint16_t p = rgb565[y * w + x]; //  从RGB565格式图像中获取像素值
            uint8_t r = ((p >> 11) & 0x1F) << 3; //  提取红色分量(R5)，并将其扩展到8位
            uint8_t g = ((p >> 5) & 0x3F) << 2; //  提取绿色分量(G6)，并将其扩展到8位
            uint8_t b = (p & 0x1F) << 3; //  提取蓝色分量(B5)，并将其扩展到8位
            
            bgr.at<cv::Vec3b>(y, x) = cv::Vec3b(b, g, r);  // BGR 顺序 //  将RGB值存储为BGR格式的OpenCV Vec3b向量
        }
    }
    
    // 直接保存为 JPG
    std::vector<int> params; //  定义一个整型向量，用于存储图像编码参数
    params.push_back(cv::IMWRITE_JPEG_QUALITY); //  添加JPEG质量参数标志
    params.push_back(95);  // 质量 0-100
    
    cv::imwrite(filename, bgr, params); //  将处理后的图像保存到文件 参数:   filename - 要保存的文件名   bgr - 要保存的BGR格式图像数据   params - 保存参数，如压缩质量等
}



//该函数处理RGB565格式的图像，检测其中的红色区域，并在检测到红色区域时，每隔一定帧数裁剪并保存该区域为JPEG图片。
void process_image(uint16_t* rgb565_image, int *frame_count) {
    
    detect_red_regions(rgb565_image, UVC_WIDTH, UVC_HEIGHT, boxes, 40, 1600); 

    // printf("Detected %zu boxes\n", boxes.size());
    (*frame_count)++;  // 帧计数+1
        printf("第%d帧：处理完成\n", *frame_count);
    
    // 每20帧自动保存一张, 并且只保存第一个检测到的红色区域（如果有的话）
    if (!boxes.empty() && (*frame_count % 20 == 0)) {   // && (frame_count % 10 == 0)
        auto& box = boxes[0]; //  获取检测到的第一个边界框

        
        // // 画框
        // draw_rect_rgb565(rgb565_image, UVC_WIDTH, UVC_HEIGHT, //  绘制矩形框，使用RGB565格式 参数：图像数据、图像宽度、图像高度、矩形坐标和尺寸、颜色、线宽
        //                 box.x - 2, box.y +2, box.w + 4, box.h+15, 
        //                 RGB565_YELLOW, 4);
        
        // // 裁剪并保存
        // uint16_t* cropped = crop_image(rgb565_image, UVC_WIDTH, UVC_HEIGHT, //  从原始图像中裁剪出指定区域         参数：原始图像、图像宽度、图像高度、裁剪起点x坐标、裁剪起点y坐标、裁剪宽度、裁剪高度
        //                               box.x - 2, box.y+2, box.w + 5, box.h+15);

                                              // 裁剪并保存
        if(box.w <= MODEL_INPUT_WIDTH-5){
            box.w = MODEL_INPUT_WIDTH-5; // 保持宽度不变，增加一些边距
        }

        uint16_t* cropped = crop_image(rgb565_image, UVC_WIDTH, UVC_HEIGHT, //  从原始图像中裁剪出指定区域         参数：原始图像、图像宽度、图像高度、裁剪起点x坐标、裁剪起点y坐标、裁剪宽度、裁剪高度
                                      box.x - 5, box.y+5, box.w + 5, 45);
        if (cropped) {
                                                                                                                               
            char path[128];
            // // 保存到 /tmp（内存中），之后用scp拷出
            // snprintf(path, sizeof(path), "/tmp/car_%d_%d.jpg",  //  格式化生成文件名，包含帧数和区域面积信息
            //         *frame_count, box.area);
            // // save_rgb565_as_jpg(path, cropped, box.w + 4, box.h + 15); //  将裁剪后的RGB565图像保存为JPG格式
            // save_rgb565_as_jpg(path, cropped, box.w + 5, 45); //  将裁剪后的RGB565图像保存为JPG格式


            // printf("Box: Area=%d at (%d,%d) size %dx%d, 文件名: %s\n",
            //    box.area, box.x, box.y, box.w, box.h, path);

            // 创建彩色Mat (BGR格式)
            cv::Mat cropped_mat(45, box.w + 5, CV_8UC3);

            // 将RGB565转换为BGR
            for (int y = 0; y < 45; y++) {
                for (int x = 0; x < box.w + 5; x++) {
                    uint16_t pixel = cropped[y * (box.w + 5) + x];
                    
                    // 提取RGB565分量
                    uint8_t r5 = (pixel >> 11) & 0x1F;  // 红色5位
                    uint8_t g6 = (pixel >> 5) & 0x3F;   // 绿色6位
                    uint8_t b5 = pixel & 0x1F;          // 蓝色5位
                    
                    // 扩展到8位
                    uint8_t r = (r5 << 3) | (r5 >> 2);
                    uint8_t g = (g6 << 2) | (g6 >> 4);
                    uint8_t b = (b5 << 3) | (b5 >> 2);
                    
                    // OpenCV使用BGR顺序
                    cv::Vec3b& dst = cropped_mat.at<cv::Vec3b>(y, x);
                    dst[0] = b;  // Blue
                    dst[1] = g;  // Green
                    dst[2] = r;  // Red
                }
            }

            // printf("转换后: %dx%d, 通道=%d, 类型=%d (CV_8UC3)\n", 
            //     cropped_mat.cols, cropped_mat.rows, 
            //     cropped_mat.channels(), cropped_mat.type());


            InferenceResult inference_result = perform_inference(
                cropped_mat, 
                *frame_count, 
                (ENABLE_FIRST_FRAME_DEBUG && !first_frame_logged), 
                *interpreter
            );

            // 更新first_frame_logged标志（如果需要）
            if (ENABLE_FIRST_FRAME_DEBUG && !first_frame_logged) {
                first_frame_logged = true;
            }
            

            // 保存到 /tmp（内存中），之后用scp拷出
            snprintf(path, sizeof(path), "/tmp/%s_%d_%d.jpg",  //  格式化生成文件名，包含帧数和区域面积信息
                    class_labels[inference_result.class_index], *frame_count, box.area);
            // save_rgb565_as_jpg(path, cropped, box.w + 4, box.h + 15); //  将裁剪后的RGB565图像保存为JPG格式
            save_rgb565_as_jpg(path, cropped, box.w + 5, 45); //  将裁剪后的RGB565图像保存为JPG格式


            printf("Box: Area=%d at (%d,%d) size %dx%d, 文件名: %s\n",
               box.area, box.x, box.y, box.w, box.h, path);
             

            free(cropped); //  释放裁剪图像的内存
        }




    }

    // if (boxes.empty()) {
    //     frame_count = 0; // 没有检测到红色区域，重置帧计数器
    // }
    

}
#include "red_detecte.h"
#include <queue>
#include <cstring>
#include <algorithm>
#include <cstdio>
#include <opencv2/opencv.hpp>

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


std::vector<RedBox> boxes;

// 函数实现（不写默认参数，只写参数类型）
bool is_red_pixel(uint16_t pixel, uint8_t min_red, uint8_t min_diff) {
    uint8_t r = (pixel >> 11) & 0x1F;
    uint8_t g = (pixel >> 5) & 0x3F;
    uint8_t b = pixel & 0x1F;
    
    return (r >= min_red) && (r > g + min_diff) && (r > b + min_diff);
}

void detect_red_regions(uint16_t* image, int width, int height,
                       std::vector<RedBox>& boxes,
                       int min_area, int max_area) {
    
    const int total_pixels = width * height;
    uint8_t* mask = new uint8_t[total_pixels];
    uint8_t* visited = new uint8_t[total_pixels];
    
    // 生成二值掩码
    for (int i = 0; i < total_pixels; i++) {
        mask[i] = is_red_pixel(image[i]) ? 255 : 0;
    }
    
    memset(visited, 0, total_pixels);
    boxes.clear();
    
    const int dx[4] = {0, 0, -1, 1};
    const int dy[4] = {-1, 1, 0, 0};
    
    // BFS 查找连通域
    for (int y = 0; y < height; y=y+4) {
        for (int x = 0; x < width; x=x+4) {
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
            
            // 过滤并记录
            if (pixel_count >= min_area && pixel_count <= max_area && min_y < 115  && min_x > 5 && max_x <155) {
                RedBox box;
                box.x = min_x;
                box.y = min_y;
                box.w = max_x - min_x + 1;
                box.h = max_y - min_y + 1;
                box.area = pixel_count;
                
                int bbox_area = box.w * box.h;
                box.density = bbox_area > 0 ? (float)pixel_count / bbox_area : 0;
                
                if (box.density > 0.2f) {
                    boxes.push_back(box);
                }
            }
        }
    }
    
    delete[] mask;
    delete[] visited;
}

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

void draw_cross_rgb565(uint16_t* image, int width, int height,
                      int cx, int cy, uint16_t color, int size) {
    for (int i = -size; i <= size; i++) {
        if (cy + i >= 0 && cy + i < height)
            image[(cy + i) * width + cx] = color;
        if (cx + i >= 0 && cx + i < width)
            image[cy * width + (cx + i)] = color;
    }
}


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
    
    return dst_image;
}



void save_rgb565_as_jpg(const char* filename, uint16_t* rgb565, int w, int h) {
    // RGB565 -> BGR (OpenCV 格式)
    cv::Mat bgr(h, w, CV_8UC3);
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint16_t p = rgb565[y * w + x];
            uint8_t r = ((p >> 11) & 0x1F) << 3;
            uint8_t g = ((p >> 5) & 0x3F) << 2;
            uint8_t b = (p & 0x1F) << 3;
            
            bgr.at<cv::Vec3b>(y, x) = cv::Vec3b(b, g, r);  // BGR 顺序
        }
    }
    
    // 直接保存为 JPG
    std::vector<int> params;
    params.push_back(cv::IMWRITE_JPEG_QUALITY);
    params.push_back(95);  // 质量 0-100
    
    cv::imwrite(filename, bgr, params);
}




void process_image(uint16_t* rgb565_image) {
    
    detect_red_regions(rgb565_image, IMG_WIDTH, IMG_HEIGHT, boxes, 20, 1100);

    // printf("Detected %zu boxes\n", boxes.size());
    
    // static int frame_count = 0;
    // frame_count++;
    
    // // 每30帧自动保存一张（约0.5秒@60fps）
    // if (!boxes.empty() ) {   // && (frame_count % 10 == 0)
    //     const auto& box = boxes[0];
    //     // printf("Box: Area=%d at (%d,%d) size %dx%d\n",
    //     //        box.area, box.x, box.y, box.w, box.h);
        
    //     // 画框
    //     draw_rect_rgb565(rgb565_image, IMG_WIDTH, IMG_HEIGHT,
    //                     box.x - 2, box.y +2, box.w + 4, box.h+15, 
    //                     RGB565_WHITE, 2);
        
    //     // 裁剪并保存
    //     uint16_t* cropped = crop_image(rgb565_image, IMG_WIDTH, IMG_HEIGHT,
    //                                   box.x - 2, box.y+2, box.w + 4, box.h+15);
    //     if (cropped) {
    //         char path[128];
    //         // 保存到 /tmp（内存中），之后用scp拷出
    //         snprintf(path, sizeof(path), "/tmp/redbox_%d_%d.jpg", 
    //                 frame_count, box.area);
    //         save_rgb565_as_jpg(path, cropped, box.w + 4, box.h + 15);
            
    //         free(cropped);
    //     }
    // }
    

}

// #include "red_detecte.h"
// #include <queue>
// #include <cstring>
// #include <algorithm>
// #include <cmath>
// #include <opencv2/opencv.hpp>

// #define IMG_WIDTH  160
// #define IMG_HEIGHT 120
// #define RGB565_YELLOW  0xFFE0
// #define RGB565_WHITE   0xFFFF

// // 红色像素判断
// static bool is_red(uint16_t p) {
//     uint8_t r = (p >> 11) & 0x1F;
//     uint8_t g = (p >> 5) & 0x3F;
//     uint8_t b = p & 0x1F;
//     return (r >= 20) && (r > g + 8) && (r > b + 8);
// }

// // 计算OBB
// static RotatedRedBox compute_obb(const std::vector<std::pair<int,int>>& pts) {
//     int n = pts.size();
//     float cx = 0, cy = 0;
//     for (auto& p : pts) { cx += p.first; cy += p.second; }
//     cx /= n; cy /= n;
    
//     float xx = 0, xy = 0, yy = 0;
//     for (auto& p : pts) {
//         float dx = p.first - cx, dy = p.second - cy;
//         xx += dx*dx; xy += dx*dy; yy += dy*dy;
//     }
//     xx/=n; xy/=n; yy/=n;
    
//     float tr = xx + yy, dt = xx*yy - xy*xy;
//     float la = (tr + sqrtf(tr*tr - 4*dt)) * 0.5f;
//     float ang = atan2f(la - xx, xy);
    
//     float c = cosf(ang), s = sinf(ang), min_u=1e9, max_u=-1e9, min_v=1e9, max_v=-1e9;
//     for (auto& p : pts) {
//         float dx = p.first - cx, dy = p.second - cy;
//         float u = dx*c + dy*s, v = -dx*s + dy*c;
//         min_u = std::min(min_u, u); max_u = std::max(max_u, u);
//         min_v = std::min(min_v, v); max_v = std::max(max_v, v);
//     }
    
//     RotatedRedBox b;
//     b.cx = cx; b.cy = cy;
//     b.w = (max_u - min_u) * 0.5f;
//     b.h = (max_v - min_v) * 0.5f;
//     b.angle = ang;
//     if (b.w < b.h) { std::swap(b.w, b.h); b.angle += 3.14159f/2; }
//     while (b.angle > 3.14159f/2) b.angle -= 3.14159f;
//     return b;
// }

// void detect_red_regions_rotated(uint16_t* img, int w, int h,
//                                std::vector<RotatedRedBox>& boxes,
//                                int min_a, int max_a) {
//     int n = w*h;
//     uint8_t *mask = new uint8_t[n], *vis = new uint8_t[n]();
//     for (int i=0;i<n;i++) mask[i] = is_red(img[i]) ? 255 : 0;
    
//     boxes.clear();
//     const int dx[4]={0,0,-1,1}, dy[4]={-1,1,0,0};
    
//     for (int y=0;y<h;y+=2) {
//         for (int x=0;x<w;x+=2) {
//             int id = y*w+x;
//             if (!mask[id] || vis[id]) continue;
            
//             std::vector<std::pair<int,int>> pts;
//             std::queue<std::pair<int,int>> q;
//             q.push({x,y}); vis[id]=1;
            
//             while (!q.empty()) {
//                 auto [cx,cy] = q.front(); q.pop();
//                 pts.push_back({cx,cy});
//                 for (int i=0;i<4;i++) {
//                     int nx=cx+dx[i], ny=cy+dy[i];
//                     if (nx<0||nx>=w||ny<0||ny>=h) continue;
//                     int nid=ny*w+nx;
//                     if (mask[nid] && !vis[nid]) { vis[nid]=1; q.push({nx,ny}); }
//                 }
//             }
            
//             if ((int)pts.size()<min_a || (int)pts.size()>max_a) continue;
//             auto b = compute_obb(pts);
//             b.area = pts.size();
//             float dens = (4*b.w*b.h)>0 ? pts.size()/(4*b.w*b.h) : 0;
//             if (dens > 0.3f) boxes.push_back(b);
//         }
//     }
//     delete[] mask; delete[] vis;
// }

// void draw_rotated_rect_rgb565(uint16_t* img, int w, int h,
//                              const RotatedRedBox& b, uint16_t col, int thick) {
//     float c=cosf(b.angle), s=sinf(b.angle);
//     float dx1=b.w*c, dy1=b.w*s, dx2=-b.h*s, dy2=b.h*c;
//     int p[4][2] = {
//         {(int)(b.cx+dx1+dx2), (int)(b.cy+dy1+dy2)},
//         {(int)(b.cx+dx1-dx2), (int)(b.cy+dy1-dy2)},
//         {(int)(b.cx-dx1-dx2), (int)(b.cy-dy1-dy2)},
//         {(int)(b.cx-dx1+dx2), (int)(b.cy-dy1+dy2)}
//     };
    
//     auto line = [&](int x0,int y0,int x1,int y1) {
//         int dx=abs(x1-x0), sx=x0<x1?1:-1;
//         int dy=-abs(y1-y0), sy=y0<y1?1:-1;
//         int err=dx+dy;
//         while (1) {
//             if (x0>=0&&x0<w&&y0>=0&&y0<h) img[y0*w+x0]=col;
//             if (x0==x1&&y0==y1) break;
//             int e2=2*err;
//             if (e2>=dy) { err+=dy; x0+=sx; }
//             if (e2<=dx) { err+=dx; y0+=sy; }
//         }
//     };
    
//     for (int t=0;t<thick;t++)
//         for (int i=0;i<4;i++)
//             line(p[i][0]+(i<2?t:-t), p[i][1]+(i==0||i==3?t:-t),
//                  p[(i+1)%4][0]+((i+1)%4<2?t:-t), p[(i+1)%4][1]+((i+1)%4==0||(i+1)%4==3?t:-t));
// }

// uint16_t* crop_rotated_image(uint16_t* src, int sw, int sh, const RotatedRedBox& b, int margin) {
//     int dw = (int)(b.w*2+0.5f)+ 2*margin, dh = (int)(b.h*2+0.5f)+ 2*margin;
//     if (dw<=0||dh<=0) return nullptr;
//     uint16_t* dst = (uint16_t*)malloc(dw*dh*sizeof(uint16_t));
//     if (!dst) return nullptr;
    
//     float c=cosf(b.angle), s=sinf(b.angle);
//     for (int y=0;y<dh;y++) {
//         for (int x=0;x<dw;x++) {
//             float dx=x-dw*0.5f, dy=y-dh*0.5f;
//             int sx=(int)(b.cx+(dx*c-dy*s)+0.5f);
//             int sy=(int)(b.cy+(dx*s+dy*c)+0.5f);
//             dst[y*dw+x] = (sx>=0&&sx<sw&&sy>=0&&sy<sh) ? src[sy*sw+sx] : 0;
//         }
//     }
//     return dst;
// }

// // 你的原有保存函数
// void save_rgb565_as_jpg(const char* fn, uint16_t* rgb565, int w, int h) {
//     cv::Mat bgr(h, w, CV_8UC3);
//     for (int y=0;y<h;y++) {
//         for (int x=0;x<w;x++) {
//             uint16_t p = rgb565[y*w+x];
//             uint8_t r = ((p>>11)&0x1F)<<3, g = ((p>>5)&0x3F)<<2, b = (p&0x1F)<<3;
//             bgr.at<cv::Vec3b>(y,x) = cv::Vec3b(b,g,r);
//         }
//     }
//     std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 95};
//     cv::imwrite(fn, bgr, params);
// }

// void process_image(uint16_t* rgb565_image) {
//     static std::vector<RotatedRedBox> boxes;
//     static int frame_count = 0;
    
//     detect_red_regions_rotated(rgb565_image, IMG_WIDTH, IMG_HEIGHT, boxes, 40, 1100);
    
//     if (!boxes.empty()) {
//         const auto& b = boxes[0];
//         frame_count++;
        
//         // 画旋转框
//         draw_rotated_rect_rgb565(rgb565_image, IMG_WIDTH, IMG_HEIGHT, b, RGB565_WHITE, 2);
        
//         // 裁剪并保存
//         uint16_t* cropped = crop_rotated_image(rgb565_image, IMG_WIDTH, IMG_HEIGHT, b,20);
//         if (cropped) {
//             char path[128];
//             int dw=(int)(b.w*2+0.5f) + 40, dh=(int)(b.h*2+0.5f) + 40;
//             snprintf(path, sizeof(path), "/tmp/redbox_%d_%d.jpg", frame_count, b.area);
//             save_rgb565_as_jpg(path, cropped, dw, dh);
//             free(cropped);
//         }
//     }
// }
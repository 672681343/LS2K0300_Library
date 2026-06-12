#include "red_detecte.h"
#include <queue>
#include <cstring>
#include <algorithm>
#include <cstdio>

#define IMG_WIDTH  160
#define IMG_HEIGHT 120

#define BGR888_RED     0x0000FF
#define BGR888_GREEN   0x00FF00
#define BGR888_BLUE    0xFF0000
#define BGR888_WHITE   0xFFFFFF
#define BGR888_BLACK   0x000000
#define BGR888_YELLOW  0x00FFFF


std::vector<RedBox> boxes;

bool is_red_pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t min_red, uint8_t min_diff) {
    return (r >= min_red) && (r > g + min_diff) && (r > b + min_diff);
}

void detect_red_regions(uint8_t* image, int width, int height,
                       std::vector<RedBox>& boxes,
                       int min_area, int max_area) {
    
    const int total_pixels = width * height;
    uint8_t* mask = new uint8_t[total_pixels];
    uint8_t* visited = new uint8_t[total_pixels];
    
    for (int i = 0; i < total_pixels; i++) {
        uint8_t b = image[i * 3];
        uint8_t g = image[i * 3 + 1];
        uint8_t r = image[i * 3 + 2];
        mask[i] = is_red_pixel(r, g, b) ? 255 : 0;
    }
    
    memset(visited, 0, total_pixels);
    boxes.clear();
    
    const int dx[4] = {0, 0, -1, 1};
    const int dy[4] = {-1, 1, 0, 0};
    
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
            
            if (pixel_count >= min_area && pixel_count <= max_area 
                && min_y < 115 && min_x > 5 && max_x < 155) {
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
uint8_t* crop_image(uint8_t* src_image, int src_width, int src_height,
                    int x, int y, int w, int h) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= src_width || y >= src_height) return nullptr;
    if (x + w > src_width) w = src_width - x;
    if (y + h > src_height) h = src_height - y;
    if (w <= 0 || h <= 0) return nullptr;
    
    uint8_t* dst_image = (uint8_t*)malloc(w * h * 3);
    if (!dst_image) return nullptr;
    
    for (int row = 0; row < h; row++) {
        int src_offset = ((y + row) * src_width + x) * 3;
        int dst_offset = row * w * 3;
        memcpy(&dst_image[dst_offset], &src_image[src_offset], w * 3);
    }
    
    return dst_image;
}

void save_bgr888_as_bmp(const char* filename, uint8_t* bgr, int w, int h) {
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
    hdr.height = h;          // 正数 = 倒序存储（BMP标准）
    hdr.image_size = row_padded * h;
    
    FILE* fp = fopen(filename, "wb");
    if (!fp) return;
    
    fwrite(&hdr, 1, sizeof(hdr), fp);
    
    uint8_t* row_buf = (uint8_t*)calloc(1, row_padded);
    for (int row = h - 1; row >= 0; row--) {
        memcpy(row_buf, &bgr[row * w * 3], w * 3);
        fwrite(row_buf, 1, row_padded, fp);
    }
    
    free(row_buf);
    fclose(fp);
}

// ============================================================
// 主处理：坐标全部按标准图像坐标（左上角原点）
// ============================================================
void process_image(uint8_t* bgr888_image) {
    detect_red_regions(bgr888_image, IMG_WIDTH, IMG_HEIGHT, boxes, 40, 1100);
    printf("Detected %zu boxes\n", boxes.size());
    
    static int frame_count = 0;
    frame_count++;
    
    if (!boxes.empty()) {
        const auto& box = boxes[0];
        
        // 标准坐标：box.x, box.y 就是左上角
        // 向外扩 2px，注意边界保护
        int draw_x = box.x - 2;
        int draw_y = box.y - 2;
        int draw_w = box.w + 4;
        int draw_h = box.h + 15;
        
        draw_rect_bgr888(bgr888_image, IMG_WIDTH, IMG_HEIGHT,
                        draw_x, draw_y, draw_w, draw_h, 
                        BGR888_WHITE, 2);
        
        // 裁剪也按标准坐标
        uint8_t* cropped = crop_image(bgr888_image, IMG_WIDTH, IMG_HEIGHT,
                                      draw_x, draw_y, draw_w, draw_h);
        if (cropped) {
            char path[128];
            snprintf(path, sizeof(path), "/tmp/redbox_%d_%d.bmp", frame_count, box.area);
            save_bgr888_as_bmp(path, cropped, draw_w, draw_h);
            free(cropped);
        }
    }
}
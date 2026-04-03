#include "ImageProc.h"

#include <cstdint>
#include <array>
#include <algorithm>

namespace ImageProc {
    
    std::array<uint8_t, WIDTH> white_line;      // 白列数组
    std::array<uint16_t, HEIGHT> l_black;       // 左边线数组
    std::array<uint16_t, HEIGHT> fit_l_black;   // 左边线拟合数组
    std::array<uint16_t, HEIGHT> r_black;       // 右边线数组
    std::array<uint16_t, HEIGHT> fit_r_black;   // 右边线拟合数组

    std::array<bool, HEIGHT> left_flags;        // 左边线丢线标志位
    std::array<bool, HEIGHT> right_flags;       // 右边线丢线标志位

    State state;                                // 图像基础参数
    BoundaryFeature Left_BF {};                 // 左边线特征
    BoundaryFeature Right_BF {};                // 右边线特征
    BoundaryFeature Mid_BF {};                  // 中线特征

    _SecLossDetect sec_loss_left {};            // 二次丢线左区域
    _SecLossDetect sec_loss_right {};           // 二次丢线右区域
    _SecLossDetect sec_loss_both {};            // 二次丢线双区域
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 图像基础测算
    // 参数说明: 处理图像和显示图像
    // 返回参数: 图像数据
    // 使用示例: ImageProc::image_measurement(Camera::bin_pixels, Camera::lcd_pixels);
    // 备注信息:
    //-------------------------------------------------------------------------------------------------------------------
    void image_measurement(uint8_t* bin, uint8_t* lcd) {
        //找起始点
        find_starting_point(bin, lcd, state.range_l, state.range_r, state.mode);
        //找边界
        finding_boundary(bin, lcd, state.cutoff);
        //丢线统计
        lost_line_statistics(state.cutoff);
        //二次丢线统计
        sec_lost_line_statistics(bin, 30, WIDTH-30, state.cutoff);
        //计算边界特征
        uint8_t start = HEIGHT - 10;
        uint8_t end = HEIGHT - state.cutoff + 5;
        Left_BF.Slope = two_segment_fitting_int(start,end,0,0,l_black,fit_l_black);
        Right_BF.Slope = two_segment_fitting_int(start,end,0,0,r_black,fit_r_black);
        Left_BF.Var  = fangcha(start,end,l_black,fit_l_black);
        Right_BF.Var = fangcha(start,end,r_black,fit_r_black);
    }

    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 寻找起始点
    // 参数说明: 原图像，屏幕图像，左限幅，右限幅，起始点模式
    // 返回参数: state.start_point
    // 使用示例: find_starting_point(bin, lcd, state.range_l, state.range_r, 0);
    // 备注信息:
    //-------------------------------------------------------------------------------------------------------------------
    void find_starting_point(uint8_t* bin, uint8_t* lcd, uint8_t start_L, uint8_t start_R, uint8_t start_mode) {
        const uint8_t center = (start_L + start_R) / 2; // 中心位置
        uint8_t L_longest = center, R_longest = center; // 最长白列
        bool L_found = false, R_found = false;          // 是否找到
        uint8_t* last_row = bin + (HEIGHT-1) * WIDTH;   // 最后一行指针（相当于最底下的一行）
        white_line.fill(0);                             // 初始化为0

        // 左侧扫描优化
        for(uint8_t j = center; j > start_L; j--) {
            // 提前终止条件优化
            if(L_found && 
            last_row[j] == 0 && 
            last_row[j+1] == 0 && 
            last_row[j+2] == 0) break;
            
            // 纵向扫描优化：使用行指针连续访问
            uint8_t* current_row = last_row + j; // 列初始位置
            for(uint8_t i = 0; i < HEIGHT; ++i) {
                if ((*(current_row + WIDTH) == 255 &&  *current_row == 0) || (*(current_row + WIDTH) == 0 && *current_row == 0)) break;
                white_line[j]++; // 统计白列长度
                current_row -= WIDTH; // 向上移动一行（从底部到顶部）
            }
            //printf("j=%d,count=%d\n",j,white_line[j]);
            // 合并更新逻辑
            if(white_line[j] > white_line[L_longest]) {
                L_longest = j;
                L_found = true;
            }
        }
        
        //查找右侧最长白列（镜像对称逻辑）
        for(uint8_t j = center + 1; j < start_R; j++) {  // end_R 需要定义，可能是右侧终止位置
            // 提前终止条件优化
            if(R_found &&                      
            last_row[j] == 0 &&                
            last_row[j-1] == 0 && 
            last_row[j-2] == 0) break;         
            
            // 纵向扫描优化：使用行指针连续访问
            uint8_t* current_row = last_row + j; // 列初始位置
            for(uint8_t i = 0; i < HEIGHT; ++i) { // HEIGHT 应已定义
                if (*current_row == 0 &&  *(current_row - WIDTH) == 0) break; 
                white_line[j]++;               
                current_row -= WIDTH;          // 向上移动一行
            }
            //printf("j=%d,count=%d\n",j,white_line[j]);
            // 合并更新逻辑
            if(white_line[j] > white_line[R_longest]) { 
                R_longest = j;
                R_found = true;
            }
        }

        // 1. 模式选择逻辑优化（减少分支）
        const uint8_t white_L = white_line[L_longest]; // 预取值减少内存访问
        const uint8_t white_R = white_line[R_longest];
        const bool use_left = (start_mode == 1) || (start_mode == 0 && white_L > white_R);

        // 2. 统一赋值操作
        const uint8_t selected_col = use_left ? L_longest : R_longest;
        state.cutoff    = use_left ? white_L : white_R;
        state.start_point = selected_col;

        // 3. 边界保护优化
        constexpr uint8_t max_col = WIDTH - BORDER_MARGIN - 1;
        state.start_point = (selected_col < BORDER_MARGIN) ? BORDER_MARGIN : 
                        (selected_col > max_col) ? max_col : selected_col;


        state.last_start_point = state.start_point;

        // // 5. 可视化标记优化（指针操作代替二维索引）
        // const uint8_t col_pos = state.start_point;
        // uint8_t* ptr = lcd + (HEIGHT - 1) * WIDTH + col_pos; // 初始位置：最底部

        // for(uint8_t i = 0; i < state.cutoff; ++i) {
        //     *ptr = 0;                // 写当前像素
        //     ptr -= WIDTH;            // 指针上移一行
        // }
    }

    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 找边线
    // 参数说明: 原图像，屏幕图像，截止行
    // 返回参数: 左右变线
    // 使用示例: finding_boundary(bin, lcd, state.cutoff);
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    void finding_boundary(uint8_t* bin, uint8_t* lcd, uint8_t cut_off) {
        constexpr uint8_t FEATURE_MARGIN = 3;  // 特征检测安全边界
        uint8_t start;
        if(state.set_start_point == 0 ){
            start = state.start_point;
        }else{
            start = state.set_start_point;
        }
        if (start < FEATURE_MARGIN) start = FEATURE_MARGIN;
        
        l_black.fill(0);          // 初始化为最小值
        r_black.fill(WIDTH-1);      // 初始化为最大值
        left_flags.fill(false);   // 初始化为false
        right_flags.fill(false);  // 初始化为false

        for(uint8_t idx = 0; idx < cut_off; ++idx) {
            uint8_t i = HEIGHT - 1 - idx; // 当前行号从底部开始
            uint8_t* row = bin + i * WIDTH;
            
            // 左侧扫描（start-2向左搜索）
            for(uint8_t j = start + FEATURE_MARGIN; j > 0; --j) {
                uint8_t* pix = row + j;
                // 检测黑-黑-白特征
                if(pix[0] == 0 && pix[1] == 0 && pix[2] == 255) {
                    l_black[i] = j + 1;  // 记录白段结束位置
                    left_flags[i] = true;
                    
                    // LCD可视化坐标计算
                    const uint8_t lcd_col = j + 2 + 1; // 右侧空一格
                    lcd[i * WIDTH + lcd_col] = 0; // 行号保持原样
                    break;
                }
                
            }
    
            // 右侧扫描（start-2向左搜索）
            for(uint8_t j = start - FEATURE_MARGIN; j < WIDTH-1; ++j) {
                uint8_t* pix = row + j;
                // 检测黑-黑-白特征（反向）
                if(pix[0] == 0 && pix[-1] == 0 && pix[-2] == 255) {
                    r_black[i] = j - 1;  // 记录白段结束位置
                    right_flags[i] = true;
                    
                    // LCD可视化坐标计算
                    const uint8_t lcd_col = j - 2 - 1; // 左侧空一格
                    lcd[i * WIDTH + lcd_col] = 0; // 行号保持原样
                    break;
                }
            }
            
        }
    }
    
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 丢线统计
    // 参数说明: 截止行
    // 返回参数: 左右丢线状态
    // 使用示例: lost_line_statistics(state.cutoff);
    // 备注信息:
    //-------------------------------------------------------------------------------------------------------------------
    void lost_line_statistics(uint8_t cut_off) {
        // 初始化追踪器）
        struct Tracker {
            bool tracking = false;
            uint8_t start = 0;
            uint8_t end = 0;
        } left_tracker, right_tracker;
    
        // 区域存储结构
        struct Region {
            uint8_t start = 0;
            uint8_t end = 0;
            uint8_t count = 0;
            uint8_t mid = 0;
        };
        Region left_lose[MAX_REGIONS], right_lose[MAX_REGIONS];

        Right_BF.LostParam.start = HEIGHT - 1;
        Right_BF.LostParam.end = HEIGHT - 1;
        Right_BF.LostParam.count = 0;

        Left_BF.LostParam.start = HEIGHT - 1;
        Left_BF.LostParam.end = HEIGHT - 1;
        Left_BF.LostParam.count = 0;
        // 处理逻辑
        auto process_side = [cut_off](auto& tracker, auto* regions, const auto& flags) -> uint8_t {
            uint8_t region_count = 0;
            bool prev_lost = false;
        
            // 从底部向上扫描 cut_off 行（注意：cut_off 表示行数，而非位置）
            for (uint8_t idx = 0; idx < cut_off; ++idx) {
                const uint8_t i = HEIGHT - 1 - idx; // 当前行号从底部开始
                const bool current_lost = (flags[i] == 0);
        
                if (!tracker.tracking && current_lost) {
                    if (prev_lost) {
                        tracker.start = i + 1;      // 记录区域起点
                        tracker.tracking = true;
                    }
                    prev_lost = true;
                } else {
                    if (tracker.tracking && !current_lost) {
                        tracker.end = i + 1;        // 记录区域终点
                        regions[region_count] = {
                            .start = tracker.start,
                            .end = tracker.end,
                            .count = static_cast<uint8_t>(tracker.start - tracker.end),
                            .mid = static_cast<uint8_t>((tracker.start + tracker.end) >> 1)
                        };
                        if (++region_count >= MAX_REGIONS) break;
                        tracker.tracking = false;
                    }
                    prev_lost = false;
                }
            }
        
            // 处理未闭合的区域（终点修正为扫描范围的最顶部）
            if (tracker.tracking && region_count < MAX_REGIONS) {
                const uint8_t scan_min_row = HEIGHT - cut_off; // 扫描的最顶部行号
                regions[region_count] = {
                    .start = tracker.start,
                    .end = scan_min_row,                      // 终点设为扫描的最顶部行号
                    .count = static_cast<uint8_t>(tracker.start - scan_min_row),
                    .mid = static_cast<uint8_t>((tracker.start + scan_min_row) >> 1)
                };
                region_count++;
            }
        
            return region_count;
        };    
        // 执行处理
        uint8_t left_regions = process_side(left_tracker, left_lose, left_flags);
        uint8_t right_regions = process_side(right_tracker, right_lose, right_flags);

        //合理性判断
        const uint8_t image_center_y = HEIGHT / 2; // 假设图像中心行号
        const uint8_t L_lose_num = left_regions;    // 左丢线区域数量
        const uint8_t R_lose_num = right_regions;   // 右丢线区域数量
        uint8_t mix_lose_num = std::max(L_lose_num, R_lose_num);
    
        int16_t min_left_dist = INT16_MAX, min_right_dist = INT16_MAX;
        uint8_t best_L_idx = 0, best_R_idx = 0;
    
        // 遍历所有区域，寻找距离中心最近的补偿后区域
        for (uint8_t i = 0; i < mix_lose_num; ++i) {
            // 左区域计算（防越界）
            if (i < L_lose_num) {
                const auto& L = left_lose[i];
                const int16_t lose_LU = L.start;  // 上边界（靠近底部）
                const int16_t lose_LD = L.end;    // 下边界（靠近顶部）
                const int16_t L_compensation = lose_LU - lose_LD; // 补偿值
                
                // 计算补偿后中心距离
                const int16_t center_dist = abs((lose_LU + lose_LD)/2 - image_center_y)
                                          - static_cast<int8_t>(L_compensation * 0.5);
                
                if (center_dist < min_left_dist) {
                    min_left_dist = center_dist;
                    best_L_idx = i;
                }
            }
    
            // 右区域计算（防越界）
            if (i < R_lose_num) {
                const auto& R = right_lose[i];
                const int16_t lose_RU = R.start;  // 上边界
                const int16_t lose_RD = R.end;    // 下边界
                const int16_t R_compensation = lose_RU - lose_RD;
                
                const int16_t center_dist = abs((lose_RU + lose_RD)/2 - image_center_y)
                                          - static_cast<int8_t>(R_compensation * 0.5);
                
                if (center_dist < min_right_dist) {
                    min_right_dist = center_dist;
                    best_R_idx = i;
                }
            }
        }

        // 存储最优左丢线区域
        
        if(L_lose_num == 0) best_L_idx = 0;
        {
            const auto& L = left_lose[best_L_idx];
            Left_BF.LostParam.start = L.start;
            Left_BF.LostParam.end = L.end;
            Left_BF.LostParam.count = L.count;
            Left_BF.LostParam.mid = L.mid;
            Left_BF.LostParam.reparation = static_cast<int8_t>((L.start - L.end) * 0.5);
            Left_BF.LostParam.sum = (L.start > L.end) ? (L.start - L.end) : 0;
        }

        // 存储最优右丢线区域
        if(R_lose_num == 0) best_R_idx = 0;
        {
            const auto& R = right_lose[best_R_idx];
            Right_BF.LostParam.start = R.start;
            Right_BF.LostParam.end = R.end;
            Right_BF.LostParam.count = R.count;
            Right_BF.LostParam.mid = R.mid;
            Right_BF.LostParam.reparation = static_cast<int8_t>((R.start - R.end) * 0.5);
            Right_BF.LostParam.sum = (R.start > R.end) ? (R.start - R.end) : 0;
        }

        // 计算双边丢线信息
        {
            const auto& L = Left_BF.LostParam;
            const auto& R = Right_BF.LostParam;
            
            // 取上下边界交集
            const uint8_t lose_both_U = std::min(L.start, R.start);
            const uint8_t lose_both_D = std::max(L.end, R.end);
            const uint8_t sum_both = (lose_both_U > lose_both_D) ? 
                                    (lose_both_U - lose_both_D) : 0;
            
            Mid_BF.LostParam.start = lose_both_U;
            Mid_BF.LostParam.end = lose_both_D;
            Mid_BF.LostParam.count = sum_both;
            Mid_BF.LostParam.mid = static_cast<uint8_t>((lose_both_U + lose_both_D) >> 1);
            Mid_BF.LostParam.reparation = static_cast<int8_t>(L.reparation + R.reparation);
            Mid_BF.LostParam.sum = sum_both;
        }        
    }

    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 二次丢线检测
    // 参数说明: 
    //   - bin: 二值化图像数据指针(0=黑线, 255=白色背景)
    //   - x_left: 左侧检测列
    //   - x_right: 右侧检测列
    //   - cut_off: 检测行数（从底部开始）
    // 返回参数: 无
    // 使用示例: secondary_loss_detection(bin_image, 10, 20, 30);
    // 备注信息: 
    //   1. 只检测从黑变白的边界开始的第一个丢线区域
    //   2. 图像数据格式: bin[y*WIDTH + x], 0=黑线, 1=白色背景
    //-------------------------------------------------------------------------------------------------------------------
    void sec_lost_line_statistics(const uint8_t* bin, uint8_t x_left, uint8_t x_right, uint8_t cut_off) {
        // 重置二次丢线区域
        sec_loss_left = {0};
        sec_loss_right = {0};
        
        // 左侧检测
        bool l_found_white = false;
        
        for (uint8_t idx = 5; idx < cut_off - 2; ++idx) {
            uint8_t row = HEIGHT - 1 - idx;  // 从底部开始计算当前行
            uint16_t pixel_offset = row * WIDTH + x_left;
            
            // 阶段1：检测跳变点开始记录（必须满足前黑后白）
            if (!l_found_white && l_black[row - 2] < x_left - 1 &&
                bin[pixel_offset]==0 && 
                bin[pixel_offset - WIDTH]==255 && 
                bin[pixel_offset - 2 * WIDTH]==255) {
                sec_loss_left.x = x_left;
                sec_loss_left.LostParam.start = row + 1;  // 跳变发生在row+1到row之间
                l_found_white = true;
            }
            
            // 阶段2：找到黑色点结束记录
            if (l_found_white) {
                if (bin[pixel_offset]==0 && bin[pixel_offset - WIDTH]==0 ) {
                    sec_loss_left.LostParam.end = row;  // 更新白区下边界
                    break;
                }
            }

            if(idx == cut_off - 3 && l_found_white) {
                sec_loss_left.LostParam.end = row;  // 如果没有找到黑色点，默认结束行
            }
        }
        
        // 后处理（确保数据有效性）
        if (l_found_white) {
            //计算衍生参数
            sec_loss_left.LostParam.count = sec_loss_left.LostParam.start - sec_loss_left.LostParam.end;
            sec_loss_left.LostParam.mid = (sec_loss_left.LostParam.start + sec_loss_left.LostParam.end) / 2;
            sec_loss_left.LostParam.sum = sec_loss_left.LostParam.count;
        } else {
            sec_loss_left = {0};  // 未检测到则清零
        }
        
        // 右侧检测（镜像对称逻辑）
        bool r_found_white = false;

        for (uint8_t idx = 0; idx < cut_off - 2; ++idx) {
            uint8_t row = HEIGHT - 1 - idx;
            uint16_t pixel_offset = row * WIDTH + x_right;
            
            // 阶段1：检测跳变点开始记录
            if (!r_found_white && r_black[row - 2] > x_right + 1 &&
                bin[pixel_offset]==0 && 
                bin[pixel_offset - WIDTH]==255 && 
                bin[pixel_offset - 2*WIDTH]==255) {
                sec_loss_right.x = x_right;
                sec_loss_right.LostParam.start = row + 1;
                r_found_white = true;
                //printf("found white at row %d\n", row);
            }
            
            // 阶段2：找到黑色点结束记录
            if (r_found_white) {
                if (bin[pixel_offset]==0 && bin[pixel_offset - WIDTH]==0) {
                    sec_loss_right.LostParam.end = row;
                    //printf("found black at row %d\n", row);
                    break;
                }
            }

            if(idx == cut_off - 3 && r_found_white) {
                sec_loss_right.LostParam.end = row;  // 如果没有找到黑色点，默认结束行
            }
        }
        
        // 后处理
        if (r_found_white) {
            sec_loss_right.LostParam.count = sec_loss_right.LostParam.start - sec_loss_right.LostParam.end;
            sec_loss_right.LostParam.mid = (sec_loss_right.LostParam.start + sec_loss_right.LostParam.end) >> 1;
            sec_loss_right.LostParam.sum = sec_loss_right.LostParam.count;
        } else {
            sec_loss_right = {0};
        }

        //共同丢线区域计算
        if (sec_loss_left.LostParam.count > 0 && sec_loss_right.LostParam.count > 0) {
            const auto& L = sec_loss_left.LostParam;  // 左丢线参数
            const auto& R = sec_loss_right.LostParam; // 右丢线参数
    
            // 计算上下边界（兼容行号倒序）
            const uint8_t lose_both_U = std::min(L.start, R.start); // 共同区域上边界
            const uint8_t lose_both_D = std::max(L.end, R.end);     // 共同区域下边界
            const uint8_t sum_both = (lose_both_U > lose_both_D) ? 
                                    (lose_both_U - lose_both_D) : 0; // 有效行数校验
    
            // 参数赋值
            sec_loss_both.LostParam.start = lose_both_U;
            sec_loss_both.LostParam.end = lose_both_D;
            sec_loss_both.LostParam.count = sum_both;
            sec_loss_both.LostParam.mid = 
                static_cast<uint8_t>((lose_both_U + lose_both_D) >> 1); // 等价于/2
            sec_loss_both.LostParam.sum = sum_both;
    
            // 可选：修复值累加（若需要）
            // sec_loss_both.LostParam.reparation = 
            //     static_cast<int8_t>(L.reparation + R.reparation);
        } else {
            sec_loss_both = {0};  // 任意一侧无丢线则清零
        }
    }
}
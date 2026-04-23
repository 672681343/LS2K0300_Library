#pragma once
#include "zf_common_headfile.h"
#include <array>

namespace ImageProc {
    // 常量定义

    constexpr uint8_t BORDER_MARGIN = 3;
    constexpr uint8_t MAX_REGIONS = 4;

    // 丢线区域结构体
    struct LoseRegion {
        uint8_t start = 0;    // 区域起始行（靠近底部）
        uint8_t end = 0;      // 区域结束行（靠近顶部）
        uint8_t count = 0;    // 区域长度
        uint8_t mid = 0;      // 区域中心行
        int8_t reparation = 0;// 补偿值（带符号）
        uint8_t sum = 0;      // 有效丢线总长度
    };

    // 核心状态结构体
    struct State {
        // 高频访问变量
        uint8_t start_point;       // 当前起始点

        uint8_t start_point_l;
        uint8_t start_point_r;

        uint8_t last_start_point;  // 上次起始点
        uint8_t cutoff;            // 扫描截止行

        uint8_t set_start_point;   // 设置起始点
        
        // 低频访问变量
        uint8_t range_l;           // 左边界限制
        uint8_t range_r;           // 右边界限制
        
        uint8_t mode;               // 起始点模式
        
        // 构造函数
        State() : start_point(WIDTH/2), last_start_point(WIDTH/2),
        cutoff(HEIGHT-1),set_start_point(0) ,range_l(10), range_r(WIDTH-10),mode(0) {}
    };

    // 全局丢线状态容器

    typedef struct {
        float Var;
        float Slope;
        LoseRegion LostParam;
    }BoundaryFeature;

    typedef struct {
        uint8_t x;
        LoseRegion LostParam;
    }_SecLossDetect;
    // 外部可见的全局状态声明
    extern State state;                  // 主状态机
    extern BoundaryFeature Left_BF;
    extern BoundaryFeature Right_BF;
    extern BoundaryFeature Mid_BF;
    extern _SecLossDetect sec_loss_left;  // 二次丢线区域
    extern _SecLossDetect sec_loss_right; // 二次丢线区域
    extern _SecLossDetect sec_loss_both;  // 二次丢线区域
    // 外部可见的边界数据
    extern std::array<uint8_t, WIDTH> white_line;  // 长白列记录
    
    extern std::array<uint16_t, HEIGHT> l_black;  // 左边界记录
    extern std::array<uint16_t, HEIGHT> r_black;  // 右边界记录

    extern std::array<uint16_t, HEIGHT> last_l_black;  // 前一帧左边界记录
    extern std::array<uint16_t, HEIGHT> last_r_black;  // 前一帧右边界记录

    extern std::array<uint16_t, HEIGHT> fit_l_black; // 左边界拟合
    extern std::array<uint16_t, HEIGHT> fit_r_black; // 右边界拟合
    extern std::array<bool, HEIGHT> left_flags;  // 左边界标志
    extern std::array<bool, HEIGHT> right_flags; // 右边界标志

    // 核心功能接口
    void image_measurement(uint8_t* bin, uint8_t* lcd);
    void find_starting_point(uint8_t* bin, uint8_t* lcd, 
                           uint8_t start_L, uint8_t start_R, uint8_t start_mode);
    void finding_boundary(uint8_t* bin, uint8_t* lcd, uint8_t cut_off);
    void lost_line_statistics(uint8_t cut_off);
    void sec_lost_line_statistics(const uint8_t* bin, uint8_t x_left, uint8_t x_right, uint8_t cut_off);

    void find_Left_Line(uint8_t* bin,uint8_t* lcd,uint8_t cut_off);
    void find_Right_Line(uint8_t* bin,uint8_t* lcd,uint8_t cut_off);

    void find_lane_boundary(uint8_t* bin, uint8_t* lcd, uint8_t cut_off);

    void last_line_update(void);

    void eight_sfinding_boundary(uint8_t* bin, uint8_t* lcd);

}
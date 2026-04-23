#pragma once

/* Includes ------------------------------------------------------------------*/
#include "zf_common_headfile.h"
#include <array>
#include <cstdint>



namespace ImageProc {
    struct State;  // 关键：前置声明结构体
}

namespace Err {
    /* Exported types ------------------------------------------------------------*/
    // 误差相关变量

    typedef struct {
        int16_t current;    // 当前Err
        int16_t previous;   // 上次Err
        int16_t last;        // 真上次Err
        int8_t mode;        // 中线模式
        uint8_t limit;      // 获取行限制
        uint8_t row;        // 当前获取行
        uint8_t tem_row;    // 实际获取行
        float rep;          // 获取行补偿
    } Error_parm;
    
    /* Exported constants --------------------------------------------------------*/
    extern Error_parm Err;
    // 中线数据
    alignas(8) extern std::array<uint16_t, HEIGHT> center_line; //  使用alignas(8)确保变量按8字节对齐，以提高内存访问效率 声明一个外部数组center_line，类型为uint16_t数组，大小为HEIGHT 数组名center_line暗示这可能用于存储中心线数据，如图像处理或信号处理中的中心线
    alignas(8) extern std::array<uint16_t, HEIGHT> fit_center_line;//拟合中线数组
    //alignas(8) extern std::array<uint16_t, HEIGHT> fit_Bessel_line;//拟合中线数组
    extern const std::array<uint8_t, HEIGHT> ROAD_WIDTH;
    /* Exported functions ------------------------------------------------------- */
    // 核心功能接口
    void error_get(const ImageProc::State& state,
                 const std::array<uint16_t, HEIGHT>& l_black,
                 const std::array<uint16_t, HEIGHT>& r_black,
                 const std::array<bool, HEIGHT>& left_flags,
                 const std::array<bool, HEIGHT>& right_flags);

    void midline_calculate(const ImageProc::State& state,
                          const std::array<uint16_t, HEIGHT>& l_black,
                          const std::array<uint16_t, HEIGHT>& r_black,
                          const std::array<bool, HEIGHT>& left_flags,
                          const std::array<bool, HEIGHT>& right_flags);
    //int16_t error_calculate(uint8_t Get_Row, const ImageProc::State& state);
    
    int16_t error_calculate(uint8_t row, const ImageProc::State& state, uint8_t limit,
                            const std::array<uint16_t, HEIGHT>& center_line);

    void last_error_update ();


}
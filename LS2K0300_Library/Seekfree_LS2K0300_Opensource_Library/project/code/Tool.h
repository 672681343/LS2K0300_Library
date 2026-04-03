#pragma once
/* Includes ------------------------------------------------------------------*/
#include "Camera.h"
#include "zf_common_headfile.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {
    DIRECTION_UP_LEFT = 1,  
    DIRECTION_DOWN_LEFT = 2,
    DIRECTION_UP_RIGHT = 3,
    DIRECTION_DOWN_RIGHT = 4
} LineDirection;

typedef enum {
    MODE_TWO_POINTS = 0,
    MODE_AUTO_SLOPE = 1,
    MODE_FIXED_SLOPE = 2
} LineMode;


/* Exported constants --------------------------------------------------------*/
constexpr int32_t SCALE = 1000;
extern std::array<uint16_t, HEIGHT> tem_fit;

extern uint8_t M_x;

/* Exported functions ------------------------------------------------------- */
void fitting_line(uint8_t start,uint8_t end,
    const std::array<uint16_t, HEIGHT>& raw_line,
    std::array<uint16_t, HEIGHT>& fit_line
);

float two_segment_fitting_int(
    uint16_t start1, uint16_t end1,
    uint16_t start2, uint16_t end2,
    const std::array<uint16_t, HEIGHT>& raw_line,
    std::array<uint16_t, HEIGHT>& fit_line
);

float fangcha(uint8_t start, uint8_t end,
    const std::array<uint16_t, HEIGHT>& raw_line,
    const std::array<uint16_t, HEIGHT>& fit_line
);

uint8_t island_Down(uint8_t start, uint8_t end, uint8_t mode,
    const std::array<uint16_t, HEIGHT>& Search_line);
    
uint8_t island_Up(uint8_t start, uint8_t end, uint8_t mode,
    const std::array<uint16_t, HEIGHT>& Search_line);

uint8_t island_sideUp(uint8_t start, uint8_t end, uint8_t mode,
    const std::array<uint16_t, HEIGHT>& Search_line);

uint8_t Circular_Detection(uint8_t type, uint8_t limit, uint8_t start, uint8_t end,
    const std::array<uint16_t, HEIGHT>& Search_line);

uint8_t Region_Search(uint8_t x, uint8_t y,
    uint8_t Kerner_size, uint8_t* bin);
    
uint8_t cross_Up(uint8_t start, uint8_t end, uint8_t mode,
    const std::array<uint16_t, HEIGHT>& Search_line);

uint8_t cross_Down(uint8_t start, uint8_t end, uint8_t mode,
    const std::array<uint16_t, HEIGHT>& Search_line);
    
uint8_t zebra_Up(uint8_t start, uint8_t end, uint8_t mode,
    const std::array<uint16_t, HEIGHT>& Search_line);

uint8_t zebra_Down(uint8_t start, uint8_t end, uint8_t mode,
    const std::array<uint16_t, HEIGHT>& Search_line);
    
uint8_t roadblack_Up(uint8_t start, uint8_t end, uint8_t mode,
    const std::array<uint16_t, HEIGHT>& Search_line);

int16_t width_anomaly_detection (uint8_t start, uint8_t end, 
    const std::array<uint16_t, HEIGHT>& l_line,
    const std::array<uint16_t, HEIGHT>& r_line,
    const std::array<uint8_t, HEIGHT>& Contrast_line);
    
float my_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2,
    LineMode mode, LineDirection dir, float slope);

uint8_t M_point(uint8_t start, uint8_t end,
    const std::array<uint8_t, WIDTH>& Search_line);
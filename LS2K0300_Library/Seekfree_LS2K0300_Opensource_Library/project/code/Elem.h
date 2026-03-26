#pragma once
#include "zf_common_headfile.h"

namespace Elem {

    typedef struct{
        uint8_t U_y;
        uint8_t U_flag;
        uint8_t D_y;
        uint8_t D_flag;
        uint8_t Curve_Row;
        uint8_t Curve_flag;
        uint8_t TriOrRound_flag;
    }_Corner;
    

    typedef struct{
        uint8_t enable;
        uint8_t similar_flag;
        uint8_t State;
        uint8_t Direction;
    }_Cross;

    typedef struct{
        uint8_t enable;
        uint8_t similar_flag;
        uint8_t State;
        uint8_t x1;  
        uint8_t x2;
    }_Island;

    typedef struct{
        uint8_t enable;
        uint8_t State;
        uint8_t y;
        uint8_t count;
        uint8_t count_threshold;
    }_Zebra;

    typedef struct{
        uint8_t enable;
        uint8_t similar_flag;
        uint8_t State;
        uint8_t black_threshold;
    }_Tilt;

    typedef struct{
        uint8_t enable;
        uint8_t State;
        float k_L_black;
        float k_R_black;
    }_Roadblock;

    typedef struct{
        uint8_t enable;
        uint8_t State;
        uint8_t y;
        float R_UP_SLPOE;
        float R_DOWN_SLPOE;
        float L_UP_SLPOE;
        float L_DOWN_SLPOE;
        float L_slope;
        float R_slope;
    }_Ramp;

    extern _Corner Right_corner;
    extern _Corner Left_corner;
    extern _Island Right_Island;
    extern _Island Left_Island;
    extern _Cross Cross;
    extern _Zebra Zebra;
    extern _Tilt Right_Tilt_Cross;
    extern _Tilt Left_Tilt_Cross;
    extern _Roadblock Right_Roadblock;
    extern _Roadblock Left_Roadblock;
    extern _Ramp Ramp;
    extern uint8_t isHandling;
    extern uint16_t black_count;
    extern float draw_slope;
    extern int16_t raw_count;
    extern float roadblack_slope;
    
    void elem_judge();
    void clear();
    void tilt_cross_detection(void);
    void cross_detection(void);
    void island_detection(void);
    void zebra_detection(void);
    void roadblock_detection(void);
    void ramp_detection(void);
    uint8_t find_zebra(uint8_t start_row, uint8_t end_row);
}
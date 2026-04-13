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
        uint8_t similar_flag;
        uint8_t State;
        uint8_t black_threshold;
    }_Tilt;



    extern _Corner Right_corner;
    extern _Corner Left_corner;
    extern _Island Right_Island;
    extern _Island Left_Island;
    extern _Cross Cross;

    extern _Tilt Right_Tilt_Cross;
    extern _Tilt Left_Tilt_Cross;

    extern uint8_t isHandling;

    extern uint8_t M_y;
    
    void elem_judge();
    void clear();
    void tilt_cross_detection(void);
    void cross_detection(void);
    void island_detection(void);

}
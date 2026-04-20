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
    

    typedef struct{ //  定义一个名为_Cross的结构体，用于存储十字路口相关信息
        uint8_t enable; //  使能标志，表示是否启用该十字路口功能
        uint8_t similar_flag; //  相似标志，用于标记相似性或匹配状态
        uint8_t State; //  状态变量，表示当前十字路口的状态
        uint8_t Direction; //  方向变量，表示当前行进方向或转向信息
    }_Cross; //  结构体类型名称，用于声明_Cross类型的变量

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


    typedef struct{
        uint8_t enable;
        uint8_t State;
        uint8_t y;
        uint8_t count;
        uint8_t count_threshold;
    }_Zebra;


    extern _Corner Right_corner;
    extern _Corner Left_corner;
    extern _Island Right_Island;
    extern _Island Left_Island;
    extern _Cross Cross;
    extern _Zebra Zebra;
    extern _Tilt Right_Tilt_Cross;
    extern _Tilt Left_Tilt_Cross;

    extern uint8_t isHandling;

    extern uint8_t M_y;
    
    void elem_judge();
    void clear();
    void tilt_cross_detection(void);
    void cross_detection(void);
    void island_detection(void);

    void zebra_detection(void);
    uint8_t find_zebra(uint8_t start_row, uint8_t end_row);

}
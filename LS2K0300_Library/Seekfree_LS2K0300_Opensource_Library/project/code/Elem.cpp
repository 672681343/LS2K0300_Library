#include "Elem.h"

namespace Elem {
    _Corner Right_corner = {};
    _Corner Left_corner = {};

    _Island Right_Island = {};
    _Island Left_Island = {};

    _Tilt Right_Tilt_Cross = {};
    _Tilt Left_Tilt_Cross = {};

    _Cross Cross = {};

    _Zebra Zebra = {};

    _Roadblock Right_Roadblock = {};
    _Roadblock Left_Roadblock = {};

    _Ramp Ramp = {};

    uint8_t isHandling = 0; // 是否正在处理元素
    uint16_t black_count = 0; // 黑色像素计数参考值为24
    float draw_slope = 0.0f;

    int16_t  raw_count = 0;
    float roadblack_slope = 0.0f;
    void elem_judge()
    {
        black_count = 0;

        Right_Tilt_Cross.similar_flag = 0;
        Left_Tilt_Cross.similar_flag = 0;
        
        Right_corner.Curve_Row = 0;
        Left_corner.Curve_Row = 0;

        Left_Island.similar_flag = 0;
        Right_Island.similar_flag = 0;
        
        zebra_detection();
        tilt_cross_detection();
        island_detection();
        cross_detection();
        roadblock_detection();
        ramp_detection();
    }

    void clear()
    {
        MenuSystem::beep_off();
        Right_corner = {};
        Left_corner = {};
        Right_corner.Curve_Row = 0;
        Left_corner.Curve_Row = 0;
        Error::Err.mode = 0;
        Error::Err.limit = HEIGHT - 1;
        ImageProc::state.mode = 0;
        ImageProc::state.range_l = 5;
        ImageProc::state.range_r = WIDTH - 5;

        raw_count = 0;
        roadblack_slope = 0;
        black_count = 0;
        draw_slope = 0.0f;

        Cross.State = 0;
        Left_Island.State = 0;
        Right_Island.State = 0;
        Zebra.State = 0;
        Zebra.y = 0;
        Right_Tilt_Cross.State = 0;
        Left_Tilt_Cross.State = 0;
        Right_Roadblock.State = 0;
        Left_Roadblock.State = 0;
        Ramp.State = 0;
        Ramp.y = 0;
        Ramp.R_UP_SLPOE = 0.0f;
        Ramp.R_DOWN_SLPOE = 0.0f;
        Ramp.L_UP_SLPOE = 0.0f;
        Ramp.L_DOWN_SLPOE = 0.0f;
        Ramp.L_slope = 0.0f;
        Ramp.R_slope = 0.0f;
        isHandling = 1; // 设置为正在处理状态
    }
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 斜元素
    // 参数说明: 
    // 返回参数: 
    // 使用示例: 
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    void tilt_cross_detection(void)
    {
        
        switch (Left_Tilt_Cross.State)
        {
            case 0:
                {
                    if (isHandling == 0 && Left_Tilt_Cross.enable &&
                        Left_Island.State == 0 && Right_Island.State == 0 && Cross.State == 0 && Right_Tilt_Cross.State == 0 &&
                        Zebra.State == 0 && Left_Roadblock.State == 0 && Right_Roadblock.State == 0 && Ramp.State == 0 &&
                        ImageProc::state.start_point < 50 && ImageProc::state.cutoff < 60 &&
                        ImageProc::Left_BF.LostParam.sum >= 38 && ((ImageProc::Right_BF.LostParam.sum < 15 && ImageProc::Right_BF.LostParam.start > 67 ) || ImageProc::Right_BF.LostParam.sum < 2) &&
                        ImageProc::Left_BF.Var < 10 && ImageProc::Right_BF.Var > 50 && ImageProc::Right_BF.Slope > 2)
                    {
                        Left_Tilt_Cross.similar_flag = 1;
                        Right_corner.Curve_Row = 0;
                        Right_corner.Curve_Row = Circular_Detection(1, 0, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 1, ImageProc::r_black);
                        
                        if(Right_corner.Curve_Row > 15 && Right_corner.Curve_Row < HEIGHT - 10)
                        {
                            Elem::black_count = Region_Search(ImageProc::r_black[Right_corner.Curve_Row] + 3, Right_corner.Curve_Row, 9, Camera::bin_pixels);
                        }
                        if (Right_corner.Curve_Row != 0 && Elem::black_count < Left_Tilt_Cross.black_threshold && Elem::black_count != 0)
                        {
                            Left_Tilt_Cross.State = 1;
                            ImageProc::state.range_r = my_max(ImageProc::r_black[Right_corner.Curve_Row] - 5, 20);
                        }
                    }
                    break;
                }
            case 1:
                {
                    Right_corner.Curve_Row = Circular_Detection(1, 0, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 1, ImageProc::r_black);
                    draw_slope = -5;
                    if (Right_corner.Curve_Row > 15 && Right_corner.Curve_Row < HEIGHT - 10)
                    {
                        ImageProc::state.range_r = my_max(ImageProc::r_black[Right_corner.Curve_Row] - 5, 20);
                        draw_slope = my_draw_line(ImageProc::r_black[Right_corner.Curve_Row], Right_corner.Curve_Row, 0, 0, MODE_AUTO_SLOPE, DIRECTION_DOWN_RIGHT, 0);
                    }
                    if (ImageProc::Left_BF.LostParam.sum < 2){
                        clear();
                        Left_Tilt_Cross.State = 0;
                    }
                    if (draw_slope > -3)
                    {
                        ImageProc::state.range_r = WIDTH - 10;
                        Left_Tilt_Cross.State = 2;
                    }
                    break;
                }
            case 2:
                {
                    Left_corner.U_y = cross_Up(ImageProc::Left_BF.LostParam.mid , HEIGHT - ImageProc::state.cutoff - 1, 0, ImageProc::l_black);
                    Left_corner.D_y = cross_Down(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff - 1, 0, ImageProc::l_black);

                    Right_corner.U_y = cross_Up(ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff - 1, 1, ImageProc::r_black);
                    Right_corner.D_y = cross_Down(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff - 1, 1, ImageProc::r_black);

                    Right_corner.Curve_Row = Circular_Detection(1, 0, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 1, ImageProc::r_black);
                    
                    if (Right_corner.Curve_Row > 15 && Right_corner.Curve_Row < HEIGHT - 10)
                    {
                        my_draw_line(ImageProc::r_black[Right_corner.Curve_Row], Right_corner.Curve_Row, 0, 0, MODE_AUTO_SLOPE, DIRECTION_DOWN_RIGHT, 0);
                    }
                    if (ImageProc::Left_BF.LostParam.sum < 2){
                        clear();
                        Left_Tilt_Cross.State = 0;
                    }
                    //左斜右环岛
                    // if ( ImageProc::state.cutoff > 55 && ImageProc::Left_BF.Var < 40 && ImageProc::Right_BF.Var > 500)
                    // {
                    //     Right_corner.Curve_Row = Circular_Detection(1, 0, Right_corner.D_y - 10, HEIGHT - ImageProc::state.cutoff + 1, ImageProc::r_black);
                    //     if (Right_corner.Curve_Row > 15 && Right_corner.Curve_Row < HEIGHT - 10)
                    //     {
                    //         clear();
                    //         Left_Tilt_Cross.State = 0;
                    //         Right_Island.State = 1;
                    //     }
                    // }
                    //左斜十字
                    if (ImageProc::state.cutoff > 55 &&
                        (((Left_corner.U_y > 10 && Left_corner.U_y < HEIGHT - 15) &&
                        (Right_corner.U_y > 10 && Right_corner.U_y < HEIGHT - 15)) ||
                        ((Left_corner.D_y > 15 && Left_corner.D_y < HEIGHT - 10) &&
                        (Right_corner.D_y > 15 && Right_corner.D_y < HEIGHT - 10)))
                        )
                    {
                        clear();
                        Left_Tilt_Cross.State = 0;
                        Cross.State = 1;
                    }
                    break;
                }
        }
        switch (Right_Tilt_Cross.State)
        {
            case 0:
                {
                    if (isHandling == 0 && Right_Tilt_Cross.enable &&
                        Right_Island.State == 0 && Left_Island.State == 0 && Cross.State == 0 && Left_Tilt_Cross.State == 0 &&
                        Zebra.State == 0 && Left_Roadblock.State == 0 && Right_Roadblock.State == 0 && Ramp.State == 0 && 
                        ImageProc::state.start_point > WIDTH - 50 && ImageProc::state.cutoff < 60 &&
                        ImageProc::Right_BF.LostParam.sum >= 38 && ((ImageProc::Left_BF.LostParam.sum < 15 && ImageProc::Left_BF.LostParam.start > 67) || ImageProc::Left_BF.LostParam.sum < 2)&&
                        ImageProc::Right_BF.Var < 10 && ImageProc::Left_BF.Var > 50 && ImageProc::Left_BF.Slope < -2)
                    {
                        Right_Tilt_Cross.similar_flag = 1;
                        Left_corner.Curve_Row = 0;
                        Left_corner.Curve_Row = Circular_Detection(0, 0, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 1, ImageProc::l_black);

                        if(Left_corner.Curve_Row > 15 && Left_corner.Curve_Row < HEIGHT - 10)
                        {
                            Elem::black_count = Region_Search(ImageProc::l_black[Left_corner.Curve_Row] - 3, Left_corner.Curve_Row, 9, Camera::bin_pixels);
                        }

                        if (Left_corner.Curve_Row != 0 && Elem::black_count < Right_Tilt_Cross.black_threshold && Elem::black_count != 0)
                        {
                            Right_Tilt_Cross.State = 1;
                            ImageProc::state.range_l = my_min(ImageProc::l_black[Left_corner.Curve_Row] + 5, WIDTH - 20);
                        }
                    }
                    break;
                }
            case 1:
                {
                    Left_corner.Curve_Row = Circular_Detection(0, 0, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 1, ImageProc::l_black);
                    draw_slope = 5;
                    if (Left_corner.Curve_Row > 15 && Left_corner.Curve_Row < HEIGHT - 10)
                    {
                        ImageProc::state.range_l = my_min(ImageProc::l_black[Left_corner.Curve_Row] + 5, WIDTH - 20);
                        draw_slope = my_draw_line(ImageProc::l_black[Left_corner.Curve_Row], Left_corner.Curve_Row, 0, 0, MODE_AUTO_SLOPE, DIRECTION_DOWN_LEFT, 0);
                    }
                    if (ImageProc::Right_BF.LostParam.sum < 2){
                        clear();
                        Right_Tilt_Cross.State = 0;
                    }
                    if (draw_slope < 3)
                    {
                        ImageProc::state.range_l = 10;
                        Right_Tilt_Cross.State = 2;
                    }
                    break;
                }
            case 2:
                {
                    Left_corner.U_y  = cross_Up(ImageProc::Left_BF.LostParam.mid , HEIGHT - ImageProc::state.cutoff - 1, 0, ImageProc::l_black);
                    Left_corner.D_y  = cross_Down(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff - 1, 0, ImageProc::l_black);

                    Right_corner.U_y = cross_Up(ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff - 1, 0, ImageProc::l_black);
                    Right_corner.D_y = cross_Down(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff - 1, 1, ImageProc::r_black);

                    Left_corner.Curve_Row = Circular_Detection(0, 0, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 1, ImageProc::l_black);
                    if (ImageProc::Right_BF.LostParam.sum < 2){
                        clear();
                        Right_Tilt_Cross.State = 0;
                    }
                    if (Left_corner.Curve_Row > 15 && Left_corner.Curve_Row < HEIGHT - 10)
                    {
                        my_draw_line(ImageProc::l_black[Left_corner.Curve_Row], Left_corner.Curve_Row, 0, 0, MODE_AUTO_SLOPE, DIRECTION_DOWN_LEFT, 0);
                    }
                    //右斜左环岛
                    // if (ImageProc::state.cutoff > 55 && ImageProc::Right_BF.Var < 40 && ImageProc::Left_BF.Var > 500)
                    // {
                    //     Left_corner.Curve_Row = Circular_Detection(0, 0, Left_corner.D_y - 10, HEIGHT - ImageProc::state.cutoff + 1, ImageProc::l_black);
                    //     if (Left_corner.Curve_Row > 15 && Left_corner.Curve_Row < HEIGHT - 10)
                    //     {
                    //         clear();
                    //         Right_Tilt_Cross.State = 0;
                    //         Left_Island.State = 1;
                    //     }
                    // }
                    //右斜十字
                    if (ImageProc::state.cutoff > 55 &&
                        (((Left_corner.U_y > 10 && Left_corner.U_y < HEIGHT - 15) &&
                        (Right_corner.U_y > 10 && Right_corner.U_y < HEIGHT - 15)) ||
                        ((Left_corner.D_y > 15 && Left_corner.D_y < HEIGHT - 10) &&
                        (Right_corner.D_y > 15 && Right_corner.D_y < HEIGHT - 10)))
                        )
                    {
                        clear();
                        Right_Tilt_Cross.State = 0;
                        Cross.State = 1;
                    }
                    break;
                }
        }
        // SetText("左斜十字状态:" + Left_Tilt_Cross_State + "   左类似斜十字:" + Left_Tilt_Cross_similar_flag);
        // SetText("右斜十字状态:" + Right_Tilt_Cross_State + "   右类似斜十字:" + Right_Tilt_Cross_similar_flag);
    }
    
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 十字补线函数(内部函数)
    // 参数说明: 
    // 返回参数: 
    // 使用示例: 
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    void Crosshairs(void){

        // uint8_t L_flag = 0;
        // uint8_t R_flag = 0;

        //左两点
        if (Left_corner.U_y != 0 && Left_corner.D_y != 0){
            my_draw_line(ImageProc::l_black[Left_corner.U_y], Left_corner.U_y,
                        ImageProc::l_black[Left_corner.D_y], Left_corner.D_y,
                        MODE_TWO_POINTS, DIRECTION_DOWN_LEFT, 0);
        }
        //左一点上
        else if (Left_corner.U_y != 0){
            my_draw_line(ImageProc::l_black[Left_corner.U_y], Left_corner.U_y,
                        0, 0,
                        MODE_AUTO_SLOPE, DIRECTION_UP_LEFT, 0);
        }
        //左一点下
        else if (Left_corner.D_y != 0){
            //printf("Left_corner.D_y = %d\n", Left_corner.D_y);
            my_draw_line(ImageProc::l_black[Left_corner.D_y], Left_corner.D_y,
                0, 0,
                MODE_AUTO_SLOPE, DIRECTION_DOWN_LEFT, 0);
        }
        else if (Left_corner.Curve_Row != 0)
        {

        }
        //左无
        else{
            // L_flag = 1;
        }

        //右两点
        if (Right_corner.U_y != 0 && Right_corner.D_y != 0){
            my_draw_line(ImageProc::r_black[Right_corner.U_y], Right_corner.U_y,
                        ImageProc::r_black[Right_corner.D_y], Right_corner.D_y,
                        MODE_TWO_POINTS, DIRECTION_DOWN_RIGHT, 0);
        }
        //右一点上
        else if (Right_corner.U_y != 0){
            my_draw_line(ImageProc::r_black[Right_corner.U_y], Right_corner.U_y,
                        0, 0,
                        MODE_AUTO_SLOPE, DIRECTION_UP_RIGHT, 0);
        }
        //右一点下
        else if (Right_corner.D_y != 0){
            my_draw_line(ImageProc::r_black[Right_corner.D_y], Right_corner.D_y,
                        0, 0,
                        MODE_AUTO_SLOPE, DIRECTION_DOWN_RIGHT, 0);
        }
        else if (Right_corner.Curve_Row != 0)
        {

        }
        //右无
        else{
            // R_flag = 1;
        }

        // //左边缺失，沿右边
        // if (L_flag == 1 && R_flag != 1){
        //     Error::Err.mode = 1;
        // }
        // //右边缺失，沿左边
        // else if (L_flag != 1 && R_flag == 1){
        //     Error::Err.mode = -1;
        // }
        // //正常
        // else{
        //     Error::Err.mode = 0;
        // }
    }
    
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 元素（十字）
    // 参数说明: 
    // 返回参数: 
    // 使用示例: 
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    void cross_detection(void)
    {
        Cross.similar_flag = 0;

        switch (Cross.State)
        {
            case 0:{
                if( isHandling == 0 && Cross.enable &&
                    Left_Island.State == 0 && Right_Island.State == 0 && Left_Tilt_Cross.State == 0 && Right_Tilt_Cross.State == 0 &&
                    Zebra.State == 0 && Left_Roadblock.State == 0 && Right_Roadblock.State == 0 && Ramp.State == 0 &&
                    ImageProc::state.start_point >= 50 &&
                    (((ImageProc::Left_BF.LostParam.sum >4 && ImageProc::Left_BF.LostParam.start < 45) &&
                     (ImageProc::Right_BF.LostParam.sum > 4 && ImageProc::Right_BF.LostParam.start < 45)&&
                    my_abs(ImageProc::Left_BF.LostParam.sum - ImageProc::Right_BF.LostParam.sum) < 10 &&
                    my_abs(ImageProc::Left_BF.LostParam.mid - ImageProc::Right_BF.LostParam.mid) < 20) ||
                    (ImageProc::sec_loss_both.LostParam.sum > 3 &&
                    ImageProc::Left_BF.LostParam.sum < 25 && ImageProc::Right_BF.LostParam.sum < 25)) &&
                    (ImageProc::Left_BF.Var > 100 || ImageProc::Right_BF.Var > 100) )
                {
                    Left_corner.U_y  = cross_Up(ImageProc::Left_BF.LostParam.mid,  HEIGHT - ImageProc::state.cutoff - 1, 0, ImageProc::l_black);
                    Right_corner.U_y = cross_Up(ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff - 1, 1, ImageProc::r_black);
                    Left_corner.D_y  = cross_Down(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff - 1, 0, ImageProc::l_black);
                    Right_corner.D_y = cross_Down(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff - 1, 1, ImageProc::r_black);

                    Cross.similar_flag = 1;
                    if (Left_corner.D_y > 15 && Left_corner.D_y < HEIGHT - 10 &&
                        Right_corner.D_y> 15 && Right_corner.D_y < HEIGHT - 10 && 
                        ImageProc::r_black[Right_corner.D_y] - ImageProc::l_black[Left_corner.D_y] > 40
                        )
                    {
                        Cross.State = 1;
                        MenuSystem::beep_on();
                    }
                }
                break;
            }
            case 1:{
                // Left_corner.U_y  = cross_Up(ImageProc::Left_BF.LostParam.mid,  HEIGHT - ImageProc::state.cutoff - 1, 0, ImageProc::l_black);
                // Right_corner.U_y = cross_Up(ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff - 1, 1, ImageProc::r_black);
                Left_corner.D_y  = cross_Down(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff - 1, 0, ImageProc::l_black);
                Right_corner.D_y = cross_Down(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff - 1, 1, ImageProc::r_black);

                if (Left_corner.D_y > 20){
                    Left_corner.U_y = cross_Up(Left_corner.D_y,  HEIGHT - ImageProc::state.cutoff - 1, 0, ImageProc::l_black);
                }else{
                    Left_corner.U_y = cross_Up(HEIGHT - 20,  HEIGHT - ImageProc::state.cutoff - 1, 0, ImageProc::l_black);
                }
                
                if (Right_corner.D_y > 20){
                    Right_corner.U_y = cross_Up(Right_corner.D_y, HEIGHT - ImageProc::state.cutoff - 1, 1, ImageProc::r_black);
                }else{
                    Right_corner.U_y = cross_Up(HEIGHT - 20, HEIGHT - ImageProc::state.cutoff - 1, 1, ImageProc::r_black);
                }

                if (Left_corner.U_y < 10) { Left_corner.U_y = 0; }
                if (Right_corner.U_y < 10) { Right_corner.U_y = 0; }
                if (Left_corner.U_y > Left_corner.D_y ) { Left_corner.D_y = 0; }
                if (Right_corner.U_y > Right_corner.D_y ) { Right_corner.D_y = 0; }
                if (Left_corner.D_y > HEIGHT - 5) { Left_corner.D_y = 0; }
                if (Right_corner.D_y > HEIGHT - 5) { Right_corner.D_y = 0; }
                Crosshairs();

                if (ImageProc::Mid_BF.LostParam.sum > 25 ){
                    Cross.State = 2;
                    Left_corner.D_y = 0;
                    Right_corner.D_y = 0;
                }
                break;
            }
            case 2:{

                if (Left_corner.U_y > 35 || Right_corner.U_y > 35){
                    Cross.State = 3;
                }
                Left_corner.U_y  = cross_Up(ImageProc::Left_BF.LostParam.mid,  ImageProc::Left_BF.LostParam.end - 20, 0, ImageProc::l_black);
                Right_corner.U_y = cross_Up(ImageProc::Right_BF.LostParam.mid, ImageProc::Right_BF.LostParam.end - 20, 1, ImageProc::r_black);

                if (Left_corner.U_y < ImageProc::Left_BF.LostParam.end - 20) { Left_corner.U_y = 0; }
                if (Right_corner.U_y < ImageProc::Right_BF.LostParam.end - 20) { Right_corner.U_y = 0; }
                if (Left_corner.U_y > Left_corner.D_y ) { Left_corner.D_y = 0; }
                if (Right_corner.U_y > Right_corner.D_y ) { Right_corner.D_y = 0; }
                if (Left_corner.D_y > HEIGHT - 5) { Left_corner.D_y = 0; }
                if (Right_corner.D_y > HEIGHT - 5) { Right_corner.D_y = 0; }
                Crosshairs();
                break;
            }
            case 3:{
                
                if (ImageProc::Mid_BF.LostParam.sum < 10 || ImageProc::Left_BF.LostParam.start < 30 || ImageProc::Right_BF.LostParam.start < 30){
                    Cross.State = 0;
                    Error::Err.mode = 0;
                    clear();
                    break;
                }
                Left_corner.U_y  = cross_Up(ImageProc::Left_BF.LostParam.mid,  ImageProc::Left_BF.LostParam.end - 20, 0, ImageProc::l_black);
                Right_corner.U_y = cross_Up(ImageProc::Right_BF.LostParam.mid, ImageProc::Right_BF.LostParam.end - 20, 1, ImageProc::r_black);
                if (Left_corner.U_y < ImageProc::Left_BF.LostParam.end - 20) { Left_corner.U_y = 0; }
                if (Right_corner.U_y < ImageProc::Right_BF.LostParam.end - 20) { Right_corner.U_y = 0; }
                if (Left_corner.U_y > Left_corner.D_y ) { Left_corner.D_y = 0; }
                if (Right_corner.U_y > Right_corner.D_y ) { Right_corner.D_y = 0; }
                if (Left_corner.D_y > HEIGHT - 5) { Left_corner.D_y = 0; }
                if (Right_corner.D_y > HEIGHT - 5) { Right_corner.D_y = 0; }
                Crosshairs();

                break;
            }
        }
    }
   
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 元素（环岛）
    // 参数说明: 
    // 返回参数: 
    // 使用示例: 
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    void island_detection(void)
    {
        //左环岛
        switch (Left_Island.State)
        {
            case 0:{
                //左边环岛
                if (isHandling == 0 && Left_Island.enable &&
                    Right_Island.State == 0 && Cross.State == 0 && Left_Tilt_Cross.State == 0 && Right_Tilt_Cross.State == 0 && 
                    Zebra.State == 0 && Left_Roadblock.State == 0 && Right_Roadblock.State == 0 && Ramp.State == 0 &&
                    ImageProc::state.cutoff >= 65 && // 截止行（不变）
                    ((ImageProc::Right_BF.LostParam.start > 67 && ImageProc::Right_BF.LostParam.sum < 15) ||
                     ImageProc::Right_BF.LostParam.sum < 2) && // 右边线特征（镜像）
                    (((ImageProc::Left_BF.LostParam.sum > 6 || ImageProc::sec_loss_left.LostParam.sum > 6)&& ImageProc::Left_BF.LostParam.start < 50) ) && // 左边线特征（镜像）
                    (ImageProc::Mid_BF.LostParam.sum < 2  )&& // 共同丢线特征（不变）
                    ImageProc::Right_BF.Var < 10 && ImageProc::Left_BF.Var > 200 // 方差特征（镜像）
                )
                {
                    Left_corner.D_y = island_Down(HEIGHT - 4, HEIGHT - ImageProc::state.cutoff - 1, 0, ImageProc::l_black);
                    Left_Island.similar_flag = 1;

                    if (Left_corner.D_y != 0)
                    {
                        MenuSystem::beep_on();
                        Left_Island.State = 1;
                    }
                }                        
                //左弯接左环
                if (isHandling == 0 && Left_Island.enable &&
                    Right_Island.State == 0 && Cross.State == 0 && // 镜像右边环岛的条件
                    ImageProc::state.cutoff >= 65 && // 截止行（不变）
                    ((ImageProc::Right_BF.LostParam.sum < 25 && ImageProc::Right_BF.LostParam.start > 67) || 
                    ImageProc::Right_BF.LostParam.start == 0) && 
                    (ImageProc::Left_BF.LostParam.sum > 20 && ImageProc::Left_BF.LostParam.start > 50) &&
                     ImageProc::Left_BF.Var > 80 && ImageProc::Right_BF.Var < 10 &&// 左边线特征（镜像）
                    ImageProc::Right_BF.Slope < 2.5
                    )
                {
                    // 调用环形检测函数处理右侧，参数调整为右侧相关
                    Left_corner.Curve_Row = Circular_Detection(0, 0, ImageProc::Left_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::l_black);
                    Right_corner.Curve_Row = Circular_Detection(1, 0, ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::r_black);

                    if (Left_corner.Curve_Row != 0) {
                        Left_corner.U_y = island_Up(Left_corner.Curve_Row, HEIGHT - ImageProc::state.cutoff + 2, 0, ImageProc::l_black);
                    }
                    Left_Island.similar_flag = 1;

                    if (Left_corner.Curve_Row != 0 && Left_corner.U_y != 0 && (Right_corner.Curve_Row < 10 || Right_corner.Curve_Row > HEIGHT - 10))
                    {
                        MenuSystem::beep_on();
                        Left_Island.State = 2;
                    }
                }
                break;
            }
            case 1:{
                Left_corner.D_y = island_Down(HEIGHT - 4, HEIGHT - ImageProc::state.cutoff + 2, 0 ,ImageProc::l_black);
                Left_corner.Curve_Row = Circular_Detection(0, 0, ImageProc::Left_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::l_black);
                
                if (Left_corner.D_y < Left_corner.Curve_Row + 10 && Left_corner.Curve_Row != 0){ Left_corner.D_y = 0;}
                if (Left_corner.D_y != 0 && Left_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.Curve_Row], Left_corner.Curve_Row, 
                                ImageProc::l_black[Left_corner.D_y], Left_corner.D_y, 
                                MODE_TWO_POINTS, DIRECTION_DOWN_LEFT, 0);
                    Error::Err.mode = 0;        
                }else if (Left_corner.D_y != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.D_y], Left_corner.D_y, 
                                0, 0, 
                                MODE_AUTO_SLOPE, DIRECTION_DOWN_LEFT, 0);
                    Error::Err.mode = 0;        
                }else if (Left_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.Curve_Row], Left_corner.Curve_Row, 
                                0, 0, 
                                MODE_FIXED_SLOPE, DIRECTION_UP_LEFT, -ImageProc::Right_BF.Slope);
                    Error::Err.mode = 0;        
                }else{
                    Error::Err.mode = 1;        
                }

                if (Left_Island.State){
                    //左下角大面积丢线
                    if (ImageProc::Left_BF.LostParam.sum > 30 && ImageProc::Left_BF.LostParam.start > 67){
                        Left_Island.State = 2;
                        Error::Err.mode = 0;
                    }
                }
                break;
            }
            case 2:{
                //左上拐点位置
                static uint8_t state_count = 0;
                Left_corner.Curve_Row = Circular_Detection(0, 0, HEIGHT - 5, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::l_black);
                if (ImageProc::Left_BF.LostParam.mid > Left_corner.Curve_Row &&
                     ImageProc::Left_BF.LostParam.start < 50){
                    Left_corner.Curve_Row = 0;
                }
                Left_corner.U_y = island_Up(ImageProc::Left_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 2, 0 ,ImageProc::l_black);
                
                if (Left_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.Curve_Row], Left_corner.Curve_Row, 
                                0, 0, 
                                MODE_FIXED_SLOPE, DIRECTION_UP_LEFT, -ImageProc::Right_BF.Slope);
                }
                
                if (Left_corner.U_y != 0 && Left_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::r_black[Left_corner.Curve_Row], Left_corner.Curve_Row, 
                                ImageProc::l_black[Left_corner.U_y], Left_corner.U_y, 
                                MODE_TWO_POINTS, DIRECTION_UP_RIGHT, 0);
                }else if (Left_corner.U_y != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.U_y], Left_corner.U_y, 
                                ImageProc::r_black[HEIGHT - 10], HEIGHT - 10, 
                                MODE_TWO_POINTS, DIRECTION_UP_RIGHT, 0);
                }
                if (Left_corner.Curve_Row != 0){
                    state_count++;
                }else{
                    state_count = 0;
                }

                if ((Left_corner.Curve_Row == 0 || Left_corner.Curve_Row > 40) && state_count > 3){
                    Left_Island.State = 3;
                }
                break;
            }
            case 3:{
                //左下角再次大面积丢线
                Left_corner.U_y = island_Up(ImageProc::Left_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 2, 0 ,ImageProc::l_black);
                if (Left_corner.U_y != 0){
                    Error::Err.limit = HEIGHT - Left_corner.U_y;
                    my_draw_line(ImageProc::l_black[Left_corner.U_y], Left_corner.U_y,
                                ImageProc::r_black[HEIGHT - 10], HEIGHT - 10, 
                                MODE_TWO_POINTS, DIRECTION_UP_RIGHT, 0);
                }
                if (ImageProc::Right_BF.LostParam.sum > 30){
                    Left_Island.State = 4;
                    ImageProc::state.range_r = ImageProc::l_black[Left_corner.U_y - 2];
                    Right_corner.U_y = Left_corner.U_y;
                }
                break;
            }
            case 4:{
                Error::Err.limit = HEIGHT - 1;
                Right_corner.U_y = island_sideUp(ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 2, 1 ,ImageProc::r_black);
                if (Right_corner.U_y == 0) { Right_corner.U_y = Left_corner.U_y; }
                if (Right_corner.U_y != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.U_y - 1], Right_corner.U_y - 1,
                                 ImageProc::r_black[HEIGHT - 5], HEIGHT - 5, 
                                 MODE_TWO_POINTS, DIRECTION_UP_RIGHT, 0);
                }

                if (ImageProc::Right_BF.LostParam.sum < 15 || 
                    (ImageProc::r_black[ImageProc::Right_BF.LostParam.end - 2] > WIDTH - 5 )){
                    Left_Island.State = 5;
                    ImageProc::state.range_r = WIDTH - 10;
                }
                break;
            }
            case 5:{
                Right_corner.Curve_Row = Circular_Detection(1, 0, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::r_black);
                if (Right_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.Curve_Row + 4], Right_corner.Curve_Row + 4, 
                                0, 0, 
                                MODE_AUTO_SLOPE, DIRECTION_DOWN_RIGHT, 0);
                }
                if (Right_corner.Curve_Row > 25 && ImageProc::Right_BF.LostParam.sum > 2 && ImageProc::Right_BF.LostParam.start < 50){
                    Left_Island.State = 6;
                }else if (Right_corner.Curve_Row == 0 && ImageProc::Right_BF.LostParam.sum > 25 && ImageProc::Right_BF.LostParam.start > 65){
                    Left_Island.State = 6;
                }
                    
                break;
            }
            case 6:{
                if (ImageProc::Right_BF.LostParam.sum > 30 && ImageProc::Right_BF.LostParam.start > 65){
                    Left_Island.State = 7;
                }
                for (uint8_t i = 10; i < 60; i++){
                    ImageProc::l_black[i] = 1;
                    ImageProc::r_black[i] = 1;
                } 
                my_draw_line(WIDTH - Left_Island.x1, 10, WIDTH - Left_Island.x2, 60,  MODE_TWO_POINTS, DIRECTION_DOWN_RIGHT, 0);

                break;
            }
            case 7:{
                Left_corner.U_y = island_Up(ImageProc::Left_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 2, 0 ,ImageProc::l_black);

                if (ImageProc::Right_BF.LostParam.sum < 15 || (Left_corner.U_y < HEIGHT - 20 && Left_corner.U_y > 5)){
                    Left_Island.State = 8;
                }
                for (uint8_t i = 10; i < 60; i++){
                    ImageProc::l_black[i] = 1;
                    ImageProc::r_black[i] = 1;
                } 
                my_draw_line(WIDTH - Left_Island.x1, 10, WIDTH - Left_Island.x2, 60,  MODE_TWO_POINTS, DIRECTION_DOWN_RIGHT, 0);
                break;
            }
            case 8:{
                Left_corner.U_y = island_sideUp(ImageProc::Left_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 2, 0 ,ImageProc::l_black);

                if (Left_corner.U_y != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.U_y], Left_corner.U_y,
                                    0, 0, 
                                    MODE_AUTO_SLOPE, DIRECTION_UP_LEFT, 0);
                }
                if ((ImageProc::Left_BF.LostParam.sum < 30 && Left_corner.U_y < 30) || ImageProc::Left_BF.LostParam.start < 45){
                    Left_Island.State = 0;
                    clear();
                }
                break;
            }
        }
        //右环岛
        switch (Right_Island.State)
        {
            case 0:{
                //右边环岛
                if (isHandling == 0 && Right_Island.enable &&
                    Left_Island.State == 0 && Cross.State == 0 && Left_Tilt_Cross.State == 0 && Right_Tilt_Cross.State == 0 && 
                    Zebra.State == 0 && Left_Roadblock.State == 0 && Right_Roadblock.State == 0 && Ramp.State == 0 &&
                    ImageProc::state.cutoff >= 65 && // 截止行（不变）
                    ((ImageProc::Left_BF.LostParam.start > 67 && ImageProc::Left_BF.LostParam.sum < 15) ||
                     ImageProc::Left_BF.LostParam.sum < 2) && // 左边线特征（镜像）
                    ((ImageProc::Right_BF.LostParam.sum > 6 || ImageProc::sec_loss_right.LostParam.sum > 6)&& ImageProc::Right_BF.LostParam.start < 50) && // 右边线特征（镜像）
                    (ImageProc::Mid_BF.LostParam.sum < 2 )&& // 共同丢线特征（不变）
                    ImageProc::Right_BF.Var > 200 && ImageProc::Left_BF.Var < 10 // 方差特征（镜像）
                   )
                {
                    Right_corner.D_y = island_Down(HEIGHT - 4, HEIGHT - ImageProc::state.cutoff - 1, 1, ImageProc::r_black);
                    Right_Island.similar_flag = 1;

                    if (Right_corner.D_y != 0)
                    {
                        MenuSystem::beep_on();
                        Right_Island.State = 1;
                    }
                }

                //右弯接右环
                if (isHandling == 0 && Right_Island.enable &&
                    Left_Island.State == 0 && Cross.State == 0 && // 镜像左边环岛的条件
                    ImageProc::state.cutoff >= 65 && // 截止行（不变）
                    ((ImageProc::Left_BF.LostParam.sum < 25 && ImageProc::Left_BF.LostParam.start > 67) ||
                    ImageProc::Left_BF.LostParam.sum < 2) && 
                    (ImageProc::Right_BF.LostParam.sum > 20 && ImageProc::Right_BF.LostParam.start > 55) &&
                     ImageProc::Right_BF.Var > 80 && ImageProc::Left_BF.Var < 10 &&
                    ImageProc::Left_BF.Slope > -2.5
                    )
                {
                    // 调用环形检测函数处理左侧，参数调整为左侧相关
                    Right_corner.Curve_Row = Circular_Detection(1, 0, ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::r_black);
                    Left_corner.Curve_Row = Circular_Detection(0, 0, ImageProc::Left_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::l_black);

                    if (Right_corner.Curve_Row != 0){
                        Right_corner.U_y = island_Up(Right_corner.Curve_Row, HEIGHT - ImageProc::state.cutoff + 2, 1 ,ImageProc::r_black);
                    }
                    Right_Island.similar_flag = 1;

                    if (Right_corner.Curve_Row != 0 && Right_corner.U_y != 0 && (Left_corner.Curve_Row < 10 || Left_corner.Curve_Row > HEIGHT - 10))
                    {
                        MenuSystem::beep_on();
                        Right_Island.State = 2;
                    }
                }

                break;
            }
            case 1:{
                Right_corner.D_y = island_Down(HEIGHT - 4, HEIGHT - ImageProc::state.cutoff + 2, 1 ,ImageProc::r_black);
                Right_corner.Curve_Row = Circular_Detection(1, 0, ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::r_black);

                if (Right_corner.D_y < Right_corner.Curve_Row + 10 && Right_corner.Curve_Row != 0){ Right_corner.D_y = 0;}
                if (Right_corner.D_y != 0 && Right_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.Curve_Row], Right_corner.Curve_Row, 
                                ImageProc::r_black[Right_corner.D_y], Right_corner.D_y, 
                                MODE_TWO_POINTS, DIRECTION_DOWN_RIGHT, 0);
                    Error::Err.mode = 0;
                }else if (Right_corner.D_y != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.D_y], Right_corner.D_y, 
                                0, 0, 
                                MODE_AUTO_SLOPE, DIRECTION_DOWN_RIGHT, 0);
                    Error::Err.mode = 0;
                }else if (Right_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.Curve_Row], Right_corner.Curve_Row, 
                                0, 0, 
                                MODE_FIXED_SLOPE, DIRECTION_UP_RIGHT, -ImageProc::Left_BF.Slope);
                    Error::Err.mode = 0;
                }else{
                    Error::Err.mode = -1;
                }

                if (ImageProc::Right_BF.LostParam.sum > 30 && ImageProc::Right_BF.LostParam.start > 67){
                    Right_Island.State = 2;
                    Error::Err.mode = 0;
                }
                break;
            }
            case 2:{
                //右上拐点位置
                static uint8_t state_count = 0;
                Right_corner.Curve_Row = Circular_Detection(1, 0, HEIGHT - 5, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::r_black);
                if (ImageProc::Right_BF.LostParam.mid > Right_corner.Curve_Row &&
                     ImageProc::Right_BF.LostParam.start < 50){
                    Right_corner.Curve_Row = 0;
                }
                Right_corner.U_y = island_Up(ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 2, 1 ,ImageProc::r_black);
                
                if (Right_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.Curve_Row], Right_corner.Curve_Row, 
                                0, 0, 
                                MODE_FIXED_SLOPE, DIRECTION_UP_RIGHT, -ImageProc::Left_BF.Slope);
                }

                if (Right_corner.U_y != 0 && Right_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::l_black[Right_corner.Curve_Row], Right_corner.Curve_Row, 
                                ImageProc::r_black[Right_corner.U_y], Right_corner.U_y, 
                                MODE_TWO_POINTS, DIRECTION_UP_LEFT, 0);
                }else if (Right_corner.U_y != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.U_y], Right_corner.U_y, 
                                ImageProc::l_black[HEIGHT - 10], HEIGHT - 10, 
                                MODE_TWO_POINTS, DIRECTION_UP_LEFT, 0);
                }

                if ((Right_corner.Curve_Row == 0 || Right_corner.Curve_Row > 40) && state_count > 3){
                    Right_Island.State = 3;
                }

                if (Right_corner.Curve_Row != 0){
                    state_count++;
                }else{
                    state_count = 0;
                }
                break;
            }
            case 3:{
                //右下角再次大面积丢线
                Right_corner.U_y = island_Up(ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 2, 1 ,ImageProc::r_black);
                if (Right_corner.U_y != 0){
                    Error::Err.limit = HEIGHT - Right_corner.U_y;
                    my_draw_line(ImageProc::r_black[Right_corner.U_y], Right_corner.U_y, 
                                ImageProc::l_black[HEIGHT - 10], HEIGHT - 10, 
                                MODE_TWO_POINTS, DIRECTION_UP_LEFT, 0);
                }

                if (ImageProc::Left_BF.LostParam.sum > 30){
                    Right_Island.State = 4;
                    ImageProc::state.range_l = ImageProc::r_black[Right_corner.U_y - 2];
                    Left_corner.U_y = Right_corner.U_y;
                }
                break;
            }
            case 4:{
                Error::Err.limit = HEIGHT - 1;
                Left_corner.U_y = island_sideUp(ImageProc::Left_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 2, 0 ,ImageProc::l_black);
                if (Left_corner.U_y == 0) { Left_corner.U_y = Right_corner.U_y; }
                if (Left_corner.U_y != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.U_y - 1], Left_corner.U_y - 1,
                                 ImageProc::l_black[HEIGHT - 5], HEIGHT - 5, 
                                 MODE_TWO_POINTS, DIRECTION_UP_LEFT, 0);
                }
                if (ImageProc::Left_BF.LostParam.sum < 15 || 
                    (ImageProc::l_black[ImageProc::Left_BF.LostParam.end - 2] < 5 )){
                    Right_Island.State = 5;
                    ImageProc::state.range_l = 10;
                }
                break;
            }
            case 5:{
                Left_corner.Curve_Row = Circular_Detection(0, 1, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::l_black);
                if (Left_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.Curve_Row + 4], Left_corner.Curve_Row + 4, 
                                0, 0, 
                                MODE_AUTO_SLOPE, DIRECTION_DOWN_LEFT, 0);
                }
                if (Left_corner.Curve_Row > 25 && ImageProc::Left_BF.LostParam.sum > 2 && ImageProc::Left_BF.LostParam.start < 50){
                    Right_Island.State = 6;
                }else if (Left_corner.Curve_Row == 0 && ImageProc::Left_BF.LostParam.sum > 25 && ImageProc::Left_BF.LostParam.start > 65){
                    Right_Island.State = 6;
                }                
                break;
            } 
            case 6:{

                if (ImageProc::Left_BF.LostParam.sum > 30 && ImageProc::Left_BF.LostParam.start > 65){
                    Right_Island.State = 7;
                }

                for (uint8_t i = 10; i < 60; i++){
                    ImageProc::r_black[i] = WIDTH - 1;
                    ImageProc::l_black[i] = WIDTH - 1;
                }
                my_draw_line(Right_Island.x1, 10, Right_Island.x2, 60,  MODE_TWO_POINTS, DIRECTION_DOWN_LEFT, 0);

                break;
            }
            case 7:{
                Right_corner.U_y = island_Up(ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 2, 1 ,ImageProc::r_black);
                if (ImageProc::Left_BF.LostParam.sum < 15 || ( Right_corner.U_y < HEIGHT - 20 && Right_corner.U_y > 5)){
                    Right_Island.State = 8;
                }
                for (uint8_t i = 10; i < 60; i++){
                    ImageProc::r_black[i] = WIDTH - 1;
                    ImageProc::l_black[i] = WIDTH - 1;
                }
                my_draw_line(Right_Island.x1, 10, Right_Island.x2, 60,  MODE_TWO_POINTS, DIRECTION_DOWN_LEFT, 0);
                break;
            }
            case 8:{
                Right_corner.U_y = island_sideUp(ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 2, 1 ,ImageProc::r_black);
                if (Right_corner.U_y != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.U_y], Right_corner.U_y,
                                    0, 0, 
                                    MODE_AUTO_SLOPE, DIRECTION_UP_RIGHT, 0);
                }
                if ((ImageProc::Right_BF.LostParam.sum < 30 && Right_corner.U_y < 30) || ImageProc::Right_BF.LostParam.start < 30){
                    Right_Island.State = 0;
                    clear();
                }
                break;
            }
        }
    }
    
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 寻找斑马线位置（内部函数）
    // 参数说明: 
    // 返回参数: 
    // 使用示例: 
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    uint8_t find_zebra(uint8_t start_row, uint8_t end_row) {
        // 参数校验

        const uint8_t MIN_CONSECUTIVE = 1;  // 最小连续行数
        const uint8_t MAX_JUMP = 2;         // 最大允许跳变行数
        const uint8_t FIXED_START_ROW = 69; // 固定插值起点
        const uint8_t FIXED_END_ROW = 15;   // 固定插值终点
        const uint8_t near_threshold = 15;   // 远端阈值
        const uint8_t far_threshold = 30;   // 近端阈值

        uint8_t state = 0;          // 状态机：0=未开始，1=检测中
        uint8_t segment_start = 0;  // 连续段的起始行
        uint8_t segment_end = 0;    // 连续段的结束行
        uint8_t valid_count = 0;    // 当前连续段的有效行数
        uint8_t gap_count = 0;      // 当前连续段的跳变计数
        for (uint8_t y = start_row; y > end_row; y--) {
            // 固定线性插值计算（基于69→15）
            uint8_t threshold = far_threshold - 
                (far_threshold - near_threshold) * 
                (FIXED_START_ROW - y) / 
                (FIXED_START_ROW - FIXED_END_ROW);

            // 使用动态阈值检测斑马线
            bool condition_met = (ImageProc::r_black[y] - ImageProc::l_black[y] < threshold);
            
            if (state == 0) {
                // 状态0：寻找连续段起点
                if (condition_met) {
                    state = 1;
                    segment_start = y;
                    segment_end = y;
                    valid_count = 1;
                    gap_count = 0;
                }
            } else {
                // 状态1：已进入连续段检测
                if (condition_met) {
                    // 有效行：更新结束位置，重置跳变计数
                    segment_end = y;
                    valid_count++;
                    gap_count = 0;
                    
                    // 检查是否达到最小连续长度
                    if (valid_count >= MIN_CONSECUTIVE) {
                        // 计算中间行：避免整数溢出 (start+end)/2
                        uint8_t mid_row = segment_start - (segment_start - segment_end) / 2;
                        return mid_row;
                    }
                } else {
                    // 不满足条件：计入跳变
                    gap_count++;
                    
                    // 检查跳变次数是否超过阈值
                    if (gap_count > MAX_JUMP) {
                        state = 0; // 跳变过多，重置状态
                    }
                }
            }
        }

        // 循环结束后检查是否有有效段
        if (state == 1 && valid_count >= MIN_CONSECUTIVE) {
            uint8_t mid_row = segment_start - (segment_start - segment_end) / 2;
            return mid_row;
        }

        return 0; // 未检测到有效斑马线
    }

    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 元素（斑马线）
    // 参数说明: 
    // 返回参数: 
    // 使用示例: 
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    void zebra_detection(void)
    {
        switch(Zebra.State){
            case 0:{
                Zebra.y = find_zebra(69, HEIGHT -ImageProc::state.cutoff+ 5);

                if (Zebra.y != 0 && Zebra.y < 50 && MenuSystem::Car_flag.start == 1  && Zebra.enable) {
                    // 在斑马线位置的上下5行范围内进行检测
                    for (int dy = -5; dy <= 5; dy++) {
                        uint8_t current_y = Zebra.y + dy;
                        
                        // 边界检查
                        if (current_y >= HEIGHT) continue;
                        
                        uint8_t black_stripe_count = 0;
                        uint8_t current_stripe_length = 0;
                        bool in_black = false;
                        uint8_t threshold = ImageProc::r_black[current_y] - ImageProc::l_black[current_y];
                        // 扫描单行像素
                        for (uint8_t x = 0; x < WIDTH; x++) {
                            uint16_t index = current_y * WIDTH + x;
                            if (Camera::bin_pixels[index] == 0) { // 黑色像素
                                if (!in_black) {
                                    in_black = true;
                                    current_stripe_length = 1;
                                } else {
                                    current_stripe_length++;
                                }
                            } else { // 白色像素
                                if (in_black) {
                                    if (current_stripe_length >= threshold - 2 && current_stripe_length <= threshold + 2) {
                                        black_stripe_count++;
                                    }
                                    in_black = false;
                                }
                            }
                        }
                        
                        // 处理行末未闭合的条纹
                        if (in_black && current_stripe_length >= threshold - 2 && current_stripe_length <= threshold + 2) {
                            black_stripe_count++;
                        }
                        
                        // 如果当前行检测到斑马线特征，记录为有效行
                        if (black_stripe_count >= 4) {
                            Zebra.State = 1; // 切换到下一个状态
                            MenuSystem::beep_on();
                            break; // 找到斑马线后退出循环
                        }
                    }
                }
                break;
            }
            case 1:{
                Zebra.y = find_zebra(69, HEIGHT - ImageProc::state.cutoff + 5);
                
                if(Zebra.y > 20){
                    Zebra.State = 2;
                }
                break;
            }
            case 2:{
                Zebra.y = find_zebra(69, HEIGHT - ImageProc::state.cutoff + 5);
                uint8_t start_row = HEIGHT - 1;
                uint8_t end_row = 20;
                Left_corner.U_y  = zebra_Up(Zebra.y, end_row, 0, ImageProc::l_black) - 2;
                Right_corner.U_y = zebra_Up(Zebra.y, end_row, 1, ImageProc::r_black) - 2;
                Left_corner.D_y  = zebra_Down(start_row, Zebra.y,0, ImageProc::l_black) + 5;
                Right_corner.D_y = zebra_Down(start_row, Zebra.y,1, ImageProc::r_black) + 5;
                // 钳制到 [0, HEIGHT-1] 范围
                // 确保下角y不小于上角y，否则钳制下角y为0
                Left_corner.D_y = my_min(Left_corner.D_y, (Left_corner.U_y > Left_corner.D_y) ? 0 : Left_corner.D_y);
                Right_corner.D_y = my_min(Right_corner.D_y, (Right_corner.U_y > Right_corner.D_y) ? 0 : Right_corner.D_y);
                Crosshairs();
                if(Zebra.y > 55 || Zebra.y == 0){
                    Zebra.State = 0;
                    MenuSystem::beep_off();
                    clear();
                    Zebra.count++;
                }
                break;
            }
        }
    }


    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 元素（避障）
    // 参数说明: 
    // 返回参数: 
    // 使用示例: 
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    void roadblock_detection(void)
    {
        //左避障
        switch (Left_Roadblock.State){
            case 0:{
                if (isHandling == 0 && Left_Roadblock.enable &&
                    (Zebra.State | Cross.State |
                    Left_Island.State | Right_Island.State |
                    Right_Tilt_Cross.State | Left_Tilt_Cross.State | Ramp.State) == 0 &&
                    ImageProc::state.cutoff >= 50 && 
                    ImageProc::Right_BF.LostParam.sum < 10 && ImageProc::Left_BF.LostParam.sum < 10 &&
                    ImageProc::Right_BF.Var < 10) {
                    // 左侧黑色区域检测
                        
                    Left_corner.U_y = roadblack_Up(60, HEIGHT - ImageProc::state.cutoff + 5, 0, ImageProc::l_black);
                    if (Left_corner.U_y < 60 && Left_corner.U_y > 15) {
                        raw_count = width_anomaly_detection(Left_corner.U_y, Left_corner.U_y - 10,
                                                ImageProc::l_black, ImageProc::r_black, Error::ROAD_WIDTH);
                        if (raw_count < -4) {
                            uint8_t start_row = Left_corner.U_y;
                            uint8_t end_row = HEIGHT - ImageProc::state.cutoff + 2;
                            uint8_t protrusions = 0;
                            for (uint8_t i = start_row; i > end_row; i--)
                            {
                                if (ImageProc::l_black[i] > ImageProc::r_black[protrusions]) protrusions = i;
                                if (my_abs((ImageProc::r_black[i] - ImageProc::l_black[i]) - Error::ROAD_WIDTH[i]) < 3){
                                    Left_corner.D_y = i;
                                    break;
                                }
                            }
                            roadblack_slope = two_segment_fitting_int(Left_corner.U_y - 2, Left_corner.D_y + 2, 0, 0,
                                                                    ImageProc::l_black, tem_fit);         
                            if (roadblack_slope < 1 && roadblack_slope > 0 &&
                                my_abs(ImageProc::l_black[protrusions] - ImageProc::l_black[Left_corner.U_y - 2]) < 3){
                                Left_Roadblock.State = 1;
                                MenuSystem::beep_on();  
                            }
                            Left_Roadblock.State = 1;
                            MenuSystem::beep_on();
                        } 
                    }
                }
                break;
            }
            case 1:{
                Left_corner.U_y = roadblack_Up(60, HEIGHT - ImageProc::state.cutoff + 5, 0, ImageProc::l_black);

                if (Left_corner.U_y < 60 && Left_corner.U_y > 15) {
                    my_draw_line(ImageProc::l_black[Left_corner.U_y - 4], Left_corner.U_y - 4, 
                                0, 0,
                                MODE_FIXED_SLOPE, DIRECTION_UP_LEFT, roadblack_slope);
                }

                if (Left_corner.U_y >= 45 || Left_corner.U_y == 0){
                    Left_Roadblock.State = 0;
                    MenuSystem::beep_off();
                    clear();
                }
                break;
            }
        }

        //右避障
        switch (Right_Roadblock.State){
            case 0:{
                if (isHandling == 0 && Right_Roadblock.enable &&
                    (Zebra.State | Cross.State |
                    Left_Island.State | Right_Island.State |
                    Right_Tilt_Cross.State | Left_Tilt_Cross.State | Ramp.State) == 0 &&
                    ImageProc::state.cutoff >= 50 && 
                    ImageProc::Right_BF.LostParam.sum < 10 && ImageProc::Left_BF.LostParam.sum < 10 &&
                    ImageProc::Left_BF.Var < 10) {

                    Right_corner.U_y = roadblack_Up(60, HEIGHT - ImageProc::state.cutoff + 5, 1, ImageProc::r_black);
                    if (Right_corner.U_y < 60 && Right_corner.U_y > 15) {
                        raw_count = width_anomaly_detection(Right_corner.U_y, Right_corner.U_y - 10,
                                                ImageProc::l_black, ImageProc::r_black, Error::ROAD_WIDTH);
                        if (raw_count < -4) {
                            uint8_t start_row = Right_corner.U_y;
                            uint8_t end_row = HEIGHT - ImageProc::state.cutoff + 2;
                            uint8_t protrusions = 0;
                            for (uint8_t i = start_row; i > end_row; i--){
                                if (ImageProc::r_black[i] < ImageProc::r_black[protrusions]) protrusions = i;
                                if (my_abs((ImageProc::r_black[i] - ImageProc::l_black[i]) - Error::ROAD_WIDTH[i]) < 3){
                                    Right_corner.D_y = i;
                                    break;
                                }
                            }
                            roadblack_slope = two_segment_fitting_int(Right_corner.U_y - 2, Right_corner.D_y + 2, 0, 0,
                                                                    ImageProc::r_black, tem_fit);         
                            if (roadblack_slope < 1 && roadblack_slope > 0 &&
                                my_abs(ImageProc::r_black[protrusions] - ImageProc::r_black[Right_corner.U_y - 2]) < 3){
                                Right_Roadblock.State = 1;
                                MenuSystem::beep_on();
                            }
                        }
                    } 
                }
                break;
            }
            case 1:{
                Right_corner.U_y = roadblack_Up(60, HEIGHT - ImageProc::state.cutoff + 5, 1, ImageProc::r_black);

                if (Right_corner.U_y < 60 && Right_corner.U_y > 15) {
                    my_draw_line(ImageProc::r_black[Right_corner.U_y - 4], Right_corner.U_y - 4, 
                                0, 0, 
                                MODE_FIXED_SLOPE, DIRECTION_UP_RIGHT, roadblack_slope);
                }
                if (Right_corner.U_y >= 45 || Right_corner.U_y == 0){
                    Right_Roadblock.State = 0;
                    MenuSystem::beep_off();
                    clear();
                }
                break;
            }
        }
    }

    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 寻找坡道位置（内部函数）
    // 参数说明: 
    // 返回参数: 
    // 使用示例: 
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    uint8_t find_ramp(uint8_t start, uint8_t end, int8_t threshold){
        const uint8_t step = 2; // 步长
        const uint8_t range = 10;
        const uint8_t condition = 8;
        uint8_t temp_row = 0; // 临时位置
        uint8_t ramp_row = 0; // 坡道位置

        if (start < end || start >= HEIGHT || end >= HEIGHT) {
            return 0; // 参数错误
        }

        for (uint8_t i = start; i > end; i -= step){

            if ((ImageProc::r_black[i] - ImageProc::l_black[i]) - Error::ROAD_WIDTH[i] > 10)
            {
                // float right_Up_slope = two_segment_fitting_int(i,i - 5 ,0,0,ImageProc::r_black,ImageProc::fit_r_black);
                // float left_Up_slope = two_segment_fitting_int(i,i - 5 ,0,0,ImageProc::l_black,ImageProc::fit_l_black);
                // float right_Down_slope = two_segment_fitting_int(i,i + 5 ,0,0,ImageProc::r_black,ImageProc::fit_r_black);
                // float left_Down_slope = two_segment_fitting_int(i,i + 5 ,0,0,ImageProc::l_black,ImageProc::fit_l_black);
                //if ((left_Up_slope - left_Down_slope) < -0.1 && (right_Up_slope - right_Down_slope) > 0.1 ){
                    // 找到坡道位置
                    temp_row = i; // 记录当前行
                    break;
                //}
            }
        }
        
        if (temp_row == 0) {
            return 0; // 没有找到坡道
        }
        if (temp_row < range || temp_row > HEIGHT - range) {
            return 0; // 坡道位置不合理
        }
        int start_row = temp_row + range; // 坡道起始行
        int end_row = temp_row - range; // 坡道结束行
        for (uint8_t i = start_row; i > end_row; i--){
            if (i < condition + 2 || i > HEIGHT - condition -2) {
                continue; // 跳过边界行
            }
            uint8_t row = ImageProc::r_black[i] - ImageProc::l_black[i];
            uint8_t up_row = ImageProc::r_black[i - condition] - ImageProc::l_black[i - condition];
            uint8_t down_row = ImageProc::r_black[i + condition] - ImageProc::l_black[i + condition];
            int8_t diff = (down_row - row) - (row - up_row);
            if (diff > threshold ) {
                ramp_row = i; // 记录坡道位置
                break; // 找到坡道位置后退出循环
            }
        }
        return ramp_row; // 返回坡道位置
    }

    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 元素（坡道）
    // 参数说明: 
    // 返回参数: 
    // 使用示例: 
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    void ramp_detection(void)
    {
        Ramp.R_UP_SLPOE = 0;
        Ramp.L_UP_SLPOE = 0;
        Ramp.R_DOWN_SLPOE = 0; 
        Ramp.L_DOWN_SLPOE = 0;
        Ramp.L_slope = 0;
        Ramp.R_slope = 0;
        switch (Ramp.State)
        {
            case 0:{
                if (isHandling == 0 && Ramp.enable &&
                    (Zebra.State | Cross.State |
                    Left_Island.State | Right_Island.State |
                    Right_Tilt_Cross.State | Left_Tilt_Cross.State |
                    Right_Roadblock.State | Left_Roadblock.State) == 0 &&
                    ImageProc::state.cutoff > 65 && 
                    (ImageProc::Right_BF.LostParam.sum < 2 || (ImageProc::Right_BF.LostParam.sum < 15 && ImageProc::Right_BF.LostParam.start)) &&
                    (ImageProc::Left_BF.LostParam.sum < 2 || (ImageProc::Left_BF.LostParam.sum < 15 && ImageProc::Left_BF.LostParam.start)) &&
                    ImageProc::Mid_BF.LostParam.sum < 2 ) {
                    
                    Ramp.y = find_ramp(HEIGHT - 10, 10, 3);

                    if (Ramp.y != 0 && Ramp.y < 45){
                        raw_count = width_anomaly_detection(Ramp.y, Ramp.y - 10,
                                                ImageProc::l_black, ImageProc::r_black, Error::ROAD_WIDTH);
                        
                        Ramp.R_UP_SLPOE = two_segment_fitting_int(Ramp.y,Ramp.y - 10 ,0,0,ImageProc::r_black,ImageProc::fit_r_black);
                        Ramp.L_UP_SLPOE = two_segment_fitting_int(Ramp.y,Ramp.y - 10 ,0,0,ImageProc::l_black,ImageProc::fit_l_black);
                        Ramp.R_DOWN_SLPOE = two_segment_fitting_int(Ramp.y,Ramp.y + 10 ,0,0,ImageProc::r_black,ImageProc::fit_r_black);
                        Ramp.L_DOWN_SLPOE = two_segment_fitting_int(Ramp.y,Ramp.y + 10 ,0,0,ImageProc::l_black,ImageProc::fit_l_black);
                    }
                    if (raw_count > 5 && 
                        (Ramp.R_DOWN_SLPOE - Ramp.R_UP_SLPOE > 0.25 &&
                        Ramp.R_DOWN_SLPOE - Ramp.R_UP_SLPOE <1 && 
                        Ramp.L_DOWN_SLPOE - Ramp.L_UP_SLPOE < -0.25 &&
                        Ramp.L_DOWN_SLPOE - Ramp.L_UP_SLPOE > -1
                        )) {
                        Ramp.State = 1;
                        ImageProc::state.range_l = WIDTH/2 - 30;
                        ImageProc::state.range_r = WIDTH/2 + 30;
                        MenuSystem::beep_on();
                    }
                }
                break;
            }
            case 1:{
                uint8_t temp_y = find_ramp(Ramp.y + 10, Ramp.y - 10, 4);
                if (temp_y != 0 && temp_y < Ramp.y + 10 && temp_y > Ramp.y - 10) {
                    Ramp.y = temp_y; // 更新坡道位置
                }
                if (Ramp.y > 35)
                {
                    Ramp.State = 2;
                }
                break;
            }
            case 2:{
                raw_count = width_anomaly_detection(Ramp.y, Ramp.y - 10,
                        ImageProc::l_black, ImageProc::r_black, Error::ROAD_WIDTH);
                
                if (raw_count > 60) {
                    Ramp.State = 3;
                    //Error::Err.limit = 12;
                    Ramp.L_slope = ImageProc::Left_BF.Slope;
                    Ramp.R_slope = ImageProc::Right_BF.Slope;
                    raw_count = 0;
                }
                break;
            }
            case 3:{
                    my_draw_line(ImageProc::l_black[HEIGHT - 2], HEIGHT - 2, 
                                 0, 0,
                                 MODE_FIXED_SLOPE, DIRECTION_DOWN_LEFT, -Ramp.L_slope);
                    my_draw_line(ImageProc::r_black[HEIGHT - 2], HEIGHT - 2, 
                                 0, 0,
                                 MODE_FIXED_SLOPE, DIRECTION_DOWN_RIGHT, -Ramp.R_slope);

                    if (my_abs(ImageProc::Left_BF.Slope) + my_abs(ImageProc::Right_BF.Slope) > 4){
                        Ramp.State = 4;
                    }
                break;
            }
            case 4:{
                if (ImageProc::state.cutoff > 68 ){
                    //Error::Err.limit = HEIGHT - 1;
                    ImageProc::state.range_l = 10;
                    ImageProc::state.range_r = WIDTH - 10;
                    Ramp.State = 5;
                }
                break;
            }
            case 5:{
                raw_count = 0;
                raw_count = width_anomaly_detection(HEIGHT - 20 , 40,
                        ImageProc::l_black, ImageProc::r_black, Error::ROAD_WIDTH);
                if (raw_count > 20) {
                    raw_count = 0;
                    Ramp.State = 0;
                    MenuSystem::beep_off();
                    clear();
                }
                break;
            }
            case 6:{
                raw_count = width_anomaly_detection(HEIGHT - 20 , 40,
                            ImageProc::l_black, ImageProc::r_black, Error::ROAD_WIDTH);
                if (raw_count < 20 && raw_count != 0) {
                    raw_count = 0;
                    Ramp.State = 0;
                    MenuSystem::beep_off();
                    clear();
                }
                break;
            }
        }
    }
}
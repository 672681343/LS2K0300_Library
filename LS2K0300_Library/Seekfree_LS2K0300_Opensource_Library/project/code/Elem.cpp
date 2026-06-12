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

    uint8_t isHandling = 0; // 是否正在处理元素
    uint8_t black_count = 0; // 黑色像素计数参考值为24
    float draw_slope = 0.0f;
 
    int16_t  raw_count = 0;
    uint8_t M_y = 0;

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

    }

    void clear()
    {
        MenuSystem::beep_off();
        Right_corner = {};
        Left_corner = {};
        Right_corner.Curve_Row = 0;
        Left_corner.Curve_Row = 0;
        Err::Err.mode = 0;
        Err::Err.limit = HEIGHT - 1;
        ImageProc::state.mode = 0;
        ImageProc::state.range_l = 5;
        ImageProc::state.range_r = WIDTH - 5;

        raw_count = 0;
        // roadblack_slope = 0;
        black_count = 0;
        draw_slope = 0.0f;

        Cross.State = 0;
        Left_Island.State = 0;
        Right_Island.State = 0;
    
        Zebra.State = 0;
        Zebra.y = 0;

        Right_Tilt_Cross.State = 0;
        Left_Tilt_Cross.State = 0;

        isHandling = 1; // 设置为正在处理状态
    }



    // // // //-------------------------------------------------------------------------------------------------------------------
    // // // // 函数简介: 斜元素
    // // // // 参数说明: 
    // // // // 返回参数: 
    // // // // 使用示例: 
    // // // // 备注信息: 
    // // // //-------------------------------------------------------------------------------------------------------------------
    // void tilt_cross_detection(void)
    // {
        
    //     switch (Right_Tilt_Cross.State)
    //     {
    //         case 0:
    //             {
    //                 if (isHandling == 0 && Right_Tilt_Cross.enable &&
    //                     Left_Island.State == 0 && Right_Island.State == 0 && Cross.State == 0 && Left_Tilt_Cross.State == 0 &&
    //                     Zebra.State == 0 && 
    //                     ImageProc::Left_BF.Slope < -0.2 && ImageProc::Left_BF.Var > 80 && ImageProc::Right_BF.Var < 50 &&
    //                     ImageProc::Right_BF.LostParam.sum >= 24

    //                     )
    //                 {
                        
    //                     Right_Tilt_Cross.similar_flag = 1;
    //                     M_y = 0;
    //                     Left_corner.Curve_Row = 0;
    //                     Left_corner.Curve_Row = Circular_Detection(0, 0, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 1, ImageProc::l_black);
    //                     if(Left_corner.Curve_Row>0 && ImageProc::l_black[Left_corner.Curve_Row] < WIDTH-10){
    //                         M_y = M_point(ImageProc::l_black[Left_corner.Curve_Row],WIDTH-10, ImageProc::white_line);
    //                     }

    //                     //这里可以加个标志点的范围

    //                     if(M_y !=0 && M_y > 9){
    //                         Right_Tilt_Cross.State=1;
    //                         M_y = 0;
    //                     }

    //                 }
    //                 break;
    //             }
    //         case 1:
    //             {
    //                 M_y = 0;
    //                 Left_corner.Curve_Row = Circular_Detection(0, 0, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 1, ImageProc::l_black);
    //                 if(Right_corner.Curve_Row < 18) Right_corner.Curve_Row=0;
    //                 if(Left_corner.Curve_Row && Left_corner.Curve_Row >20 && ImageProc::l_black[Left_corner.Curve_Row] < WIDTH-10 ){
    //                     M_y = M_point(ImageProc::l_black[Left_corner.Curve_Row],WIDTH-10, ImageProc::white_line);
    //                     if(M_x < WIDTH-22 && M_x > WIDTH-71 ){
    //                         ImageProc::state.set_start_point = M_x + 20 ;
    //                     }
    //                 }else{
    //                     M_y =  M_point(10,WIDTH-10, ImageProc::white_line);
    //                     if(M_x < WIDTH-22 && M_x > WIDTH-71 ){
    //                         ImageProc::state.set_start_point = M_x + 20 ;
    //                     }

    //                 }   
                                       
    //                 if(Left_corner.Curve_Row && M_y!=0 && ImageProc::l_black[M_y] > ImageProc::l_black[Left_corner.Curve_Row ] ){
    //                     if(Left_corner.Curve_Row > M_y){
    //                         my_draw_line(ImageProc::l_black[M_y], M_y, ImageProc::l_black[Left_corner.Curve_Row],Left_corner.Curve_Row, MODE_TWO_POINTS, DIRECTION_DOWN_LEFT, 0);
    //                     }else {
    //                         Left_corner.Curve_Row=0;
    //                          my_draw_line(ImageProc::l_black[M_y], M_y,  ImageProc::l_black[HEIGHT-5], HEIGHT-1, MODE_TWO_POINTS, DIRECTION_UP_LEFT, 0);
    //                     }
                        
    //                 }else if(Left_corner.Curve_Row ){
    //                     my_draw_line(ImageProc::l_black[Left_corner.Curve_Row + 1], Left_corner.Curve_Row + 1, 0, 0, MODE_AUTO_SLOPE, DIRECTION_DOWN_LEFT, 0);
    //                 }else if(M_y!=0){
    //                     my_draw_line(ImageProc::l_black[M_y], M_y, 0, 0, MODE_AUTO_SLOPE, DIRECTION_UP_LEFT, 0);
    //                 }

    //                 if(ImageProc::Mid_BF.LostParam.sum > 15 || ((ImageProc::Right_BF.Slope<1 && ImageProc::Right_BF.Slope>0)&&(ImageProc::Left_BF.Slope>-1 && ImageProc::Left_BF.Slope<0))){
    //                     Cross.State = 2;
    //                     Right_Tilt_Cross.State = 0;
    //                     ImageProc::state.set_start_point = 0;
    //                     M_y = 0;
    //                 }
                    


    //                 break;
    //             }

    //     }
    //     switch (Left_Tilt_Cross.State)
    //     {
    //         case 0:
    //             {
    //                 if (isHandling == 0 && Left_Tilt_Cross.enable &&
    //                     Left_Island.State == 0 && Right_Island.State == 0 && Cross.State == 0 && Left_Tilt_Cross.State == 0 &&
    //                     Zebra.State == 0 && 
    //                     ImageProc::Right_BF.Slope > 0.5 && ImageProc::Right_BF.Var > 80 && ImageProc::Left_BF.Var < 30 &&
    //                     ImageProc::Left_BF.LostParam.sum >= 24
    //                     )
    //                 {
                        
    //                     Left_Tilt_Cross.similar_flag = 1;
    //                     M_y = 0;
    //                     Right_corner.Curve_Row = 0;
    //                     Right_corner.Curve_Row = Circular_Detection(1, 0, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 1, ImageProc::r_black);
    //                     if(Right_corner.Curve_Row>0 && ImageProc::r_black[Right_corner.Curve_Row] > 10){
    //                         M_y = M_point(10,ImageProc::r_black[Right_corner.Curve_Row], ImageProc::white_line);
    //                     }else if(Right_corner.Curve_Row == 0){
    //                         M_y = M_point(10,ImageProc::state.start_point,ImageProc::white_line);
    //                     }

                       
    //                     //这里可以加个标志点的范围

    //                     if(M_y != 0 && M_y > 9){
    //                         Left_Tilt_Cross.State=1;
    //                         M_y = 0;
    //                     }

    //                 }
    //                 break;
    //             }
    //         case 1:
    //             {
    //                 M_y = 0;
    //                 Right_corner.Curve_Row = Circular_Detection(1, 0, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 1, ImageProc::r_black);
    //                 if(Right_corner.Curve_Row < 18) Right_corner.Curve_Row=0;
    //                 if(Right_corner.Curve_Row && Right_corner.Curve_Row >20 && ImageProc::r_black[Right_corner.Curve_Row] > 10 ){
    //                     M_y =  M_point(10,ImageProc::r_black[Right_corner.Curve_Row], ImageProc::white_line);
    //                     if(M_x > 21 && M_x < 70 ){
    //                         ImageProc::state.set_start_point = M_x - 20 ;
    //                     }
    //                 }else{
    //                     M_y =  M_point(10,WIDTH-10, ImageProc::white_line);
    //                     if(M_x > 21 && M_x < 70 ){
    //                         ImageProc::state.set_start_point = M_x - 20 ;
    //                     }
                        
    //                 }                  
    //                 if(Right_corner.Curve_Row && M_y!=0 && ImageProc::r_black[M_y] < ImageProc::r_black[Right_corner.Curve_Row] ){
    //                     if(Right_corner.Curve_Row > M_y ){
    //                         my_draw_line(ImageProc::r_black[M_y], M_y , ImageProc::r_black[Right_corner.Curve_Row + 2],Right_corner.Curve_Row + 2, MODE_TWO_POINTS, DIRECTION_DOWN_RIGHT, 0);
    //                     }else{
    //                         Right_corner.Curve_Row=0;
    //                         my_draw_line(ImageProc::r_black[M_y], M_y, ImageProc::r_black[HEIGHT-5], HEIGHT-1, MODE_TWO_POINTS, DIRECTION_UP_RIGHT, 0);
    //                     }
                        
    //                 }else if(Right_corner.Curve_Row ){
    //                     my_draw_line(ImageProc::r_black[Right_corner.Curve_Row + 1], Right_corner.Curve_Row + 1, 0, 0, MODE_AUTO_SLOPE, DIRECTION_DOWN_RIGHT, 0);
    //                 }else if(M_y!=0){

    //                     my_draw_line(ImageProc::r_black[M_y], M_y, 0, 0, MODE_AUTO_SLOPE, DIRECTION_UP_RIGHT, 0);
    //                 }

    //                 if(ImageProc::Mid_BF.LostParam.sum > 15 ||((ImageProc::Right_BF.Slope<0.9 && ImageProc::Right_BF.Slope>0)&&(ImageProc::Left_BF.Slope>-1 && ImageProc::Left_BF.Slope<0))){
    //                     Cross.State = 2;
    //                     Left_Tilt_Cross.State = 0;
    //                     M_y = 0;
    //                     ImageProc::state.set_start_point = 0 ;
    //                 }


                    
    //                 break;
    //             }

    //     }


    // }


    
    // //-------------------------------------------------------------------------------------------------------------------
    // // 函数简介: 十字补线函数(内部函数)
    // // 参数说明: 
    // // 返回参数: 
    // // 使用示例: 
    // // 备注信息: 
    // //-------------------------------------------------------------------------------------------------------------------
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

        //左无
        else{

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

        //右无
        else{

        }

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
                        Zebra.State == 0 &&
                        ImageProc::state.start_point < 60 && 
                        ImageProc::Left_BF.LostParam.sum >= 15 && ((ImageProc::Right_BF.LostParam.sum < 15 && ImageProc::Right_BF.LostParam.start > 67 ) || ImageProc::Right_BF.LostParam.sum < 2) &&
                         ImageProc::Right_BF.Slope > 1.2)
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
                    Left_corner.U_y  = cross_Up(ImageProc::Left_BF.LostParam.mid,  HEIGHT - ImageProc::state.cutoff , 0, ImageProc::l_black);
                    Right_corner.U_y = cross_Up(ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff , 1, ImageProc::r_black);
                    Left_corner.D_y  = cross_Down(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff , 0, ImageProc::l_black);
                    Right_corner.D_y = cross_Down(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff , 1, ImageProc::r_black);

                    Right_corner.Curve_Row = Circular_Detection(1, 0, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 1, ImageProc::r_black);
                    
                    if (Right_corner.Curve_Row > 22 && Right_corner.Curve_Row < HEIGHT - 10)
                    {
                        my_draw_line(ImageProc::r_black[Right_corner.Curve_Row], Right_corner.Curve_Row, 0, 0, MODE_AUTO_SLOPE, DIRECTION_DOWN_RIGHT, 0);
                    } else if(!Right_corner.D_y||!Left_corner.D_y||!Right_corner.U_y||!Left_corner.U_y){
                        if (Left_corner.U_y < 10 ) { Left_corner.U_y = 0;}//
                        if (Right_corner.U_y < 10 ) { Right_corner.U_y = 0;}//
                        if (Left_corner.U_y > Left_corner.D_y ) { Left_corner.D_y = 0;}
                        if (Right_corner.U_y > Right_corner.D_y ) { Right_corner.D_y = 0;}
                        if (Left_corner.D_y > HEIGHT - 5) { Left_corner.D_y = 0; }
                        if (Right_corner.D_y > HEIGHT - 5) { Right_corner.D_y = 0;}
                        Crosshairs();
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
                        Zebra.State == 0 && 
                        ImageProc::state.start_point > WIDTH - 60 && 
                        ImageProc::Right_BF.LostParam.sum >= 15 && ((ImageProc::Left_BF.LostParam.sum < 15 && ImageProc::Left_BF.LostParam.start > 67) || ImageProc::Left_BF.LostParam.sum < 2)&&
                        ImageProc::Left_BF.Slope < -1.2)
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
                    Left_corner.U_y  = cross_Up(ImageProc::Left_BF.LostParam.mid,  HEIGHT - ImageProc::state.cutoff , 0, ImageProc::l_black);
                    Right_corner.U_y = cross_Up(ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff , 1, ImageProc::r_black);
                    Left_corner.D_y  = cross_Down(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff , 0, ImageProc::l_black);
                    Right_corner.D_y = cross_Down(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff , 1, ImageProc::r_black);

                    Left_corner.Curve_Row = Circular_Detection(0, 0, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 1, ImageProc::l_black);
                    if (ImageProc::Right_BF.LostParam.sum < 2){
                        clear();
                        Right_Tilt_Cross.State = 0;
                    }
                    if (Left_corner.Curve_Row > 22 && Left_corner.Curve_Row < HEIGHT - 10)
                    {
                        my_draw_line(ImageProc::l_black[Left_corner.Curve_Row], Left_corner.Curve_Row, 0, 0, MODE_AUTO_SLOPE, DIRECTION_DOWN_LEFT, 0);
                    } else if(!Right_corner.D_y||!Left_corner.D_y||!Right_corner.U_y||!Left_corner.U_y){
                        if (Left_corner.U_y < 10 ) { Left_corner.U_y = 0;}//
                        if (Right_corner.U_y < 10 ) { Right_corner.U_y = 0;}//
                        if (Left_corner.U_y > Left_corner.D_y ) { Left_corner.D_y = 0;}
                        if (Right_corner.U_y > Right_corner.D_y ) { Right_corner.D_y = 0;}
                        if (Left_corner.D_y > HEIGHT - 5) { Left_corner.D_y = 0; }
                        if (Right_corner.D_y > HEIGHT - 5) { Right_corner.D_y = 0;}
                        Crosshairs();
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
                    Zebra.State == 0 && 
                    
                    (( ImageProc::Mid_BF.LostParam.sum > 1 || ImageProc::sec_loss_both.LostParam.sum > 1) &&  
                    ImageProc::Left_BF.LostParam.sum < 35 && ImageProc::Right_BF.LostParam.sum < 35) && //
                    (ImageProc::Left_BF.Var > 100 || ImageProc::Right_BF.Var > 100) )
                {
                    Left_corner.U_y  = cross_Up(ImageProc::Left_BF.LostParam.mid,  HEIGHT - ImageProc::state.cutoff , 0, ImageProc::l_black);
                    Right_corner.U_y = cross_Up(ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff , 1, ImageProc::r_black);
                    Left_corner.D_y  = cross_Down(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff , 0, ImageProc::l_black);
                    Right_corner.D_y = cross_Down(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff , 1, ImageProc::r_black);


                    Cross.similar_flag = 1;
                    if (Left_corner.D_y > 15 && Left_corner.D_y < HEIGHT - 10 &&//这边只对下点进行了限幅，后续上点没更新，就会置0
                        Right_corner.D_y> 15 && Right_corner.D_y < HEIGHT - 10 
                        )
                    {
                        Cross.State = 1;
                    }

                }

                break;
            }
            case 1:{


                Left_corner.D_y  = cross_Down(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff , 0, ImageProc::l_black);
                Right_corner.D_y = cross_Down(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff , 1, ImageProc::r_black);

                if (Left_corner.D_y > 25 ){
                    Left_corner.U_y = cross_Up(Left_corner.D_y,  HEIGHT - ImageProc::state.cutoff , 0, ImageProc::l_black);
                }else{
                    Left_corner.U_y = cross_Up(HEIGHT - 20,  HEIGHT - ImageProc::state.cutoff , 0, ImageProc::l_black);
                }
                
                if (Right_corner.D_y > 25 ){
                    Right_corner.U_y = cross_Up(Right_corner.D_y, HEIGHT - ImageProc::state.cutoff , 1, ImageProc::r_black);
                }else{
                    Right_corner.U_y = cross_Up(HEIGHT - 20, HEIGHT - ImageProc::state.cutoff , 1, ImageProc::r_black);
                }


                if (Left_corner.U_y < 10 ) { Left_corner.U_y = 0;}//
                if (Right_corner.U_y < 10 ) { Right_corner.U_y = 0;}//
                if (Left_corner.U_y > Left_corner.D_y ) { Left_corner.D_y = 0;}
                if (Right_corner.U_y > Right_corner.D_y ) { Right_corner.D_y = 0;}
                if (Left_corner.D_y > HEIGHT - 5) { Left_corner.D_y = 0; }
                if (Right_corner.D_y > HEIGHT - 5) { Right_corner.D_y = 0;}
                Crosshairs();



                if (ImageProc::Mid_BF.LostParam.sum > 20 ){
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
                Left_corner.U_y  = cross_Up(ImageProc::Left_BF.LostParam.mid + 5,  ImageProc::Left_BF.LostParam.end - 15, 0, ImageProc::l_black);
                Right_corner.U_y = cross_Up(ImageProc::Right_BF.LostParam.mid + 5, ImageProc::Right_BF.LostParam.end - 15, 1, ImageProc::r_black);

                if (Left_corner.U_y < ImageProc::Left_BF.LostParam.end - 20) { Left_corner.U_y = 0; }
                if (Right_corner.U_y < ImageProc::Right_BF.LostParam.end - 20) { Right_corner.U_y = 0; }


                if(!Left_corner.U_y){
                    cross_Down(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff , 0, ImageProc::l_black);
                }
                if(!Right_corner.U_y){
                    cross_Down(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff , 1, ImageProc::r_black);
                }


                if (Left_corner.U_y > Left_corner.D_y ) { Left_corner.D_y = 0; }
                if (Right_corner.U_y > Right_corner.D_y ) { Right_corner.D_y = 0; }
                if (Left_corner.D_y > HEIGHT - 5) { Left_corner.D_y = 0; }
                if (Right_corner.D_y > HEIGHT - 5) { Right_corner.D_y = 0; }
                Crosshairs();
                break;
            }
            case 3:{
                
                if (ImageProc::Mid_BF.LostParam.sum < 4 || ImageProc::Left_BF.LostParam.start < 30 || ImageProc::Right_BF.LostParam.start < 30){
                    Cross.State = 0;
                    Err::Err.mode = 0;
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
   
    // //-------------------------------------------------------------------------------------------------------------------
    // // 函数简介: 环岛
    // // 参数说明: 
    // // 返回参数: 
    // // 使用示例: 
    // // 备注信息: 
    // //-------------------------------------------------------------------------------------------------------------------

    void island_detection(){
        
        //左环
        switch(Left_Island.State){
            //判断圆环
            case 0:{
                if(  Right_Tilt_Cross.State==0 && Left_Tilt_Cross.State==0 && Left_Island.enable &&
                    Right_Island.State==0 && Cross.State == 0 &&
                    Zebra.State == 0 && 
                    ImageProc::state.cutoff >= 54 &&  
                    ((ImageProc::Right_BF.LostParam.start > 67 && ImageProc::Right_BF.LostParam.sum < 15) ||
                     ImageProc::Right_BF.LostParam.sum < 2) && 
                    ((ImageProc::Left_BF.LostParam.sum > 15 || ImageProc::sec_loss_left.LostParam.sum > 6)&& (ImageProc::Left_BF.LostParam.start < 50 || ImageProc::sec_loss_left.LostParam.start<50 ) ) && 
                    (ImageProc::Mid_BF.LostParam.sum < 2  )&& 
                    ImageProc::Right_BF.Var < 10 && ImageProc::Left_BF.Var > 100 ){
                    Left_corner.D_y = island_Down(HEIGHT - 4, HEIGHT - ImageProc::state.cutoff , 0, ImageProc::l_black);
                    Left_Island.similar_flag = 1;
                    Left_corner.Curve_Row = Circular_Detection(0, 0, ImageProc::Left_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::l_black);
                    
                    if(Left_corner.D_y < 28) Left_corner.D_y = 0;

                    if(Left_corner.Curve_Row != 0 && Left_corner.D_y != 0){
                        
                        if(Left_corner.D_y != 0 && Left_corner.Curve_Row != 0){
                            my_draw_line(ImageProc::l_black[Left_corner.D_y],Left_corner.D_y,ImageProc::l_black[Left_corner.Curve_Row],Left_corner.Curve_Row,MODE_TWO_POINTS,DIRECTION_DOWN_LEFT,0);  
                        }else{
                            my_draw_line(ImageProc::l_black[Left_corner.Curve_Row],Left_corner.Curve_Row,0,0,MODE_FIXED_SLOPE,DIRECTION_UP_LEFT,-ImageProc::Right_BF.Slope);
                        }
                        Left_Island.State = 1;
                    }
                    else if(Left_corner.Curve_Row != 0 && ImageProc::Left_BF.LostParam.sum > 20 && ImageProc::Left_BF.LostParam.start == 69){
                        Left_Island.State = 2;
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
                        Left_Island.State = 2;
                    }
                }


    

                break;
            }

            //靠近补线（下拐点，突变点）
            case 1:{
                Left_corner.D_y = island_Down(HEIGHT - 4, HEIGHT - ImageProc::state.cutoff + 2, 0, ImageProc::l_black);
                Left_corner.Curve_Row = Circular_Detection(0, 0, ImageProc::Left_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::l_black);
                if((Left_corner.D_y < Left_corner.Curve_Row + 10 && Left_corner.Curve_Row != 0) || Left_corner.D_y < ImageProc::Left_BF.LostParam.mid ) Left_corner.D_y = 0;
                if(Left_corner.D_y != 0 && Left_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.D_y],Left_corner.D_y,ImageProc::l_black[Left_corner.Curve_Row],Left_corner.Curve_Row,MODE_TWO_POINTS,DIRECTION_DOWN_LEFT,0);
                    Err::Err.mode = 0;  
                }else if(Left_corner.D_y != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.D_y],Left_corner.D_y,0,0,MODE_AUTO_SLOPE,DIRECTION_DOWN_LEFT,0);                   
                    Err::Err.mode = 0;  
                }else if(Left_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.Curve_Row],Left_corner.Curve_Row,0,0,MODE_FIXED_SLOPE,DIRECTION_UP_LEFT,-ImageProc::Right_BF.Slope);
                }
                else{
                    Err::Err.mode = 1; 
                }

                
                if (ImageProc::Left_BF.LostParam.sum > 24 && ImageProc::Left_BF.LostParam.start > 67){
                    Left_Island.State = 2;
                    Err::Err.mode = 0;

                }

                break;
            }
            //靠近补线（突变点，上拐点）
            case 2:{
                static uint8_t state_count = 0;
                Left_corner.Curve_Row = Circular_Detection(0, 0, HEIGHT - 10, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::l_black);
                Left_corner.U_y = island_Up(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff , 0, ImageProc::l_black);
                if(Left_corner.Curve_Row < 15) Left_corner.Curve_Row = 0;
                if(ImageProc::Left_BF.LostParam.mid > Left_corner.Curve_Row &&
                     ImageProc::Left_BF.LostParam.start < 20){
                    Left_corner.Curve_Row = 0;
                }
                if(Left_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.Curve_Row],Left_corner.Curve_Row,0,0,MODE_FIXED_SLOPE,DIRECTION_UP_LEFT,-ImageProc::Right_BF.Slope);
                }


                // else if(Left_corner.U_y != 0){
                //     my_draw_line(ImageProc::l_black[Left_corner.U_y],Left_corner.U_y,ImageProc::r_black[HEIGHT-10],HEIGHT-10,MODE_TWO_POINTS,DIRECTION_DOWN_RIGHT,0);
                // }

                if (Left_corner.Curve_Row != 0 && Left_corner.Curve_Row > 20){
                    state_count++;
                }
                if((Left_corner.Curve_Row == 0 || Left_corner.Curve_Row > 40)&& state_count > 4){
                    Left_Island.State=3;
                    state_count = 0;
                }
                break;
            }
            //准备入环（上拐点）
            case 3:{
                Left_corner.U_y = island_Up(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff + 2 , 0, ImageProc::l_black);
                if(Left_corner.U_y != 0 ){
                    Err::Err.limit = HEIGHT - Left_corner.U_y;
                    my_draw_line(ImageProc::l_black[Left_corner.U_y],Left_corner.U_y,ImageProc::r_black[HEIGHT-10],HEIGHT-10,MODE_TWO_POINTS,DIRECTION_DOWN_RIGHT,0);
                }
                if (ImageProc::Right_BF.LostParam.sum > 25 ){
                    Left_Island.State = 4;
                    ImageProc::state.range_r = ImageProc::l_black[Left_corner.U_y - 2];
                    Right_corner.U_y = Left_corner.U_y;
                }
                break;
            }
            //入环（左上拐点变右上拐点）
            case 4:{
            
                Right_corner.U_y = island_Up(ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 2, 1 ,ImageProc::r_black);
                if (Right_corner.U_y == 0 && (Left_corner.U_y > (HEIGHT - ImageProc::state.cutoff))) { Right_corner.U_y = Left_corner.U_y; }
                if(Right_corner.U_y != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.U_y-1],Right_corner.U_y-1,ImageProc::r_black[HEIGHT-5],HEIGHT-5,MODE_TWO_POINTS,DIRECTION_UP_RIGHT,0);
                }
                if (ImageProc::Right_BF.LostParam.sum < 10 ){
                    Left_Island.State = 5;

                    Err::Err.limit = HEIGHT - 1; 
                }
                break;
            }
            //入环成功，找出环标志（有突变点）
            case 5:{
                Right_corner.D_y = island_Down(HEIGHT - 1 , ImageProc::Right_BF.LostParam.mid,  1 ,ImageProc::r_black);
                Right_corner.Curve_Row = Circular_Detection(1, 0, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::r_black);
                if (Right_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.Curve_Row + 4], Right_corner.Curve_Row + 4, 5, 20, MODE_TWO_POINTS, DIRECTION_DOWN_RIGHT, 0);
                }else if(Right_corner.D_y != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.D_y],Right_corner.D_y,5,20,MODE_TWO_POINTS,DIRECTION_DOWN_RIGHT,0);
                }
                if (Right_corner.Curve_Row > 35 && ImageProc::Right_BF.LostParam.sum > 2 && ImageProc::Right_BF.LostParam.start < 65){
                    ImageProc::state.range_r = WIDTH - 10;
                    Left_Island.State = 6;
                }else if (Right_corner.Curve_Row == 0 && ImageProc::Right_BF.LostParam.sum > 20 && ImageProc::Right_BF.LostParam.start > 50 && ImageProc::Left_BF.LostParam.start < 58 && ImageProc::Mid_BF.LostParam.sum > 10){
                    Left_Island.State = 6;
                    ImageProc::state.range_r = WIDTH - 10;
                }
                break;
            }
            //出环标志（无突变点）
            case 6:{
                Left_corner.U_y = 0;
                my_draw_line(ImageProc::r_black[HEIGHT-10],HEIGHT-10,5,20,MODE_TWO_POINTS,DIRECTION_DOWN_RIGHT,0);
                Left_corner.U_y = island_Up(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff + 2 , 0, ImageProc::l_black);
                if((ImageProc::Right_BF.LostParam.sum < 15  && ImageProc::Right_BF.Var < 150 && ImageProc::Right_BF.LostParam.end > 40) || Left_corner.U_y ){
                    Left_Island.State = 7;
                }
                break;           
            }
            case 7:{
                Left_corner.U_y = island_Up(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff , 0, ImageProc::l_black);
                Left_corner.Curve_Row = Circular_Detection(0, 0, ImageProc::Left_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::l_black);
                if(Left_corner.Curve_Row < 25) Left_corner.Curve_Row = 0;
                if(Left_corner.U_y !=0 && Left_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.U_y],Left_corner.U_y,ImageProc::l_black[HEIGHT-1],HEIGHT-1,MODE_TWO_POINTS,DIRECTION_DOWN_LEFT,0);
                }else if(Left_corner.U_y !=0){
                    my_draw_line(ImageProc::l_black[Left_corner.U_y],Left_corner.U_y,0,0,MODE_AUTO_SLOPE,DIRECTION_UP_LEFT,0);
                }else if(Left_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.Curve_Row],Left_corner.Curve_Row,0,0,MODE_FIXED_SLOPE,DIRECTION_DOWN_LEFT,-ImageProc::Right_BF.Slope);
                }
                if((ImageProc::Left_BF.LostParam.end > 48  ) && (Left_corner.Curve_Row == 0 || Left_corner.Curve_Row < 30)){
                    Left_Island.State = 8;
                }
                break;
            }
            case 8:{
                Left_corner.U_y = island_Up(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff , 0, ImageProc::l_black);
                if(Left_corner.U_y < 15) Left_corner.U_y = 0;
                if(Left_corner.U_y !=0){
                    my_draw_line(ImageProc::l_black[Left_corner.U_y],Left_corner.U_y,0,0,MODE_AUTO_SLOPE,DIRECTION_UP_LEFT,0);
                }

                if((ImageProc::Mid_BF.LostParam.sum < 2 && ImageProc::Left_BF.Var< 100 && ImageProc::Right_BF.Var< 100)|| (ImageProc::Left_BF.LostParam.start < 30 && ImageProc::Left_BF.LostParam.start != 0 )|| (ImageProc::Right_BF.LostParam.start < 30 && ImageProc::Right_BF.LostParam.start != 0)){
                    Left_Island.State = 0;
                    clear();
                    break;
                }
            }
            

        }


        //右环
        switch(Right_Island.State){
            //判断圆环
            case 0:{
                if(Right_Tilt_Cross.State==0 && Left_Tilt_Cross.State==0 && Right_Island.enable &&
                    Left_Island.State == 0 && Cross.State == 0  && Zebra.State == 0 && 
                    ImageProc::state.cutoff >= 54 && 
                    ((ImageProc::Left_BF.LostParam.start > 67 && ImageProc::Left_BF.LostParam.sum < 15) ||
                     ImageProc::Left_BF.LostParam.sum < 2) && 
                    ((ImageProc::Right_BF.LostParam.sum > 15 || ImageProc::sec_loss_right.LostParam.sum > 6) && (ImageProc::Right_BF.LostParam.start < 50 || ImageProc::sec_loss_right.LostParam.start < 50)) && 
                    (ImageProc::Mid_BF.LostParam.sum < 2 )&& 
                    ImageProc::Right_BF.Var > 100 && ImageProc::Left_BF.Var < 10 
                   ){
                    Right_corner.D_y = island_Down(HEIGHT - 4, HEIGHT - ImageProc::state.cutoff , 0, ImageProc::r_black);
                    Right_Island.similar_flag = 1;
                    Right_corner.Curve_Row = Circular_Detection(1, 0, ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::r_black);
                    
                    
                    if(Right_corner.Curve_Row != 0 && Right_corner.D_y != 0){

                        if(Right_corner.D_y != 0 && Right_corner.Curve_Row != 0){
                            my_draw_line(ImageProc::r_black[Right_corner.D_y],Right_corner.D_y,ImageProc::r_black[Right_corner.Curve_Row],Right_corner.Curve_Row,MODE_TWO_POINTS,DIRECTION_DOWN_RIGHT,0);
                        }else {
                            my_draw_line(ImageProc::r_black[Right_corner.Curve_Row],Right_corner.Curve_Row,0,0,MODE_FIXED_SLOPE,DIRECTION_UP_RIGHT,-ImageProc::Left_BF.Slope);
                        }
                        Right_Island.State = 1;
                        
                    }
                    else if(Right_corner.Curve_Row != 0 && ImageProc::Right_BF.LostParam.sum > 20 && ImageProc::Right_BF.LostParam.start == 69){
                        Left_Island.State = 2;
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
                    if(Left_corner.Curve_Row < 28) Left_corner.Curve_Row = 0;
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

            //靠近补线（下拐点，突变点）
            case 1:{
                Right_corner.D_y = island_Down(HEIGHT - 4, HEIGHT - ImageProc::state.cutoff + 2, 1, ImageProc::r_black);
                Right_corner.Curve_Row = Circular_Detection(1, 0, ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::r_black);
                if((Right_corner.D_y < Right_corner.Curve_Row + 10 && Right_corner.Curve_Row != 0) || Right_corner.D_y < ImageProc::Right_BF.LostParam.mid ) Right_corner.D_y = 0;
                if(Right_corner.D_y != 0 && Right_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.D_y],Right_corner.D_y,ImageProc::r_black[Right_corner.Curve_Row],Right_corner.Curve_Row,MODE_TWO_POINTS,DIRECTION_DOWN_RIGHT,0);
                    Err::Err.mode = 0; 
                }else if(Right_corner.D_y != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.D_y],Right_corner.D_y,0,0,MODE_AUTO_SLOPE,DIRECTION_DOWN_RIGHT,0);
                   Err::Err.mode = 0; 
                }else if(Right_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.Curve_Row],Right_corner.Curve_Row,0,0,MODE_FIXED_SLOPE,DIRECTION_UP_RIGHT,-ImageProc::Left_BF.Slope);
                }else{
                    Err::Err.mode = -1; 
                }

                
                if (ImageProc::Right_BF.LostParam.sum > 24 && ImageProc::Right_BF.LostParam.start > 67){
                    Right_Island.State = 2;
                    Err::Err.mode = 0;

                }

                break;
            }
            //靠近补线（突变点，上拐点）
            case 2:{
                static uint8_t state_count = 0;
                Right_corner.Curve_Row = Circular_Detection(1, 0, HEIGHT - 10, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::r_black);
                Right_corner.U_y = island_Up(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff , 1, ImageProc::r_black);
                if(Right_corner.Curve_Row < 15) Right_corner.Curve_Row = 0;
                if(ImageProc::Right_BF.LostParam.mid > Right_corner.Curve_Row &&
                     ImageProc::Right_BF.LostParam.start < 20 ){
                    Right_corner.Curve_Row = 0;
                }
                if(Right_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.Curve_Row],Right_corner.Curve_Row,0,0,MODE_FIXED_SLOPE,DIRECTION_UP_RIGHT,-ImageProc::Left_BF.Slope);
                }

                // else if(Right_corner.U_y != 0){
                //     my_draw_line(ImageProc::r_black[Right_corner.U_y],Right_corner.U_y,ImageProc::l_black[HEIGHT-10],HEIGHT-10,MODE_TWO_POINTS,DIRECTION_DOWN_LEFT,0);
                // }

                if (Right_corner.Curve_Row != 0 && Right_corner.Curve_Row > 20){
                    state_count++;
                }

                if((Right_corner.Curve_Row == 0 || Right_corner.Curve_Row > 40)&& state_count > 4){
                    Right_Island.State=3;
                    state_count = 0;
                }
                break;
            }
            //准备入环（上拐点）
            case 3:{
                Right_corner.U_y = island_Up(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff + 2 , 1, ImageProc::r_black);
                if(Right_corner.U_y != 0 ){
                    Err::Err.limit = HEIGHT - Right_corner.U_y;

                    my_draw_line(ImageProc::r_black[Right_corner.U_y],Right_corner.U_y,ImageProc::l_black[HEIGHT-15],HEIGHT-15,MODE_TWO_POINTS,DIRECTION_DOWN_LEFT,0);
                    
                }

                if (ImageProc::Left_BF.LostParam.sum > 25 ){
                    Right_Island.State = 4;
                    ImageProc::state.range_l = ImageProc::r_black[Right_corner.U_y - 2];
                    Left_corner.U_y = Right_corner.U_y;
                }
                break;
            }
            //入环（左上拐点变右上拐点）
            case 4:{
                
                Left_corner.U_y = island_Up(ImageProc::Left_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 2, 0 ,ImageProc::l_black);
                if (Left_corner.U_y == 0 && (Right_corner.U_y > (HEIGHT - ImageProc::state.cutoff))) { Left_corner.U_y = Right_corner.U_y; }
                if(Left_corner.U_y != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.U_y-1],Left_corner.U_y-1,ImageProc::l_black[HEIGHT-5],HEIGHT-5,MODE_TWO_POINTS,DIRECTION_UP_LEFT,0);
                }

                if (ImageProc::Left_BF.LostParam.sum < 10 ){
                    Right_Island.State = 5;

                    Err::Err.limit = HEIGHT - 1; 
                }
                break;  //48s
            }
            //入环成功，找出环标志（有突变点）
            case 5:{
                Left_corner.D_y = island_Down(HEIGHT - 1,ImageProc::Left_BF.LostParam.mid, 0 ,ImageProc::l_black);
                Left_corner.Curve_Row = Circular_Detection(0, 1, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::l_black);
                if (Left_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.Curve_Row + 4], Left_corner.Curve_Row + 4, WIDTH-5, 20, MODE_TWO_POINTS, DIRECTION_DOWN_LEFT, 0);
                }else if(Left_corner.D_y != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.D_y], Left_corner.D_y, WIDTH-5, 20, MODE_TWO_POINTS, DIRECTION_DOWN_LEFT, 0);
                }
                if (Left_corner.Curve_Row > 35 && ImageProc::Left_BF.LostParam.sum > 2 && ImageProc::Left_BF.LostParam.start < 65  ){
                    Right_Island.State = 6;
                    ImageProc::state.range_l = 10;
                }else if (Left_corner.Curve_Row == 0 && ImageProc::Left_BF.LostParam.sum > 20 && ImageProc::Left_BF.LostParam.start > 50 && ImageProc::Right_BF.LostParam.start < 58 && ImageProc::Mid_BF.LostParam.sum > 10 ){
                    Right_Island.State = 6;
                    ImageProc::state.range_l = 10;
                }
                break;
            }
            //出环标志（无突变点）
            case 6:{
                Right_corner.U_y = 0;
                my_draw_line(ImageProc::l_black[HEIGHT-5],HEIGHT-5,WIDTH-5,20,MODE_TWO_POINTS,DIRECTION_DOWN_LEFT,0);
                Right_corner.U_y = island_Up(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff , 1, ImageProc::r_black);
                if((ImageProc::Left_BF.LostParam.sum < 15  && ImageProc::Left_BF.Slope < -1 && ImageProc::Left_BF.LostParam.end > 40 )|| Right_corner.U_y ){
                    Right_Island.State = 7;
                }
                break;             
            }
            case 7:{
                Right_corner.U_y = island_Up(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff , 1, ImageProc::r_black);
                Right_corner.Curve_Row = Circular_Detection(0, 0, HEIGHT-1, Right_corner.U_y, ImageProc::r_black);
                if(Right_corner.Curve_Row < 25) Right_corner.Curve_Row = 0;
                if(Right_corner.U_y >= Right_corner.Curve_Row)  Right_corner.Curve_Row=0;
                if(Right_corner.U_y !=0 && Right_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.U_y],Right_corner.U_y,ImageProc::r_black[HEIGHT-1],HEIGHT-1,MODE_TWO_POINTS,DIRECTION_DOWN_RIGHT,0);
                 }else if(Right_corner.U_y !=0){
                    my_draw_line(ImageProc::r_black[Right_corner.U_y],Right_corner.U_y,0,0,MODE_AUTO_SLOPE,DIRECTION_UP_RIGHT,0);
                }else if(Right_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.Curve_Row],Right_corner.Curve_Row,0,0,MODE_FIXED_SLOPE,DIRECTION_DOWN_RIGHT,-ImageProc::Left_BF.Slope);
                }
                if((ImageProc::Right_BF.LostParam.end > 48 ) && (Right_corner.Curve_Row == 0 || Right_corner.Curve_Row < 30)){
                    Right_Island.State = 8;
                }
                break;
            }



            case 8:{
                Right_corner.U_y = island_sideUp(ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 2, 1 ,ImageProc::r_black);
                if(Right_corner.U_y < 15) Right_corner.U_y = 0;
                if(Right_corner.U_y !=0){
                    my_draw_line(ImageProc::r_black[Right_corner.U_y],Right_corner.U_y,0,0,MODE_AUTO_SLOPE,DIRECTION_UP_RIGHT,0);
                }

                if((ImageProc::Mid_BF.LostParam.sum < 2 && ImageProc::Right_BF.Var< 100 && ImageProc::Left_BF.Var< 100) || (ImageProc::Left_BF.LostParam.start < 30 && ImageProc::Left_BF.LostParam.start != 0 )|| (ImageProc::Right_BF.LostParam.start < 30 && ImageProc::Right_BF.LostParam.start != 0)){
                    Right_Island.State = 0;
                    clear();
                    break;
                }
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
    

}
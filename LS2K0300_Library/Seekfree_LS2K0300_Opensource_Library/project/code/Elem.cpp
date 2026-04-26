#include "Elem.h"

namespace Elem {
    _Corner Right_corner = {};
    _Corner Left_corner = {};

    _Island Right_Island = {};
    _Island Left_Island = {};

    _Tilt Right_Tilt_Cross = {};
    _Tilt Left_Tilt_Cross = {};

    _Cross Cross = {};


    uint8_t isHandling = 0; // 是否正在处理元素
 

    uint8_t M_y = 0;

    void elem_judge()
    {


        Right_Tilt_Cross.similar_flag = 0;
        Left_Tilt_Cross.similar_flag = 0;
        
        Right_corner.Curve_Row = 0;
        Left_corner.Curve_Row = 0;

        Left_Island.similar_flag = 0;
        Right_Island.similar_flag = 0;

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
        Error::Err.mode = 0;
        Error::Err.limit = HEIGHT - 1;
        ImageProc::state.mode = 0;
        ImageProc::state.range_l = 5;
        ImageProc::state.range_r = WIDTH - 5;


        Cross.State = 0;
        Left_Island.State = 0;
        Right_Island.State = 0;

        Right_Tilt_Cross.State = 0;
        Left_Tilt_Cross.State = 0;

        isHandling = 1; // 设置为正在处理状态
    }

    // // //-------------------------------------------------------------------------------------------------------------------
    // // // 函数简介: 斜元素
    // // // 参数说明: 
    // // // 返回参数: 
    // // // 使用示例: 
    // // // 备注信息: 
    // // //-------------------------------------------------------------------------------------------------------------------
    void tilt_cross_detection(void)
    {
        
        switch (Right_Tilt_Cross.State)
        {
            case 0:
                {
                    if (isHandling == 0 && Right_Tilt_Cross.enable &&
                        Left_Island.State == 0 && Right_Island.State == 0 && Cross.State == 0 && Left_Tilt_Cross.State == 0 &&
                        ImageProc::Left_BF.Slope < -0.2 && ImageProc::Left_BF.Var > 80 && ImageProc::Right_BF.Var < 50 &&
                        ImageProc::Right_BF.LostParam.sum >= 24

                        )
                    {
                        
                        Right_Tilt_Cross.similar_flag = 1;
                        M_y = 0;
                        Left_corner.Curve_Row = 0;
                        Left_corner.Curve_Row = Circular_Detection(0, 0, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 1, ImageProc::l_black);
                        if(Left_corner.Curve_Row>0 && ImageProc::l_black[Left_corner.Curve_Row] < WIDTH-10){
                            M_y = M_point(ImageProc::l_black[Left_corner.Curve_Row],WIDTH-10, ImageProc::white_line);
                        }

                        //这里可以加个标志点的范围

                        if(M_y !=0 && M_y > 9){
                            Right_Tilt_Cross.State=1;
                            M_y = 0;
                        }

                    }
                    break;
                }
            case 1:
                {
                    M_y = 0;
                    Left_corner.Curve_Row = Circular_Detection(0, 0, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 1, ImageProc::l_black);
                    if(Right_corner.Curve_Row < 18) Right_corner.Curve_Row=0;
                    if(Left_corner.Curve_Row && Left_corner.Curve_Row >20 && ImageProc::l_black[Left_corner.Curve_Row] < WIDTH-10 ){
                        M_y = M_point(ImageProc::l_black[Left_corner.Curve_Row],WIDTH-10, ImageProc::white_line);
                        if(M_x < WIDTH-22 && M_x > WIDTH-71 ){
                            ImageProc::state.set_start_point = M_x + 20 ;
                        }
                    }else{
                        M_y =  M_point(10,WIDTH-10, ImageProc::white_line);
                        if(M_x < WIDTH-22 && M_x > WIDTH-71 ){
                            ImageProc::state.set_start_point = M_x + 20 ;
                        }

                    }   
                                       
                    if(Left_corner.Curve_Row && M_y!=0 && ImageProc::l_black[M_y] > ImageProc::l_black[Left_corner.Curve_Row ] ){
                        if(Left_corner.Curve_Row > M_y){
                            my_draw_line(ImageProc::l_black[M_y], M_y, ImageProc::l_black[Left_corner.Curve_Row],Left_corner.Curve_Row, MODE_TWO_POINTS, DIRECTION_DOWN_LEFT, 0);
                        }else {
                            Left_corner.Curve_Row=0;
                             my_draw_line(ImageProc::l_black[M_y], M_y,  ImageProc::l_black[HEIGHT-5], HEIGHT-1, MODE_TWO_POINTS, DIRECTION_UP_LEFT, 0);
                        }
                        
                    }else if(Left_corner.Curve_Row ){
                        my_draw_line(ImageProc::l_black[Left_corner.Curve_Row + 1], Left_corner.Curve_Row + 1, 0, 0, MODE_AUTO_SLOPE, DIRECTION_DOWN_LEFT, 0);
                    }else if(M_y!=0){
                        my_draw_line(ImageProc::l_black[M_y], M_y, 0, 0, MODE_AUTO_SLOPE, DIRECTION_UP_LEFT, 0);
                    }

                    if(ImageProc::Mid_BF.LostParam.sum > 15 || ((ImageProc::Right_BF.Slope<1 && ImageProc::Right_BF.Slope>0)&&(ImageProc::Left_BF.Slope>-1 && ImageProc::Left_BF.Slope<0))){
                        Cross.State = 2;
                        Right_Tilt_Cross.State = 0;
                        ImageProc::state.set_start_point = 0;
                        M_y = 0;
                    }
                    


                    break;
                }

        }
        switch (Left_Tilt_Cross.State)
        {
            case 0:
                {
                    if (isHandling == 0 && Left_Tilt_Cross.enable &&
                        Left_Island.State == 0 && Right_Island.State == 0 && Cross.State == 0 && Left_Tilt_Cross.State == 0 &&
                       
                        ImageProc::Right_BF.Slope > 0.5 && ImageProc::Right_BF.Var > 80 && ImageProc::Left_BF.Var < 30 &&
                        ImageProc::Left_BF.LostParam.sum >= 24
                        )
                    {
                        
                        Left_Tilt_Cross.similar_flag = 1;
                        M_y = 0;
                        Right_corner.Curve_Row = 0;
                        Right_corner.Curve_Row = Circular_Detection(1, 0, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 1, ImageProc::r_black);
                        if(Right_corner.Curve_Row>0 && ImageProc::r_black[Right_corner.Curve_Row] > 10){
                            M_y = M_point(10,ImageProc::r_black[Right_corner.Curve_Row], ImageProc::white_line);
                        }else if(Right_corner.Curve_Row == 0){
                            M_y = M_point(10,ImageProc::state.start_point,ImageProc::white_line);
                        }

                       
                        //这里可以加个标志点的范围

                        if(M_y != 0 && M_y > 9){
                            Left_Tilt_Cross.State=1;
                            M_y = 0;
                        }

                    }
                    break;
                }
            case 1:
                {
                    M_y = 0;
                    Right_corner.Curve_Row = Circular_Detection(1, 0, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 1, ImageProc::r_black);
                    if(Right_corner.Curve_Row < 18) Right_corner.Curve_Row=0;
                    if(Right_corner.Curve_Row && Right_corner.Curve_Row >20 && ImageProc::r_black[Right_corner.Curve_Row] > 10 ){
                        M_y =  M_point(10,ImageProc::r_black[Right_corner.Curve_Row], ImageProc::white_line);
                        if(M_x > 21 && M_x < 70 ){
                            ImageProc::state.set_start_point = M_x - 20 ;
                        }
                    }else{
                        M_y =  M_point(10,WIDTH-10, ImageProc::white_line);
                        if(M_x > 21 && M_x < 70 ){
                            ImageProc::state.set_start_point = M_x - 20 ;
                        }
                        
                    }                  
                    if(Right_corner.Curve_Row && M_y!=0 && ImageProc::r_black[M_y] < ImageProc::r_black[Right_corner.Curve_Row] ){
                        if(Right_corner.Curve_Row > M_y ){
                            my_draw_line(ImageProc::r_black[M_y], M_y , ImageProc::r_black[Right_corner.Curve_Row + 2],Right_corner.Curve_Row + 2, MODE_TWO_POINTS, DIRECTION_DOWN_RIGHT, 0);
                        }else{
                            Right_corner.Curve_Row=0;
                            my_draw_line(ImageProc::r_black[M_y], M_y, ImageProc::r_black[HEIGHT-5], HEIGHT-1, MODE_TWO_POINTS, DIRECTION_UP_RIGHT, 0);
                        }
                        
                    }else if(Right_corner.Curve_Row ){
                        my_draw_line(ImageProc::r_black[Right_corner.Curve_Row + 1], Right_corner.Curve_Row + 1, 0, 0, MODE_AUTO_SLOPE, DIRECTION_DOWN_RIGHT, 0);
                    }else if(M_y!=0){

                        my_draw_line(ImageProc::r_black[M_y], M_y, 0, 0, MODE_AUTO_SLOPE, DIRECTION_UP_RIGHT, 0);
                    }

                    if(ImageProc::Mid_BF.LostParam.sum > 15 ||((ImageProc::Right_BF.Slope<0.9 && ImageProc::Right_BF.Slope>0)&&(ImageProc::Left_BF.Slope>-1 && ImageProc::Left_BF.Slope<0))){
                        Cross.State = 2;
                        Left_Tilt_Cross.State = 0;
                        M_y = 0;
                        ImageProc::state.set_start_point = 0 ;
                    }


                    
                    break;
                }

        }


    }
    
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
                    
                    (ImageProc::Mid_BF.LostParam.sum > 1 &&
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

                if (Left_corner.D_y > 20 ){
                    Left_corner.U_y = cross_Up(Left_corner.D_y,  HEIGHT - ImageProc::state.cutoff , 0, ImageProc::l_black);
                }else{
                    Left_corner.U_y = cross_Up(HEIGHT - 20,  HEIGHT - ImageProc::state.cutoff , 0, ImageProc::l_black);
                }
                
                if (Right_corner.D_y > 20 ){
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
                Left_corner.U_y  = cross_Up(ImageProc::Left_BF.LostParam.mid,  ImageProc::Left_BF.LostParam.end - 15, 0, ImageProc::l_black);
                Right_corner.U_y = cross_Up(ImageProc::Right_BF.LostParam.mid, ImageProc::Right_BF.LostParam.end - 15, 1, ImageProc::r_black);

                if (Left_corner.U_y < 28 ) { Left_corner.U_y = 0;}
                if (Right_corner.U_y > 28 ) { Right_corner.U_y = 0;}


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
                    Right_Island.State==0 && Cross.State == 0 && ImageProc::state.cutoff >= 65 &&  
                    ((ImageProc::Right_BF.LostParam.start > 67 && ImageProc::Right_BF.LostParam.sum < 15) ||
                     ImageProc::Right_BF.LostParam.sum < 2) && 
                    (((ImageProc::Left_BF.LostParam.sum > 6 || ImageProc::sec_loss_left.LostParam.sum > 6)&& ImageProc::Left_BF.LostParam.start < 50) ) && 
                    (ImageProc::Mid_BF.LostParam.sum < 2  )&& 
                    ImageProc::Right_BF.Var < 10 && ImageProc::Left_BF.Var > 140 ){
                    Left_corner.D_y = island_Down(HEIGHT - 4, HEIGHT - ImageProc::state.cutoff - 1 , 0, ImageProc::l_black);
                    Left_Island.similar_flag = 1;
                    Left_corner.Curve_Row = Circular_Detection(0, 0, ImageProc::Left_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::l_black);
                    
                    if(Left_corner.Curve_Row != 0 ){
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

                }else if(Left_corner.D_y != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.D_y],Left_corner.D_y,0,0,MODE_AUTO_SLOPE,DIRECTION_DOWN_LEFT,0);
                   
                }else if(Left_corner.Curve_Row !=0){
                    my_draw_line(ImageProc::l_black[Left_corner.Curve_Row],Left_corner.Curve_Row,0,0,MODE_FIXED_SLOPE,DIRECTION_UP_LEFT,-ImageProc::Right_BF.Slope);
                     
                }

                
                if (ImageProc::Left_BF.LostParam.sum > 30 && ImageProc::Left_BF.LostParam.start > 67){
                    Left_Island.State = 2;
                    Error::Err.mode = 0;

                }

                break;
            }
            //靠近补线（突变点，上拐点）
            case 2:{
                Left_corner.Curve_Row = Circular_Detection(0, 0, ImageProc::Left_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::l_black);
                Left_corner.U_y = island_Up(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff , 0, ImageProc::l_black);
                if(ImageProc::Left_BF.LostParam.mid > Left_corner.Curve_Row && ImageProc::Left_BF.LostParam.start < 50){
                    Left_corner.Curve_Row = 0;
                }
                if(Left_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.Curve_Row],Left_corner.Curve_Row,0,0,MODE_FIXED_SLOPE,DIRECTION_UP_LEFT,-ImageProc::Right_BF.Slope);
                }


                // else if(Left_corner.U_y != 0){
                //     my_draw_line(ImageProc::l_black[Left_corner.U_y],Left_corner.U_y,ImageProc::r_black[HEIGHT-10],HEIGHT-10,MODE_TWO_POINTS,DIRECTION_DOWN_RIGHT,0);
                // }
                if(Left_corner.Curve_Row == 0 || Left_corner.Curve_Row > 40){
                    Left_Island.State=3;
                }
                break;
            }
            //准备入环（上拐点）
            case 3:{
                Left_corner.U_y = island_Up(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff + 2 , 0, ImageProc::l_black);
                if(Left_corner.U_y != 0 && ImageProc::Left_BF.LostParam.start > 60){
                    Error::Err.limit = HEIGHT - Left_corner.U_y;
                    my_draw_line(ImageProc::l_black[Left_corner.U_y],Left_corner.U_y,ImageProc::r_black[HEIGHT-10],HEIGHT-10,MODE_TWO_POINTS,DIRECTION_DOWN_RIGHT,0);
                }
                if (ImageProc::Right_BF.LostParam.sum > 36 && Left_corner.U_y != 0){
                    Left_Island.State = 4;
                    ImageProc::state.range_r = ImageProc::l_black[Left_corner.U_y - 2];
                    Right_corner.U_y = Left_corner.U_y;
                }
                break;
            }
            //入环（左上拐点变右上拐点）
            case 4:{
            
                Right_corner.U_y = island_Up(ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 2, 1 ,ImageProc::r_black);

                if(Right_corner.U_y != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.U_y-1],Right_corner.U_y-1,ImageProc::r_black[HEIGHT-5],HEIGHT-5,MODE_TWO_POINTS,DIRECTION_UP_RIGHT,0);
                }
                if (ImageProc::Right_BF.LostParam.sum < 10 ){
                    Left_Island.State = 5;
                    ImageProc::state.range_r = WIDTH - 10;
                    Error::Err.limit = HEIGHT - 1; 
                }
                break;
            }
            //入环成功，找出环标志（有突变点）
            case 5:{
                Right_corner.Curve_Row = Circular_Detection(1, 0, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::r_black);
                if (Right_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.Curve_Row + 4], Right_corner.Curve_Row + 4, 20, 20, MODE_TWO_POINTS, DIRECTION_DOWN_RIGHT, 0);
                }
                if (Right_corner.Curve_Row > 45 && ImageProc::Right_BF.LostParam.sum > 2 && ImageProc::Right_BF.LostParam.start < 50){
                    Left_Island.State = 6;
                }else if (Right_corner.Curve_Row == 0 && ImageProc::Right_BF.LostParam.sum > 25 && ImageProc::Right_BF.LostParam.start > 65 && ImageProc::Left_BF.LostParam.start < 55 && ImageProc::Mid_BF.LostParam.sum > 10){
                    Left_Island.State = 6;
                }
                break;
            }
            //出环标志（无突变点）
            case 6:{
                Left_corner.U_y = 0;
                my_draw_line(ImageProc::r_black[HEIGHT-10],HEIGHT-10,20,20,MODE_TWO_POINTS,DIRECTION_DOWN_RIGHT,0);
                Left_corner.U_y = island_Up(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff + 2 , 0, ImageProc::l_black);
                if((ImageProc::Right_BF.LostParam.sum < 15  && ImageProc::Right_BF.Var < 150) || Left_corner.U_y ){
                    Left_Island.State = 7;
                }
                break;           
            }
            case 7:{
                Left_corner.U_y = island_Up(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff , 0, ImageProc::l_black);
                Left_corner.Curve_Row = Circular_Detection(0, 0, ImageProc::Left_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::l_black);
                if(Left_corner.U_y !=0 && Left_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.U_y],Left_corner.U_y,ImageProc::l_black[HEIGHT-1],HEIGHT-1,MODE_TWO_POINTS,DIRECTION_DOWN_LEFT,0);
                }else if(Left_corner.U_y !=0){
                    my_draw_line(ImageProc::l_black[Left_corner.U_y],Left_corner.U_y,0,0,MODE_AUTO_SLOPE,DIRECTION_UP_LEFT,0);
                }else if(Left_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.Curve_Row],Left_corner.Curve_Row,0,0,MODE_FIXED_SLOPE,DIRECTION_DOWN_LEFT,-ImageProc::Right_BF.Slope);
                }
                if((ImageProc::Left_BF.LostParam.mid > 55 || ImageProc::Left_BF.LostParam.mid < 28 ) && (Left_corner.Curve_Row == 0 || Left_corner.Curve_Row < 20)){
                    Left_Island.State = 8;
                }
                break;
            }
            case 8:{
                if((ImageProc::Mid_BF.LostParam.sum < 2 && ImageProc::Left_BF.Var< 100 && ImageProc::Right_BF.Var< 100)|| ImageProc::Left_BF.LostParam.start < 30 || ImageProc::Right_BF.LostParam.start < 30){
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
                if((Right_Tilt_Cross.State==0 && Left_Tilt_Cross.State==0 && Right_Island.enable &&
                    Left_Island.State == 0 && Cross.State == 0  &&
                    ImageProc::state.cutoff >= 65 && 
                    ((ImageProc::Left_BF.LostParam.start > 67 && ImageProc::Left_BF.LostParam.sum < 15) ||
                     ImageProc::Left_BF.LostParam.sum < 2) && 
                    ((ImageProc::Right_BF.LostParam.sum > 6 || ImageProc::sec_loss_right.LostParam.sum > 6)&& ImageProc::Right_BF.LostParam.start < 50) && 
                    (ImageProc::Mid_BF.LostParam.sum < 2 )&& 
                    ImageProc::Right_BF.Var > 140 && ImageProc::Left_BF.Var < 10 
                   )){
                    Right_corner.D_y = island_Down(HEIGHT - 4, HEIGHT - ImageProc::state.cutoff , 0, ImageProc::r_black);
                    Right_Island.similar_flag = 1;
                    Right_corner.Curve_Row = Circular_Detection(1, 0, ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::r_black);
                    
                    
                    if(Right_corner.Curve_Row != 0){
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

            //靠近补线（下拐点，突变点）
            case 1:{
                Right_corner.D_y = island_Down(HEIGHT - 4, HEIGHT - ImageProc::state.cutoff + 2, 1, ImageProc::r_black);
                Right_corner.Curve_Row = Circular_Detection(1, 0, ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::r_black);
                if((Right_corner.D_y < Right_corner.Curve_Row + 10 && Right_corner.Curve_Row != 0) || Right_corner.D_y < ImageProc::Right_BF.LostParam.mid ) Right_corner.D_y = 0;
                if(Right_corner.D_y != 0 && Right_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.D_y],Right_corner.D_y,ImageProc::r_black[Right_corner.Curve_Row],Right_corner.Curve_Row,MODE_TWO_POINTS,DIRECTION_DOWN_RIGHT,0);

                }else if(Right_corner.D_y != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.D_y],Right_corner.D_y,0,0,MODE_AUTO_SLOPE,DIRECTION_DOWN_RIGHT,0);
                   
                }else if(Right_corner.Curve_Row !=0){
                    my_draw_line(ImageProc::r_black[Right_corner.Curve_Row],Right_corner.Curve_Row,0,0,MODE_FIXED_SLOPE,DIRECTION_UP_RIGHT,-ImageProc::Left_BF.Slope);
                     
                }

                
                if (ImageProc::Right_BF.LostParam.sum > 30 && ImageProc::Right_BF.LostParam.start > 67){
                    Right_Island.State = 2;
                    Error::Err.mode = 0;

                }

                break;
            }
            //靠近补线（突变点，上拐点）
            case 2:{
                Right_corner.Curve_Row = Circular_Detection(1, 0, ImageProc::Right_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::r_black);
                Right_corner.U_y = island_Up(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff , 1, ImageProc::r_black);
                if(ImageProc::Right_BF.LostParam.mid > Right_corner.Curve_Row && ImageProc::Right_BF.LostParam.start < 50){
                    Right_corner.Curve_Row = 0;
                }
                if(Right_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.Curve_Row],Right_corner.Curve_Row,0,0,MODE_FIXED_SLOPE,DIRECTION_UP_RIGHT,-ImageProc::Left_BF.Slope);
                }

                // else if(Right_corner.U_y != 0){
                //     my_draw_line(ImageProc::r_black[Right_corner.U_y],Right_corner.U_y,ImageProc::l_black[HEIGHT-10],HEIGHT-10,MODE_TWO_POINTS,DIRECTION_DOWN_LEFT,0);
                // }
                if(Right_corner.Curve_Row == 0 || Right_corner.Curve_Row > 40){
                    Right_Island.State=3;
                }
                break;
            }
            //准备入环（上拐点）
            case 3:{
                Right_corner.U_y = island_Up(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff + 2 , 1, ImageProc::r_black);
                if(Right_corner.U_y != 0 && ImageProc::Right_BF.LostParam.start > 60){
                    Error::Err.limit = HEIGHT - Right_corner.U_y;

                    my_draw_line(ImageProc::r_black[Right_corner.U_y],Right_corner.U_y,ImageProc::l_black[HEIGHT-15],HEIGHT-15,MODE_TWO_POINTS,DIRECTION_DOWN_LEFT,0);
                    
                }

                if (ImageProc::Left_BF.LostParam.sum > 30 ){
                    Right_Island.State = 4;
                    ImageProc::state.range_l = ImageProc::r_black[Right_corner.U_y - 2];
                    Left_corner.U_y = Right_corner.U_y;
                }
                break;
            }
            //入环（左上拐点变右上拐点）
            case 4:{
                
                Left_corner.U_y = island_Up(ImageProc::Left_BF.LostParam.mid, HEIGHT - ImageProc::state.cutoff + 2, 0 ,ImageProc::l_black);
                if(Left_corner.U_y != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.U_y-1],Left_corner.U_y-1,ImageProc::l_black[HEIGHT-5],HEIGHT-5,MODE_TWO_POINTS,DIRECTION_UP_LEFT,0);
                }

                if (ImageProc::Left_BF.LostParam.sum < 10 ){
                    Right_Island.State = 5;
                    ImageProc::state.range_l = 10;
                    Error::Err.limit = HEIGHT - 1; 
                }
                break;  //48s
            }
            //入环成功，找出环标志（有突变点）
            case 5:{
                Left_corner.Curve_Row = Circular_Detection(0, 1, HEIGHT - 1, HEIGHT - ImageProc::state.cutoff + 5, ImageProc::l_black);
                if (Left_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::l_black[Left_corner.Curve_Row + 4], Left_corner.Curve_Row + 4, WIDTH-10, 20, MODE_TWO_POINTS, DIRECTION_DOWN_LEFT, 0);
                }
                if (Left_corner.Curve_Row > 42 && ImageProc::Left_BF.LostParam.sum > 2 && ImageProc::Left_BF.LostParam.start < 50  ){
                    Right_Island.State = 6;
                }else if (Left_corner.Curve_Row == 0 && ImageProc::Left_BF.LostParam.sum > 25 && ImageProc::Left_BF.LostParam.start > 65 && ImageProc::Right_BF.LostParam.start < 55 && ImageProc::Mid_BF.LostParam.sum > 10 ){
                    Right_Island.State = 6;
                }
                break;
            }
            //出环标志（无突变点）
            case 6:{
                Right_corner.U_y = 0;
                my_draw_line(ImageProc::l_black[HEIGHT-5],HEIGHT-5,WIDTH-10,20,MODE_TWO_POINTS,DIRECTION_DOWN_LEFT,0);
                Right_corner.U_y = island_Up(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff , 1, ImageProc::r_black);
                if((ImageProc::Left_BF.LostParam.sum < 15  && ImageProc::Left_BF.Slope < -1 )|| Right_corner.U_y ){
                    Right_Island.State = 7;
                }
                break;             
            }
            case 7:{
                Right_corner.U_y = island_Up(HEIGHT - 3, HEIGHT - ImageProc::state.cutoff , 1, ImageProc::r_black);
                Right_corner.Curve_Row = Circular_Detection(0, 0, HEIGHT-1, Right_corner.U_y, ImageProc::r_black);
                if(Right_corner.U_y >= Right_corner.Curve_Row)  Right_corner.Curve_Row=0;
                if(Right_corner.U_y !=0 && Right_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.U_y],Right_corner.U_y,ImageProc::r_black[HEIGHT-1],HEIGHT-1,MODE_TWO_POINTS,DIRECTION_DOWN_RIGHT,0);
                 }else if(Right_corner.U_y !=0){
                    my_draw_line(ImageProc::r_black[Right_corner.U_y],Right_corner.U_y,0,0,MODE_AUTO_SLOPE,DIRECTION_UP_RIGHT,0);
                }else if(Right_corner.Curve_Row != 0){
                    my_draw_line(ImageProc::r_black[Right_corner.Curve_Row],Right_corner.Curve_Row,0,0,MODE_FIXED_SLOPE,DIRECTION_DOWN_RIGHT,-ImageProc::Left_BF.Slope);
                }
                if((ImageProc::Right_BF.LostParam.mid > 55 || ImageProc::Right_BF.LostParam.mid < 28) && (Right_corner.Curve_Row == 0 || Right_corner.Curve_Row < 20)){
                    Right_Island.State = 8;
                }
                break;
            }



            case 8:{
                if((ImageProc::Mid_BF.LostParam.sum < 2 && ImageProc::Right_BF.Var< 100 && ImageProc::Left_BF.Var< 100) || ImageProc::Left_BF.LostParam.start < 30 || ImageProc::Right_BF.LostParam.start < 30 ){
                    Right_Island.State = 0;
                    clear();
                    break;
                }
            }
            

        }


    }
    

}
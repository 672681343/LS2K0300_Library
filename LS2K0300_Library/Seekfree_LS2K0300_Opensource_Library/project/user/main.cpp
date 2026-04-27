#include "zf_common_headfile.h"
using namespace cv;

void sigint_handler(int signum) 
{
    printf("收到Ctrl+C，程序即将退出\n");
    exit(0);
}

void cleanup()
{
    printf("程序异常退出，执行清理操作\n");
    // 处理程序退出！！！
    // 这里需要关闭电机，关闭电调等。
}




// ======================== 日志回调函数 ========================
void debug_log_printf_callback(const char* s) 
{
    if (s == NULL)  return;
    printf("%s", s);
}



// const char* class_labels[] = {"materials","traffic","weapon"}; //需要与train.py提示顺序一致


int main(int, char**) 
{
    // ======================== 识别初始化 ========================

    // 重定向TFLM的MicroPrintf日志输出
    RegisterDebugLogCallback(debug_log_printf_callback);

    // tflite库初始化
    tflite::InitializeTarget();

    // 导入模型
    // 传入模型文件生成的.h文件当中的数组名
    const tflite::Model* model = ::tflite::GetModel(loong_cnn_model_simple_tflite);
    TFLITE_CHECK_EQ(model->version(), TFLITE_SCHEMA_VERSION);

    // 构建算子解析器（使用宏定义指定最大算子数）
    tflite::MicroMutableOpResolver<TFLITE_OP_RESOLVER_MAX_NUM> resolver;

    /* 添加所有可能会用到的算子 */
	resolver.AddConv2D();
	resolver.AddDepthwiseConv2D();
	resolver.AddMaxPool2D();
	resolver.AddFullyConnected();
	resolver.AddRelu6();
	resolver.AddSoftmax();
	resolver.AddReshape();
	resolver.AddShape();
	resolver.AddSlice();
	resolver.AddQuantize();
	resolver.AddDequantize();
	resolver.AddCast();
	resolver.AddSqueeze();
	resolver.AddExpandDims();
	resolver.AddConcatenation();
	resolver.AddTranspose();
	resolver.AddStridedSlice();
	resolver.AddPack();

    // 创建模型解析器并分配张量空间（使用宏定义指定张量空间大小）
    uint8_t tensor_arena[TENSOR_ARENA_SIZE];
    tflite::MicroInterpreter interpreter(model, resolver, tensor_arena, TENSOR_ARENA_SIZE);
    interpreter.AllocateTensors();

    // 注册清理函数
    atexit(cleanup);

    // 注册SIGINT信号的处理函数
    signal(SIGINT, sigint_handler);

    // 初始化屏幕
    ips200_init("/dev/fb0");

    //摄像头初始化
    printf("===== 程序初始化 =====\n");
    if(uvc_camera_init("/dev/video0") < 0)
    {
        printf("❌ 摄像头初始化失败！\n");
        return -1;
    }
    printf("✅ 摄像头初始化成功\n");

    //cv图像矩阵初始化
    Mat float_img,resized_img;

    // 日志控制变量
    int frame_count = 0;          // 帧计数器
    bool first_frame_logged = false;  // 第一帧日志是否已打印

    printf("===== 开始实时推理 =====\n");
  
    // ======================== 基础初始化 ========================
    TCP::init();
    Motor::Init();                              // 初始化电机
    Motor::goMotor(0, 0);                       // 电机清空
    Key::Init();                                // 初始化按键
    PID::PID_init();                            // PID初始化
    // MenuSystem::init();                         // 菜单初始化
    Pit::Init();                                // 中断初始化

    while(1)
    {

        frame_count++;  // 帧计数+1
        
        //摄像头采集数据
        if(wait_image_refresh() < 0)
        {
            printf("❌ 第%d帧：摄像头采集异常，程序退出！\n", frame_count);
            exit(0);   // 摄像头采集异常，退出程序，防止程序卡死
        }
        //获取摄像头采集原始数据

        if(frame_rgb.empty())
        {
            printf("❌ 第%d帧：获取图像数据为空！\n", frame_count);
            continue;  // 跳过空帧，不退出程序
        }

        // 将原来的预处理、推理、输出打印代码替换为：
        InferenceResult inference_result = perform_inference(
            frame_rgb, 
            frame_count, 
            (ENABLE_FIRST_FRAME_DEBUG && !first_frame_logged), 
            interpreter
        );

        // 更新first_frame_logged标志（如果需要）
        if (ENABLE_FIRST_FRAME_DEBUG && !first_frame_logged) {
            first_frame_logged = true;
        }

        // ======================== 图像和控制处理 ========================

        Camera::basic_image_processing(rgay_image);
        ImageProc::image_measurement(Camera::bin_pixels,Camera::lcd_pixels);
        Elem::elem_judge();
        Err::error_get(ImageProc::state, ImageProc::l_black, ImageProc::r_black, ImageProc::left_flags, ImageProc::right_flags);
        
        // ips200_show_gray_image(0, 0, rgay_image, UVC_WIDTH, UVC_HEIGHT);

        if (MenuSystem::Car_flag.init == 1  ){

            if(MenuSystem::Car_flag.start == 1){
                Control::RoadSegment_t = Control::classifyRoadSegment();
                Control::ControlOutput(Control::RoadSegment_t);
                DataSaving::saveTask(1);
                TCP::send_data();
            }
            Control::stop(0);
        }

        Err::last_error_update();
        // MenuSystem::key_shutdown();
        // MenuSystem::Menu_display();


        // ips200_show_rgb565_image(0,0,rgb565_image,160,120,60,20,0);


        process_image(rgb565_image);
        if(!boxes.empty()){
            // const auto& box = boxes[0];
            printf("Box: Area=%d at (%d,%d) size %dx%d\n",
                   boxes[0].area, boxes[0].x, boxes[0].y, boxes[0].w, boxes[0].h);            
        }
        
        

        // ips200_show_rgb565_image(0, 0, rgb565_image, 160, 120, 160, 120, 0);
        ips200_show_gray_image(0, 0, rgay_image, 160, 120);
        
    }
}
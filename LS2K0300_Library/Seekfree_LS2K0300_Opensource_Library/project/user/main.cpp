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

// ======================== 模型初始化函数 ========================
// ======================== 模型初始化函数 ========================
bool init_tflite_interpreter(tflite::MicroInterpreter*& interpreter, uint8_t* tensor_arena, 
                             int& frame_count, bool& first_frame_logged)
{
    // 导入模型
    const tflite::Model* model = ::tflite::GetModel(loong_cnn_model_simple_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        printf("❌ 模型版本不匹配！\n");
        return false;
    }

    // 构建算子解析器
    tflite::MicroMutableOpResolver<TFLITE_OP_RESOLVER_MAX_NUM> resolver;
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

    // 创建模型解析器
    static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, TENSOR_ARENA_SIZE);
    TfLiteStatus allocate_status = static_interpreter.AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        printf("❌ 分配张量空间失败！\n");
        return false;
    }
    
    interpreter = &static_interpreter;

    // 初始化帧计数变量
    frame_count = 0;
    first_frame_logged = false;
    
    return true;
}
const char* class_labels[] = {"materials","traffic","weapon"}; //需要与train.py提示顺序一致

int main(int, char**) 
{
    // ======================== 识别初始化 ========================
    uint8_t tensor_arena[TENSOR_ARENA_SIZE];
    tflite::MicroInterpreter* interpreter = nullptr;  // 改为指针
    int frame_count = 0;
    bool first_frame_logged = false;
    
    // cv::Mat 在循环中定义，不需要传入初始化函数
    cv::Mat float_img, resized_img;

    // 重定向TFLM的MicroPrintf日志输出
    RegisterDebugLogCallback(debug_log_printf_callback);

    // tflite库初始化
    tflite::InitializeTarget();

    // 初始化模型解释器
    if (!init_tflite_interpreter(interpreter, tensor_arena, frame_count, first_frame_logged)) {
        printf("❌ 模型解释器初始化失败！\n");
        return -1;
    }
    
    if (interpreter == nullptr) {
        printf("❌ 模型解释器指针为空！\n");
        return -1;
    }

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

    printf("===== 开始实时推理 =====\n");
  
    // ======================== 基础初始化 ========================
    TCP::init();
    Motor::Init();                              // 初始化电机
    Motor::goMotor(0, 0);                       // 电机清空
    Key::Init();                                // 初始化按键
    PID::PID_init();                            // PID初始化
    MenuSystem::init();                         // 菜单初始化
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

        // ======================== 图像预处理 ========================
        // 1. 先缩放图像到模型要求的输入尺寸
        resize(frame_rgb, resized_img, Size(MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT), INTER_LINEAR);

        // 2. 类型转换（转为32位浮点型，可选归一化）
        resized_img.convertTo(float_img, CV_32FC3, 1.0);  // 训练时归一化则用1.0/255.0，否则用1.0

        // 3. 色彩空间转换（BGR→RGB，适配模型输入）
        cvtColor(float_img, float_img, cv::COLOR_BGR2RGB);

        // 4. 确保内存连续，避免拷贝错误（可选）
        Mat continuous_float_img = float_img.isContinuous() ? float_img : float_img.clone();
        float* model_input_ptr = continuous_float_img.ptr<float>(0);

        // ===== 第一帧打印完整预处理日志（仅打印一次）=====
        if(ENABLE_FIRST_FRAME_DEBUG && !first_frame_logged)
        {
            printf("\n===== 第1帧 完整预处理日志 =====\n");
            // 原始图像信息
            printf("1. 原始图像：%d x %d (宽x高)，通道数：%d\n", frame_rgb.cols, frame_rgb.rows, frame_rgb.channels());
            // 缩放后图像信息
            printf("2. 缩放后图像：%d x %d (宽x高)，通道数：%d\n", resized_img.cols, resized_img.rows, resized_img.channels());
            // 内存连续性检查
            printf("3. 浮点图像内存连续：%s\n", float_img.isContinuous() ? "是" : "否");
            printf("4. 模型输入尺寸匹配：%s\n", 
                   (continuous_float_img.total() * continuous_float_img.elemSize()) == (MODEL_INPUT_SIZE * sizeof(float)) 
                   ? "✅ 匹配" : "❌ 不匹配");
            printf("=================================\n");
            first_frame_logged = true;  // 标记第一帧日志已打印
        }

        // ======================== 模型输入 ========================
        float* input_data = interpreter->input(0)->data.f;  // 改为指针访问
        memcpy(input_data, model_input_ptr, MODEL_INPUT_SIZE * sizeof(float));

        // ======================== 模型推理（计时） ========================
        auto start_time = std::chrono::high_resolution_clock::now();
        interpreter->Invoke();  // 改为指针访问
        auto end_time = std::chrono::high_resolution_clock::now();

        // 计算推理耗时
        auto time_cnt = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

        // ======================== 模型输出解析 ========================
        float* output_data = interpreter->output(0)->data.f;  // 改为指针访问
        float max_v = 0;
        int max_i = 0;
        
        // 查找置信度最高的类及其索引
        for (int n = 0; n < MODEL_OUTPUT_CLASS_NUM; n++) 
        {
            float v = output_data[n];
            if (v > max_v) {
                max_v = v;
                max_i = n;
            }
        }

        // ===== 可控频率打印推理结果 =====
        if(frame_count % LOG_PRINT_FREQUENCY == 0)
        {
            // 打印简洁的推理结果（每N帧一次）
            printf("[第%d帧] 预测类别：%s | 置信度：%.4f | 推理耗时：%ld us\n",
                   frame_count, class_labels[max_i], max_v, time_cnt);
        }

        // ======================== 图像和控制处理 ========================
        Camera::basic_image_processing(rgay_image);
        ImageProc::image_measurement(Camera::bin_pixels,Camera::lcd_pixels);
        Elem::elem_judge();
        Err::error_get(ImageProc::state, ImageProc::l_black, ImageProc::r_black, ImageProc::left_flags, ImageProc::right_flags);
        
        if (MenuSystem::Car_flag.init == 1) {
            if(MenuSystem::Car_flag.start == 1) {
                Control::RoadSegment_t = Control::classifyRoadSegment();
                Control::ControlOutput(Control::RoadSegment_t);
                DataSaving::saveTask(1);
                TCP::send_data();
            }
            Control::stop(0);
        }

        Err::last_error_update();
        MenuSystem::key_shutdown();
        MenuSystem::Menu_display();

        process_image(rgb565_image);
        if(!boxes.empty()) {
            printf("Box: Area=%d at (%d,%d) size %dx%d\n",
                   boxes[0].area, boxes[0].x, boxes[0].y, boxes[0].w, boxes[0].h);            
        }
    }
}
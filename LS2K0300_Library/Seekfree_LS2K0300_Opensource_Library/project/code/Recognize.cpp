#include "Recognize.h"

// 全局指针定义
tflite::MicroInterpreter* interpreter = nullptr; // 用来存储模型解释器的指针

// 类别标签定义
static const char* class_labels[] = {"materials", "traffic", "weapon",};

//cv图像矩阵初始化
Mat float_img,resized_img;

// 日志控制变量
int frame_count = 0;          // 帧计数器
bool first_frame_logged = false;  // 第一帧日志是否已打印

// ======================== 日志回调函数 ========================
void debug_log_printf_callback(const char* s) 
{
    if (s == NULL)  return;
    printf("%s", s);
}



// 初始化函数 - 无返回值版本
bool init_tflite_simple(uint8_t* tensor_arena, size_t arena_size) {
    // 如果已经初始化，先清理
    if (interpreter != nullptr) {
        delete interpreter;
        interpreter = nullptr;
    }
    
    // 设置日志回调
    RegisterDebugLogCallback(debug_log_printf_callback);
    
    // TFLite初始化
    tflite::InitializeTarget();
    
    static tflite::MicroMutableOpResolver<TFLITE_OP_RESOLVER_MAX_NUM> resolver;
    
    // 添加所有算子
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
    
    // 加载模型
    const tflite::Model* model = ::tflite::GetModel(loong_cnn_model_simple_tflite);
    
    // 检查模型版本
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        printf("Error: Model version mismatch!\n");
        return false;
    }
    
    // 创建解释器（直接赋值给全局变量）
    interpreter = new tflite::MicroInterpreter(model, resolver, tensor_arena, arena_size);
    
    // 分配张量
    if (interpreter->AllocateTensors() != kTfLiteOk) {
        printf("Error: Failed to allocate tensors!\n");
        delete interpreter;
        interpreter = nullptr;
        return false;
    }
    
    printf("✅ Model初始化成功!\n");
    printf("Input tensor shape: ");
    TfLiteTensor* input = interpreter->input(0);
    for (int i = 0; i < input->dims->size; i++) {
        printf("%d ", input->dims->data[i]);
    }
    printf("\n");
    
    return true;
}

// 清理函数
void cleanup_tflite() {
    if (interpreter != nullptr) {
        delete interpreter;
        interpreter = nullptr;
        printf("Model interpreter cleaned up.\n");
    }
}





InferenceResult perform_inference(
    cv::Mat input_image,
    int frame_count,
    bool is_first_frame_debug,
    tflite::MicroInterpreter& interpreter
) {
    InferenceResult result = {0};
    cv::Mat resized_img, float_img;
    
    // 图像预处理
    cv::resize(input_image, resized_img, cv::Size(MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT), cv::INTER_LINEAR); //  1. 调整输入图像大小以匹配模型输入尺寸
    // ips200_show_rgb565_image(0, 0, resized_img, UVC_WIDTH, UVC_HEIGHT, UVC_WIDTH, UVC_HEIGHT, 0);//调试用


    

    // show_rgb565_image(0, 0, resized_img, 40, 40, 0, 0, 1);

        show_cv_mat_on_ips200(0, 0, resized_img, 0);

    //cv::resize：这是OpenCV库中用于调整图像大小的函数，它可以将源图像缩放到指定尺寸。
    //input_image：这是函数的第一个参数，表示需要进行尺寸调整的输入图像，它是一个OpenCV的Mat对象。
    //resized_img：这是函数的第二个参数，表示调整大小后的输出图像，也是一个Mat对象。
    //cv::Size(MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT)：这是函数的第三个参数，指定了输出图像的目标尺寸。
    //cv::INTER_LINEAR：这是函数的第四个参数，指定了图像缩放时使用的插值方法。cv::INTER_LINEAR表示使用双线性插值法，这是一种常用的图像缩放方法，它在保持图像质量的同时计算效率较高。
    resized_img.convertTo(float_img, CV_32FC3, 1.0);
    cv::cvtColor(float_img, float_img, cv::COLOR_BGR2RGB); //  3. 将图像颜色空间从BGR转换为RGB 
    //输入和输出使用的是同一个变量float_img，这意味着转换结果会覆盖原始图像数据。
    //cv::COLOR_BGR2RGB：这是颜色空间转换的代码，指定了从BGR（蓝-绿-红）到RGB（红-绿-蓝）的转换。
    cv::Mat continuous_float_img = float_img.isContinuous() ? float_img : float_img.clone(); //  确保图像数据是连续存储的，以提高访问效率
    //如果返回false（即图像数据不连续），则调用float_img.clone()方法创建一个float_img的完整副本，这个副本的数据在内存中将是连续存储的。
    float* model_input_ptr = continuous_float_img.ptr<float>(0); //  获取模型输入数据的指针

    // 第一帧调试信息i8
    if(is_first_frame_debug) {
        printf("\n===== 第1帧 完整预处理日志 =====\n");
        printf("1. 原始图像：%d x %d\n", input_image.cols, input_image.rows); //  打印原始图像的尺寸信息
        printf("2. 缩放后图像：%d x %d\n", resized_img.cols, resized_img.rows); //  打印缩放后的图像尺寸
        printf("3. 浮点图像内存连续：%s\n", float_img.isContinuous() ? "是" : "否"); //  检查浮点图像在内存中是否连续存储
        printf("4. 模型输入尺寸匹配：%s\n", 
               (continuous_float_img.total() * continuous_float_img.elemSize()) == (MODEL_INPUT_SIZE * sizeof(float)) 
               ? "✅ 匹配" : "❌ 不匹配"); //  验证预处理后的图像尺寸是否符合模型输入要求
        printf("=================================\n");
    }

    // 模型输入
    float* input_data = interpreter.input(0)->data.f;
    memcpy(input_data, model_input_ptr, MODEL_INPUT_SIZE * sizeof(float)); //  将模型输入指针的数据复制到input_data中，复制大小为MODEL_INPUT_SIZE个float类型的数据

    // 模型推理
    auto start_time = std::chrono::high_resolution_clock::now(); //  记录推理开始时间
    interpreter.Invoke();
    auto end_time = std::chrono::high_resolution_clock::now(); //  记录推理结束时间
    auto time_cnt = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count(); //  计算推理耗时，并将结果转换为微秒(μs)存储在result结构体中
    result.inference_time_us = time_cnt; //  记录模型推理耗时（单位：微秒）

    // 输出解析
    float* output_data = interpreter.output(0)->data.f;
    float max_v = 0; //  用于记录最大置信度值
    int max_i = 0; //  用于记录最大置信度对应的类别索引
    
    for (int n = 0; n < MODEL_OUTPUT_CLASS_NUM; n++) { //  遍历模型输出结果，找出最大值及其索引
        float v = output_data[n];
        if (v > max_v) {
            max_v = v;
            max_i = n;
        }
    }
    
    result.class_index = max_i;
    result.confidence = max_v;

    // 打印结果
    if(frame_count % LOG_PRINT_FREQUENCY == 0) { //  每隔LOG_PRINT_FREQUENCY帧打印一次
        printf("[第%d帧] 预测类别：%s | 置信度：%.4f | 推理耗时：%ld us\n", //  打印帧数、预测类别、置信度和推理耗时
               frame_count, class_labels[max_i], max_v, time_cnt);
    }

    return result; //  返回包含推理结果的结构体
}
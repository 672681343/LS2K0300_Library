#include "Recognize.h"

// 全局指针定义
tflite::MicroInterpreter* interpreter = nullptr;

// ========== 6小类标签定义 ==========
const char* class_labels[] = {
    "scope_1",      // 0 - 望远镜
    // "medkit_2",     // 1 - 急救包
    "tank_1",       // 2 - 装甲车
    // "ambulance_2",  // 3 - 救护车
    // "gun_1",        // 4 - 枪
    "explosive_2" ,  // 5 - 爆炸物
};

// cv图像矩阵初始化
Mat float_img, resized_img;

// 日志控制变量
int frame_count = 0;
bool first_frame_logged = false;

// ======================== 日志回调函数 ========================
void debug_log_printf_callback(const char* s) 
{
    if (s == NULL) return;
    printf("%s", s);
}

// ======================== 初始化函数 ========================
bool init_tflite_simple(uint8_t* tensor_arena, size_t arena_size, bool use_int8) {
    if (interpreter != nullptr) {
        delete interpreter;
        interpreter = nullptr;
    }

    RegisterDebugLogCallback(debug_log_printf_callback);
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
    resolver.AddAdd();
    resolver.AddMul();

    //添加一些常用的算子，确保模型中使用的算子都被注册了
    resolver.AddMean();

    // 根据开关加载不同模型
    const tflite::Model* model;
    if (use_int8) {
        model = ::tflite::GetModel(loong_cnn_model_simple_tflite);
        printf("Loading INT8 model...\n");
    } else {
        model = ::tflite::GetModel(loong_cnn_model_simple_tflite);
        printf("Loading FP32 model...\n");
    }

    if (model->version() != TFLITE_SCHEMA_VERSION) {
        printf("Error: Model version mismatch!\n");
        return false;
    }

    interpreter = new tflite::MicroInterpreter(model, resolver, tensor_arena, arena_size);

    if (interpreter->AllocateTensors() != kTfLiteOk) {
        printf("Error: Failed to allocate tensors!\n");
        delete interpreter;
        interpreter = nullptr;
        return false;
    }

    // 检查输入输出类型
    TfLiteTensor* input = interpreter->input(0);
    TfLiteTensor* output = interpreter->output(0);

    printf("✅ 模型初始化成功! (模式: %s)\n", use_int8 ? "INT8" : "FP32");
    printf("Input tensor shape: ");
    for (int i = 0; i < input->dims->size; i++) {
        printf("%d ", input->dims->data[i]);
    }
    printf("\nInput type: %s\n", TfLiteTypeGetName(input->type));
    printf("Output type: %s\n", TfLiteTypeGetName(output->type));
    printf("Output classes: %d\n", MODEL_OUTPUT_CLASS_NUM);

    return true;
}

// ======================== 清理函数 ========================
void cleanup_tflite() {
    if (interpreter != nullptr) {
        delete interpreter;
        interpreter = nullptr;
        printf("Model interpreter cleaned up.\n");
    }
}

// ======================== 推理函数 ========================
InferenceResult perform_inference(
    cv::Mat input_image,
    int frame_count,
    bool is_first_frame_debug,
    tflite::MicroInterpreter& interpreter,
    bool use_int8
) {

    // 在函数开头加：
    // // 直接用 data 指针访问第一个像素
    // uchar* ptr = input_image.data;
    // printf("帧%d 图像首像素: [%d, %d, %d], 图像地址: %p\n", 
    //     frame_count, ptr[0], ptr[1], ptr[2], 
    //     (void*)input_image.data);


    InferenceResult result = {0};
    cv::Mat resized_img, float_img;

    // 图像预处理
    cv::resize(input_image, resized_img, cv::Size(MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT), cv::INTER_LINEAR);
    resized_img.convertTo(float_img, CV_32FC3, 1.0/255.0);  // 归一化到[0,1] !!!!!!!注意如果训练文件已经归一化了才需要这么写，如果没归一化，就直接写1.0
    cv::cvtColor(float_img, float_img, cv::COLOR_BGR2RGB);
    cv::Mat continuous_float_img = float_img.isContinuous() ? float_img : float_img.clone();
    float* model_input_ptr = continuous_float_img.ptr<float>(0);

    // show_cv_mat_on_ips200(0, 0, resized_img, 0);//显示预处理后的图像，调试用


    // 第一帧调试信息
    if (is_first_frame_debug) {
        printf("\n===== 第1帧 完整预处理日志 =====\n");
        printf("1. 原始图像: %d x %d\n", input_image.cols, input_image.rows);
        printf("2. 缩放后图像: %d x %d\n", resized_img.cols, resized_img.rows);
        printf("3. 浮点图像内存连续: %s\n", float_img.isContinuous() ? "是" : "否");
        printf("4. 模型输入尺寸匹配: %s\n", 
               (continuous_float_img.total() * continuous_float_img.elemSize()) == (MODEL_INPUT_SIZE * sizeof(float)) 
               ? "✅ 匹配" : "❌ 不匹配");
        printf("5. 量化模式: %s\n", use_int8 ? "INT8" : "FP32");
        printf("=================================\n");
    }

    // ========== 模型输入 ==========
    if (use_int8) {
        // INT8 输入预处理
        int8_t* input_data = interpreter.input(0)->data.int8;
        for (int i = 0; i < MODEL_INPUT_SIZE; i++) {
            float val = model_input_ptr[i] * 255.0f;  // [0, 255]
            val = val - 128.0f;                          // [-128, 127]
            if (val < -128.0f) val = -128.0f;
            if (val > 127.0f) val = 127.0f;
            input_data[i] = (int8_t)val;
        }
            // === 新增：确认写入后的数据在变 ===
        // printf("[调试]帧%d 写入后input_data[0..2]: [%d, %d, %d], ptr=%p\n",frame_count, input_data[0], input_data[1], input_data[2], (void*)input_data);

    } else {
        // FP32 直接复制
        float* input_data = interpreter.input(0)->data.f;
        memcpy(input_data, model_input_ptr, MODEL_INPUT_SIZE * sizeof(float));
    }



    // 模型推理
    auto start_time = std::chrono::high_resolution_clock::now();
    interpreter.Invoke();
    auto end_time = std::chrono::high_resolution_clock::now();
    auto time_cnt = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    result.inference_time_us = time_cnt;

    // ========== 输出解析 ==========
    float scores[MODEL_OUTPUT_CLASS_NUM];
    float max_v = -999.0f;
    int max_i = 0;
    float second_v = -999.0f;
    int second_i = 0;

    if (use_int8) {
        // INT8 输出反量化
        int8_t* output_int8 = interpreter.output(0)->data.int8;
        TfLiteTensor* output_tensor = interpreter.output(0);
        float output_scale = output_tensor->params.scale;
        int output_zero_point = output_tensor->params.zero_point;

        for (int n = 0; n < MODEL_OUTPUT_CLASS_NUM; n++) {
            float v = (output_int8[n] - output_zero_point) * output_scale;
            scores[n] = v;
            result.all_scores[n] = v;

            if (v > max_v) {
                second_v = max_v;
                second_i = max_i;
                max_v = v;
                max_i = n;
            } else if (v > second_v) {
                second_v = v;
                second_i = n;
            }
        }
    } else {
        // FP32 直接读取
        float* output_data = interpreter.output(0)->data.f;

        for (int n = 0; n < MODEL_OUTPUT_CLASS_NUM; n++) {
            float v = output_data[n];
            scores[n] = v;
            result.all_scores[n] = v;

            if (v > max_v) {
                second_v = max_v;
                second_i = max_i;
                max_v = v;
                max_i = n;
            } else if (v > second_v) {
                second_v = v;
                second_i = n;
            }
        }
    }

    result.class_index = max_i;
    result.confidence = max_v;

    // 打印结果
    if (frame_count % LOG_PRINT_FREQUENCY == 0) {
        printf("[第%d帧] 预测为：%s, 置信度: %.4f, 推理耗时: %ld us", 
               frame_count, class_labels[max_i], max_v, time_cnt);
        printf("         次优预测：%s, 置信度: %.4f\n", 
               class_labels[second_i], second_v);
    }

    return result;
}
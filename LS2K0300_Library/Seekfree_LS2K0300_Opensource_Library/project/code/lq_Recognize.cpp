//龙邱识别方案
#include "lq_Recognize.h"
#include <opencv2/opencv.hpp>
#include <limits>
#include <stdexcept>
#include <opencv2/imgproc.hpp>

// 定义全局 NCNN 对象
LQ_NCNN g_ncnn;




/********************************************************************************
 * @file    lq_ncnn_demo.cpp
 * @brief   NCNN 测试.
 * @author  龙邱科技-006
 * @date    2026-01-10
 * @version V2.1.0
 * @note    适用与龙芯 2K0300/0301 平台.
 *          本 demo 实现 NCNN 功能，用于测试 NCNN 模型的基本功能.
 ********************************************************************************/

// 获取当前时间字符串（格式：YYYY-MM-DD HH:MM:SS）
static std::string get_time_str(void)
{
    // 时间获取对象
    static lq_ntp g_ntp;
    return g_ntp.get_local_time_str();
}



bool ncnn_init_model(void)
{
    extern LQ_NCNN g_ncnn;  // 声明外部对象

    g_ncnn.SetModelPath("/home/root/tiny_classifier_fp32.ncnn.param",
                        "/home/root/tiny_classifier_fp32.ncnn.bin");
    g_ncnn.SetInputSize(48, 48);
    g_ncnn.SetLabels(CLASS_NAMES);  // 传给 NCNN 内部的标签数组


// NORM_MEAN = [0.4629, 0.4272, 0.4120] # 直道+左转弯+右转弯
// NORM_STD  = [ 0.1333, 0.1765, 0.1648]

    float mean_vals[3] = {118.0395f, 108.9360f, 105.0600f};
    float norm_vals[3] = {0.029419f, 0.022219f, 0.023795f};

    // float mean_vals[3] = {121.533f, 112.0725f, 107.661f};
    // float norm_vals[3] = {0.031651f, 0.0226287f, 0.023956f};

    // float mean_vals[3] = {127.466850f, 116.576565f, 110.429025f};
    // float norm_vals[3] = {0.041489f, 0.023834f, 0.024194f};
    g_ncnn.SetNormalize(mean_vals, norm_vals);

    printf("[NCNN] 正在加载模型...\n");
    if (!g_ncnn.Init()) {
        printf("[NCNN] 模型加载失败!\n");
        return false;
    }
    printf("[NCNN] 模型加载成功!\n");
    return true;
}


void lq_ncnn_video_demo(void)
{
    printf("========================================\n");
    printf("       NCNN 视频流分类推理示例\n");
    printf("========================================\n\n");

    // ==================== 配置区域（按需修改） ====================
    // 视频源： "0"=默认摄像头，或写视频文件路径如 "/home/root/test.mp4"
    std::string video_source = "0";

    std::string model_param = "/home/root/tiny_classifier_fp32.ncnn.param";
    std::string model_bin   = "/home/root/tiny_classifier_fp32.ncnn.bin";

    int input_width  = 48;
    int input_height = 48;

    // 类别标签（顺序必须和训练时 labels.txt 一致）
    // 不限数量，2个、4个、8个都可以，调色盘会自动循环
    std::vector<std::string> labels = {"materials", "traffic", "weapon_1", "weapon_2"};

    float mean_vals[3] = {127.466850f, 116.576565f, 110.429025f};
    float norm_vals[3] = {0.041489f, 0.023834f, 0.024194f};

    // 预定义调色盘（BGR 格式）。类别数超过时会自动循环使用。
    // 想换颜色直接改下面的值，范围 0~255，格式 (B, G, R)
    static const std::vector<cv::Scalar> COLOR_PALETTE = {
        cv::Scalar(0, 128, 255),    // 橙色
        cv::Scalar(0, 255, 0),      // 绿色
        cv::Scalar(255, 0, 255),    // 紫色
        cv::Scalar(0, 255, 255),    // 黄色
        cv::Scalar(255, 0, 0),      // 蓝色
        cv::Scalar(0, 0, 255),      // 红色
        cv::Scalar(255, 255, 0),    // 青色
        cv::Scalar(128, 128, 255),  // 粉色
    };
    // =============================================================

    // 获取颜色辅助函数（预测中的类别自动加亮）
    auto getColor = [&](size_t idx, bool is_predicted) -> cv::Scalar {
        cv::Scalar base = COLOR_PALETTE[idx % COLOR_PALETTE.size()];
        if (is_predicted) {
            return cv::Scalar(
                std::min(base[0] + 50, 255.0),
                std::min(base[1] + 50, 255.0),
                std::min(base[2] + 50, 255.0)
            );
        }
        return base;
    };

    // 初始化 NCNN
    LQ_NCNN ncnn;
    ncnn.SetModelPath(model_param, model_bin);
    ncnn.SetInputSize(input_width, input_height);
    ncnn.SetLabels(labels);
    ncnn.SetNormalize(mean_vals, norm_vals);

    printf("[%s] 正在加载模型...\n", get_time_str().c_str());
    if (!ncnn.Init()) {
        printf("[%s] 模型加载失败!\n", get_time_str().c_str());
        return;
    }
    printf("[%s] 模型加载成功!\n\n", get_time_str().c_str());

    // 打开视频源
    cv::VideoCapture cap;
    int camera_id = -1;
    try {
        camera_id = std::stoi(video_source);
    } catch (...) {
        camera_id = -1;
    }

    if (camera_id >= 0) {
        cap.open(camera_id);
    } else {
        cap.open(video_source);
    }

    if (!cap.isOpened()) {
        printf("无法打开视频源: %s\n", video_source.c_str());
        return;
    }

    printf("[%s] 视频流已启动，分辨率: %dx%d, 按 Q 退出...\n\n",
           get_time_str().c_str(),
           (int)cap.get(cv::CAP_PROP_FRAME_WIDTH),
           (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT));

    cv::Mat frame;
    const int font = cv::FONT_HERSHEY_SIMPLEX;

    while (true) {
        if (!cap.read(frame)) {
            printf("视频流读取结束\n");
            break;
        }

        // 推理
        auto t0 = std::chrono::high_resolution_clock::now();
        std::pair<std::string, std::vector<float>> result = ncnn.InferWithProbs(frame);
        auto t1 = std::chrono::high_resolution_clock::now();

        std::string pred_label = result.first;
        std::vector<float> probs = result.second;

        long ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

        // 找最大概率索引
        size_t pid = 0;
        float max_prob = probs[0];
        for (size_t i = 1; i < probs.size(); ++i) {
            if (probs[i] > max_prob) {
                max_prob = probs[i];
                pid = i;
            }
        }

        float fps = (ms > 0) ? (1000.0f / ms) : 0.0f;

        // ========== 绘制（类似 Python 版本） ==========
        // 1. 预测结果（左上角，绿色）
        std::string pred_txt = "Pred: " + pred_label + " (" + std::to_string(static_cast<int>(max_prob * 100)) + "%)";
        cv::putText(frame, pred_txt, cv::Point(10, 30), font, 0.7, cv::Scalar(0, 255, 0), 2);

        // 2. 耗时和 FPS（第二行，青色）
        std::string fps_txt = std::to_string(ms) + "ms (" + std::to_string(static_cast<int>(fps)) + " FPS)";
        cv::putText(frame, fps_txt, cv::Point(10, 60), font, 0.6, cv::Scalar(255, 255, 0), 2);

        // 3. 各类别置信度（动态调色盘）
        int y = 95;
        for (size_t i = 0; i < labels.size(); ++i) {
            bool is_pred = (i == pid);
            cv::Scalar color = getColor(i, is_pred);
            int thickness = is_pred ? 2 : 1;
            double fontScale = is_pred ? 0.55 : 0.5;

            std::string txt = labels[i] + ": " + std::to_string(static_cast<int>(probs[i] * 100)) + "%";
            cv::putText(frame, txt, cv::Point(10, y), font, fontScale, color, thickness);
            y += 24;
        }

        // 显示
        cv::imshow("NCNN Video", frame);

        // 按 Q 退出
        int key = cv::waitKey(1) & 0xFF;
        if (key == 'q' || key == 'Q') {
            printf("用户退出\n");
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();
    printf("视频流已关闭\n");
    return;
}







/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 * @函数名称：std::pair<std::string, std::vector<float>> LQ_NCNN::InferWithProbs(const cv::Mat& bgr_image)
 * @功能说明：推理并返回各类别概率
 * @参数说明：bgr_image : 输入BGR图像
 * @函数返回：{类别名称, 各类别概率向量}
 * @备注说明：内部做 Softmax
 QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
std::pair<std::string, std::vector<float>> LQ_NCNN::InferWithProbs(const cv::Mat& bgr_image)
{
    if (!m_initialized) {
        throw std::runtime_error("NCNN not initialized. Call Init() first.");
    }
    if (bgr_image.empty()) {
        throw std::invalid_argument("Input image is empty.");
    }

    cv::Mat resized;
    cv::resize(bgr_image, resized, cv::Size(m_input_width, m_input_height));

    cv::Mat rgb;
    cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);

    ncnn::Mat input = ncnn::Mat::from_pixels(
        rgb.data,
        ncnn::Mat::PIXEL_RGB,
        m_input_width,
        m_input_height
    );
    input.substract_mean_normalize(m_mean_vals, m_norm_vals);

    ncnn::Extractor ex = m_net.create_extractor();
    ex.input(m_input_name.c_str(), input);

    ncnn::Mat logits;
    ex.extract(m_output_name.c_str(), logits);

    // Softmax
    std::vector<float> probs;
    float max_val = -std::numeric_limits<float>::infinity();
    for (int i = 0; i < logits.w; ++i) {
        if (logits[i] > max_val) max_val = logits[i];
    }

    float sum_exp = 0.0f;
    for (int i = 0; i < logits.w; ++i) {
        float e = std::exp(logits[i] - max_val);
        probs.push_back(e);
        sum_exp += e;
    }
    for (auto& p : probs) {
        p /= sum_exp;
    }

    int class_id = Argmax(logits);
    std::string label = (class_id >= 0 && class_id < static_cast<int>(m_labels.size()))
                        ? m_labels[class_id]
                        : std::to_string(class_id);

    return {label, probs};
}


void lq_ncnn_demo(void)
{
    try {
        std::cout << "========== NCNN 环境自检(无模型) ==========" << std::endl;

        // 创建 ncnn::Net
        ncnn::Net net;
        net.opt.num_threads = 2;
        net.opt.use_vulkan_compute = false;  // 基础检查

        // 创建测试输入张量，验证 Mat 内存与基础操作
        ncnn::Mat input(8, 8, 3); // w, h, c
        for (int c = 0; c < input.c; ++c) {
            float* ptr = input.channel(c);
            for (int i = 0; i < input.w * input.h; ++i) {
                ptr[i] = static_cast<float>(c * 100 + i);
            }
        }

        ncnn::Mat input_clone = input.clone();
        float checksum = 0.0f;
        for (int c = 0; c < input_clone.c; ++c) {
            const float* ptr = input_clone.channel(c);
            for (int i = 0; i < input_clone.w * input_clone.h; ++i) {
                checksum += ptr[i];
            }
        }

        // 创建提取器
        ncnn::Extractor ex = net.create_extractor();

        // 未加载模型时，输入层不存在，返回非 0 属于预期行为
        int ret = ex.input("data", input_clone);

        std::cout << "Mat 维度: w=" << input_clone.w
                  << ", h=" << input_clone.h
                  << ", c=" << input_clone.c << std::endl;
        std::cout << "Mat 校验和: " << checksum << std::endl;
        std::cout << "Extractor input 返回值(无模型预期非0): " << ret << std::endl;

        if (ret != 0) {
            std::cout << "[PASS] ncnn 基础环境可用（头文件/链接/运行时对象均正常）。" << std::endl;
            return;
        }

        std::cout << "[WARN] 未加载模型却返回0，建议进一步检查 ncnn 版本与调用逻辑。" << std::endl;
        return;
    } catch (const std::exception& e) {
        std::cerr << "[FAIL] ncnn 环境自检异常: " << e.what() << std::endl;
        return;
    } catch (...) {
        std::cerr << "[FAIL] ncnn 环境自检出现未知异常" << std::endl;
        return;
    }
}







void lq_ncnn_photo_demo(void)
{
    printf("========================================\n");
    printf("       NCNN 图像分类推理示例\n");
    printf("========================================\n\n");

    // ==================== 配置区域（已按你的模型修改） ====================
    std::string test_image_path = "/home/root/test.bmp";  // 改成你的测试图绝对路径
    
    // 模型文件（已传到 /home/root/）
    std::string model_param = "/home/root/tiny_classifier_fp32.ncnn.param";
    std::string model_bin   = "/home/root/tiny_classifier_fp32.ncnn.bin";
    
    // 输入尺寸：必须和训练时一致！
    int input_width  = 48;
    int input_height = 48;
    
    // 类别标签（顺序必须和训练时 labels.txt 一致）
std::vector<std::string> labels = {"materials", "traffic", "weapon_1", "weapon_2"};
    
    // 新：对应 Normalize([0.499870,0.457163,0.433055], [0.094522,0.164531,0.162088])
    // 公式：mean_val = mean * 255,  norm_val = 1 / (std * 255)
    float mean_vals[3] = {127.466850f, 116.576565f, 110.429025f};
    float norm_vals[3] = {0.041489f, 0.023834f, 0.024194f};
    // =================================================

    // 创建NCNN对象并配置
    LQ_NCNN ncnn;
    ncnn.SetModelPath(model_param, model_bin);
    ncnn.SetInputSize(input_width, input_height);
    ncnn.SetLabels(labels);
    ncnn.SetNormalize(mean_vals, norm_vals);

    // 初始化模型
    printf("[%s] 正在加载模型...\n", get_time_str().c_str());
    if (!ncnn.Init()) {
        printf("[%s] 模型加载失败!\n", get_time_str().c_str());
        return;
    }
    printf("[%s] 模型加载成功!\n\n", get_time_str().c_str());

    // 读取测试图片
    printf("[%s] 读取图片: %s\n", get_time_str().c_str(), test_image_path.c_str());
    cv::Mat image = cv::imread(test_image_path);

    if (image.empty()) {
        printf("无法读取图片: %s\n", test_image_path.c_str());
        printf("请确保图片文件存在\n");
        return;
    }
    printf("[%s] 图片尺寸: %d x %d\n\n", get_time_str().c_str(), image.cols, image.rows);

    // 推理
    printf("[%s] 开始推理...\n", get_time_str().c_str());
    auto start = std::chrono::high_resolution_clock::now();
    std::string result = ncnn.Infer(image);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // 输出结果
    printf("\n========================================\n");
    printf("推理结果: %s\n", result.c_str());
    printf("推理耗时: %ld ms\n", duration.count());
    printf("========================================\n");

    return;
}



//lq_ntp部分

/********************************************************************************
 * @fn      lq_ntp::lq_ntp();
 * @brief   构造函数，用于初始化 lq_ntp 对象.
 * @param   none.
 * @return  none.
 * @date    2026-03-13.
 ********************************************************************************/
lq_ntp::lq_ntp()
{
    std::lock_guard<std::mutex> lock(this->mtx_);
    // 获取网络时间
    uint32_t unix_time = 0;
    if (this->get_ntp_time(&(unix_time)) != 0) {
        lq_log_warn("获取网络时间失败，使用系统默认时间");
        return;
    }
    // 自动 +8 小时 → 北京时间
    unix_time += this->ntp_time_zone_offset_;
    // 设置系统时间
    if (this->set_system_time(unix_time) != 0) {
        lq_log_error("设置系统时间失败");
        return;
    }
    // 同步到硬件时间
    this->sync_hw_clock();
    lq_log_info("同步 NTP 时间成功");
}

/********************************************************************************
 * @fn      lq_ntp::~lq_ntp();
 * @brief   析构函数，用于释放 lq_ntp 对象所占用的资源.
 * @param   none.
 * @return  none.
 * @date    2026-03-13.
 ********************************************************************************/
lq_ntp::~lq_ntp()
{
}

/********************************************************************************
 * @fn      bool lq_ntp::sync_ntp_time(void);
 * @brief   主动同步时间，重新从 NTP 服务器获取时间并设置系统时间.
 * @param   none.
 * @return  true: 成功, false: 失败.
 * @date    2026-03-13.
 ********************************************************************************/
bool lq_ntp::sync_ntp_time(void)
{
    std::lock_guard<std::mutex> lock(this->mtx_);
    uint32_t unix_time = 0;
    if (this->get_ntp_time(&unix_time) != 0) {
        lq_log_error("NTP 时间同步失败");
        return false;
    }
    uint32_t local_time = unix_time + this->ntp_time_zone_offset_;
    this->set_system_time(local_time);
    this->sync_hw_clock();
    lq_log_info("NTP 时间同步成功");
    return true;
}

/********************************************************************************
 * @fn      uint64_t lq_ntp::get_local_time_s(void);
 * @brief   获取以秒为单位时间戳.
 * @param   none.
 * @return  以为秒为单位的时间戳.
 * @date    2026-03-13.
 ********************************************************************************/
uint64_t lq_ntp::get_local_time_s(void)
{
    std::lock_guard<std::mutex> lock(this->mtx_);
    struct timeval tv{};
    gettimeofday(&tv, nullptr);
    return (uint64_t)tv.tv_sec;
}

/********************************************************************************
 * @fn      uint64_t lq_ntp::get_local_time_ms(void);
 * @brief   获取以毫秒为单位时间戳.
 * @param   none.
 * @return  以毫秒为单位的时间戳.
 * @date    2026-03-13.
 ********************************************************************************/
uint64_t lq_ntp::get_local_time_ms(void)
{
    std::lock_guard<std::mutex> lock(this->mtx_);
    struct timeval tv{};
    gettimeofday(&tv, nullptr);
    return (uint64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

/********************************************************************************
 * @fn      uint64_t lq_ntp::get_local_time_us(void);
 * @brief   获取以微秒为单位时间戳.
 * @param   none.
 * @return  以微秒为单位的时间戳.
 * @date    2026-03-13.
 ********************************************************************************/
uint64_t lq_ntp::get_local_time_us(void)
{
    std::lock_guard<std::mutex> lock(this->mtx_);
    struct timeval tv{};
    gettimeofday(&tv, nullptr);
    return (uint64_t)(tv.tv_sec * 1000000 + tv.tv_usec);
}

/********************************************************************************
 * @fn      std::string lq_ntp::get_local_time_str(void);
 * @brief   获取本地时间，格式：YYYY-MM-DD HH:MM:SS.
 * @param   none.
 * @return  本地时间字符串.
 * @date    2026-03-13.
 ********************************************************************************/
std::string lq_ntp::get_local_time_str(void)
{
    std::lock_guard<std::mutex> lock(this->mtx_);
    time_t now = time(nullptr);
    struct tm local_time{};
    localtime_r(&now, &local_time);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local_time);
    return std::string(buffer);
}

/********************************************************************************
 * @fn      std::string lq_ntp::get_local_year(uint32_t *_year);
 * @brief   获取本地时间戳的年份.
 * @param   _year: 输出参数，用于存储年份.
 * @return  std::string: 返回获取到的年份字符串.
 * @date    2026-03-13.
 ********************************************************************************/
std::string lq_ntp::get_local_year(uint32_t *_year)
{
    struct tm ntp_time{};
    this->get_system_tm(ntp_time);
    if (_year != nullptr) {
        *_year = ntp_time.tm_year + 1900;
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", ntp_time.tm_year + 1900);
    return std::string(buf);
}

/********************************************************************************
 * @fn      std::string lq_ntp::get_local_month(uint8_t *_month);
 * @brief   获取本地时间戳的月份.
 * @param   _month: 输出参数，用于存储月份.
 * @return  std::string: 返回月份的字符串表示.
 * @date    2026-03-13.
 ********************************************************************************/
std::string lq_ntp::get_local_month(uint8_t *_month)
{
    struct tm ntp_time{};
    this->get_system_tm(ntp_time);
    if (_month != nullptr) {
        *_month = ntp_time.tm_mon + 1;
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "%02u", ntp_time.tm_mon + 1);
    return std::string(buf);
}

/********************************************************************************
 * @fn      std::string lq_ntp::get_local_day(uint16_t *_day);
 * @brief   获取本地时间戳的日期.
 * @param   _day: 输出参数，用于存储日期.
 * @return  std::string: 返回日期字符串.
 * @date    2026-03-13.
 ********************************************************************************/
std::string lq_ntp::get_local_day(uint16_t *_day)
{
    struct tm ntp_time{};
    this->get_system_tm(ntp_time);
    if (_day != nullptr) {
        *_day = ntp_time.tm_mday;
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "%02u", ntp_time.tm_mday);
    return std::string(buf);
}

/********************************************************************************
 * @fn      std::string lq_ntp::get_local_hour(uint8_t *_hour);
 * @brief   获取本地时间戳的小时.
 * @param   _hour: 输出参数，用于存储小时.
 * @return  std::string: 返回格式化后的小时字符串.
 * @date    2026-03-13.
 ********************************************************************************/
std::string lq_ntp::get_local_hour(uint8_t *_hour)
{
    struct tm ntp_time{};
    this->get_system_tm(ntp_time);
    if (_hour != nullptr) {
        *_hour = ntp_time.tm_hour;
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "%02u", ntp_time.tm_hour);
    return std::string(buf);
}

/********************************************************************************
 * @fn      std::string lq_ntp::get_local_minute(uint8_t *_minute);
 * @brief   获取本地时间戳的分钟.
 * @param   _minute: 输出参数，用于存储分钟.
 * @return  std::string: 返回格式化后的分钟字符串.
 * @date    2026-03-13.
 ********************************************************************************/
std::string lq_ntp::get_local_minute(uint8_t *_minute)
{
    struct tm ntp_time{};
    this->get_system_tm(ntp_time);
    if (_minute != nullptr) {
        *_minute = ntp_time.tm_min;
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "%02u", ntp_time.tm_min);
    return std::string(buf);
}

/********************************************************************************
 * @fn      std::string lq_ntp::get_local_second(uint8_t *_second);
 * @brief   获取本地时间戳的秒数.
 * @param   _second: 输出参数，用于存储秒数.
 * @return  std::string: 返回格式化后的字符串.
 * @date    2026-03-13.
 ********************************************************************************/
std::string lq_ntp::get_local_second(uint8_t *_second)
{
    struct tm ntp_time{};
    this->get_system_tm(ntp_time);
    if (_second != nullptr) {
        *_second = ntp_time.tm_sec;
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "%02u", ntp_time.tm_sec);
    return std::string(buf);
}

/********************************************************************************
 * @fn      int lq_ntp::get_ntp_time(uint32_t *_unix_time);
 * @brief   从 NTP 服务器获取 Unix 时间戳.
 * @param   _unix_time: 输出参数，用于存储获取到的 Unix 时间戳.
 * @return  0: 获取成功; -1: 获取失败.
 * @date    2026-03-13.
 ********************************************************************************/
int lq_ntp::get_ntp_time(uint32_t *_unix_time)
{
    if (_unix_time == nullptr) {
        lq_log_error("Invalid pointer");
        return -1;
    }
    uint8_t ntp_packet[48] = {0};   // NTP 数据包固定 48 字节
    // 创建 UDP 套接字
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        lq_log_error("socket failed");
        return -1;
    }
    // 超时设置
    struct timeval tv{};
    tv.tv_sec  = 1;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    // 直接填入 IP 地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(ntp_port_);
    if (inet_pton(AF_INET, ntp_server_.c_str(), &server_addr.sin_addr) < 1) {
        lq_log_error("inet_pton failed");
        close(sockfd);
        return -1;
    }
    // NTP 协议要求：第一个字节 = 0x1B
    ntp_packet[0] = 0x1B;
    // 发送请求
    if (sendto(sockfd, ntp_packet, sizeof(ntp_packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        lq_log_error("sendto failed");
        close(sockfd);
        return -1;
    }
    // 接收响应
    if (recv(sockfd, ntp_packet, sizeof(ntp_packet), 0) < 0) {
        lq_log_error("recv failed");
        close(sockfd);
        return -1;
    }
    // 提取 NTP 时间戳（网络字节序转主机序）
    uint32_t ntp_time = (ntohl(*((uint32_t*)(ntp_packet + 40))));
    // 转换未 Unix 时间戳
    *_unix_time = ntp_time - ntp_timestamp_delta_;
    close(sockfd);
    return 0;
}

/********************************************************************************
 * @fn      int lq_ntp::set_system_time(uint32_t _unix_time);
 * @brief   设置系统时间.
 * @param   _unix_time: 要设置的 Unix 时间戳.
 * @return  0: 设置成功; -1: 设置失败.
 * @date    2026-03-13.
 ********************************************************************************/
int lq_ntp::set_system_time(uint32_t _unix_time)
{
    struct timeval tv{};
    tv.tv_sec  = _unix_time;
    tv.tv_usec = 0;
    // 设置系统时间
    if (settimeofday(&tv, nullptr) < 0) {
        lq_log_error("settimeofday failed");
        return -1;
    }
    return 0;
}

/********************************************************************************
 * @fn      void lq_ntp::sync_hw_clock(void);
 * @brief   同步到硬件时间.
 * @param   none.
 * @return  none.
 * @date    2026-03-13.
 ********************************************************************************/
void lq_ntp::sync_hw_clock(void)
{
    // 同步到硬件时间
    system("hwclock -w");
}

/********************************************************************************
 * @fn      bool lq_ntp::get_system_tm(struct tm& out_tm);
 * @brief   获取系统时间的 tm 结构体.
 * @param   out_tm: 输出参数，用于存储获取到的 tm 结构体.
 * @return  true: 获取成功，false: 获取失败.
 * @date    2026-03-13.
 ********************************************************************************/
bool lq_ntp::get_system_tm(struct tm& out_tm)
{
    time_t now = time(nullptr);
    if (localtime_r(&now, &out_tm) == nullptr) {
        lq_log_error("获取系统时间失败");
        return false;
    }
    return true;
}


//lq_ncnn
/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@编   写：龙邱科技
@邮   箱：chiusir@163.com
@编译IDE：Linux 环境、VSCode_1.93 及以上版本、Cmake_3.16 及以上版本
@使用平台：龙芯2K0300久久派和北京龙邱智能科技龙芯久久派拓展板
@相关信息参考下列地址
    网      站：http://www.lqist.cn
    淘 宝 店 铺：http://longqiu.taobao.com
    程序配套视频：https://space.bilibili.com/95313236
@软件版本：V1.0 版权所有，单位使用请先联系授权
@参考项目链接：https://github.com/AirFortressIlikara/ls2k0300_peripheral_library

@修改日期：2025-03-03
@修改内容：整理代码风格，与其他驱动库保持一致
@注意事项：注意查看路径的修改
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/



/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 * @函数名称：LQ_NCNN::LQ_NCNN()
 * @功能说明：NCNN类的构造函数
 * @参数说明：无
 * @函数返回：无
 * @调用方法：LQ_NCNN ncnn;
 * @备注说明：初始化默认参数
 QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
LQ_NCNN::LQ_NCNN()
    : m_initialized(false)
    , m_input_width(96)
    , m_input_height(96)
    , m_input_name("in0")
    , m_output_name("out0")
{
    // 默认使用ImageNet标准归一化
    m_mean_vals[0] = 123.675f;
    m_mean_vals[1] = 116.28f;
    m_mean_vals[2] = 103.53f;
    m_norm_vals[0] = 0.01712475f;
    m_norm_vals[1] = 0.017507f;
    m_norm_vals[2] = 0.01742919f;
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 * @函数名称：bool LQ_NCNN::Init()
 * @功能说明：初始化NCNN模型
 * @参数说明：无
 * @函数返回：true:成功 false:失败
 * @调用方法：ncnn.Init();
 * @备注说明：加载模型文件和参数
 QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
bool LQ_NCNN::Init()
{
    // 设置NCNN选项
    m_net.opt.use_vulkan_compute = false;
    m_net.opt.num_threads = 1;

    // 加载模型参数
    if (m_param_path.empty() || m_net.load_param(m_param_path.c_str()) != 0) {
        printf("NCNN: 加载参数文件失败: %s\n", m_param_path.c_str());
        return false;
    }

    // 加载模型权重
    if (m_bin_path.empty() || m_net.load_model(m_bin_path.c_str()) != 0) {
        printf("NCNN: 加载模型文件失败: %s\n", m_bin_path.c_str());
        return false;
    }

    m_initialized = true;
    printf("NCNN: 模型加载成功 (%s)\n", m_param_path.c_str());

    return true;
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 * @函数名称：std::string LQ_NCNN::Infer(const cv::Mat& bgr_image)
 * @功能说明：推理函数
 * @参数说明：bgr_image : 输入BGR图像
 * @函数返回：推理结果（类别名称或编号）
 * @调用方法：std::string result = ncnn.Infer(image);
 * @备注说明：无
 QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
std::string LQ_NCNN::Infer(const cv::Mat& bgr_image)
{
    if (!m_initialized) {
        throw std::runtime_error("NCNN not initialized. Call Init() first.");
    }
    if (bgr_image.empty()) {
        throw std::invalid_argument("Input image is empty.");
    }

    // 调整图像尺寸
    cv::Mat resized;
    cv::resize(bgr_image, resized, cv::Size(m_input_width, m_input_height));

    // 转换色彩空间: BGR -> RGB
    // 训练时使用RGB格式(OpenCV读取默认是BGR，需要转换)
    cv::Mat rgb;
    cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);

    // 转换为NCNN格式
    ncnn::Mat input = ncnn::Mat::from_pixels(
        rgb.data,
        ncnn::Mat::PIXEL_RGB,
        m_input_width,
        m_input_height
    );
    input.substract_mean_normalize(m_mean_vals, m_norm_vals);

    // 创建提取器并推理
    ncnn::Extractor ex = m_net.create_extractor();
    ex.input(m_input_name.c_str(), input);

    ncnn::Mat logits;
    ex.extract(m_output_name.c_str(), logits);

    // 获取最大概率类别
    int class_id = Argmax(logits);
    if (class_id < 0) {
        throw std::runtime_error("Failed to get class id from output logits.");
    }

    // 返回类别名称或编号
    if (class_id >= 0 && class_id < static_cast<int>(m_labels.size())) {
        return m_labels[class_id];
    }
    return std::to_string(class_id);
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 * @函数名称：void LQ_NCNN::SetModelPath(const std::string& param_path, const std::string& bin_path)
 * @功能说明：设置模型路径
 * @参数说明：param_path : 模型参数文件路径
 * @参数说明：bin_path   : 模型权重文件路径
 * @函数返回：无
 * @调用方法：ncnn.SetModelPath("model.ncnn.param", "model.ncnn.bin");
 * @备注说明：应在Init()之前调用
 QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
void LQ_NCNN::SetModelPath(const std::string& param_path, const std::string& bin_path)
{
    m_param_path = param_path;
    m_bin_path = bin_path;
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 * @函数名称：void LQ_NCNN::SetInputSize(int width, int height)
 * @功能说明：设置输入尺寸
 * @参数说明：width  : 输入宽度
 * @参数说明：height : 输入高度
 * @函数返回：无
 * @调用方法：ncnn.SetInputSize(224, 224);
 * @备注说明：应在Init()之前调用
 QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
void LQ_NCNN::SetInputSize(int width, int height)
{
    m_input_width = width;
    m_input_height = height;
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 * @函数名称：void LQ_NCNN::SetLabels(const std::vector<std::string>& labels)
 * @功能说明：设置类别标签
 * @参数说明：labels : 类别名称列表
 * @函数返回：无
 * @调用方法：ncnn.SetLabels({"cat", "dog", "bird"});
 * @备注说明：应在Init()之前调用
 QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
void LQ_NCNN::SetLabels(const std::vector<std::string>& labels)
{
    m_labels = labels;
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 * @函数名称：void LQ_NCNN::SetNormalize(const float mean_vals[3], const float norm_vals[3])
 * @功能说明：设置归一化参数
 * @参数说明：mean_vals  : 均值
 * @参数说明：norm_vals : 标准差
 * @函数返回：无
 * @调用方法：ncnn.SetNormalize(mean, norm);
 * @备注说明：应在Init()之前调用
 QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
void LQ_NCNN::SetNormalize(const float mean_vals[3], const float norm_vals[3])
{
    m_mean_vals[0] = mean_vals[0];
    m_mean_vals[1] = mean_vals[1];
    m_mean_vals[2] = mean_vals[2];
    m_norm_vals[0] = norm_vals[0];
    m_norm_vals[1] = norm_vals[1];
    m_norm_vals[2] = norm_vals[2];
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 * @函数名称：int LQ_NCNN::Argmax(const ncnn::Mat& logits)
 * @功能说明：获取最大概率类别索引
 * @参数说明：logits : 模型输出
 * @函数返回：类别索引
 * @调用方法：内部调用
 * @备注说明：无
 QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
int LQ_NCNN::Argmax(const ncnn::Mat& logits)
{
    if (logits.w <= 0) {
        return -1;
    }

    int best_index = 0;
    float best_value = -std::numeric_limits<float>::infinity();

    for (int i = 0; i < logits.w; ++i) {
        const float value = logits[i];
        if (value > best_value) {
            best_value = value;
            best_index = i;
        }
    }
    return best_index;
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 * @函数名称：LQ_NCNN::~LQ_NCNN()
 * @功能说明：NCNN类的析构函数
 * @参数说明：无
 * @函数返回：无
 * @调用方法：创建的对象生命周期结束后系统自动调用
 * @备注说明：无
 QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
LQ_NCNN::~LQ_NCNN()
{
    // NCNN会自动释放资源
}
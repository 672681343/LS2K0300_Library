#pragma once
#include "zf_common_headfile.h"

#include <iostream>
#include <string>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>
#include <mutex>

#include <vector>
#include <opencv2/core.hpp>
#include <ncnn/net.h>

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

void lq_ncnn_demo(void);            // NCNN 测试
void lq_ncnn_photo_demo(void);      // NCNN 图像分类测试


class lq_ntp
{
public:
    lq_ntp();       // 构造函数
    ~lq_ntp();      // 析构函数

    lq_ntp(const lq_ntp &ntp) = delete;             // 复制构造函数
    lq_ntp &operator=(const lq_ntp &ntp) = delete;  // 赋值运算符
    lq_ntp(lq_ntp &&ntp) = delete;                  // 移动构造函数
    lq_ntp &operator=(lq_ntp &&ntp) = delete;       // 移动赋值运算符

public:
    uint64_t get_local_time_s (void);   // 获取以秒为单位时间戳
    uint64_t get_local_time_ms(void);   // 获取以毫秒为单位时间戳
    uint64_t get_local_time_us(void);   // 获取以微秒为单位时间戳

    std::string get_local_time_str  (void);             // 获取本地时间，格式：YYYY-MM-DD HH:MM:SS
    std::string get_local_year      (uint32_t *_year);  // 获取本地时间戳的年份
    std::string get_local_month     (uint8_t  *_month); // 获取本地时间戳的月份
    std::string get_local_day       (uint16_t *_day);   // 获取本地时间戳的日期
    std::string get_local_hour      (uint8_t  *_hour);  // 获取本地时间戳的小时
    std::string get_local_minute    (uint8_t  *_minute);// 获取本地时间戳的分钟
    std::string get_local_second    (uint8_t  *_second);// 获取本地时间戳的秒数

    // 主动同步时间，重新从 NTP 服务器获取时间并设置系统时间
    bool sync_ntp_time(void);

private:
    int  get_ntp_time   (uint32_t *_unix_time); // 从 NTP 服务器获取 Unix 时间戳
    int  set_system_time(uint32_t _unix_time);  // 设置系统时间
    void sync_hw_clock  (void);                 // 同步到硬件时间

    bool get_system_tm  (struct tm& out_tm);    // 获取系统时间的 tm 结构体

private:
    std::mutex        mtx_;                                     // 互斥锁
    const std::string ntp_server_           = "182.92.12.11";   // NTP 服务器地址
    const int         ntp_port_             = 123;              // NTP 端口号
    const uint64_t    ntp_timestamp_delta_  = 2208988800ULL;    // NTP 时间戳偏移量，单位：秒
    const uint32_t    ntp_time_zone_offset_ = (8 * 3600);       // NTP 时区偏移量，单位：秒
};



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


/*!
 * @brief NCNN推理类
 * @details 用于加载NCNN模型并进行图像分类推理
 */
class LQ_NCNN
{
public:
    /*!
     * @brief   NCNN无参构造函数
     */
    LQ_NCNN();

    /*!
     * @brief   初始化NCNN模型
     * @return  true:成功 false:失败
     */
    bool Init();

    /*!
     * @brief   推理函数
     * @param   bgr_image : 输入BGR图像
     * @return  推理结果（类别名称或编号）
     */
    std::string Infer(const cv::Mat& bgr_image);

    /*!
     * @brief   设置模型路径
     * @param   param_path : 模型参数文件路径(.param)
     * @param   bin_path   : 模型权重文件路径(.bin)
     */
    void SetModelPath(const std::string& param_path, const std::string& bin_path);

    /*!
     * @brief   设置输入尺寸
     * @param   width  : 输入宽度
     * @param   height : 输入高度
     */
    void SetInputSize(int width, int height);

    /*!
     * @brief   设置类别标签
     * @param   labels : 类别名称列表
     */
    void SetLabels(const std::vector<std::string>& labels);

    /*!
     * @brief   设置归一化参数
     * @param   mean_vals  : 均值
     * @param   norm_vals : 标准差
     */
    void SetNormalize(const float mean_vals[3], const float norm_vals[3]);

    /*!
     * @brief   NCNN析构函数
     */
    ~LQ_NCNN();

private:
    /*!
     * @brief   获取最大概率类别索引
     * @param   logits : 模型输出
     * @return  类别索引
     */
    int Argmax(const ncnn::Mat& logits);

private:
    ncnn::Net                 m_net;              // NCNN网络
    std::vector<std::string>  m_labels;           // 类别标签
    bool                     m_initialized;      // 初始化标志

    // 模型路径
    std::string              m_param_path;
    std::string              m_bin_path;

    // 输入参数
    int                      m_input_width;
    int                      m_input_height;
    float                    m_mean_vals[3];
    float                    m_norm_vals[3];

    // 输入输出blob名称
    std::string              m_input_name;
    std::string              m_output_name;
};



// ========== 供 process_image 直接调用的全局接口 ==========
bool ncnn_init(void);                           // main 里初始化一次
int  ncnn_perform_inference(const cv::Mat& bgr_image);  // 返回类别索引 0/1/2







/********************************************************************************
 * @brief   适配 Linux 开发板的断言与日志打印模块
 * @note    支持：编译期错误终止 + 运行期分级日志 + 文件/函数/行号/PID定位
 ********************************************************************************/



/* ==================== 辅助宏（内部使用，加下划线区分） ==================== */
// 辅助宏：将数字转为字符串（用于行号拼接）
#define _lq_stringify(x)                #x
#define _lq_to_string(x)                _lq_stringify(x)

/* ==================== 编译期强制检查 ==================== */
// 编译器断言宏（支持自定义提示，编译失败时显示信息）
#define lq_compile_assert(expr, msg)    static_assert(expr, "[" __FILE__ ":" _lq_to_string(__LINE__) "] " msg)

// 编译期错误终止宏（强制停止编译，打印文件 + 行号 + 自定义信息）
#define lq_compile_error(msg)           _Pragma(_lq_to_string(GCC error "[" __FILE__ ":" _lq_to_string(__LINE__) "] compile error: " msg))

/* ==================== 配置区 ==================== */
// 日志总开关：1-启用，0-禁用（编译期控制，0 时所有日志宏为空操作）
#define LQ_ENABLE_ASSERT_LOG            ( 1 )

// 日志级别控制：按需开启/关闭对应级别(仅打印不终止)
#define LQ_LOG_LEVEL_INFO               ( 1 )
#define LQ_LOG_LEVEL_WARN               ( 1 )
#define LQ_LOG_LEVEL_ERROR              ( 1 )

// 断言开关：1-启用（error级会触发断言），0-禁用（仅打印不终止）
#define LQ_ENABLE_ASSERT                ( 1 )

// 日志输出方式：1-标准输出（stdout），2-系统日志（syslog），3-指定文件（仅1/2/3合法）
#define LQ_LOG_OUTPUT_MODE              ( 1 )

// 若选择文件输出，指定日志文件路径（Linux开发板路径）
#define LQ_LOG_FILE_PATH                ( "/tmp/app_log.txt" )

// 最大日志缓冲区大小（必须 > 0）
#define LQ_LOG_BUF_SIZE                 ( 512 )

/* ==================== 编译期合法性检查 ==================== */
// 检查 1：日志级别只能是 0 或 1
lq_compile_assert((LQ_LOG_LEVEL_INFO  == 0 || LQ_LOG_LEVEL_INFO  == 1), "LQ_LOG_LEVEL_INFO must be 0 or 1");
lq_compile_assert((LQ_LOG_LEVEL_WARN  == 0 || LQ_LOG_LEVEL_WARN  == 1), "LQ_LOG_LEVEL_WARN must be 0 or 1");
lq_compile_assert((LQ_LOG_LEVEL_ERROR == 0 || LQ_LOG_LEVEL_ERROR == 1), "LQ_LOG_LEVEL_ERROR must be 0 or 1");

// 检查 2：断言开关只能是 0 或 1
lq_compile_assert((LQ_ENABLE_ASSERT == 0 || LQ_ENABLE_ASSERT == 1), "LQ_ENABLE_ASSERT must be 0 or 1");

// 检查 3：输出模式只能是 1/2/3
lq_compile_assert((LQ_LOG_OUTPUT_MODE >= 1 && LQ_LOG_OUTPUT_MODE <= 3), "LQ_LOG_OUTPUT_MODE must be 1, 2, or 3");

// 检查 4：缓冲区大小是否合法
lq_compile_assert((LQ_LOG_BUF_SIZE >0 && LQ_LOG_BUF_SIZE <= 1024), "LQ_LOG_BUF_SIZE must be between 1 and 1024");

// 检查 5：Linux 系统环境
#if !defined(__linux__)
lq_compile_assert("This program only supports Linux platform!");
#endif

// 检查 6：龙芯架构检查
#if !defined(__loongarch__)
lq_compile_assert("This program only supports LoongArch (Loongson) platform!");
#endif

// 检查 7：文件输出模式时路径非空
#if LQ_LOG_OUTPUT_MODE == 3
lq_compile_assert((LQ_LOG_FILE_PATH[0] != '\0'), "LQ_LOG_FILE_PATH cannot be empty when LQ_LOG_OUTPUT_MODE=3");
#endif

/********************************************************************************
 * @brief   提取文件名（去掉路径）.
 * @param   _path : 文件路径.
 * @return  去掉路径的文件名.
 ********************************************************************************/
//inline 让函数可以安全地定义在头文件里被多个源文件共用，同时建议编译器在调用处直接展开以提升性能。
inline const char* lq_get_filename(const char* _path)
{
    // 空指针保护
    if (_path == NULL || *_path == '\0') {
        return "unknow_file";
    }

    const char* filename = strrchr(_path, '/');
    return (filename != NULL) ? (filename + 1) : _path;
}

/********************************************************************************
 * @brief   Linux 平台格式化输出函数.
 * @param   _fmt : 输出内容.
 * @param   ...  : 可变参数.
 * @return  none.
 ********************************************************************************/
inline void lq_linux_printf(const char *_fmt, ...)
{
    // 空指针保护
    if (_fmt == NULL) {
        return;
    }

    va_list args;
    va_start(args, _fmt);

#if LQ_LOG_OUTPUT_MODE == 1
    // 标准输出
    vprintf(_fmt, args);
#elif LQ_LOG_OUTPUT_MODE == 2
    // 系统日志
    vsyslog(LOG_INFO, _fmt, args);
#elif LQ_LOG_OUTPUT_MODE == 3
    // 文件输出
    int fd = open(LQ_LOG_FILE_PATH, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
        char buf[LQ_LOG_BUF_SIZE] = {0};
        int len = vsnprintf(buf, sizeof(buf)-1, _fmt, args);
        len = (len < 0) ? 0 : (len >= (int)sizeof(buf)) ? ((int)sizeof(buf) - 1) : len;
        write(fd, buf, (size_t)len);
        close(fd);
    }
#endif

    va_end(args);
}

/* ==================== 核心宏定义 ==================== */
#if LQ_ENABLE_ASSERT_LOG

// 基础日志宏
#define _lq_base_log(level, fmt, ...)           do { \
                                                    lq_linux_printf("[LQ] [%s] [PID:%d] <%s:%s:%d> " fmt "\n", \
                                                        level, (int)getpid(), lq_get_filename(__FILE__), __func__, __LINE__, ##__VA_ARGS__); \
                                                } while (0)

// 分级日志宏
#if LQ_LOG_LEVEL_INFO
#define lq_log_info(fmt, ...)                   _lq_base_log("INFO ", fmt, ##__VA_ARGS__)
#else
#define lq_log_info(fmt, ...)                   do {} while (0)
#endif

#if LQ_LOG_LEVEL_WARN
#define lq_log_warn(fmt, ...)                   _lq_base_log("WARN ", fmt, ##__VA_ARGS__)
#else
#define lq_log_warn(fmt, ...)                   do {} while (0)
#endif

#if LQ_LOG_LEVEL_ERROR
#define lq_log_error(fmt, ...)                  _lq_base_log("ERROR", fmt, ##__VA_ARGS__)
#else
#define lq_log_error(fmt, ...)                  do {} while (0)
#endif

// 断言宏
#if LQ_ENABLE_ASSERT
#define lq_assert(expr)                         do { \
                                                    if (!(expr)) { \
                                                        lq_log_error("ASSERT FAILED: %s", #expr); \
                                                        assert(expr);   /* Linux 下终止进程，生成core dump */ \
                                                    } \
                                                } while (0)
#else
#define lq_assert(expr)                         do { \
                                                    if (!(expr)) { \
                                                        lq_log_error("ASSERT FAILED: %s", #expr); \
                                                    } \
                                                } while (0)
#endif

#else
// 禁用日志时的空宏
#define lq_log_info(fmt, ...)                   do {} while (0)
#define lq_log_warn(fmt, ...)                   do {} while (0)
#define lq_log_error(fmt, ...)                  do {} while (0)
#define lq_assert(expr)                         do {} while (0)

#endif // LQ_ENABLE_ASSERT_LOG

#include "Saving.h"
#include <opencv2/imgcodecs.hpp>
#include <fstream>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>  // 新增头文件
#include <unistd.h>    // 新增头文件


namespace {
    using namespace DataSaving;
    
    // 目录常量
    constexpr const char* ORIGIN_DIR  = "./image/origin";
    constexpr const char* FITTING_DIR = "./image/fitting";
    constexpr const char* DATA_DIR    = "./image/data";
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 创建上位机目录
    // 参数说明: 无
    // 返回参数: 无
    // 使用示例: create_directories();
    // 备注信息: 放在初始化防止文件夹缺失
    //-------------------------------------------------------------------------------------------------------------------
    void create_directories() {
        // 使用C标准库创建目录
        mkdir(ORIGIN_DIR, 0755);  // 权限设置为rwxr-xr-x
        mkdir(FITTING_DIR, 0755);
        mkdir(DATA_DIR, 0755);
    }
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 清空上位机文件夹
    // 参数说明: 无
    // 返回参数: 无
    // 使用示例: DataSaving::clear_folders(Car_flag.saving);
    // 备注信息: 建议放到菜单启动
    //-------------------------------------------------------------------------------------------------------------------
    void clear_single_folder(const char* path) {
        DIR *dir;
        struct dirent *entry;
        char full_path[1024];
    
        // 打开目标目录
        if (!(dir = opendir(path))) {
            fprintf(stderr, "[ERROR] Cannot open directory %s: %s\n", path, strerror(errno));
            return;
        }
    
        // 遍历目录项
        while ((entry = readdir(dir)) != NULL) {
            // 跳过特殊目录项
            if (strcmp(entry->d_name, ".") == 0 || 
                strcmp(entry->d_name, "..") == 0) {
                continue;
            }
    
            // 构建完整路径
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
            
            // 删除文件或空目录
            if (remove(full_path) != 0) {
                fprintf(stderr, "[WARNING] Failed to delete %s: %s\n", 
                        full_path, strerror(errno));
            }
        }
    
        // 关闭目录句柄
        closedir(dir);
    }
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 构建路径
    // 参数说明: 填写的地址字符串，尺寸，地址字符串，场数量，后缀
    // 返回参数: 地址字符串
    // 使用示例: build_path(path_buf, sizeof(path_buf), DATA_DIR, index, ".txt");
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    char* build_path(char* buffer, size_t size, const char* dir, int index, const char* ext) {
        snprintf(buffer, size, "%s/%d%s", dir, index, ext);
        return buffer;
    }

    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 上位机参数保存（显式字段版本）
    // 参数说明: 场数量，左边线，中线，右线，路径
    // 返回参数: 生成固定格式txt文件
    // 使用示例: clear_single_folder(DATA_DIR);
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    void data_save(int index,
        const std::array<uint16_t, HEIGHT>& l_black,
        const std::array<uint16_t, HEIGHT>& mid_line,
        const std::array<uint16_t, HEIGHT>& r_black,
        const std::array<uint16_t, HEIGHT>& fit_l_black,
        const std::array<uint16_t, HEIGHT>& fit_r_black) {
        char path_buf[256];
        build_path(path_buf, sizeof(path_buf), DATA_DIR, index, ".txt");

        std::ofstream file(path_buf);
        if (!file) return;

        // 拟线数据部分
        file << "[拟线数据]\n";
        file << "左始=";
        for (auto val : fit_l_black) file << val << " ";
        file << "\n左线=";
        for (auto val : l_black) file << val << " ";
        file << "\n左末=";
        for (size_t i = 0; i < HEIGHT; ++i) file << "255 ";
        file << "\n中线=";
        for (auto val : mid_line) file << val << " ";
        file << "\n右始=";
        for (auto val : fit_r_black) file << val << " ";
        file << "\n右线=";
        for (auto val : r_black) file << val << " ";
        file << "\n右末=";
        for (size_t i = 0; i < HEIGHT; ++i) file << "255 ";
        file << "\n\n";

        // 反馈数据部分
        file << "[反馈数据]\n";
        int file_index = 0; // 初始化序号计数器
        #if Saving_param
            file << file_index++ << "=" << static_cast<int>(Error::Err.current) << "\n";
            file << file_index++ << "=" << static_cast<int>(Error::Err.tem_row) << "\n";

            // file << file_index++ << "=" << static_cast<int>(Servo::duty.cur) << "\n";
            file << file_index++ << "=" << static_cast<int>(Encoder::count.L) << "\n";    
            file << file_index++ << "=" << static_cast<int>(Encoder::count.R) << "\n";

            file << file_index++ << "=" << static_cast<int>(ImageProc::state.start_point) << "\n";
            file << file_index++ << "=" << static_cast<int>(ImageProc::state.cutoff) << "\n";

            file << file_index++ << "=" << static_cast<int>(ImageProc::Left_BF.LostParam.start) << "\n";
            file << file_index++ << "=" << static_cast<int>(ImageProc::Left_BF.LostParam.end) << "\n";
            file << file_index++ << "=" << static_cast<int>(ImageProc::Left_BF.LostParam.sum) << "\n";
            file << file_index++ << "=" << static_cast<int>(ImageProc::Right_BF.LostParam.start) << "\n";
            file << file_index++ << "=" << static_cast<int>(ImageProc::Right_BF.LostParam.end) << "\n";
            file << file_index++ << "=" << static_cast<int>(ImageProc::Right_BF.LostParam.sum) << "\n";
            file << file_index++ << "=" << static_cast<int>(ImageProc::Mid_BF.LostParam.start) << "\n";
            file << file_index++ << "=" << static_cast<int>(ImageProc::Mid_BF.LostParam.end) << "\n";
            file << file_index++ << "=" << static_cast<int>(ImageProc::Mid_BF.LostParam.sum) << "\n";

            file << file_index++ << "=" << static_cast<int>(ImageProc::sec_loss_left.LostParam.start) << "\n";
            file << file_index++ << "=" << static_cast<int>(ImageProc::sec_loss_left.LostParam.end) << "\n";
            file << file_index++ << "=" << static_cast<int>(ImageProc::sec_loss_right.LostParam.start) << "\n";
            file << file_index++ << "=" << static_cast<int>(ImageProc::sec_loss_right.LostParam.end) << "\n";
            file << file_index++ << "=" << static_cast<int>(ImageProc::sec_loss_both.LostParam.start) << "\n";
            file << file_index++ << "=" << static_cast<int>(ImageProc::sec_loss_both.LostParam.end) << "\n";

            file << file_index++ << "=" << static_cast<int>(ImageProc::Left_BF.Var) << "\n";
            file << file_index++ << "=" << static_cast<int>(ImageProc::Right_BF.Var) << "\n";

            file << file_index++ << "=" << static_cast<int>(Elem::Left_corner.U_y) << "\n";
            file << file_index++ << "=" << static_cast<int>(Elem::Left_corner.D_y) << "\n";
            file << file_index++ << "=" << static_cast<int>(Elem::Right_corner.U_y) << "\n";
            file << file_index++ << "=" << static_cast<int>(Elem::Right_corner.D_y) << "\n";

            file << file_index++ << "=" << static_cast<int>(Elem::Left_corner.Curve_Row) << "\n";
            file << file_index++ << "=" << static_cast<int>(Elem::Right_corner.Curve_Row) << "\n";

            file << file_index++ << "=" << static_cast<int>(my_max(Elem::Right_Island.State,Elem::Left_Island.State)) << "\n";
            file << file_index++ << "=" << static_cast<int>(Elem::Left_Island.similar_flag) << "\n";
            file << file_index++ << "=" << static_cast<int>(Elem::Right_Island.similar_flag) << "\n";
            file << file_index++ << "=" << static_cast<int>(Elem::Cross.State) << "\n";
            file << file_index++ << "=" << static_cast<int>(Elem::Cross.similar_flag) << "\n";

            file << file_index++ << "=" << static_cast<int>(Elem::Zebra.State) << "\n";
            file << file_index++ << "=" << static_cast<int>(Elem::Zebra.y) << "\n";

            file << file_index++ << "=" << static_cast<int>(my_max(Elem::Left_Tilt_Cross.State,Elem::Right_Tilt_Cross.State)) << "\n";
            file << file_index++ << "=" << static_cast<int>(Elem::Left_Tilt_Cross.similar_flag) << "\n";
            file << file_index++ << "=" << static_cast<int>(Elem::Right_Tilt_Cross.similar_flag) << "\n";
            file << file_index++ << "=" << static_cast<int>(Elem::black_count) << "\n";

            file << file_index++ << "=" << static_cast<int>(Control::RoadSegment_t) << "\n";
            file << file_index++ << "=" << static_cast<float>(ImageProc::Mid_BF.Slope) << "\n";
            file << file_index++ << "=" << static_cast<int>(ImageProc::Mid_BF.Var) << "\n";
            file << file_index++ << "=" << static_cast<float>(Elem::draw_slope) << "\n";
            file << file_index++ << "=" << static_cast<float>(ImageProc::Left_BF.Slope) << "\n";
            file << file_index++ << "=" << static_cast<float>(ImageProc::Right_BF.Slope) << "\n";

            file << file_index++ << "=" << static_cast<int>(Elem::raw_count) << "\n";
            file << file_index++ << "=" << static_cast<float>(Elem::roadblack_slope) << "\n";
            file << file_index++ << "=" << static_cast<int>(my_max(Elem::Left_Roadblock.State,Elem::Right_Roadblock.State)) << "\n";

            // file << file_index++ << "=" << static_cast<int>(Elem::Ramp.y) << "\n";
            // file << file_index++ << "=" << static_cast<int>(Elem::Ramp.State) << "\n";
        #else 
            // file << file_index++ << "=" << static_cast<int>(Servo::duty.cur) << "\n";
            file << file_index++ << "=" << static_cast<int>(Error::Err.tem_row) << "\n";
            file << file_index++ << "=" << static_cast<int>(Control::RoadSegment_t) << "\n";

            file << file_index++ << "=" << static_cast<int>(Error::Err.current) << "\n";
            file << file_index++ << "=" << static_cast<int>(PID::turn_pid.prev_error) << "\n";
            
            file << file_index++ << "=" << static_cast<int>(PID::turn_pid.p_term) << "\n";
            file << file_index++ << "=" << static_cast<int>(PID::turn_pid.d_term) << "\n"; 
            // file << file_index++ << "=" << static_cast<int>(Servo::duty.cal) << "\n";

            
            file << file_index++ << "=" << static_cast<int>(Control::aim_speed.cur_L) << "\n";
            file << file_index++ << "=" << static_cast<int>(Encoder::count.L) << "\n";
            file << file_index++ << "=" << static_cast<int>(PID::speed_pid_L.prev_error) << "\n";
            file << file_index++ << "=" << static_cast<int>(PID::speed_pid_L.p_term) << "\n";
            file << file_index++ << "=" << static_cast<int>(PID::speed_pid_L.i_term) << "\n";
            file << file_index++ << "=" << static_cast<int>(PID::speed_pid_L.d_term) << "\n";
            file << file_index++ << "=" << static_cast<int>(Control::pwm.L) << "\n";

            file << file_index++ << "=" << static_cast<int>(Control::aim_speed.cur_R) << "\n";
            file << file_index++ << "=" << static_cast<int>(Encoder::count.R) << "\n";
            file << file_index++ << "=" << static_cast<int>(PID::speed_pid_R.prev_error) << "\n";
            file << file_index++ << "=" << static_cast<int>(PID::speed_pid_R.p_term) << "\n";
            file << file_index++ << "=" << static_cast<int>(PID::speed_pid_R.i_term) << "\n";
            file << file_index++ << "=" << static_cast<int>(PID::speed_pid_R.d_term) << "\n";
            file << file_index++ << "=" << static_cast<int>(Control::pwm.R) << "\n";

            file << file_index++ << "=" << static_cast<int>(Elem::Ramp.y) << "\n";
            file << file_index++ << "=" << static_cast<int>(Elem::Ramp.State) << "\n";
            file << file_index++ << "=" << static_cast<int>(Elem::raw_count) << "\n";

            file << file_index++ << "=" << static_cast<float>(Elem::Ramp.R_UP_SLPOE) << "\n";
            file << file_index++ << "=" << static_cast<float>(Elem::Ramp.R_DOWN_SLPOE) << "\n";
            file << file_index++ << "=" << static_cast<float>(Elem::Ramp.L_UP_SLPOE) << "\n";
            file << file_index++ << "=" << static_cast<float>(Elem::Ramp.L_DOWN_SLPOE) << "\n";
            file << file_index++ << "=" << static_cast<float>(Elem::Ramp.L_slope) << "\n";
            file << file_index++ << "=" << static_cast<float>(Elem::Ramp.R_slope) << "\n";

            file << file_index++ << "=" << static_cast<int>(Error::Err.current) << "\n";
            file << file_index++ << "=" << static_cast<int>(Error::Err.current - Error::Err.last) << "\n";
            file << file_index++ << "=" << static_cast<float>(PID::turn_pid.kp) << "\n";
            file << file_index++ << "=" << static_cast<float>(PID::turn_pid.kd) << "\n";
            file << file_index++ << "=" << static_cast<int>(PID::turn_pid.p_term) << "\n";
            file << file_index++ << "=" << static_cast<int>(PID::turn_pid.d_term) << "\n"; 
            // file << file_index++ << "=" << static_cast<int>(Servo::duty.cal) << "\n";
            //file << file_index++ << "=" << static_cast<int>(TCP::test[0]) << "\n";

        #endif
        // 自动填充剩余序号为0，直到49
        while (file_index <= 59) {
            file << file_index++ << "=0\n";
        }
        // 控制固参
        file << "[控制固参]\n0=0\n";
        file.flush();
    }

    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 上位机原图保存
    // 参数说明: 场数量，图片数组
    // 返回参数: bmp图片
    // 使用示例: binary_image_save(img_index,Camera::bin_pixels);
    // 备注信息: 
    //-------------------------------------------------------------------------------------------------------------------
    void binary_image_save(int index, uint8_t* image) {
        char path_buf[256];
        cv::Mat src(HEIGHT, WIDTH, CV_8UC1, image);
        cv::imwrite(build_path(path_buf, sizeof(path_buf), ORIGIN_DIR, index, ".bmp"), src);
    }

    #if Saving_mode
        //-------------------------------------------------------------------------------------------------------------------
        // 函数简介: 保存拟合图像（内部实现）
        // 参数说明: 场数量
        // 返回参数: bmp图片
        // 使用示例: fitting_image_save(img_index);
        // 备注信息: 
        //-------------------------------------------------------------------------------------------------------------------
        void fitting_image_save(int index) {
            char path_buf[256];
            cv::Mat img(HEIGHT, WIDTH, CV_8UC1, cv::Scalar(255)); // 全白背景
        
            const int start_row = 69;
            const int end_row = std::max(0, HEIGHT - ImageProc::state.cutoff);
        
            for (int i = start_row; i >= end_row; --i) {
                // 获取当前行的指针
                uchar* row_ptr = img.ptr<uchar>(i);
        
                // 提取左、右、中线坐标
                const int left_col = ImageProc::l_black[i];
                const int right_col = ImageProc::r_black[i];
                const int mid_col = Error::center_line[i];
                // const int fit_left_col = ImageProc::fit_l_black[i];
                // const int fit_right_col = ImageProc::fit_r_black[i];
                // 绘制左边界（黑）
                if (left_col >= 0 && left_col < WIDTH) {
                    row_ptr[left_col] = 0; // 直接通过指针访问
                }
        
                // 绘制右边界（黑）
                if (right_col >= 0 && right_col < WIDTH) {
                    row_ptr[right_col] = 0;
                }
        
                // 绘制中线（黑）
                if (mid_col >= 0 && mid_col < WIDTH) {
                    row_ptr[mid_col] = 0;
                }
                // // 绘制拟合左边界（黑）
                // if (fit_left_col >= 0 && fit_left_col < WIDTH) {
                //     row_ptr[fit_left_col] = 0;
                // }
                // // 绘制拟合右边界（黑）
                // if (fit_right_col >= 0 && fit_right_col < WIDTH) {
                //     row_ptr[fit_right_col] = 0;
                // }
            }    
            cv::imwrite(build_path(path_buf, sizeof(path_buf), FITTING_DIR, index, ".bmp"), img);
        }
    #else
        //-------------------------------------------------------------------------------------------------------------------
        // 函数简介: 保存拟合图像（保存lcd）
        // 参数说明: 场数量
        // 返回参数: bmp图片
        // 使用示例: fitting_image_save(img_index);
        // 备注信息: 
        //-------------------------------------------------------------------------------------------------------------------
        void fitting_image_save(int index)
        {
            char path_buf[256];
            cv::Mat src(HEIGHT, WIDTH, CV_8UC1, Camera::lcd_pixels);
            cv::imwrite(build_path(path_buf, sizeof(path_buf), FITTING_DIR, index, ".bmp"), src);
        }
    #endif
}

namespace DataSaving {
    int img_index = 1;
    void init(){
        create_directories();
    }
    
    void clear_folders(uint8_t mode) {
        if(mode){
            clear_single_folder(ORIGIN_DIR);
            clear_single_folder(FITTING_DIR);
            clear_single_folder(DATA_DIR);    
        }
    }

    void saveTask(uint8_t mode) {
        // 保持原有数据初始化方式
        if(mode){
            binary_image_save(img_index,Camera::bin_pixels);
            fitting_image_save(img_index);
            data_save(img_index,ImageProc::l_black,Error::center_line,ImageProc::r_black,
                ImageProc::fit_l_black,ImageProc::fit_r_black);
    
            ++img_index;    
        }
    }
}
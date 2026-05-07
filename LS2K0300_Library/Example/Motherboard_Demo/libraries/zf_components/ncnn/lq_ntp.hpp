#pragma once

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
#include <chrono>
#include <cstring>
#include <iomanip>
#include <sstream>

class lq_ntp
{
public:
    lq_ntp() = default;     // 使用默认构造函数
    ~lq_ntp() = default;    // 使用默认析构函数

    lq_ntp(const lq_ntp &ntp) = delete;             // 复制构造函数
    lq_ntp &operator=(const lq_ntp &ntp) = delete;  // 赋值运算符
    lq_ntp(lq_ntp &&ntp) = delete;                  // 移动构造函数
    lq_ntp &operator=(lq_ntp &&ntp) = delete;       // 移动赋值运算符

public:
    uint64_t get_local_time_s (void) {
        std::lock_guard<std::mutex> lock(mtx_);
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return static_cast<uint64_t>(tv.tv_sec);
    }
    
    uint64_t get_local_time_ms(void) {
        std::lock_guard<std::mutex> lock(mtx_);
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return static_cast<uint64_t>(tv.tv_sec * 1000ULL + tv.tv_usec / 1000);
    }
    
    uint64_t get_local_time_us(void) {
        std::lock_guard<std::mutex> lock(mtx_);
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return static_cast<uint64_t>(tv.tv_sec * 1000000ULL + tv.tv_usec);
    }

    std::string get_local_time_str(void) {
        std::lock_guard<std::mutex> lock(mtx_);
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
        return std::string(buffer);
    }
    
    std::string get_local_year(uint32_t *_year) {
        std::lock_guard<std::mutex> lock(mtx_);
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        
        if (_year) {
            *_year = static_cast<uint32_t>(timeinfo->tm_year + 1900);
        }
        
        return std::to_string(timeinfo->tm_year + 1900);
    }
    
    std::string get_local_month(uint8_t *_month) {
        std::lock_guard<std::mutex> lock(mtx_);
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        
        if (_month) {
            *_month = static_cast<uint8_t>(timeinfo->tm_mon + 1);
        }
        
        return std::to_string(timeinfo->tm_mon + 1);
    }
    
    std::string get_local_day(uint16_t *_day) {
        std::lock_guard<std::mutex> lock(mtx_);
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        
        if (_day) {
            *_day = static_cast<uint16_t>(timeinfo->tm_mday);
        }
        
        return std::to_string(timeinfo->tm_mday);
    }
    
    std::string get_local_hour(uint8_t *_hour) {
        std::lock_guard<std::mutex> lock(mtx_);
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        
        if (_hour) {
            *_hour = static_cast<uint8_t>(timeinfo->tm_hour);
        }
        
        return std::to_string(timeinfo->tm_hour);
    }
    
    std::string get_local_minute(uint8_t *_minute) {
        std::lock_guard<std::mutex> lock(mtx_);
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        
        if (_minute) {
            *_minute = static_cast<uint8_t>(timeinfo->tm_min);
        }
        
        return std::to_string(timeinfo->tm_min);
    }
    
    std::string get_local_second(uint8_t *_second) {
        std::lock_guard<std::mutex> lock(mtx_);
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        
        if (_second) {
            *_second = static_cast<uint8_t>(timeinfo->tm_sec);
        }
        
        return std::to_string(timeinfo->tm_sec);
    }

    // 主动同步时间，重新从 NTP 服务器获取时间并设置系统时间
    bool sync_ntp_time(void) {
        std::lock_guard<std::mutex> lock(mtx_);
        uint32_t unix_time = 0;
        
        if (get_ntp_time(&unix_time) != 0) {
            return false;
        }
        
        if (set_system_time(unix_time) != 0) {
            return false;
        }
        
        sync_hw_clock();
        return true;
    }

private:
    int get_ntp_time(uint32_t *_unix_time) {
        // 简化版的 NTP 客户端实现
        int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sockfd < 0) {
            return -1;
        }
        
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(ntp_port_);
        inet_pton(AF_INET, ntp_server_.c_str(), &serv_addr.sin_addr);
        
        // 设置超时
        struct timeval timeout = {2, 0};
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        
        // NTP 协议数据包
        char ntp_packet[48] = {0};
        ntp_packet[0] = 0x1B;  // LI=0, VN=3, Mode=3
        
        if (sendto(sockfd, ntp_packet, sizeof(ntp_packet), 0, 
                  (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            close(sockfd);
            return -1;
        }
        
        if (recv(sockfd, ntp_packet, sizeof(ntp_packet), 0) < 0) {
            close(sockfd);
            return -1;
        }
        
        close(sockfd);
        
        // 解析 NTP 时间戳
        uint32_t ntp_time = ntohl(*(uint32_t*)(&ntp_packet[40]));
        if (_unix_time) {
            *_unix_time = ntp_time - ntp_timestamp_delta_ + ntp_time_zone_offset_;
        }
        
        return 0;
    }
    
    int set_system_time(uint32_t _unix_time) {
        struct timeval tv;
        tv.tv_sec = static_cast<time_t>(_unix_time);
        tv.tv_usec = 0;
        
        if (settimeofday(&tv, nullptr) != 0) {
            return -1;
        }
        
        return 0;
    }
    
    void sync_hw_clock(void) {
        system("hwclock -w --utc 2>/dev/null");
    }
    
    bool get_system_tm(struct tm& out_tm) {
        std::lock_guard<std::mutex> lock(mtx_);
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        
        if (!timeinfo) {
            return false;
        }
        
        out_tm = *timeinfo;
        return true;
    }

private:
    std::mutex        mtx_;                                     // 互斥锁
    const std::string ntp_server_           = "182.92.12.11";   // NTP 服务器地址
    const int         ntp_port_             = 123;              // NTP 端口号
    const uint64_t    ntp_timestamp_delta_  = 2208988800ULL;    // NTP 时间戳偏移量，单位：秒
    const uint32_t    ntp_time_zone_offset_ = (8 * 3600);       // NTP 时区偏移量，单位：秒
};
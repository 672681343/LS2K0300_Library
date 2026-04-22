#pragma once

#define WIDTH           (186)
#define HEIGHT          (70)
//上位机保存模式 1：保存拟图  0：保存lcd图
#define Saving_mode     (1)
//上位机保存参数 1：图像参数 0：控制参数
#define Saving_param    (1)
//TCP传参  1：启动  0关闭
#define TCP_MODE        (0)
// 配置参数（类型与SDK保持一致）
#define SERVER_IP "192.168.1.108"
// 端口号
#define PORT            (8888)
//图像处理模式 0：opencv图像处理 1：使用自己的图像处理 
#define CAMERA_MODE     (1)
//反转
#define _Reverse        (0)

//交换函数
#define SWAP(a, b) { uint8_t t = a; a = b; b = t; }
//取大值函数
#define my_max(a, b) ((a) > (b) ? (a) : (b))
//取小值函数    
#define my_min(a, b) ((a) < (b) ? (a) : (b))
//取绝对值函数
#define my_abs(a) ((a) > 0 ? (a) : -(a))

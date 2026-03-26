// File: TCP.cpp
#include "TCP.h"



namespace TCP {
    int16_t test[8]= {0,0,0,0,0,0,0,0};
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: TCP传参初始化
    // 参数说明: 无
    // 返回参数: 无
    // 使用示例: TCP::init();
    // 备注信息: TCP_MODE 1启动，0不启动，不传参时不能启动会影响
    //-------------------------------------------------------------------------------------------------------------------
    void init() {
        if(TCP_MODE){
            tcp_client_init(SERVER_IP, PORT);
            seekfree_assistant_interface_init(tcp_client_send_data, tcp_client_read_data);
        }
    }
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: 上传参数
    // 参数说明: 无
    // 返回参数: 无
    // 使用示例: TCP::send_data();
    // 备注信息: TCP_MODE 1启动，0不启动，不传参时不能启动会影响
    //-------------------------------------------------------------------------------------------------------------------
    void send_data() {
        if(TCP_MODE){
            seekfree_assistant_oscilloscope_data.data[0] = Control::aim_speed.cur_L;
            seekfree_assistant_oscilloscope_data.data[1] = Encoder::count.L;
            seekfree_assistant_oscilloscope_data.data[2] = Control::aim_speed.cur_R;
            seekfree_assistant_oscilloscope_data.data[3] = Encoder::count.R;
            
            seekfree_assistant_oscilloscope_data.data[4] = Error::Err.current ;
            seekfree_assistant_oscilloscope_data.data[5] = PID::speed_pid_L.p_term + PID::speed_pid_L.i_term;

            seekfree_assistant_oscilloscope_data.data[6] = PID::speed_pid_L.i_term;
            seekfree_assistant_oscilloscope_data.data[7] = PID::speed_pid_L.d_term;
            
            seekfree_assistant_oscilloscope_data.channel_num = 8;

            seekfree_assistant_oscilloscope_send(&seekfree_assistant_oscilloscope_data);
        }
    }
    //-------------------------------------------------------------------------------------------------------------------
    // 函数简介: TCP数据解析
    // 参数说明: 无
    // 返回参数: 无
    // 使用示例: TCP::data_analysis();
    // 备注信息: 放在中断中  TCP_MODE 1启动，0不启动，不传参时不能启动会影响
    //-------------------------------------------------------------------------------------------------------------------
    void data_analysis() {
        if(TCP_MODE){
            seekfree_assistant_data_analysis();
        }
    }
}
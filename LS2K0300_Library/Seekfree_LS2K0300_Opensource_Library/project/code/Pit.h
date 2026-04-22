#pragma once
#include "zf_common_headfile.h"

namespace Pit{
    
    void Init();
    void callback_5ms();
    void callback_10ms();
    void callback_20ms();
    void callback_50ms();

    extern int16_t Err_out ;
    extern int16_t Angle_out ;


}
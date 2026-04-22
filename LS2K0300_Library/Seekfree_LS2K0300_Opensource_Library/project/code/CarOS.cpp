//CarOS.h
#include "CarOS.h"
#include <stdio.h>
#include <string.h>
#include <unordered_set>
namespace MenuSystem {
    // 全局状态初始化-----------------------------------------
    const Menu* current_menu = nullptr;
    uint16_t cursor_pos = 0;
    bool in_adjust_mode = false;
    float adjust_step = 1.0f;
    static float battery_voltage = 0;
    std::vector<ParamGroup> registered_groups;
    std::vector<MenuStackEntry> menu_stack;
    static constexpr std::array<float, 7> STEP_LEVELS = {
        100.0f, 10.0f, 1.0f, 0.1f, 0.01f, 0.001f
    };
    size_t current_step_index = 2; // 默认1.0
    Flags Car_flag = {
        .init   = 0, // 共用1个uint8_t的存储空间
        .start  = 0,
        .test   = 0,
        .stop   = 0,
        .turn   = 0,
        .speed_loop  = 0,
        .bldc   = 0,
        .menu   = 0,
        .protect  = 0,
        .set_time = 0,       // 普通成员变量
        .saving = 0,
        .version = 0, // 版本号
    };
    uint8_t menu_state = 1;
    // 菜单项声明-------------------------------------------------
    static const MenuItem main_items[] = {
        {"Start",    ACTION_SUBMENU, "start", {0,0}, MenuID::START},
        {"Group",    ACTION_SUBMENU, "turn",  {0,0}, MenuID::GROUP},
        {"Speed PID",ACTION_SUBMENU, "speed", {0,0}, MenuID::SPEED},
        {"Angle PID",ACTION_SUBMENU, "angle", {0,0}, MenuID::ANGLE},
        {"Err",      ACTION_SUBMENU, "err",   {0,0}, MenuID::ERR},
        {"Elem",     ACTION_SUBMENU, "elem",  {0,0}, MenuID::ELEM},
        {"System",   ACTION_SUBMENU, "sys",   {0,0}, MenuID::SYSTEM}
    };

    static const MenuItem start_items[] = {
        {"Normal Start",ACTION_CALLBACK, "start.normal",       {0,0},   MenuID::CONFIRM},
        {"Test Mode",   ACTION_CALLBACK, "test.mode",          {0,0},   MenuID::CONFIRM},
        {"Menu",        ACTION_SWITCH,   "car_flag.menu",      {0,1},   MenuID::NONE},
        {"Turn",        ACTION_SWITCH,   "car_flag.turn",      {0,1},   MenuID::NONE},
        {"Sloop",       ACTION_SWITCH,   "car_flag.speedloop", {0,1},   MenuID::NONE},
        {"PWM",         ACTION_CALLBACK, "car_flag.pwm",       {0,0},   MenuID::PWM},

        {"Timing",      ACTION_ADJUST,   "car_flag.set_time",  {0, 60}, MenuID::NONE},
        {"Protect",     ACTION_SWITCH,   "car_flag.protect",   {0,1},   MenuID::NONE},
        {"beep",        ACTION_SWITCH,   "car_flag.beep",      {0,1},   MenuID::NONE},
    };

    static const MenuItem confirm_items[] = {
        
        {"Officially Start", ACTION_CALLBACK, "officially.start",  {0,0}, MenuID::NONE},
    };

    static const MenuItem pwm_items[] = {
        
        {"PWM_L", ACTION_ADJUST, "pwm.L",  {0.0f, 5000.0f}, MenuID::NONE},
        {"PWM_R", ACTION_ADJUST, "pwm.R",  {0.0f, 5000.0f}, MenuID::NONE},
    };

    static const MenuItem group_items[] = {

        {"Curve",   ACTION_SUBMENU, "Group.crv", {0,0}, MenuID::CURVE},
        {"Short",   ACTION_SUBMENU, "Group.sht", {0,0}, MenuID::SHORT},
        {"Long",    ACTION_SUBMENU, "Group.log", {0,0}, MenuID::LONG},
        {"Island",  ACTION_SUBMENU, "Group.isl", {0,0}, MenuID::ISLAND},

    };

    static const MenuItem curve_items[] = {
        {"crvP",  ACTION_ADJUST, "Group.crvkp", {0.0f, 1000.0f}, MenuID::NONE},
        {"crvP2",  ACTION_ADJUST, "Group.crvkp2", {0.0f, 100.0f}, MenuID::NONE},
        {"crvD",  ACTION_ADJUST, "Group.crvkd", {0.0f, 1000.0f}, MenuID::NONE},
        {"crvGD",  ACTION_ADJUST, "Group.crvgkd", {0.0f, 100.0f}, MenuID::NONE},
        {"dif",   ACTION_ADJUST, "Group.crvdif", {0.0f, 100.0f}, MenuID::NONE},
        {"rep" , ACTION_ADJUST, "Group.difrep", {0.0f, 1.0f}, MenuID::NONE},
        {"speed", ACTION_ADJUST, "Group.crvspd", {0,0}, MenuID::NONE},
        {"crvspp",  ACTION_ADJUST, "Group.crvspkp", {0.0f, 1000.0f}, MenuID::NONE},
        {"crvspi",  ACTION_ADJUST, "Group.crvspki", {0.0f, 100.0f}, MenuID::NONE},
        {"crvspD",  ACTION_ADJUST, "Group.crvspkd", {0.0f, 1000.0f}, MenuID::NONE},
        {"Err",   ACTION_ADJUST, "Group.crvErr", {10, 70}, MenuID::NONE},
        {"pwm_cl",  ACTION_ADJUST, "Group.pwm_cl", {0.0f, 5000.0f}, MenuID::NONE},
        {"pwm_cr",  ACTION_ADJUST, "Group.pwm_cr", {0.0f, 5000.0f}, MenuID::NONE},


    };
    static const MenuItem short_items[] = {
        {"shtP", ACTION_ADJUST, "Group.shtkp", {0.0f, 1000.0f}, MenuID::NONE},
        {"shtP2", ACTION_ADJUST, "Group.shtkp2", {0.0f, 100.0f}, MenuID::NONE},
        {"shtD", ACTION_ADJUST, "Group.shtkd", {0.0f, 1000.0f}, MenuID::NONE},
        {"shtGD", ACTION_ADJUST, "Group.shtgkd", {0.0f, 100.0f}, MenuID::NONE},
        {"dif" , ACTION_ADJUST, "Group.shtdif", {0.0f, 100.0f}, MenuID::NONE},
        {"speed",ACTION_ADJUST, "Group.shtspd", {0,0}, MenuID::NONE},
        {"shtspp",  ACTION_ADJUST, "Group.shtspkp", {0.0f, 1000.0f}, MenuID::NONE},
        {"shtspi",  ACTION_ADJUST, "Group.shtspki", {0.0f, 100.0f}, MenuID::NONE},
        {"shtspD",  ACTION_ADJUST, "Group.shtspkd", {0.0f, 1000.0f}, MenuID::NONE},
        {"Err",  ACTION_ADJUST, "Group.shtErr", {10, 70}, MenuID::NONE},
        {"pwm_sl",  ACTION_ADJUST, "Group.pwm_sl", {0.0f, 5000.0f}, MenuID::NONE},
        {"pwm_sr",  ACTION_ADJUST, "Group.pwm_sr", {0.0f, 5000.0f}, MenuID::NONE},


    };
    static const MenuItem long_items[] = {
        {"logP", ACTION_ADJUST, "Group.logkp", {0.0f, 1000.0f}, MenuID::NONE},
        {"logP2", ACTION_ADJUST, "Group.logkp2", {0.0f, 100.0f}, MenuID::NONE},
        {"logD", ACTION_ADJUST, "Group.logkd", {0.0f, 1000.0f}, MenuID::NONE},
        {"logGD", ACTION_ADJUST, "Group.loggkd", {0.0f, 100.0f}, MenuID::NONE},
        {"dif" , ACTION_ADJUST, "Group.logdif", {0.0f, 100.0f}, MenuID::NONE},
        {"speed",ACTION_ADJUST, "Group.logspd", {0,0}, MenuID::NONE},
        {"logspp",  ACTION_ADJUST, "Group.logspkp", {0.0f, 1000.0f}, MenuID::NONE},
        {"logspi",  ACTION_ADJUST, "Group.logspki", {0.0f, 100.0f}, MenuID::NONE},
        {"logspD",  ACTION_ADJUST, "Group.logspkd", {0.0f, 1000.0f}, MenuID::NONE},
        {"Err",  ACTION_ADJUST, "Group.logErr", {10, 70}, MenuID::NONE},
        {"pwm_ll",  ACTION_ADJUST, "Group.pwm_ll", {0.0f, 5000.0f}, MenuID::NONE},
        {"pwm_lr",  ACTION_ADJUST, "Group.pwm_lr", {0.0f, 5000.0f}, MenuID::NONE},
  

    };
    static const MenuItem island_items[] = {
        {"islP", ACTION_ADJUST, "Group.islandkp", {0.0f, 1000.0f}, MenuID::NONE},
        {"islP2", ACTION_ADJUST, "Group.islandkp2", {0.0f, 100.0f}, MenuID::NONE},
        {"islD", ACTION_ADJUST, "Group.islandkd", {0.0f, 1000.0f}, MenuID::NONE},
        {"islGD", ACTION_ADJUST, "Group.islandgkd", {0.0f, 100.0f}, MenuID::NONE},
        {"dif", ACTION_ADJUST, "Group.islanddif", {0.0f, 100.0f}, MenuID::NONE},
        {"speed", ACTION_ADJUST, "Group.islandspd", {0,0}, MenuID::NONE},
        {"islspp",  ACTION_ADJUST, "Group.islandspkp", {0.0f, 1000.0f}, MenuID::NONE},
        {"islspi",  ACTION_ADJUST, "Group.islandspki", {0.0f, 100.0f}, MenuID::NONE},
        {"islspD",  ACTION_ADJUST, "Group.islandspkd", {0.0f, 1000.0f}, MenuID::NONE},
        {"Err", ACTION_ADJUST, "Group.islandErr", {10, 70}, MenuID::NONE},
        {"pwm_il",  ACTION_ADJUST, "Group.pwm_il", {0.0f, 5000.0f}, MenuID::NONE},
        {"pwm_ir",  ACTION_ADJUST, "Group.pwm_ir", {0.0f, 5000.0f}, MenuID::NONE},

    };

    static const MenuItem speed_items[] = {
        
        {"L_P",   ACTION_ADJUST, "speed_pid_L.kp", {0.0f, 100.0f}, MenuID::NONE},
        {"L_I",   ACTION_ADJUST, "speed_pid_L.ki", {0.0f, 100.0f}, MenuID::NONE},
        {"L_D",   ACTION_ADJUST, "speed_pid_L.kd", {0.0f, 100.0f}, MenuID::NONE},
        {"R_P",   ACTION_ADJUST, "speed_pid_R.kp", {0.0f, 100.0f}, MenuID::NONE},
        {"R_I",   ACTION_ADJUST, "speed_pid_R.ki", {0.0f, 100.0f}, MenuID::NONE},
        {"R_D",   ACTION_ADJUST, "speed_pid_R.kd", {0.0f, 100.0f}, MenuID::NONE},

        {"encL",ACTION_VIEW,   "count.L",        {0.0f, 0.0f},   MenuID::NONE},
        {"encR",ACTION_VIEW,   "count.R",        {0.0f, 0.0f},   MenuID::NONE},
        {"pwmL",ACTION_VIEW,   "pwm.L",          {0.0f, 0.0f},   MenuID::NONE},
        {"pwmR",ACTION_VIEW,   "pwm.R",          {0.0f, 0.0f},   MenuID::NONE},

    };

    static const MenuItem angle_items[] = {
        {"ang_P",   ACTION_ADJUST, "angle_pid.kp", {0.0f, 100.0f}, MenuID::NONE},
        {"ang_I",   ACTION_ADJUST, "angle_pid.ki", {0.0f, 100.0f}, MenuID::NONE},
        {"ang_D",   ACTION_ADJUST, "angle_pid.kd", {0.0f, 100.0f}, MenuID::NONE},

    };



    static const MenuItem elem_items[] = {
        {"Left Tilt",       ACTION_SUBMENU, "elem_left_tilt",       {0,0}, MenuID::ELEM_LEFT_TILT},
        {"Right Tilt",      ACTION_SUBMENU, "elem_right_tilt",      {0,0}, MenuID::ELEM_RIGHT_TILT},
        {"Cross",           ACTION_SUBMENU, "elem_cross",           {0,0}, MenuID::ELEM_CROSS},
        {"Left Island",     ACTION_SUBMENU, "elem_left_island",     {0,0}, MenuID::ELEM_LEFT_ISLAND},
        {"Right Island",    ACTION_SUBMENU, "elem_right_island",    {0,0}, MenuID::ELEM_RIGHT_ISLAND},
 
 
    };
    static const MenuItem elem_left_tilt_items[] = {
        {"Enable:",  ACTION_SWITCH,   "elem_left_tilt.enable", {0,1}, MenuID::NONE},
        {"black:",   ACTION_ADJUST,   "elem_left_tilt.black",  {15,35},MenuID::NONE},
        {"State:",   ACTION_VIEW,     "elem_left_tilt.state",  {0.0f, 0.0f}, MenuID::NONE}
    };
    static const MenuItem elem_right_tilt_items[] = {
        {"Enable:",  ACTION_SWITCH,   "elem_right_tilt.enable", {0,1}, MenuID::NONE},
        {"black:",   ACTION_ADJUST,   "elem_right_tilt.black",  {15,35},MenuID::NONE},
        {"State:",   ACTION_VIEW,     "elem_right_tilt.state",  {0.0f, 0.0f}, MenuID::NONE}
    };
    static const MenuItem elem_cross_items[] = {
        {"Enable:",  ACTION_SWITCH,   "elem_cross.enable", {0,1}, MenuID::NONE},
        {"State:",   ACTION_VIEW,     "elem_cross.state",  {0.0f, 0.0f}, MenuID::NONE}
    };
    static const MenuItem elem_left_island_items[] = {
        {"Enable:", ACTION_SWITCH,   "elem_left_island.enable", {0,1}, MenuID::NONE},
        {"x1:",     ACTION_ADJUST,   "elem_left_island.x1", {90, 170}, MenuID::NONE},
        {"x2:",     ACTION_ADJUST,   "elem_left_island.x2", {0, 55}, MenuID::NONE},
        {"State:",  ACTION_VIEW,     "elem_left_island.state",  {0.0f, 0.0f}, MenuID::NONE}
    };
    static const MenuItem elem_right_island_items[] = {
        {"Enable:", ACTION_SWITCH,   "elem_right_island.enable", {0,1}, MenuID::NONE},
        {"x1:",     ACTION_ADJUST,   "elem_right_island.x1", {90, 170}, MenuID::NONE},
        {"x2:",     ACTION_ADJUST,   "elem_right_island.x2", {0, 55}, MenuID::NONE},
        {"State:",  ACTION_VIEW,     "elem_right_island.state",  {0.0f, 0.0f}, MenuID::NONE}
    };



    static const MenuItem err_items[] = {
        {"cur", ACTION_VIEW,   "Err.cur", {0.0f, 0.0f},   MenuID::NONE},
        {"pre", ACTION_VIEW,   "Err.pre", {0.0f, 0.0f},   MenuID::NONE},
        {"row", ACTION_ADJUST, "Err.row", {0.0f, 70.0f},  MenuID::NONE},
        {"rep", ACTION_ADJUST, "Err.rep", {1.0f, 2.0f},   MenuID::NONE}
    };
    static const MenuItem sys_items[] = {
        {"Saving",   ACTION_SWITCH,   "car_flag.saving", {0,1}, MenuID::NONE},
        {"BackUp",   ACTION_SUBMENU,  "sys.backup", {0,0}, MenuID::VERSION},
        {"thr",      ACTION_VIEW,     "car_flag.thr",    {0.0f, 0.0f}, MenuID::NONE},
        {"camp",     ACTION_ADJUST,   "car_flag.thrcamp",{0.0f, 0.0f}, MenuID::NONE},
        {"Reboot",   ACTION_CALLBACK, "reboot",   {0.0f, 0.0f}, MenuID::NONE},
        {"Shutdown", ACTION_CALLBACK, "shutdown", {0.0f, 0.0f}, MenuID::NONE},
    };
    static const MenuItem version_items[] = {
        {"1", ACTION_CALLBACK, "sys.version1", {0,0}, MenuID::V_ACTION},
        {"2", ACTION_CALLBACK, "sys.version2", {0,0}, MenuID::V_ACTION},
        {"3", ACTION_CALLBACK, "sys.version3", {0,0}, MenuID::V_ACTION},
        {"4", ACTION_CALLBACK, "sys.version4", {0,0}, MenuID::V_ACTION},
        {"5", ACTION_CALLBACK, "sys.version5", {0,0}, MenuID::V_ACTION},
        {"Ctrl + Z", ACTION_CALLBACK, "sys.quash", {0,0}, MenuID::MAIN},
    };
    static const MenuItem version_action_items[] = {
        {"V:", ACTION_VIEW, "car_flag.version", {0,0}, MenuID::NONE},
        {"Write", ACTION_CALLBACK, "write.version", {0,0}, MenuID::MAIN},
        {"Read", ACTION_CALLBACK, "read.version", {0,0}, MenuID::MAIN},
    };
    // 菜单实例声明-----------------------------------------------
    const Menu main_menu    = {"Main Menu",     main_items, sizeof(main_items)/sizeof(MenuItem)};
    const Menu start_menu   = {"Start Config",  start_items,sizeof(start_items)/sizeof(MenuItem)};
    const Menu group_menu   = {"Group",         group_items, sizeof(group_items)/sizeof(MenuItem)};
    const Menu curve_menu   = {"Curve",         curve_items, sizeof(curve_items)/sizeof(MenuItem)};
    const Menu short_menu   = {"Short",         short_items, sizeof(short_items)/sizeof(MenuItem)};
    const Menu long_menu    = {"Long",          long_items,  sizeof(long_items)/sizeof(MenuItem)};
    const Menu island_menu  = {"Island",        island_items,sizeof(island_items)/sizeof(MenuItem)};
 
 
    const Menu speed_menu   = {"Speed Settings",speed_items,sizeof(speed_items)/sizeof(MenuItem)};

    const Menu angle_menu   = {"Speed Settings",angle_items,sizeof(angle_items)/sizeof(MenuItem)};

    const Menu err_menu     = {"Err Config",    err_items,  sizeof(err_items)/sizeof(MenuItem)};
    const Menu sys_menu     = {"System",        sys_items,  sizeof(sys_items)/sizeof(MenuItem)};
    const Menu elem_menu    = {"Elem",          elem_items,  sizeof(elem_items)/sizeof(MenuItem)};
    const Menu elem_left_tilt_menu = {"Left Tilt", elem_left_tilt_items, sizeof(elem_left_tilt_items)/sizeof(MenuItem)};
    const Menu elem_right_tilt_menu = {"Right Tilt", elem_right_tilt_items, sizeof(elem_right_tilt_items)/sizeof(MenuItem)};
    const Menu elem_cross_menu = {"Cross", elem_cross_items, sizeof(elem_cross_items)/sizeof(MenuItem)};
    const Menu elem_left_island_menu = {"Left Island", elem_left_island_items, sizeof(elem_left_island_items)/sizeof(MenuItem)};
    const Menu elem_right_island_menu = {"Right Island", elem_right_island_items, sizeof(elem_right_island_items)/sizeof(MenuItem)};

    const Menu pwm_menu   = {"PWM Settings",pwm_items,sizeof(pwm_items)/sizeof(MenuItem)};

    const Menu confirm_menu = {"Confirm",       confirm_items,sizeof(confirm_items)/sizeof(MenuItem)};
    const Menu version_menu = {"Version",       version_items, sizeof(version_items)/sizeof(MenuItem)};
    const Menu version_action_menu = {"Version Action", version_action_items, sizeof(version_action_items)/sizeof(MenuItem)};
    // 菜单列表（必须与MenuID顺序完全一致）----------------------
    static const Menu* const menu_list[] = {
        nullptr,                // [0] MenuID::NONE
        &main_menu,             // [1] MenuID::MAIN
        &start_menu,            // [2] MenuID::START
        &group_menu,            // [3] MenuID::GROUP
        &curve_menu,            // [4] MenuID::CURVE
        &short_menu,            // [5] MenuID::SHORT
        &long_menu,             // [6] MenuID::LONG
        &island_menu,           // [7] MenuID::ISLAND

        &pwm_menu,              // [8] MenuID::PWM

        &speed_menu,            // [10] MenuID::SPEED

        &angle_menu,

        &elem_menu,             // [11] MenuID::ELEM
        &elem_left_tilt_menu,   // [12] MenuID::ELEM_LEFT_TILT
        &elem_right_tilt_menu,  // [13] MenuID::ELEM_RIGHT_TILT
        &elem_cross_menu,       // [14] MenuID::ELEM_CROSS
        &elem_left_island_menu, // [15] MenuID::ELEM_LEFT_ISLAND
        &elem_right_island_menu,// [16] MenuID::ELEM_RIGHT_ISLAND


        &err_menu,              // [22] MenuID::ERR
        &sys_menu,              // [23] MenuID::SYSTEM
        &confirm_menu,          // [24] MenuID::CONFIRM
        &version_menu,          // [25] MenuID::VERSION
        &version_action_menu    // [26] MenuID::V_ACTION

    };
 
    // 编译期安全检查
    static_assert(
        sizeof(menu_list)/sizeof(Menu*) == static_cast<size_t>(MenuID::COUNT),
        "菜单枚举与数组长度不匹配！"
    );
  
    // // 按键状态 --------------------------------------------------

    static KeyState key_up = {0};
    static KeyState key_down = {0};
    static KeyState key_left = {0};
    static KeyState key_right = {0};
    static KeyState key_center = {0};

    // 参数访问 --------------------------------------------------
    // 首先在文件作用域定义映射数组

    static const ParamMapping speed_pid_L_params[] = {
        {"kp", TYPE_FLOAT, .as_float = &PID::speed_pid_L.kp, .persist = true},
        {"ki", TYPE_FLOAT, .as_float = &PID::speed_pid_L.ki, .persist = true},
        {"kd", TYPE_FLOAT, .as_float = &PID::speed_pid_L.kd, .persist = true}
    };
    static const ParamMapping speed_pid_R_params[] = {
        {"kp", TYPE_FLOAT, .as_float = &PID::speed_pid_R.kp, .persist = true},
        {"ki", TYPE_FLOAT, .as_float = &PID::speed_pid_R.ki, .persist = true},
        {"kd", TYPE_FLOAT, .as_float = &PID::speed_pid_R.kd, .persist = true}
    };


    static const ParamMapping angle_pid_params[] = {
        {"kp", TYPE_FLOAT, .as_float = &PID::angle_pid.kp, .persist = true},
        {"ki", TYPE_FLOAT, .as_float = &PID::angle_pid.ki, .persist = true},
        {"kd", TYPE_FLOAT, .as_float = &PID::angle_pid.kd, .persist = true}
    };

    static const ParamMapping pwm_params[] = {
        {"L", TYPE_INT16, .as_int16 = &Control::pwm.L, .persist = false},
        {"R", TYPE_INT16, .as_int16 = &Control::pwm.R, .persist = false},

    };
    static const ParamMapping count_params[] = {
        {"L", TYPE_INT16, .as_int16 = &Encoder::count.L, .persist = false},
        {"R", TYPE_INT16, .as_int16 = &Encoder::count.R, .persist = false},
    };

    static const ParamMapping lose_state_params[] = {
        {"l_str", TYPE_UINT8, .as_uint8 = &ImageProc::Left_BF.LostParam.start, .persist = false},
        {"l_end", TYPE_UINT8, .as_uint8 = &ImageProc::Left_BF.LostParam.end, .persist = false},
        {"r_str", TYPE_UINT8, .as_uint8 = &ImageProc::Right_BF.LostParam.start, .persist = false},
        {"r_end", TYPE_UINT8, .as_uint8 = &ImageProc::Right_BF.LostParam.end, .persist = false},
    };
    static const ParamMapping elem_left_tilt_params[] = {
        {"enable", TYPE_UINT8, .as_uint8 = &Elem::Left_Tilt_Cross.enable, .persist = true},
        {"black",  TYPE_UINT8, .as_uint8 = &Elem::Left_Tilt_Cross.black_threshold, .persist = true},
        {"state",  TYPE_UINT8, .as_uint8 = &Elem::Left_Tilt_Cross.State, .persist = false}
    };
    static const ParamMapping elem_right_tilt_params[] = {
        {"enable", TYPE_UINT8, .as_uint8 = &Elem::Right_Tilt_Cross.enable, .persist = true},
        {"black",  TYPE_UINT8, .as_uint8 = &Elem::Right_Tilt_Cross.black_threshold, .persist = true},
        {"state",  TYPE_UINT8, .as_uint8 = &Elem::Right_Tilt_Cross.State, .persist = false}
    };
    static const ParamMapping elem_cross_params[] = {
        {"enable", TYPE_UINT8, .as_uint8 = &Elem::Cross.enable, .persist = true},
        {"state",  TYPE_UINT8, .as_uint8 = &Elem::Cross.State, .persist = false}
    };
    static const ParamMapping elem_left_island_params[] = {
        {"enable", TYPE_UINT8, .as_uint8 = &Elem::Left_Island.enable, .persist = true},
        {"x1",     TYPE_UINT8, .as_uint8 = &Elem::Left_Island.x1, .persist = true},
        {"x2",     TYPE_UINT8, .as_uint8 = &Elem::Left_Island.x2, .persist = true},
        {"state",  TYPE_UINT8, .as_uint8 = &Elem::Left_Island.State, .persist = false}
    };
    static const ParamMapping elem_right_island_params[] = {
        {"enable", TYPE_UINT8, .as_uint8 = &Elem::Right_Island.enable, .persist = true},
        {"x1",     TYPE_UINT8, .as_uint8 = &Elem::Right_Island.x1, .persist = true},
        {"x2",     TYPE_UINT8, .as_uint8 = &Elem::Right_Island.x2, .persist = true},
        {"state",  TYPE_UINT8, .as_uint8 = &Elem::Right_Island.State, .persist = false}
    };

    static const ParamMapping BF_params[] = {
        {"l_var", TYPE_FLOAT, .as_float = &ImageProc::Left_BF.Var, .persist = false},
        {"r_var", TYPE_FLOAT, .as_float = &ImageProc::Right_BF.Var, .persist = false},
    };

    static const ParamMapping group_params[] = {
        {"logkp", TYPE_FLOAT,  .as_float = &Control::LongStrPid.kp, .persist = true},
        {"logkp2", TYPE_FLOAT,  .as_float = &Control::LongStrPid.kp2, .persist = true},
        {"logkd", TYPE_FLOAT,  .as_float = &Control::LongStrPid.kd, .persist = true},
        {"loggkd", TYPE_FLOAT,  .as_float = &Control::LongStrPid.gkd, .persist = true},

        {"logdif", TYPE_FLOAT, .as_float = &Control::LongStrSpeed.dif, .persist = true},
        {"logspd", TYPE_INT16, .as_int16 = &Control::LongStrSpeed.set_speed, .persist = true},
        {"logErr", TYPE_UINT8, .as_uint8 = &Control::LongStrSpeed.err_row, .persist = true},

        {"logspkp", TYPE_FLOAT, .as_float = &Control::LongStrSpeed.l_kp, .persist = true},
        {"logspki", TYPE_FLOAT, .as_float = &Control::LongStrSpeed.l_ki, .persist = true},
        {"logspkd", TYPE_FLOAT, .as_float = &Control::LongStrSpeed.l_kd, .persist = true},


        {"pwm_ll", TYPE_INT16, .as_int16 = &Control::LongStr_pwm.L, .persist = true},
        {"pwm_lr", TYPE_INT16, .as_int16 = &Control::LongStr_pwm.R, .persist = true},

        {"shtkp", TYPE_FLOAT,  .as_float = &Control::ShortStrPid.kp, .persist = true},
        {"shtkp2", TYPE_FLOAT,  .as_float = &Control::ShortStrPid.kp2, .persist = true},
        {"shtkd", TYPE_FLOAT,  .as_float = &Control::ShortStrPid.kd, .persist = true},
        {"shtgkd", TYPE_FLOAT,  .as_float = &Control::ShortStrPid.gkd, .persist = true},

        {"shtdif", TYPE_FLOAT, .as_float = &Control::ShortStrSpeed.dif, .persist = true},
        {"shtspd", TYPE_INT16, .as_int16 = &Control::ShortStrSpeed.set_speed, .persist = true},
        {"shtErr", TYPE_UINT8, .as_uint8 = &Control::ShortStrSpeed.err_row, .persist = true},

        {"shtspkp", TYPE_FLOAT, .as_float = &Control::ShortStrSpeed.l_kp, .persist = true},
        {"shtspki", TYPE_FLOAT, .as_float = &Control::ShortStrSpeed.l_ki, .persist = true},
        {"shtspkd", TYPE_FLOAT, .as_float = &Control::ShortStrSpeed.l_kd, .persist = true},


        {"pwm_sl", TYPE_INT16, .as_int16 = &Control::ShortStr_pwm.L, .persist = true},
        {"pwm_sr", TYPE_INT16, .as_int16 = &Control::ShortStr_pwm.R, .persist = true},


        {"crvkp", TYPE_FLOAT,  .as_float = &Control::CurvePid.kp, .persist = true},
        {"crvkp2", TYPE_FLOAT,  .as_float = &Control::CurvePid.kp2, .persist = true},
        {"crvkd", TYPE_FLOAT,  .as_float = &Control::CurvePid.kd, .persist = true},
        {"crvgkd", TYPE_FLOAT,  .as_float = &Control::CurvePid.gkd, .persist = true},

        {"crvdif", TYPE_FLOAT, .as_float = &Control::CurveSpeed.dif, .persist = true},

        {"crvspd", TYPE_INT16, .as_int16 = &Control::CurveSpeed.set_speed, .persist = true},
        {"crvErr", TYPE_UINT8, .as_uint8 = &Control::CurveSpeed.err_row, .persist = true},

        {"crvspkp", TYPE_FLOAT, .as_float = &Control::CurveSpeed.l_kp, .persist = true},
        {"crvspki", TYPE_FLOAT, .as_float = &Control::CurveSpeed.l_ki, .persist = true},
        {"crvspkd", TYPE_FLOAT, .as_float = &Control::CurveSpeed.l_kd, .persist = true},

        {"pwm_cl", TYPE_INT16, .as_int16 = &Control::Curve_pwm.L, .persist = true},
        {"pwm_cr", TYPE_INT16, .as_int16 = &Control::Curve_pwm.R, .persist = true},

        {"islandkp", TYPE_FLOAT,  .as_float = &Control::IslandPid.kp, .persist = true},
        {"islandkp2", TYPE_FLOAT,  .as_float = &Control::IslandPid.kp2, .persist = true},
        {"islandkd", TYPE_FLOAT,  .as_float = &Control::IslandPid.kd, .persist = true},
        {"islandgkd", TYPE_FLOAT,  .as_float = &Control::IslandPid.gkd, .persist = true},

        {"islanddif", TYPE_FLOAT, .as_float = &Control::IslandSpeed.dif, .persist = true},
        {"islandspd", TYPE_INT16, .as_int16 = &Control::IslandSpeed.set_speed, .persist = true},
        {"islandErr", TYPE_UINT8, .as_uint8 = &Control::IslandSpeed.err_row, .persist = true},

        {"islandspkp", TYPE_FLOAT, .as_float = &Control::IslandSpeed.l_kp, .persist = true},
        {"islandspki", TYPE_FLOAT, .as_float = &Control::IslandSpeed.l_ki, .persist = true},
        {"islandspkd", TYPE_FLOAT, .as_float = &Control::IslandSpeed.l_kd, .persist = true},

        {"pwm_il", TYPE_INT16, .as_int16 = &Control::Island_pwm.L, .persist = true},
        {"pwm_ir", TYPE_INT16, .as_int16 = &Control::Island_pwm.R, .persist = true},


    };

    //车辆标志位    
    static const ParamMapping car_flag_params[] = {
        {"turn",     TYPE_UINT8, .as_uint8 = &Car_flag.turn, .persist = true},
        {"speedloop",TYPE_UINT8, .as_uint8 = &Car_flag.speed_loop, .persist = true},
        {"pwm",      TYPE_INT16, .as_int16 = &Car_flag.pwm, .persist = true},
        {"menu",     TYPE_UINT8, .as_uint8 = &Car_flag.menu, .persist = true},
        {"protect",  TYPE_UINT8, .as_uint8 = &Car_flag.protect, .persist = true},
        {"set_time", TYPE_UINT8, .as_uint8 = &Car_flag.set_time, .persist = true},
        {"saving",   TYPE_UINT8, .as_uint8 = &Car_flag.saving, .persist = true},
        {"beep",     TYPE_UINT8, .as_uint8 = &Car_flag.beep, .persist = true},
        {"thr",      TYPE_UINT8, .as_uint8 = &Camera::threshold, .persist = true},
        {"thrcamp",  TYPE_INT8,  .as_int8  = &Camera::threshold_camp, .persist = true},
        {"version",  TYPE_UINT8, .as_uint8 = &Car_flag.version, .persist = false},
    };

    static const ParamMapping err_params[] = {
        {"cur",     TYPE_INT16, .as_int16 = &Error::Err.current, .persist = false},
        {"pre",     TYPE_INT16, .as_int16 = &Error::Err.previous, .persist = false},
        {"row",     TYPE_UINT8, .as_uint8 = &Error::Err.row, .persist = true},
        {"rep",     TYPE_FLOAT, .as_float = &Error::Err.rep, .persist = true},
    };


    // 参数访问函数
    ParamMapping* find_param(const char* path) {
        for (auto& group : registered_groups) {
            const size_t prefix_len = strlen(group.prefix);
            
            // 路径前缀匹配检查
            if (strncmp(path, group.prefix, prefix_len) != 0) continue;
            if (path[prefix_len] != group.delimiter) continue;
            
            // 提取字段名称
            const char* field = path + prefix_len + 1;
            
            // 线性搜索匹配字段
            for (size_t j = 0; j < group.param_count; ++j) {
                if (strcmp(field, group.params[j].field) == 0) {
                    return &const_cast<ParamMapping&>(group.params[j]);
                }
            }
        }
        return nullptr;
    }
    
    float get_parameter_value(const ParamMapping& param) {
        switch(param.type) {
        case TYPE_UINT8:  return static_cast<float>(*param.as_uint8);
        case TYPE_INT8:   return static_cast<float>(*param.as_int8);
        case TYPE_UINT16: return static_cast<float>(*param.as_uint16);
        case TYPE_INT16:  return static_cast<float>(*param.as_int16);
        case TYPE_FLOAT:  return *param.as_float;
        default:
            // 错误处理（可根据需要添加蜂鸣器提示）
            ips200_show_string(0, 0, "E1:Bad Type");
            return 0.0f;
        }
    }
    
    void set_parameter_value(ParamMapping& param, float value) {
        switch(param.type) {
        case TYPE_UINT8:  
            *param.as_uint8 = static_cast<uint8_t>(std::clamp(value, 0.0f, 255.0f));
            break;
        case TYPE_INT8:   
            *param.as_int8 = static_cast<int8_t>(std::clamp(value, -128.0f, 127.0f));
            break;
        case TYPE_UINT16: 
            *param.as_uint16 = static_cast<uint16_t>(std::clamp(value, 0.0f, 65535.0f));
            break;
        case TYPE_INT16:  
            *param.as_int16 = static_cast<int16_t>(std::clamp(value, -32768.0f, 32767.0f));
            break;
        case TYPE_FLOAT:  
            *param.as_float = value;
            break;
        default:
            ips200_show_string(0, 0, "E2:Bad Type");
            break;
        }

    }

    // 核心逻辑实现 --------------------------------------------------
    // 初始化函数
    void init() {
        // 安全初始化显示设备
        ips200_init("/dev/fb0"); // 直接初始化显示

        //注册速度PID参数组
        register_param_group({
            "speed_pid_L",
            '.',
            speed_pid_L_params,  // 使用统一命名的全局定义
            sizeof(speed_pid_L_params)/sizeof(ParamMapping)
        });
        //注册速度PID参数组
        register_param_group({
            "speed_pid_R",
            '.',
            speed_pid_R_params,  // 使用统一命名的全局定义
            sizeof(speed_pid_R_params)/sizeof(ParamMapping)
        });

        //注册角度环PID参数组
        register_param_group({
            "angle_pid",
            '.',
            angle_pid_params,  // 使用统一命名的全局定义
            sizeof(angle_pid_params)/sizeof(ParamMapping)
        });

        //注册PWM参数组
        register_param_group({
            "pwm",
            '.',
            pwm_params,  // 使用统一命名的全局定义
            sizeof(pwm_params)/sizeof(ParamMapping)
        });
        //注册编码器参数组
        register_param_group({
            "count",
            '.',
            count_params,  // 使用统一命名的全局定义
            sizeof(count_params)/sizeof(ParamMapping)
        });
        //注册车辆标志位参数组
        register_param_group({
            "car_flag",
            '.',
            car_flag_params,  // 使用统一命名的全局定义
            sizeof(car_flag_params)/sizeof(ParamMapping)
        });

        //注册误差参数组
        register_param_group({
            "Err",
            '.',
            err_params,  // 使用统一命名的全局定义
            sizeof(err_params)/sizeof(ParamMapping)
        });
        //注册丢线参数组
        register_param_group({
            "lose_state",
            '.',
            lose_state_params,  // 使用统一命名的全局定义
            sizeof(lose_state_params)/sizeof(ParamMapping)
        });
        //注册方差参数组
        register_param_group({
            "BF",
            '.',
            BF_params,  // 使用统一命名的全局定义
            sizeof(BF_params)/sizeof(ParamMapping)
        });
        //注册左倾斜交叉口参数组
        register_param_group({
            "elem_left_tilt",
            '.',
            elem_left_tilt_params,  // 使用统一命名的全局定义
            sizeof(elem_left_tilt_params)/sizeof(ParamMapping)
        });
        //注册右倾斜交叉口参数组
        register_param_group({
            "elem_right_tilt",
            '.',
            elem_right_tilt_params,  // 使用统一命名的全局定义
            sizeof(elem_right_tilt_params)/sizeof(ParamMapping)
        });
        //注册交叉口参数组
        register_param_group({
            "elem_cross",
            '.',
            elem_cross_params,  // 使用统一命名的全局定义
            sizeof(elem_cross_params)/sizeof(ParamMapping)
        });
        //注册左岛屿参数组
        register_param_group({
            "elem_left_island",
            '.',
            elem_left_island_params,  // 使用统一命名的全局定义
            sizeof(elem_left_island_params)/sizeof(ParamMapping)
        });
        //注册右岛屿参数组
        register_param_group({
            "elem_right_island",
            '.',
            elem_right_island_params,  // 使用统一命名的全局定义
            sizeof(elem_right_island_params)/sizeof(ParamMapping)
        });

        //注册组参数组
        register_param_group({
            "Group",
            '.',
            group_params,  // 使用统一命名的全局定义
            sizeof(group_params)/sizeof(ParamMapping)
        });

        //电池读取
        Battery_Init();
        //参数装载
        json_load(JSON_FILE);
        // 初始化主菜单
        current_menu = &main_menu; // 硬编码主菜单
        cursor_pos = 0;
        if(current_menu) {
            draw_menu();
        }
    }
    //需要保存的参数组
    const std::vector<std::string> save_groups{
    "speed_pid_L", 
    "speed_pid_R",
    "angle_pid",
    "car_flag", 
    "duty", 
    "Err", 
    "Group", 
    "SegmentPD",
    "elem_left_tilt",
    "elem_right_tilt",
    "elem_cross",
    "elem_left_island",
    "elem_right_island",

    };
    void register_param_group(const ParamGroup& group) {
        registered_groups.push_back(group);
    }

    // 实现菜单跳转核心逻辑
    void navigate_to(MenuID target) {
        if (target == MenuID::NONE) {
            return;
        }
        // 直接转换枚举值为索引
        const uint8_t id = static_cast<uint8_t>(target);
        
        // 保存当前状态
        menu_stack.push_back({current_menu, static_cast<uint16_t>(cursor_pos)});
        
        // 强制访问菜单列表
        current_menu = menu_list[id];
        
        // 重置光标位置
        cursor_pos = 0;
        
        // 刷新显示
        draw_menu();
    }

    void toggle_param(const char* path) {
        // 查找参数映射
        ParamMapping* param = find_param(path);
        if (!param) {
            ips200_show_string(0, 0, "E3:Param Not Found");
            return;
        }
    
        // 类型安全判断
        switch(param->type) {
        case TYPE_UINT8:
        case TYPE_INT8:
        case TYPE_UINT16:
        case TYPE_INT16:
            break; // 支持整数类型
        default:
            ips200_show_string(0, 0, "E4:Unsupported Type");
            return;
        }
    
        // 获取当前值并切换
        float current = get_parameter_value(*param);
        if (current == 0.0f || current == 1.0f) {
            set_parameter_value(*param, 1.0f - current);
        } else {
            // 扩展处理：非0/1值时的行为
            ips200_show_string(0, 0, "W1:Value Not 0/1");
            // 可选：强制设置为默认值
            // set_parameter_value(*param, 0.0f); 
        }
    }

    // 通用长按检测函数
    bool check_long_press(KeyState& key, uint64_t threshold_ms = 500) {
        if (!key.pressed) return false;
        
        const uint64_t duration = timespec_diff_ms(
            get_monotonic_time(),
            key.press_time
        );
        
        return duration >= threshold_ms;
    }
    // 完善按键处理函数
    void process_keys() {
        // 按键状态更新Lambda
        auto update_key = [](KeyState& key, const char* physical_key) {
            bool current_state = !Key::GetDebouncedState(physical_key);
            
            if (current_state != key.pressed) {
                key.pressed = current_state;
                key.processed = false;
                
                if (current_state) {
                    clock_gettime(CLOCK_MONOTONIC, &key.press_time);
                    key.last_trigger = key.press_time; // 初始化最后触发时间
                }
            }
        };
    
        // 更新物理按键映射（根据实际硬件定义）
        update_key(key_up,    Key::SWITCH_1);  // 物理按键3对应向上
        update_key(key_down,  Key::KEY_1);  // 物理按键1对应向下
        update_key(key_left,  Key::KEY_3);  // 物理按键4对应向左
        update_key(key_right, Key::SWITCH_0);  // 物理按键2对应向右
        update_key(key_center, Key::KEY_2);
        //按键映射
        // update_key(key_up,     Key::KEY_1);      // 向上
        // update_key(key_down,   Key::SWITCH_0);   // 向下
        // update_key(key_left,   Key::KEY_0);      // 向左
        // update_key(key_right,  Key::KEY_2);      // 向右
        // update_key(key_center, Key::KEY_3);      // 向中间
        // 长按处理Lambda
        auto handle_continuous_press = [](KeyState& key, auto action, 
            uint64_t initial_delay = 200, 
            uint64_t repeat_interval = 100) {
            if (!key.pressed) return false;

            // 记录首次按下时间
            const uint64_t press_duration = timespec_diff_ms(get_monotonic_time(), key.press_time);

            // 短按检测（<200ms）
            if (!key.processed && press_duration < initial_delay) {
                action();           // 立即执行一次
                key.processed = true;
                return true;
            }

            // 长按连续触发
            static timespec last_trigger = {0, 0};
            const uint64_t since_last = timespec_diff_ms(get_monotonic_time(), last_trigger);

            if (press_duration >= initial_delay && since_last >= repeat_interval) {
                action();
                last_trigger = get_monotonic_time();
                return true;
            }
            return false;
        };

        if (in_adjust_mode) {
            // 参数调整模式处理
            const MenuItem& item = current_menu->items[cursor_pos];
    
            // 左键：切换到更小步长（固定等级）
            handle_continuous_press(key_right, [&]{
                current_step_index = (current_step_index + 1) % STEP_LEVELS.size();
                adjust_step = STEP_LEVELS[current_step_index];
                draw_menu();
            }, 200, 100); // 200ms延迟，100ms间隔
            
            // 右键：切换到更大步长（固定等级）
            handle_continuous_press(key_left, [&]{
                current_step_index = (current_step_index - 1 + STEP_LEVELS.size()) % STEP_LEVELS.size();
                adjust_step = STEP_LEVELS[current_step_index];
                draw_menu();
            }, 200, 100);
            // 上键：参数增加（长按连续）
            handle_continuous_press(key_up, [&]{
                ParamMapping* param = find_param(item.param_path);
                if (param) {
                    // 获取当前值（自动类型转换）
                    float current_val = get_parameter_value(*param);
                    
                    // 计算新值
                    float new_val = current_val + adjust_step;
                    
                    // 应用菜单项定义的范围限制（保持原有逻辑）
                    if (!(std::abs(item.range.min) < 1e-6 && std::abs(item.range.max) < 1e-6)) {
                        new_val = std::clamp(new_val, item.range.min, item.range.max);
                    }
                    
                    // 设置新值（自动类型转换+参数类型范围保护）
                    set_parameter_value(*param, new_val);
                    
                    draw_menu();
                }
            });
            
            // 下键：参数减少（长按连续） 
            handle_continuous_press(key_down, [&]{
                ParamMapping* param = find_param(item.param_path);
                if (param) {
                    // 获取当前参数值（自动类型转换）
                    float current_val = get_parameter_value(*param);
                    
                    // 计算新值并应用范围限制
                    float new_val = current_val - adjust_step;
                    
                    // 保留原有的无限幅判断逻辑
                    if (!(std::abs(item.range.min) < 1e-6 && std::abs(item.range.max) < 1e-6)) {
                        new_val = std::clamp(new_val, item.range.min, item.range.max);
                    }
                    
                    // 设置新值（自动类型转换+参数类型范围保护）
                    set_parameter_value(*param, new_val);
                    
                    draw_menu();
                }
            });

            if (check_long_press(key_left, 500)) { // 500ms长按阈值
                in_adjust_mode = false;
                key_left.processed = true; // 标记为已处理
                //参数保存
                //json_save();
                Parameter_saving();
                // 刷新界面
                ips200_clear();
                draw_menu();
            }

            // 新增中间键短按直接退出
            if (key_center.pressed && !key_center.processed) {
                in_adjust_mode = false;
                key_center.processed = true;
                Parameter_saving();
                ips200_clear();
                draw_menu();
            }
        } else {
            // 导航模式处理
            auto handle_nav_scroll = [&](KeyState& key, auto scroll_action) {
                if (handle_continuous_press(key, scroll_action)) {
                    ips200_clear();
                    draw_menu();
                }
            };
            // 常规菜单导航模式
            // 上键滚动
            handle_nav_scroll(key_up, [&]{
                cursor_pos = (cursor_pos > 0) ? cursor_pos - 1 : current_menu->item_count - 1;
            });
            
            // 下键滚动
            handle_nav_scroll(key_down, [&]{
                cursor_pos = (cursor_pos < current_menu->item_count - 1) ? cursor_pos + 1 : 0;
            });
            if (key_right.pressed && !key_right.processed) {
                const MenuItem& item = current_menu->items[cursor_pos];
                if (item.action == ACTION_SUBMENU) {
                    navigate_to(item.next_menu);
                } else if (item.action == ACTION_ADJUST) {
                    in_adjust_mode = true;
                    auto it = std::lower_bound(STEP_LEVELS.rbegin(), STEP_LEVELS.rend(), adjust_step);
                    current_step_index = STEP_LEVELS.size() - 1 - std::distance(STEP_LEVELS.rbegin(), it);
                    adjust_step = STEP_LEVELS[current_step_index];
                }else if (item.action == ACTION_SWITCH) {
                    toggle_param(item.param_path);
                    //参数保存
                    //json_save();
                    Parameter_saving();
                } else if (item.action == ACTION_CALLBACK) {
                    execute_callback(item.param_path);
                    //在这里添加菜单跳转
                    navigate_to(item.next_menu);
                }
                
                key_right.processed = true;
                ips200_clear();
                draw_menu();
            }
            if (key_left.pressed && !key_left.processed) {
                if (!menu_stack.empty()) {
                    // 返回上级菜单
                    const MenuStackEntry& prev = menu_stack.back();
                    current_menu = prev.menu;
                    cursor_pos = prev.cursor_pos;
                    menu_stack.pop_back();
                } else {
                    // 退回主菜单
                    current_menu = &main_menu;
                    cursor_pos = 0;
                }
                key_left.processed = true;
                ips200_clear();
                draw_menu();
            }
        }
    }
    //绘制图像
    void draw_menu() {
        const uint16_t camera_x = 27;
        const uint16_t camera_y = 0;
        const uint16_t base_y = 72;     // 基础Y坐标
        const uint16_t value_x = 70;    // 数值显示起始X坐标
        const uint16_t range_x = 110;   // 范围显示起始X坐标
        const uint16_t mode_x = 200;    // 模式显示起始X坐标

        //显示lcd
        ips200_show_gray_image(camera_x,camera_y,Camera::lcd_pixels,186,70);
        ips200_draw_line(camera_x-1,camera_y,camera_x-1,camera_y+70,0xFFFF);
        ips200_draw_line(camera_x+186,camera_y,camera_x+186,camera_y+70,0xFFFF);
        ips200_draw_line(camera_x,camera_y+70,camera_x+186,camera_y+70,0xFFFF);
        // 显示电池电压
        //ips200_show_gray_image(0, 0, rgay_image, UVC_WIDTH, UVC_HEIGHT);
        // 清屏并绘制标题
        
        char title_buf[32];
        snprintf(title_buf, sizeof(title_buf), "%s %s V:%.1f",
               in_adjust_mode ? "[*]" : "   ",
               current_menu->title,
               battery_voltage);
        ips200_show_string(10, base_y, title_buf);
    
        // 分页显示控制
        constexpr uint8_t items_per_page = 6;
        const uint8_t current_page = cursor_pos / items_per_page;
        const uint16_t start_index = current_page * items_per_page;
        const uint16_t end_index = std::min<uint16_t>(
            static_cast<uint16_t>(start_index + items_per_page),
            current_menu->item_count
        );
        // 绘制可见菜单项
        
        static int refresh_count = 0;
        for (uint16_t i = start_index; i < end_index; ++i) {
            const uint8_t relative_pos = i - start_index;
            const uint16_t y_pos = base_y + 30 + relative_pos * 20;
            const MenuItem& item = current_menu->items[i];
            char display_buf[32] = {0};
    
            // 光标指示（选中项前后添加符号）
            if (i == cursor_pos) {
                if(in_adjust_mode)
                    snprintf(display_buf, sizeof(display_buf), "*%s", item.title);
                else
                    snprintf(display_buf, sizeof(display_buf), ">%s", item.title);
            } else {
                snprintf(display_buf, sizeof(display_buf), " %s", item.title);
            }
            ips200_show_string(10, y_pos, display_buf);
    
            // 参数显示处理
            if (item.action == ACTION_SWITCH || 
                item.action == ACTION_ADJUST ||
                item.action == ACTION_VIEW)  // 新增查看模式
            {
                ParamMapping* param = find_param(item.param_path);
                if (!param) {
                    ips200_show_string(value_x, y_pos, "N/A");
                    continue;
                }
    
                // 获取参数值并格式化
                float value = get_parameter_value(*param);
                char value_buf[16];
                switch(param->type) {
                    case TYPE_FLOAT: 
                        snprintf(value_buf, sizeof(value_buf), "%.2f ", value); 
                        break;
                    case TYPE_UINT8: 
                        snprintf(value_buf, sizeof(value_buf), "%u ", 
                               static_cast<uint8_t>(value));
                        break;
                    case TYPE_UINT16: 
                        snprintf(value_buf, sizeof(value_buf), "%u ", 
                               static_cast<uint16_t>(value));
                        break;
                    default: 
                        snprintf(value_buf, sizeof(value_buf), "%d ", 
                               static_cast<int>(value));
                }
                ips200_show_string(value_x, y_pos, value_buf);
                
                        // 查看模式显示刷新标识
                
                if (item.action == ACTION_VIEW) {
                    static bool blink = false;
                    if (++refresh_count % 10 == 0) blink = !blink;
                    ips200_show_char(220, y_pos, blink ? '*' : ' ');
                }
                // 显示参数范围
                if (item.action == ACTION_ADJUST) {
                    char range_buf[24];
                    if (std::abs(item.range.min) < 1e-6 && std::abs(item.range.max) < 1e-6) {
                        snprintf(range_buf, sizeof(range_buf), "[INF]");
                    } else {
                        if (param->type == TYPE_FLOAT) {
                            snprintf(range_buf, sizeof(range_buf), "[%.1f-%.1f]", 
                                   item.range.min, item.range.max);
                        } else {
                            snprintf(range_buf, sizeof(range_buf), "[%d-%d]", 
                                   static_cast<int>(item.range.min), 
                                   static_cast<int>(item.range.max));
                        }
                    }
                    ips200_show_string(range_x, y_pos, range_buf);
                }
            }
    
            // 操作指示符
            const char* indicator = "";
            switch(item.action) {
                case ACTION_SWITCH:  indicator = "[SW]"; break;
                case ACTION_ADJUST:  indicator = in_adjust_mode ? "[ADJ]" : "[SET]"; break;
                case ACTION_SUBMENU: indicator = " > "; break;
                default :
                    if (item.action == ACTION_CALLBACK) {
                        // 回调动作的特殊处理
                    }
                    break;
            }
            ips200_show_string(mode_x, y_pos, indicator);
        }
    
        // 分页指示器
        if (current_menu->item_count > items_per_page) {
            char page_info[12];
            snprintf(page_info, sizeof(page_info), "%d/%d", 
                   current_page + 1, 
                   (current_menu->item_count + items_per_page - 1) / items_per_page);
            ips200_show_string(200, 220, page_info);
        }
    
        // 步长显示
        if (in_adjust_mode) {
            char step_buf[16];
            snprintf(step_buf, sizeof(step_buf), "Step:%.3f", adjust_step);
            ips200_show_string(10, 220, step_buf);
        }
    }  

    struct timespec get_monotonic_time() {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts;
    }
    uint64_t timespec_diff_ms(const struct timespec& end, const struct timespec& start) {
        const int64_t sec_diff = end.tv_sec - start.tv_sec;
        const int64_t nsec_diff = end.tv_nsec - start.tv_nsec;
        const int64_t total_ms = sec_diff * 1000 + nsec_diff / 1000000;
        return (total_ms >= 0) ? static_cast<uint64_t>(total_ms) : 0;
    }
    //回调函数
    void execute_callback(const char* path){
        static uint8_t menu_state_temp = 0;
        //正常模式
        if (strcmp(path, "start.normal") == 0) {
            menu_state_temp = 0;
            if(battery_voltage <= 12.0){
                beep_on();
                ips200_clear();
                ips200_show_string(20, 120, "Power Low!!!");
                system_delay_ms(500);
                beep_off();
            }
            Elem::clear();
            Car_flag.init = 1;
            Car_flag.test = 0;
            //system_delay_ms(500);
        }
        //测试模式
        if(strcmp(path, "test.mode") == 0){

            menu_state_temp = Car_flag.menu;
            Car_flag.stop_times = Car_flag.set_time*20;
            if(battery_voltage <= 12.0){
                beep_on();
                ips200_clear();
                ips200_show_string(20, 120, "Power Low!!!");
                system_delay_ms(500);
                beep_off();
            }
            Elem::clear();
            Car_flag.init = 1;
            Car_flag.test = 1;
            //system_delay_ms(500);
        }
        //正式启动确认
        if(strcmp(path, "officially.start") == 0){
            menu_state = menu_state_temp;
            DataSaving::clear_folders(Car_flag.saving);
            Car_flag.start = 1;
            //system_delay_ms(500);
        }

        //重启
        if(strcmp(path, "reboot") == 0){
            re_boot();
        }
        //关机
        if(strcmp(path, "shutdown") == 0){
            sh_down();
        }


        if(strcmp(path, "sys.version1") == 0){
            Car_flag.version = 1;
            
        }
        if(strcmp(path, "sys.version2") == 0){
            Car_flag.version = 2;
            
        }
        if(strcmp(path, "sys.version3") == 0){
            Car_flag.version = 3;
            
        }
        if(strcmp(path, "sys.version4") == 0){
            Car_flag.version = 4;
            
        }
        if(strcmp(path, "sys.version5") == 0){
            Car_flag.version = 5;
            
        }
        if(strcmp(path, "write.version") == 0){
            char file_path[256];
            snprintf(file_path, sizeof(file_path), 
            "%s%d.json", JSON_BACKUP_DIR, Car_flag.version);
            json_save(save_groups, file_path);
        }
        if(strcmp(path, "read.version") == 0){
            char file_path[256];
            //当前参数存储到临时文件
            json_save(save_groups, JSON_TEMP_FILE);
            snprintf(file_path, sizeof(file_path), 
            "%s%d.json", JSON_BACKUP_DIR, Car_flag.version);
            //从指定文件加载参数
            json_load(file_path);
            //保存到默认文件
            Parameter_saving();
            system_delay_ms(500);
            // sh_down();
        }
        if(strcmp(path,"sys.quash") == 0){
            json_load(JSON_TEMP_FILE);
        }
    }

    // void start_config(){
    //     ips200_show_string(40, 60, "Right to start");
    //     while(1){
    //         if ((!Key::GetDebouncedState(Key::KEY_2)) ){
    //             ips200_clear();
    //             system_delay_ms(1000);
    //             break;
    //         }
    //     }
    //     Car_flag.start = 1;
    // }

    void re_boot()
    {

        Motor::deinit();
        // BLDC::deinit();

        ips200_clear();
        ips200_show_string(40, 60, "Rebooting...");
        ips200_show_string(40, 80, "Wait one seconds.");
        MenuSystem::beep_off();
        // int8 result = system("./car_mode 0");
        // if(result==-1){
        //     printf("Error executing the command!");
        // }
        printf("程序正常退出\n");
        exit(0); 
    }
    void sh_down()
    {

        Motor::deinit();
        // BLDC::deinit();

        MenuSystem::beep_off();
        ips200_clear();
        ips200_show_string(40, 60, "Shutting Down...");

        ips200_show_string(20, 100, "Press button  to wake up");
        ips200_show_string(20, 120, "Or wake up on your computer");


        int8 result = system("./car_mode 1");
        
        if(result==-1)
        {
            printf("Error executing the command!");
        }
        printf("程序正常退出\n");
        exit(0); 
    }
    
    //按键关机
    void key_shutdown(){
        if ((!Key::GetDebouncedState(Key::KEY_0)) ) {
            sh_down();
        }
        // if ((!Key::GetDebouncedState(Key::SWITCH_1)) ) {
        //     sh_down();
        // }
    }



    //参数保存
    void Parameter_saving(){
        json_save(save_groups, JSON_FILE);
    }

    bool json_save(const std::vector<std::string>& groups_to_save, 
                   const char* filename) {
        FILE* fp = fopen(filename, "w");
        if (!fp) return false;
    
        // 将需要保存的组名转换为集合，便于快速查找
        std::unordered_set<std::string> save_groups(groups_to_save.begin(), groups_to_save.end());
    
        fprintf(fp, "{\n");
    
        bool first_group = true;
        for (size_t g = 0; g < registered_groups.size(); ++g) {
            const ParamGroup* group = &registered_groups[g];
            
            // 如果有指定需要保存的组，且当前组不在其中，则跳过
            if (!groups_to_save.empty() && !save_groups.count(group->prefix)) {
                continue;
            }
    
            // 分组开始
            if (!first_group) fprintf(fp, ",\n");
            first_group = false;
            fprintf(fp, "    \"%s\": {\n", group->prefix);
    
            bool first_param = true;
            for (size_t p = 0; p < group->param_count; ++p) {
                const ParamMapping* param = &group->params[p];
                
                // 新增持久化判断
                if (!param->persist) continue;
    
                // 参数项
                if (!first_param) fprintf(fp, ",\n");
                first_param = false;
    
                const float value = get_parameter_value(*param);
                switch(param->type) {
                case TYPE_FLOAT:
                    fprintf(fp, "        \"%s\": %.6f", param->field, value);
                    break;
                default:
                    fprintf(fp, "        \"%s\": %d", param->field, (int)value);
                }
            }
            fprintf(fp, "\n    }");
        }
        
        fprintf(fp, "\n}\n");
        fclose(fp);
        return true;
    }

    // JSON加载函数
    bool json_load(const char* filename) {
        FILE* fp = fopen(filename, "r");
        if (!fp) {
            printf("[ERROR] 无法打开配置文件 %s\n", JSON_FILE);
            return false;
        }
    
        char line[256];
        char current_group[32] = {0};
        int line_num = 0;
        
        while (fgets(line, sizeof(line), fp)) {
            line_num++;
            
            // 解析分组
            if (strstr(line, "{")) {
                char* quote = strchr(line, '"');
                if (quote) {
                    char* end_quote = strchr(quote+1, '"');
                    if (end_quote) {
                        *end_quote = '\0';
                        strncpy(current_group, quote+1, sizeof(current_group)-1);
                        printf("[DEBUG] 进入分组: %s\n", current_group);
                    }
                }
                continue;
            }
    
            // 解析参数项
            char* colon = strchr(line, ':');
            if (!colon) continue;
    
            // 提取字段名
            *colon = '\0';
            char* key_start = strchr(line, '"');
            if (!key_start) {
                printf("[WARN] L%d: 无效键格式\n", line_num);
                continue;
            }
            char* key_end = strchr(key_start+1, '"');
            if (!key_end) {
                printf("[WARN] L%d: 未闭合的引号\n", line_num);
                continue;
            }
            *key_end = '\0';
            const char* field = key_start+1;
    
            // 构建完整路径
            char path[64];
            snprintf(path, sizeof(path), "%s.%s", current_group, field);
            printf("[LOAD] 处理参数: %s\n", path);
    
            // 查找参数
            ParamMapping* param = find_param(path);
            if (!param) {
                printf("[WARN] L%d: 未注册参数 %s\n", line_num, path);
                continue;
            }
    
            // 持久化校验
            if (!param->persist) {
                printf("[WARN] L%d: 跳过非持久化参数 %s\n", line_num, path);
                continue;
            }
    
            // 类型安全解析
            char* value_str = colon + 1;
            char* endptr = nullptr;
            float parsed_value = 0.0f;
    
            if (param->type == TYPE_FLOAT) {
                parsed_value = strtof(value_str, &endptr);
                if (endptr == value_str) {
                    printf("[ERROR] L%d: 浮点数解析失败: %s\n", line_num, value_str);
                    continue;
                }
            } else {
                long int_value = strtol(value_str, &endptr, 10);
                if (endptr == value_str) {
                    printf("[ERROR] L%d: 整型解析失败: %s\n", line_num, value_str);
                    continue;
                }
                parsed_value = static_cast<float>(int_value);
            }
    
            // 数值范围校验
            const float clamped_value = parsed_value;

            // 内存写入
            printf("[DEBUG] 设置 %s (类型:%d 地址:%p) = %.2f\n", 
                  path, param->type, param->ptr, clamped_value);
            
            switch(param->type) {
            case TYPE_UINT8:
                *param->as_uint8 = static_cast<uint8_t>(clamped_value);
                break;
            case TYPE_INT8:
                *param->as_int8 = static_cast<int8_t>(clamped_value);
                break;
            case TYPE_UINT16:
                *param->as_uint16 = static_cast<uint16_t>(clamped_value);
                break;
            case TYPE_INT16:
                *param->as_int16 = static_cast<int16_t>(clamped_value);
                break;
            case TYPE_FLOAT:
                *param->as_float = clamped_value;
                break;
            default:
                printf("[ERROR] 未知参数类型 %d\n", param->type);
            }
    
            // 验证写入
            float current_value = get_parameter_value(*param);
            printf("[VERIFY] %s 当前值 = %.2f\n\n", path, current_value);
        }
    
        fclose(fp);
        printf("[SUCCESS] 参数加载完成\n");
        return true;
    }
    //屏幕显示
    void Menu_display(){
        static uint8_t cur_state = 1;
        if(menu_state==1){
            if(cur_state==0){
                ips200_clear();
                cur_state=1;
            }
            //显示菜单
            process_keys();
            draw_menu();
        }
        //屏幕灭
        else{
            if(cur_state==1){
                ips200_clear();
                cur_state=0;
            }
        }
    }
    //电池电压读取
    void Battery_Init () {
        // 静态变量存储测量数据
        static uint16_t raw_value = 0;
        static float scale_factor = 0.0f;
        
        raw_value = adc_convert(ADC_REG_PATH);
        scale_factor = adc_get_scale(ADC_SCALE_PATH);
        battery_voltage = static_cast<float>(raw_value * scale_factor * 11) / 1000;
    }


    //蜂鸣器控制
    void beep_on(){
        if (Car_flag.init && Car_flag.beep && Car_flag.test) {
            gpio_set_level(BEEP_PATH, 0x1);
        }
    }
    void beep_off(){
        gpio_set_level(BEEP_PATH, 0x0);
    }
}

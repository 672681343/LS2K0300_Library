// CarOS.h
#pragma once
#include <stdint.h>
#include <vector>
#include <string>
#include "zf_common_headfile.h"

#define MENU_WIDTH 240
#define MENU_HEIGHT (320 - 80)

namespace MenuSystem {
    //电池模块定义-----------------------------------------
    constexpr const char* ADC_REG_PATH = "/sys/bus/iio/devices/iio:device0/in_voltage7_raw";
    constexpr const char* ADC_SCALE_PATH = "/sys/bus/iio/devices/iio:device0/in_voltage_scale";
    constexpr const char* BEEP_PATH = "/dev/zf_driver_gpio_beep";
    //电池模块定义-----------------------------------------

    //菜单模块定义-----------------------------------------
    typedef struct {
        bool pressed;
        bool processed;
        timespec press_time = {0, 0}; // 新增按压时间记录
        timespec last_trigger = {0, 0}; // 最后触发时间
    } KeyState;

    enum ActionType {
        ACTION_NONE,
        ACTION_SWITCH,
        ACTION_ADJUST,
        ACTION_VIEW,
        ACTION_SUBMENU,
        ACTION_CALLBACK
    };

    enum class MenuID : uint8_t {
        NONE,
        MAIN,
        START,
        GROUP,
        CURVE,
        SHORT,
        LONG,
        ISLAND,

        PWM,

        SPEED,

        ANGLE,

        ELEM,
        ELEM_LEFT_TILT,
        ELEM_RIGHT_TILT,
        ELEM_CROSS,
        ELEM_LEFT_ISLAND,
        ELEM_RIGHT_ISLAND,

        ERR,
        SYSTEM,
        CONFIRM,
        VERSION,
        V_ACTION,
        COUNT,
    };

    struct MenuItem {
        const char* title;
        ActionType action;
        const char* param_path;
        struct { 
            float min; 
            float max; 
        } range;
        MenuID next_menu;
    };

    struct Menu {
        const char* title;
        const MenuItem* items;
        uint16_t item_count;
    };

    enum ParamType {
        TYPE_UINT8,
        TYPE_INT8,
        TYPE_UINT16,
        TYPE_INT16, 
        TYPE_FLOAT
    };

    struct ParamMapping {
        const char* field;  // 参数名称
        ParamType type;      // 新增类型标识
        union {
            void* ptr;       // 通用指针
            uint8_t* as_uint8;
            int8_t* as_int8;
            uint16_t* as_uint16;
            int16_t* as_int16;
            float* as_float;
        };
        bool persist; // 是否持久化
    };

    // 参数组结构体
    struct ParamGroup {
        const char* prefix;
        char delimiter;
        const ParamMapping* params;
        size_t param_count;
    };

    // 新增菜单堆栈条目
    struct MenuStackEntry {
        const Menu* menu;
        uint16_t cursor_pos;
    };
    //车辆标志位
    struct Flags {
        uint8_t init      ; // 共用1个uint8_t的存储空间
        uint8_t start     ; 
        uint8_t test      ;
        uint8_t stop      ;
        uint8_t turn      ;
        uint8_t speed_loop;
        int16_t pwm       ;
        uint8_t bldc      ;
        uint8_t menu      ;
        uint8_t protect   ;
        uint8_t set_time  ;       // 普通成员变量
        uint16_t stop_times = 0;
        uint8_t saving    ;
        uint8_t beep      ; 
        uint8_t version   ; // 版本号z
    };
    //菜单模块定义-----------------------------------------

    // 参数保存模块定义------------------------------------


    // 参数保存模块定义---------------------------------------

    // 菜单全局状态声明---------------------------------------
    extern Flags Car_flag;
    extern const Menu* current_menu;
    extern uint16_t cursor_pos;
    extern bool in_adjust_mode;
    extern float adjust_step;
    extern std::vector<ParamGroup> registered_groups;
    extern std::vector<MenuStackEntry> menu_stack;
    // 菜单全局状态声明---------------------------------------

    
    // 菜单函数声明-----------------------------------------------
    void init();
    void register_param_group(const ParamGroup& group);
    ParamMapping* find_param(const char* path);
    float get_parameter_value(const ParamMapping& param);
    void set_parameter_value(ParamMapping& param, float value);

    void process_keys();
    void draw_menu();
    void toggle_param();
    void execute_callback(const char* path);
    struct timespec get_monotonic_time();
    uint64_t timespec_diff_ms(const struct timespec& end, const struct timespec& start);
    void Menu_display();
    // 菜单函数声明-----------------------------------------------
    // void start_config();
    void sh_down();
    void re_boot();
    void key_shutdown();
    // 参数保存函数声明-------------------------------------------

    #define JSON_FILE "/home/root/settings.json"
    #define JSON_BACKUP_DIR "/home/root/history/"
    #define JSON_TEMP_FILE "/home/root/history/temp_backup.json"
    //bool json_save();
    void Parameter_saving();
    bool json_save(const std::vector<std::string>& groups_to_save = {}, 
        const char* filename = JSON_FILE);
    bool json_load(const char* filename = JSON_FILE);
    // 电池读取函数声明-------------------------------------------
    void Battery_Init ();
    // 蜂鸣器函数声明---------------------------------------------
    void beep_on();
    void beep_off();
}

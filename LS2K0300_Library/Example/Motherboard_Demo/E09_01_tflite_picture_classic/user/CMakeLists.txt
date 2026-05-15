# # ===================== 基础配置 =====================
# cmake_minimum_required(VERSION 3.5.0)
# set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})
# get_filename_component(PARENT_PARENT_DIR ${CURRENT_DIR}/.. ABSOLUTE)
# get_filename_component(PARENT_PARENT_FOLDER_NAME ${PARENT_PARENT_DIR} NAME)
# project(${PARENT_PARENT_FOLDER_NAME} VERSION 0.1.0 LANGUAGES C CXX)

# # 定义绝对路径
# set(NCNN_BASE "/home/kaka/桌面/LS2K0300_Library/LS2K0300_Library/Seekfree_LS2K0300_Opensource_Library/libraries/zf_components/ncnn")
# set(TFLM_BASE "/home/kaka/桌面/LS2K0300_Library/LS2K0300_Library/Example/Motherboard_Demo/libraries/zf_components/tflm")
      
# message(STATUS "ncnn基础路径: ${NCNN_BASE}")
# message(STATUS "tflm基础路径: ${TFLM_BASE}")

# # ===================== 编译参数 =====================
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -pthread -Wall")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall")

# # ===================== 交叉编译工具链 =====================
# include(cross.cmake)

# # ===================== OpenCV配置 =====================
# set(CMAKE_PREFIX_PATH "/opt/ls_2k0300_env/opencv_4_10_build")
# find_package(OpenCV REQUIRED)
# include_directories(${OpenCV_INCLUDE_DIRS})

# # ===================== 头文件路径 =====================
# include_directories(${OpenCV_INCLUDE_DIRS})
# include_directories(../user)
# include_directories(../code)
# include_directories(../../libraries/zf_common)
# include_directories(../../libraries/zf_device)
# include_directories(../../libraries/zf_driver)
# include_directories(../../libraries/zf_components/seekfree_assistant)

# # 使用绝对路径包含ncnn
# include_directories(${NCNN_BASE}/include)  # 用于 #include "ncnn/net.h"
# include_directories(${NCNN_BASE})         # 用于 #include "lq_ncnn.hpp"

# # 使用绝对路径包含tflm
# include_directories(${TFLM_BASE})
# include_directories(${TFLM_BASE}/third_party/flatbuffers/include)

# # 可选的第三方库路径
# include_directories(${TFLM_BASE}/third_party)
# include_directories(${TFLM_BASE}/third_party/gemmlowp)
# include_directories(${TFLM_BASE}/third_party/ruy)
# include_directories(${TFLM_BASE}/third_party/kissfft)

# # ===================== 库文件搜索路径 =====================
# link_directories(${TFLM_BASE}/lib)
# link_directories(${NCNN_BASE}/lib)

# # ===================== 源文件 =====================
# aux_source_directory(../user DIR_SRCS)
# aux_source_directory(../code DIR_SRCS)
# aux_source_directory(../model DIR_SRCS)
# aux_source_directory(../../libraries/zf_common DIR_SRCS)
# aux_source_directory(../../libraries/zf_device DIR_SRCS)
# aux_source_directory(../../libraries/zf_driver DIR_SRCS)
# aux_source_directory(../../libraries/zf_components/seekfree_assistant DIR_SRCS)

# # ===================== 创建可执行文件 =====================
# add_executable(${PROJECT_NAME} ${DIR_SRCS})

# # ===================== 链接库 =====================
# target_link_libraries(${PROJECT_NAME} 
#     ${OpenCV_LIBS} 
#     ncnn
#     tflm
#     pthread
#     m
#     dl
#     stdc++
# )

# # ===================== 调试信息 =====================
# message(STATUS "=== 文件存在性检查 ===")
# execute_process(
#     COMMAND bash -c "if [ -f '${NCNN_BASE}/include/ncnn/net.h' ]; then echo '✅ ncnn/net.h 文件存在'; else echo '❌ ncnn/net.h 文件不存在: ${NCNN_BASE}/include/ncnn/net.h'; fi"
#     OUTPUT_VARIABLE NCNN_CHECK
# )
# message(STATUS "${NCNN_CHECK}")

# execute_process(
#     COMMAND bash -c "if [ -f '${TFLM_BASE}/third_party/flatbuffers/include/flatbuffers/flatbuffers.h' ]; then echo '✅ flatbuffers.h 文件存在'; else echo '❌ flatbuffers.h 文件不存在: ${TFLM_BASE}/third_party/flatbuffers/include/flatbuffers/flatbuffers.h'; fi"
#     OUTPUT_VARIABLE FLAT_CHECK
# )
# message(STATUS "${FLAT_CHECK}")



# ===================== 基础配置（龙芯适配）=====================
cmake_minimum_required(VERSION 3.5.0)

# 获取项目名称
set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})
get_filename_component(PARENT_PARENT_DIR ${CURRENT_DIR}/.. ABSOLUTE)
get_filename_component(PARENT_PARENT_FOLDER_NAME ${PARENT_PARENT_DIR} NAME)
message("Parent parent folder name: ${PARENT_PARENT_FOLDER_NAME}")

# 项目配置
project(${PARENT_PARENT_FOLDER_NAME} VERSION 0.1.0 LANGUAGES C CXX)

# ===================== 编译参数（龙芯LoongArch适配）=====================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译优化
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -pthread -Wall -Wno-delete-non-virtual-dtor -Wno-placement-new")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=loongarch64 -mtune=loongarch64 -fPIC -ffunction-sections -fdata-sections")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=loongarch64 -mtune=loongarch64 -fPIC -ffunction-sections -fdata-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

# ===================== 交叉编译工具链 =====================
include(cross.cmake)

# ===================== 依赖库（OpenCV）=====================
# 设置OpenCV的安装路径
set(CMAKE_PREFIX_PATH "/opt/ls_2k0300_env/opencv_4_10_build") 

# 查找OpenCV库，确保安装了所需的依赖
find_package(OpenCV REQUIRED)

# 包含OpenCV的头文件路径
include_directories(${OpenCV_INCLUDE_DIRS})
message(STATUS "OpenCV_INCLUDE_DIRS: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV_LIBS: ${OpenCV_LIBS}")

# ===================== 头文件路径（精简版）=====================
# 业务代码头文件
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(../user)
include_directories(../code)
include_directories(../tflite)
include_directories(../../libraries/zf_common)
include_directories(../../libraries/zf_device)
include_directories(../../libraries/zf_driver)
include_directories(../../libraries/zf_components/seekfree_assistant)

# TFLM核心头文件
include_directories(../../libraries/zf_components/tflm)
include_directories(../../libraries/zf_components/tflm/third_party/flatbuffers/include)
include_directories(../../libraries/zf_components/tflm/third_party/gemmlowp)
include_directories(../../libraries/zf_components/tflm/third_party/gemmlowp/fixedpoint)
include_directories(../../libraries/zf_components/tflm/third_party/ruy)
include_directories(../../libraries/zf_components/tflm/third_party/ruy/ruy)
include_directories(../../libraries/zf_components/tflm/tensorflow/lite/micro)
include_directories(../../libraries/zf_components/tflm/tensorflow/lite/micro/kernels)
include_directories(../../libraries/zf_components/tflm/tensorflow/lite/kernels/internal)
include_directories(../../libraries/zf_components/tflm/tensorflow/lite/core/api)
include_directories(../../libraries/zf_components/tflm/tensorflow/lite/core/c)
include_directories(../../libraries/zf_components/tflm/tensorflow/lite/schema)


# ===================== 静态库路径配置 =====================
# TFLM静态库路径
link_directories(../../libraries/zf_components/tflm/lib)
# NCNN静态库路径
include_directories(../../libraries/zf_components/ncnn/include/)
link_directories(../../libraries/zf_components/ncnn/lib)

# ===================== 业务源文件配置 =====================
aux_source_directory(../user DIR_SRCS)
aux_source_directory(../code DIR_SRCS)
aux_source_directory(../model DIR_SRCS)
aux_source_directory(../../libraries/zf_common DIR_SRCS)
aux_source_directory(../../libraries/zf_device DIR_SRCS)
aux_source_directory(../../libraries/zf_driver DIR_SRCS)
aux_source_directory(../../libraries/zf_components/seekfree_assistant DIR_SRCS)

# ===================== 创建可执行文件 =====================
add_executable(${PROJECT_NAME} ${DIR_SRCS})

# ===================== 目标属性 =====================
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    C_STANDARD 99
    INSTALL_RPATH "/opt/ls_2k0300_env/opencv_4_10_build/lib"
    BUILD_WITH_INSTALL_RPATH ON
)

# ===================== 链接所有静态库 =====================
target_link_libraries(${PROJECT_NAME} 
    ${OpenCV_LIBS} 
    ncnn          # ncnn静态库
    tflm          # TFLM静态库
    pthread       # 线程库
    m             # 数学库
)
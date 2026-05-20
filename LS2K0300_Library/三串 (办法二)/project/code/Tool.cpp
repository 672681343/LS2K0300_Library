#include "Tool.h"
#include <array>
#include <cstdint>

std::array<uint16_t, HEIGHT> tem_fit;

uint8_t M_x;
//-------------------------------------------------------------------------------------------------------------------
// 函数简介: 拟合线
// 参数说明: 拟合起始点，拟合结束点，原曲线数组，拟合数组
// 返回参数: 拟合数组
// 使用示例: fitting_line(start,end,l_black,fit_l_black);
// 备注信息: 
//-------------------------------------------------------------------------------------------------------------------
void fitting_line(uint8_t start,uint8_t end,
    const std::array<uint16_t, HEIGHT>& raw_line,
    std::array<uint16_t, HEIGHT>& fit_line
) {
    // 1. 打印调试信息
    // printf("Fitting between rows %d and %d\n", start, end);
    // printf("Raw values: [%d]=%d, [%d]=%d\n", start, raw_line[start], end, raw_line[end]);

    // 2. 自动交换顺序确保 start <= end
    if (start > end) {
        std::swap(start, end);
    }

    // 3. 边界检查
    if (start >= HEIGHT || end >= HEIGHT) {
        //printf("Error: Invalid range [%d, %d]\n", start, end);
        return;
    }

    // 4. 计算斜率和截距
    float k = 0.0f, b = 0.0f;
    if (start != end) {
        k = static_cast<float>(raw_line[end] - raw_line[start]) / (end - start);  // 注意分母是 end - start
        b = raw_line[start] - k * start;
    } else {
        b = raw_line[start];  // 单点情况
    }

    // 5. 拟合并存储结果
    for (uint8_t i = start; i <= end; ++i) {
        float fitted_value = k * i + b;
        fit_line[i] = static_cast<uint16_t>(std::round(fitted_value));
        
        // // 6. 绘制到屏幕（可选）
        // if (fit_line[i] < WIDTH) {
        //     Camera::lcd_pixels[i * WIDTH + fit_line[i]] = 0;
        // }
    }
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介: 二段式最小二乘法拟合
// 参数说明: 
// 返回参数: 
// 使用示例: 
// 备注信息: 
//-------------------------------------------------------------------------------------------------------------------
float two_segment_fitting_int(
    uint16_t start1, uint16_t end1,
    uint16_t start2, uint16_t end2,
    const std::array<uint16_t, HEIGHT>& raw_line,
    std::array<uint16_t, HEIGHT>& fit_line
) {
    int64_t sumX = 0;
    int64_t sumY = 0;
    uint32_t count = 0;

    // 处理第一个区间（支持反向遍历）
    auto process_sum = [&](uint16_t start, uint16_t end) {
        if (start <= end) {
            for (uint16_t i = start; i < end && i < HEIGHT; ++i) {
                sumX += i;
                sumY += raw_line[i];
                count++;
            }
        } else {
            for (uint16_t i = start; i > end && i < HEIGHT; --i) {
                sumX += i;
                sumY += raw_line[i];
                count++;
            }
        }
    };

    process_sum(start1, end1);
    process_sum(start2, end2);

    if (count == 0) {
        fit_line.fill(0);
        return 0.0f;
    }

    // 计算平均值（浮点运算）
    const double avgX = static_cast<double>(sumX) / count;
    const double avgY = static_cast<double>(sumY) / count;

    // 计算协方差和方差
    double sumUp = 0.0;
    double sumDown = 0.0;

    auto process_covariance = [&](uint16_t start, uint16_t end) {
        if (start <= end) {
            for (uint16_t i = start; i < end && i < HEIGHT; ++i) {
                const double dx = static_cast<double>(i) - avgX;
                const double dy = static_cast<double>(raw_line[i]) - avgY;
                sumUp += dy * dx;
                sumDown += dx * dx;
            }
        } else {
            for (uint16_t i = start; i > end && i < HEIGHT; --i) {
                const double dx = static_cast<double>(i) - avgX;
                const double dy = static_cast<double>(raw_line[i]) - avgY;
                sumUp += dy * dx;
                sumDown += dx * dx;
            }
        }
    };

    process_covariance(start1, end1);
    process_covariance(start2, end2);

    // 计算斜率
    const double parameterB = (sumDown != 0.0) ? (sumUp / sumDown) : 0.0;
    const double parameterA = avgY - parameterB * avgX;

    // 生成拟合结果
    for (uint16_t i = 0; i < HEIGHT; ++i) {
        const double y = parameterA + parameterB * i;
        fit_line[i] = static_cast<uint16_t>(std::max(0, std::min(static_cast<int>(y), WIDTH - 1)));
    }

    return static_cast<float>(parameterB);
}
//-------------------------------------------------------------------------------------------------------------------
// 函数简介: 方差
// 参数说明: 拟合起始点，拟合结束点，原曲线数组，拟合数组
// 返回参数: 方差
// 使用示例:Left_BF.Var  = fangcha(start,end,l_black,fit_l_black);
// 备注信息:
//-------------------------------------------------------------------------------------------------------------------
float fangcha(uint8_t start, uint8_t end,
        const std::array<uint16_t, HEIGHT>& raw_line,
        const std::array<uint16_t, HEIGHT>& fit_line)
{
    int i = 0;
    int sumlines = start - end;
    if (sumlines < 0) return 0;
    int sum = 0;
    float sumE = 0;

    for (i = start; i >= end; --i)
    {
        sum += (raw_line[i] - fit_line[i]) * (raw_line[i] - fit_line[i]);
    }

    if (sumlines != 0)
    {
        sumE = (float)((sum) / sumlines);
        return sumE;
    }
    else
        return 0;
}


//-------------------------------------------------------------------------------------------------------------------
// 函数简介: 环岛上拐点
// 参数说明: 
// 返回参数: 
// 使用示例: 
// 备注信息: 
//-------------------------------------------------------------------------------------------------------------------
uint8_t island_Up(uint8_t start, uint8_t end, uint8_t mode,
    const std::array<uint16_t, HEIGHT>& Search_line)
{
    uint8_t i = 0;
    uint8_t U_point = 0;
    if (end < 3) { end = 3; }
    if (start > HEIGHT - 5) { start = HEIGHT - 5; }
    uint8_t maxDelta = 5;
    int jumpThreshold = 15;
    if (mode == 0)
    {
        for (i = start; i > end; i--)
        {
            if (abs(Search_line[i - 2] - Search_line[i - 1]) < maxDelta &&
                abs(Search_line[i - 1] - Search_line[i]) < maxDelta &&
                Search_line[i] - Search_line[i + 2] > jumpThreshold &&
                Search_line[i] - Search_line[i + 3] > jumpThreshold && i < 50
                )
            {
                U_point = i;
                break;
            }
        }
    }
    else
    {
        for (i = start; i > end; i--)
        {

            if (abs(Search_line[i - 2] - Search_line[i - 1]) < maxDelta &&
                abs(Search_line[i - 1] - Search_line[i]) < maxDelta &&
                Search_line[i] - Search_line[i + 2] < -jumpThreshold &&
                Search_line[i] - Search_line[i + 3] < -jumpThreshold && i < 50
                )
            {
                U_point = i;
                break;
            }
        }
    }
    return U_point;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介: 环岛下拐点
// 参数说明: 
// 返回参数: 
// 使用示例: 
// 备注信息: 
//-------------------------------------------------------------------------------------------------------------------
uint8_t island_Down(uint8_t start, uint8_t end, uint8_t mode,
    const std::array<uint16_t, HEIGHT>& Search_line)
{
    uint8_t i = 0;
    uint8_t D_point = 0;
    if (end < 3) { end = 3; }
    if (start > HEIGHT - 3) { start = HEIGHT - 3; }
    uint8_t maxDelta = 3;
    uint8_t jumpThreshold = 10;
    if (mode == 0)
    {
        for (i = start; i > end; i--)
        {
            if ((Search_line[i] - Search_line[i-2]) >= jumpThreshold &&
                (Search_line[i] - Search_line[i-3]) >= jumpThreshold &&
                abs(Search_line[i] - Search_line[i + 1]) < maxDelta &&
                abs(Search_line[i + 1] - Search_line[i + 2]) < maxDelta 
                )
            {
                D_point = i;
                break;
            }
        }
    }
    else
    {
        for (i = start; i > end; i--)
        {
            if ((Search_line[i] - Search_line[i-3]) <= -jumpThreshold &&
                (Search_line[i] - Search_line[i-2]) <= -jumpThreshold &&
                abs(Search_line[i] - Search_line[i + 1]) < maxDelta &&
                abs(Search_line[i + 1] - Search_line[i + 2]) < maxDelta
                )
            {
                D_point = i;
                break;
            }
        }

    }
    return D_point;

}
//-------------------------------------------------------------------------------------------------------------------
// 函数简介: 环岛侧拐点
// 参数说明: 
// 返回参数: 
// 使用示例: 
// 备注信息: 
//-------------------------------------------------------------------------------------------------------------------
uint8_t island_sideUp(uint8_t start, uint8_t end, uint8_t mode,
    const std::array<uint16_t, HEIGHT>& Search_line)
{
    uint8_t i = 0;
    uint8_t U_point = 0;
    if (end < 3) { end = 3; }
    if (start > HEIGHT - 5) { start = HEIGHT - 5; }
    uint8_t maxDelta = 15;
    uint8_t jumpThreshold = 10;
    if (mode == 0)
    {
        for (i = start; i > end; i--)
        {
            if (abs(Search_line[i - 2] - Search_line[i - 1]) < maxDelta &&
                abs(Search_line[i - 1] - Search_line[i]) < maxDelta &&
                Search_line[i] - Search_line[i + 2] > jumpThreshold
                )
            {
                U_point = i;
                break;
            }
        }
    }
    else
    {
        for (i = start; i > end; i--)
        {

            if (abs(Search_line[i - 2] - Search_line[i - 1]) < maxDelta &&
                abs(Search_line[i - 1] - Search_line[i]) < maxDelta &&
                Search_line[i] - Search_line[i + 2] < -jumpThreshold
                )
            {
                U_point = i;
                break;
            }
        }
    }
    return U_point;
}
//-------------------------------------------------------------------------------------------------------------------
// 函数简介: 突变检测
// 参数说明: 方向，阈值，搜索开始，搜素截止，边线
// 返回参数: 圆弧中点
// 使用示例: 
// 备注信息: 
//-------------------------------------------------------------------------------------------------------------------
uint8_t Circular_Detection(uint8_t type, uint8_t limit, uint8_t start, uint8_t end,
    const std::array<uint16_t, HEIGHT>& Search_line)
{
    uint8_t n = 0;
    uint8_t dec = 0;
    uint8_t inc = 0;
    uint8_t Curve_Row = 0;
    
    // 验证开始和结束参数
    if (start >= HEIGHT) start = HEIGHT - 1;
    if (end >= HEIGHT) end = HEIGHT - 1;
    if (start < end) start = end;  // 确保起点高于终点
    
    if (type == 0)
    {
        for (uint8_t i = start; i > end; i--)
        {
            // 数组访问边界检查
            if (i == 0 || i == HEIGHT - 1) continue;
            
            if (Search_line[i] > 10 && Search_line[i - 1] > 10 && Search_line[i + 1] > 10)
            {
                if (Search_line[i] - Search_line[i - 1] == 0)
                {
                    n++;
                }
                else if (Search_line[i] - Search_line[i - 1] > 0)
                {
                    inc++;
                    inc += n;
                    n = 0;
                }
                else if (Search_line[i] - Search_line[i + 1] > 0)
                {
                    dec++;
                    dec += n;
                    n = 0;
                }
                
                if (inc > limit && dec > limit)
                {
                    Curve_Row = i;
                    break;
                    //return Curve_Row;
                }
            }
        }
        if (Curve_Row > 2)
        {
            //SetText("LCurve_Row_temp:" + Curve_Row);
            uint8_t max_x = 0;
            start = Curve_Row - 2;
            end = Curve_Row + 2;
            for (uint8_t j = start; j < end; j++)
            {
                //SetText("j:" + j + "R_black" + R_black[j]);
                if (Search_line[j] > max_x)
                {
                    Curve_Row = j;
                    max_x = Search_line[j];
                }
            }
            return Curve_Row;
        }
    }
    else if (type == 1)
    {
        for (uint8_t i = start; i > end; i--)
        {
            // 数组访问边界检查
            if (i == 0 || i == HEIGHT - 1) continue;
            
            if (Search_line[i] < 170 && Search_line[i - 1] < 170 && Search_line[i + 1] < 170)
            {
                if (Search_line[i] - Search_line[i - 1] == 0)
                {
                    n++;
                }
                else if (Search_line[i - 1] - Search_line[i] > 0)
                {
                    dec++;
                    dec += n;
                    n = 0;
                }
                else if (Search_line[i + 1] - Search_line[i] > 0)
                {
                    inc++;
                    inc += n;
                    n = 0;
                }
                
                if (inc > limit && dec > limit)
                {
                    Curve_Row = i;
                    break;
                    //return Curve_Row;
                }
            }
        }
        if(Curve_Row > 2)
        {
            //SetText("RCurve_Row_temp:" + Curve_Row);
            uint8_t min_x = 250;
            start = Curve_Row - 2;
            end = Curve_Row + 2;
            for (uint8_t j = start; j < end; j++)
            {
                //SetText("j:" + j + "R_black" + R_black[j]);
                if (Search_line[j] < min_x)
                {
                    Curve_Row = j;
                    min_x = Search_line[j];
                }
            }
            return Curve_Row;
        }
    }
    return 0;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介: 判断三角圆弧
// 参数说明: 无需多言
// 返回参数: 
// 使用示例: 
// 备注信息: 
//-------------------------------------------------------------------------------------------------------------------
uint8_t Region_Search(uint8_t x, uint8_t y,
    uint8_t Kerner_size, uint8_t* bin) 
{
    if (Kerner_size % 2 == 0) return 0;  // 仅支持奇数尺寸

    uint8_t window = Kerner_size / 2;

    // 动态边界处理，防止窗口越界
    x = (x < window) ? window : (x >= WIDTH - window) ? WIDTH - window - 1 : x;
    y = (y < window) ? window : (y >= HEIGHT - window) ? HEIGHT - window - 1 : y;

    uint8_t start_x = x - window;
    uint8_t end_x = x + window + 1;
    uint8_t start_y = y - window;
    uint8_t end_y = y + window + 1;
    uint8_t count = 0;

    for (uint8_t i = start_y; i < end_y; i++) {
        for (uint8_t j = start_x; j < end_x; j++) {
            uint16_t index = i * WIDTH + j;
            if (bin[index] == 0) count++;
        }
    }
    return count;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介: 十字找上拐点
// 参数说明: 
// 返回参数: 
// 使用示例: 
// 备注信息: 
//-------------------------------------------------------------------------------------------------------------------
uint8_t cross_Up(uint8_t start, uint8_t end, uint8_t mode,
    const std::array<uint16_t, HEIGHT>& Search_line)
{
    uint8_t i = 0;
    uint8_t U_point = 0;
    if (end < 3) { end = 3; }
    if (start > HEIGHT - 6) { start = HEIGHT - 6; }
    int8_t maxDelta = 7;
    int8_t jumpThreshold = 15;
    if(mode == 0)
    {
        for (i = start; i > end; i--)
        {
            if (abs(Search_line[i - 2] - Search_line[i - 1]) < maxDelta &&
                abs(Search_line[i - 1] - Search_line[i]) < maxDelta &&
                Search_line[i] - Search_line[i + 2] > jumpThreshold &&
                Search_line[i] - Search_line[i + 3] > jumpThreshold && i < 50
                )
            {
                U_point = i-1;
                break;
            }
    
        }
    }
    else
    {
        for (i = start; i > end; i--)
        {
            if (abs(Search_line[i - 2] - Search_line[i - 1]) < maxDelta &&
                abs(Search_line[i - 1] - Search_line[i]) < maxDelta &&
                Search_line[i] - Search_line[i + 2] < -jumpThreshold &&
                Search_line[i] - Search_line[i + 3] < -jumpThreshold && i < 50
                )
            {
                U_point = i-1;
                break;
            }
        }
    }
    return U_point;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介: 十字下拐点
// 参数说明: 
// 返回参数: 
// 使用示例: 
// 备注信息: 
//-------------------------------------------------------------------------------------------------------------------
uint8 cross_Down(uint8 start, uint8 end, uint8 mode,
    const std::array<uint16_t, HEIGHT>& Search_line)
{
    uint8_t i = 0;
    uint8_t D_point = 0;
    if (end < 3) { end = 3; }
    if (start > HEIGHT - 8) { start = HEIGHT - 8; }
    int8_t maxDelta = 5;
    int8_t jumpThreshold = 9;
    if(mode == 0)
    {
        for (i = start; i > end; i--)
        {
            if ((Search_line[i] - Search_line[i-2]) >= jumpThreshold &&
                (Search_line[i] - Search_line[i-3]) >= jumpThreshold &&
                abs(Search_line[i] - Search_line[i + 1]) < maxDelta &&
                abs(Search_line[i + 1] - Search_line[i + 2]) < maxDelta 
                )
            {
                D_point = i + 1;
                break;
            }
        }
    }
    else
    {
        for (i = start; i > end; i--)
        {
            if ((Search_line[i] - Search_line[i-3]) <= -jumpThreshold &&
                (Search_line[i] - Search_line[i-2]) <= -jumpThreshold &&
                abs(Search_line[i] - Search_line[i + 1]) < maxDelta &&
                abs(Search_line[i + 1] - Search_line[i + 2]) < maxDelta
                )
            {
                D_point = i + 1;
                break;
            }
        }

    }
    return D_point;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介: 斑马线上拐点
// 参数说明: 
// 返回参数: 
// 使用示例: 
// 备注信息: 
//-------------------------------------------------------------------------------------------------------------------
uint8_t zebra_Up(uint8_t start, uint8_t end, uint8_t mode,
    const std::array<uint16_t, HEIGHT>& Search_line)
{
    uint8_t i = 0;
    uint8_t U_point = 0;
    if (end < 3) { end = 3; }
    if (start > HEIGHT - 5) { start = HEIGHT - 5; }
    int8_t maxDelta = 6;
    int8_t jumpThreshold = 19;
    if (mode == 0)
    {
        for (i = start; i > end; i--)
        {
            if (abs(Search_line[i - 2] - Search_line[i - 1]) < maxDelta &&
                abs(Search_line[i - 1] - Search_line[i]) < maxDelta &&
                Search_line[i] - Search_line[i + 2] < -jumpThreshold
                )
            {
                U_point = i;
                break;
            }
        }
    }
    else
    {
        for (i = start; i > end; i--)
        {
            if (abs(Search_line[i - 2] - Search_line[i - 1]) < maxDelta &&
                abs(Search_line[i - 1] - Search_line[i]) < maxDelta &&
                Search_line[i] - Search_line[i + 2] > jumpThreshold
                )
            {
                U_point = i;
                break;
            }
        }
    }
    return U_point;
}
//-------------------------------------------------------------------------------------------------------------------
// 函数简介: 斑马线下拐点
// 参数说明: 
// 返回参数: 
// 使用示例: 
// 备注信息: 
//-------------------------------------------------------------------------------------------------------------------
uint8_t zebra_Down(uint8_t start, uint8_t end, uint8_t mode,
    const std::array<uint16_t, HEIGHT>& Search_line){
    uint8_t i = 0;
    uint8_t D_point = 0;
    if (end < 3) { end = 3; }
    if (start > HEIGHT - 3) { start = HEIGHT - 3; }
    int8_t maxDelta = 5;
    int8_t jumpThreshold = 23;
    if(mode == 0)
    {
        for (i = start; i > end; i--)
        {
            if ((Search_line[i + 2] - Search_line[i]) < -jumpThreshold &&
                abs(Search_line[i] - Search_line[i - 1]) < maxDelta &&
                abs(Search_line[i - 1] - Search_line[i - 2]) < maxDelta
                )
            {
                D_point = i;
                break;
            }
        }
    }
    else
    {
        for (i = start; i > end; i--)
        {
            if ((Search_line[i + 2] - Search_line[i]) > jumpThreshold &&
                abs(Search_line[i] - Search_line[i - 1]) < maxDelta &&
                abs(Search_line[i - 1] - Search_line[i - 2]) < maxDelta
                )
            {
                D_point = i;
                break;
            }
        }

    }
    return D_point;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介: 避障上拐点
// 参数说明: 
// 返回参数: 
// 使用示例: 
// 备注信息: 
//-------------------------------------------------------------------------------------------------------------------
uint8_t roadblack_Up(uint8_t start, uint8_t end, uint8_t mode,
    const std::array<uint16_t, HEIGHT>& Search_line)
{
    uint8_t i = 0;
    uint8_t U_point = 0;
    if (end < 3) { end = 3; }
    if (start > HEIGHT - 5) { start = HEIGHT - 5; }
    int8_t maxDelta = 5;
    int8_t jumpThreshold = 5;
    if (mode == 0)
    {
        for (i = start; i > end; i--)
        {
            if (abs(Search_line[i - 2] - Search_line[i - 1]) < maxDelta &&
                abs(Search_line[i - 1] - Search_line[i]) < maxDelta &&
                Search_line[i] - Search_line[i + 2] > jumpThreshold
                )
            {
                U_point = i;
                break;
            }
        }
    }
    else
    {
        for (i = start; i > end; i--)
        {
            if (abs(Search_line[i - 2] - Search_line[i - 1]) < maxDelta &&
                abs(Search_line[i - 1] - Search_line[i]) < maxDelta &&
                Search_line[i] - Search_line[i + 2] < -jumpThreshold
                )
            {
                U_point = i;
                break;
            }
        }
    }
    return U_point;
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介: 赛宽异常检测
// 参数说明: 
// 返回参数: 
// 使用示例: 
// 备注信息: 
//-------------------------------------------------------------------------------------------------------------------
int16_t width_anomaly_detection (uint8_t start, uint8_t end, 
    const std::array<uint16_t, HEIGHT>& l_line,
    const std::array<uint16_t, HEIGHT>& r_line,
    const std::array<uint8_t, HEIGHT>& Contrast_line)
{

    if (start >= HEIGHT || end >= HEIGHT || start <= end) {
        return 0;  // 无效范围
    }
    if (start >= HEIGHT) start = HEIGHT - 1;  // 确保起点不超过高度
    if (start < 0) start = 0;  // 确保起点不小于0
    if (end >= HEIGHT) end = HEIGHT - 1;  // 确保终点不超过高度
    if (end < 0) end = 0;  // 确保终点不小于0
    uint8_t row_count = start - end;  // 计算行数
    int16_t weight_sum = 0;  // 初始化赛宽总和
    for (uint8_t i = end; i < start; i++){
        uint8_t weight = r_line[i] - l_line[i];  // 计算实际赛宽
        weight_sum = weight_sum + (weight - Contrast_line[i]);  // 累加赛宽
    }
    // 计算平均赛宽
    int16_t average_weight = static_cast<int16_t>(weight_sum) / row_count;
    return average_weight;
}


//补线内部函数--------------------------------------------------------------------------------------
static inline void clamp_coords(uint8_t *x, uint8_t *y) {
    *x = (*x >= WIDTH - 1) ? WIDTH - 1 : (*x < 0) ? 0 : *x;
    *y = (*y >= HEIGHT - 1) ? HEIGHT - 1 : (*y < 0) ? 0 : *y;
}

static inline void update_visual(uint8_t y, uint8_t hx, 
    uint16_t *target, uint16_t *other, 
    int is_left) {
    // 强化边界检查（保持不变）
    hx = (hx >= WIDTH - 2) ? WIDTH - 2 : (hx < 2) ? 2 : hx;
    y = (y >= HEIGHT - 2) ? HEIGHT - 2 : (y < 2) ? 2 : y;

    // 更新目标边界
    target[y] = (uint16_t)hx;


    // 修正后的冲突处理（容差机制）
    if (is_left) {
        if (hx >= other[y]) other[y] = WIDTH - 1;  // 左边界覆盖右边界时，右边界同步到左边界坐标[1,5](@ref)
    } else {
        if (hx <= other[y]) other[y] = 1;  // 右边界覆盖左边界时，左边界同步到右边界坐标[1,5](@ref)
    }

    // // 安全写入可视化数据（保持不变）
    // uint32_t index = (uint32_t)y * WIDTH + hx;
    // if (index < WIDTH * HEIGHT) {
    //     Camera::lcd_pixels[index] = 1;
    // }
}
//补线内部函数--------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
// 函数简介: 补线函数
// 参数说明: 起始点1坐标，起始点2坐标，斜率类型，斜率方向，初始斜率
// 返回参数: 图像补线画线
// 使用示例: 
// 备注信息: 两点模式不在乎绘制方向，一点延长模式和斜率模式需要注意绘制方向，一点延长模式y1是自动获取
//-------------------------------------------------------------------------------------------------------------------
float my_draw_line(uint8_t x1, 
                  uint8_t y1, 
                  uint8_t x2, 
                  uint8_t y2,
                  LineMode mode, 
                  LineDirection dir, 
                  float slope) {
    uint8_t start, end;
    int is_left = (dir == DIRECTION_UP_LEFT || dir == DIRECTION_DOWN_LEFT);
    int is_upward = (dir == DIRECTION_UP_LEFT || dir == DIRECTION_UP_RIGHT);
    uint16_t *target = is_left ? ImageProc::l_black.data() : ImageProc::r_black.data();
    uint16_t *other = is_left ? ImageProc::r_black.data() : ImageProc::l_black.data();
    float k = 0.0f;
    switch (mode) {
        case MODE_TWO_POINTS: {
            clamp_coords(&x1, &y1);
            clamp_coords(&x2, &y2);

            // 保持y降序（适配左上角坐标系）
            if (y1 < y2) {
                SWAP(y1, y2);
                SWAP(x1, x2);
            }

            k = (y1 == y2) ? 0 : (float)(x2 - x1) / (y1 - y2);  // 注意分母取反

            for (uint8_t y = y1; y >= y2; --y) {
                uint8_t dx = x1 + (uint8_t)(k * (y1 - y));
                update_visual(y, dx, target, other, is_left);
            }
            break;
        }

        
        case MODE_AUTO_SLOPE: {
            clamp_coords(&x1, &y1);

            // 自动获取参考点（适配坐标系方向）
            int y_offset = is_upward ? -5 : 5;
            uint8_t ref_y = y1 + y_offset;
            if (ref_y >= HEIGHT || ref_y < 0) break;
            x1 = (uint8_t)target[y1];
            x2 = (uint8_t)target[ref_y];
            y2 = ref_y;

            k = (y1 == y2) ? 0 : (float)(x2 - x1) / (y1 - y2);

            start = y1;
            end = is_upward ? my_min(HEIGHT-1, y1 + 50) : my_max(1, y1 - 50);
            
            if (is_upward) {
                for (uint8_t y = start; y <= end; ++y) {
                    uint8_t dx = x1 + (uint8_t)(k * (y1 - y));
                    // 边界冲突检查
                    if (dx >= 185 || dx <= 1) break; // 左边线遇到右边线则停止
                    update_visual(y, dx, target, other, is_left);
                }
            } else {
                for (uint8_t y = start; y >= end; --y) {
                    uint8_t dx = x1 + (uint8_t)(k * (y1 - y));
                    // 边界冲突检查
                    if (dx >= 185 || dx <= 1) break; // 左边线遇到右边线则停止
                    update_visual(y, dx, target, other, is_left);
                }
            }
            break;
        
        }
        
        /*case MODE_AUTO_SLOPE:{
            clamp_coords(&x1, &y1);
            // 定义数据拟合区间
            int y_offset = is_upward ? -5 : 5;
            uint8_t fitStart = y1 + y_offset;
            uint8_t fitEnd = y1;
            // 检查拟合区间合法性
            if (fitStart < 0 || fitStart >= HEIGHT) break;
            if (fitEnd < 0 || fitEnd >= HEIGHT) break;
            //调用工具函数拟合
            if(is_left){
                k = two_segment_fitting_int(fitStart, fitEnd, 0, 0, ImageProc::l_black, ImageProc::fit_l_black);
            }else{
                k = two_segment_fitting_int(fitStart, fitEnd, 0, 0, ImageProc::r_black, ImageProc::fit_r_black);
            }

            if (!is_upward)
            {
                k = -k; // 反转斜率方向
            }
            start = y1;
            end = is_upward ? my_min(HEIGHT-1, y1 + 50) : my_max(1, y1 - 50);
            if (is_upward) {
                for (uint8_t y = start; y <= end; ++y) {
                    uint8_t dx = x1 + (uint8_t)(k * (y - y1));
                    //printf("dx: %d", dx);
                    // if (is_left) {
                    //     dx = my_max(dx, target[y]);
                    // }else
                    // {
                    //     dx = my_min(dx, target[y]);
                    // }
                    
                    // 边界冲突检查
                    if (is_left) {
                        if (dx >= 185 || dx <= 1) break; // 左边线遇到右边线则停止
                    } else {
                        if (dx >= 185 || dx <= 1) break; // 右边线遇到左边线则停止
                    }
                    update_visual(y, dx, target, other, is_left);
                }
            } else {
                for (uint8_t y = start; y >= end; --y) {
                    uint8_t dx = x1 + (uint8_t)(k * (y1 - y));
                    // if (is_left) {
                    //     dx = my_max(dx, target[y]);
                    // }else
                    // {
                    //     dx = my_min(dx, target[y]);
                    // }
                    // 边界冲突检查
                    if (is_left) {
                        if (dx >= 185 || dx <= 1) break; // 左边线遇到右侧则停止
                    } else {
                        if (dx >= 185 || dx <= 1) break; // 右边线遇到左侧则停止
                    }
                    update_visual(y, dx, target, other, is_left);
                }
            }
            break;
        }*/
        case MODE_FIXED_SLOPE: {
            clamp_coords(&x1, &y1);
            start = y1;
            end = is_upward ? my_min(HEIGHT-1, y1 + 50) : my_max(1, y1 - 50);
            //printf("y1: %d, x1: %d, start: %d, end: %d\n", y1, x1, start, end);
            if (is_upward) {
                for (uint8_t y = start; y <= end; ++y) {
                    uint8_t dx = x1 + (uint8_t)(slope * (y - y1));
                    //printf("dx: %d", dx);
                    if (is_left) {
                        dx = my_max(dx, target[y]);
                    }else
                    {
                        dx = my_min(dx, target[y]);
                    }
                    // 边界冲突检查
                    if (is_left) {
                        if (dx >= 185 || dx <= 1) break; // 左边线遇到右侧则停止
                    } else {
                        if (dx >= 185 || dx <= 1) break; // 右边线遇到左侧则停止
                    }
                    update_visual(y, dx, target, other, is_left);
                }
            } else {
                for (uint8_t y = start; y >= end; --y) {
                    uint8_t dx = x1 + (uint8_t)(slope * (y1 - y));
                    if (is_left) {
                        dx = my_max(dx, target[y]);
                    }else
                    {
                        dx = my_min(dx, target[y]);
                    }
                    // 边界冲突检查
                    if (is_left) {
                        if (dx >= 185 || dx <= 1) break; // 左边线遇到右侧则停止
                    } else {
                        if (dx >= 185 || dx <= 1) break; // 右边线遇到左侧则停止
                    }
                    update_visual(y, dx, target, other, is_left);
                }
            }
            break;
        }
    }
    return k;  // 返回斜率
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介: 斜入十字上的抖M拐点
// 参数说明: 
// 返回参数: 
// 使用示例: 
// 备注信息: 
//-------------------------------------------------------------------------------------------------------------------

uint8_t M_point(uint8_t start, uint8_t end,
    const std::array<uint8_t, WIDTH>& Search_line){

    // 验证开始和结束参数
    if (start >= WIDTH-6) start = WIDTH - 6;
    if (start <= 6) start = 6;
    if (end >= WIDTH-6) end = WIDTH - 6;
    if (end <= 6) end = 6;
    if (start > end) start = end;  // 确保起点在终点左侧



        uint8_t j = 0;
        for(j = start + 5  ; j < end - 5 ; j++){
            int8_t left_diff = Search_line[j-5] - Search_line[j];
            int8_t right_diff = Search_line[j+5] - Search_line[j];
            if (left_diff >= 1 && right_diff >= 1 && Search_line[j] != 0 && Search_line[j] <= 68) {
                M_x = j;
                return HEIGHT-Search_line[j]-2;
            }
        }
    return 0;

}


// //-------------------------------------------------------------------------------------------------------------------
// // 函数简介: 去零漂
// // 参数说明: 
// // 返回参数: 
// // 使用示例: 
// // 备注信息: 
// //-------------------------------------------------------------------------------------------------------------------

// int16_t linpiao(){
//     uint8_t count =0;
//     while()
// }
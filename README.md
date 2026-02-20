# 天气时钟显示系统

基于STM32F407和LVGL的智能天气时钟显示系统，支持WiFi连接、温湿度监测、网络时间同步和实时天气信息显示。

## 项目简介

本项目是一个功能完整的嵌入式系统，通过LCD屏幕实时显示时间、日期、室内温湿度和室外天气信息。系统使用FreeRTOS进行多任务管理，LVGL作为图形用户界面库，ESP32c3模块实现WiFi连接和网络通信。

## 硬件平台

### 主控芯片
- **MCU**: STM32F407ZGT6
- **主频**: 168MHz
- **Flash**: 1MB
- **RAM**: 192KB (128KB + 64KB CCM)

### 外设模块

- **LCD屏幕**: ST7789控制器，240x320分辨率，RGB565颜色格式
- **WiFi模块**: ESP32c3 (ESP-AT固件)
- **温湿度传感器**: DHT11
- **实时时钟**: STM32内部RTC
- **按键**: 3个按键（KEY0、KEY1、KEY2）
- **LED**: 板载LED
- **蜂鸣器**: 板载蜂鸣器

## 软件架构

### 操作系统

- **FreeRTOS**: 实时操作系统，实现多任务调度
- **CMSIS-RTOS2**: 标准RTOS接口

### 图形库
- **LVGL 8.x**: 轻量级嵌入式图形库
- **配置**: RGB565颜色深度，48KB内存池

### 网络协议
- **HTTP**: 用于获取天气API数据
- **SNTP**: 用于网络时间同步

## 项目结构

```
lvgl/
├── Core/                          # STM32核心代码
│   ├── Inc/                       # 头文件
│   │   ├── FreeRTOSConfig.h       # FreeRTOS配置
│   │   ├── main.h                # 主程序头文件
│   │   ├── stm32f4xx_hal_conf.h # HAL库配置
│   │   └── ...
│   └── Src/                       # 源文件
│       ├── freertos.c             # FreeRTOS任务和初始化
│       ├── main.c                # 主程序入口
│       ├── stm32f4xx_it.c       # 中断处理
│       └── ...
├── Drivers/                       # 驱动代码
│   ├── BSP/                       # 板级支持包
│   │   ├── bsp_lcd/              # LCD驱动
│   │   │   ├── lcd.c            # LCD底层驱动
│   │   │   ├── lcd.h
│   │   │   └── lcd_ex.c         # LCD控制器初始化
│   │   ├── bsp_delay.c           # 延时函数
│   │   ├── bsp_dht11.c          # DHT11温湿度传感器
│   │   ├── bsp_espat.c          # ESP8266 AT指令
│   │   ├── bsp_key.c            # 按键驱动
│   │   ├── bsp_rtc.c            # RTC驱动
│   │   ├── bsp_usart.c          # 串口驱动
│   │   ├── led.c                # LED驱动
│   │   └── bsp_beep.c           # 蜂鸣器驱动
│   ├── CMSIS/                    # CMSIS库
│   └── STM32F4xx_HAL_Driver/     # STM32 HAL库
├── APP/                          # 应用层代码
│   ├── lvgl_ui/                   # LVGL用户界面
│   │   ├── lv_page_manager.c     # 页面管理器
│   │   ├── lv_page_manager.h
│   │   ├── lv_test_page.c       # LVGL测试页面
│   │   ├── lv_test_page.h
│   │   ├── lv_welcome_page.c    # 欢迎页面
│   │   ├── lv_welcome_page.h
│   │   ├── lv_wifi_page.c        # WiFi连接页面
│   │   ├── lv_wifi_page.h
│   │   ├── lv_main_page.c       # 主显示页面
│   │   ├── lv_main_page.h
│   │   ├── lv_error_page.c      # 错误页面
│   │   └── lv_error_page.h
│   ├── lvgl_port/                 # LVGL移植代码
│   │   ├── lv_port_disp.c       # LVGL显示驱动
│   │   ├── lv_port_disp.h
│   │   ├── lv_port_indev.c      # LVGL输入设备驱动
│   │   └── lv_port_indev.h
│   ├── page/                      # 旧版LCD页面代码
│   │   ├── main_page.c
│   │   ├── welcome_page.c
│   │   ├── wifi_page.c
│   │   └── error_page.c
│   ├── weather/                   # 天气相关代码
│   │   ├── weather.c            # 天气API解析
│   │   └── weather.h
│   ├── imag/                      # 图片资源
│   │   ├── icon_wifi.c          # WiFi图标
│   │   ├── icon_wenduji.c       # 温度计图标
│   │   ├── icon_qing.c          # 晴天图标
│   │   ├── icon_yintian.c       # 阴天图标
│   │   ├── icon_duoyun.c        # 多云图标
│   │   ├── icon_zhongyu.c       # 中雨图标
│   │   ├── icon_zhongxue.c      # 中雪图标
│   │   ├── icon_leizhenyu.c     # 雷阵雨图标
│   │   ├── icon_yueliang.c      # 月亮图标
│   │   ├── icon_na.c            # 未知图标
│   │   ├── img_chengpingan.c    # 岁岁平安图片
│   │   ├── img_error.c          # 错误图片
│   │   └── imag.h
│   ├── font/                      # 字体资源
│   │   ├── lcdfont.c           # LCD字体
│   │   ├── font16_maple.c      # 16号字体
│   │   ├── font20_maple_bold.c # 20号粗体
│   │   └── ...
│   ├── wifi.c                     # WiFi管理
│   ├── app.h                      # 应用头文件
│   └── mloop.c                    # 主循环
├── LVGL/                         # LVGL图形库
│   ├── src/                       # LVGL源代码
│   ├── examples/                  # LVGL示例
│   ├── demos/                     # LVGL演示
│   └── lv_conf.h                 # LVGL配置文件
├── Middlewares/                   # 中间件
│   └── Third_Party/
│       └── FreeRTOS/             # FreeRTOS源代码
├── .eide/                       # EIDE工程配置
├── .vscode/                     # VSCode配置
├── .trae/                       # Trae配置
└── README.md                     # 项目说明文档
```

## 主要功能

### 1. 多页面UI系统

- **欢迎页面**: 显示"岁岁平安"标题和加载动画
- **WiFi页面**: 显示WiFi连接状态
- **主页面**: 显示时间、日期、室内温湿度和室外天气
- **错误页面**: 显示系统错误信息

### 2. 实时时间显示
- **RTC时间**: 使用STM32内部RTC
- **网络同步**: 通过SNTP协议同步网络时间
- **自动校准**: 定期同步网络时间

### 3. 温湿度监测

- **室内温湿度**: 通过DHT11传感器读取
- **实时更新**: 每3秒更新一次
- **数据验证**: 自动检测传感器故障

### 4. 天气信息显示
- **天气API**: 使用心知天气API
- **实时天气**: 显示当前温度和天气状况
- **天气图标**: 根据天气代码显示对应图标
- **自动更新**: 每60秒更新一次

### 5. WiFi连接

- **自动连接**: 启动时自动连接到配置的WiFi
- **断线重连**: WiFi断开后自动重连
- **状态监控**: 实时监控WiFi连接状态

## FreeRTOS任务

系统使用FreeRTOS实现多任务调度，主要任务包括：

| 任务名称 | 优先级 | 堆栈大小 | 功能描述 |
|---------|---------|-----------|---------|
| lvglTask | High | 8KB | LVGL图形库主循环，处理UI渲染和事件 |
| wifiTask | Normal | 2KB | WiFi连接管理，监控WiFi状态 |
| timeTask | Normal | 2KB | 时间管理，SNTP时间同步 |
| sensorTask | Normal | 2KB | 传感器数据读取，温湿度更新 |
| weatherTask | Normal | 2KB | 天气信息获取和更新 |
| defaultTask | Normal | 2KB | 默认任务，等待事件 |

## 事件标志

系统使用FreeRTOS事件标志进行任务间同步：

| 事件标志 | 描述 | 触发任务 |
|---------|------|---------|
| EVENT_LVGL_READY | LVGL初始化完成 | wifiTask |
| EVENT_WIFI_CONNECTED | WiFi连接成功 | timeTask, weatherTask |
| EVENT_TIME_SYNCED | 时间同步完成 | sensorTask |
| EVENT_MAIN_PAGE_READY | 主页面准备完成 | sensorTask |

## LVGL配置

### 颜色配置
- **颜色深度**: 16位 (RGB565)
- **字节交换**: 启用 (LV_COLOR_16_SWAP = 1)
- **色键**: 纯绿色 (0x00FF00)

### 内存配置

- **内存池大小**: 48KB
- **显示缓冲区**: 240x5像素
- **层缓冲区**: 8KB

### 组件配置

- **启用的组件**: 
  - Label (标签)
  - Button (按钮)
  - Image (图片)
  - Bar (进度条)
  - Slider (滑块)
  - Switch (开关)
  - Checkbox (复选框)
- **禁用的组件**: 
  - Arc (弧形)
  - Canvas (画布)
  - Button Matrix (按钮矩阵)
  - Dropdown (下拉列表)
  - Keyboard (键盘)
  - List (列表)
  - Roller (滚动器)
  - Textarea (文本框)

### 字体配置
- **默认字体**: Montserrat系列
- **启用的字体**: 
  - Montserrat 14
  - Montserrat 16
  - Montserrat 20

## 编译和烧录

### 开发环境
- **IDE**: EIDE (Embedded IDE)
- **编译器**: ARM GCC
- **调试器**: ST-Link V2
- **目标板**: STM32F407ZGT6

### 编译步骤

1. 打开EIDE工程
2. 选择Debug配置
3. 点击"Build"按钮
4. 等待编译完成

### 烧录步骤

1. 连接ST-Link调试器到开发板
2. 在EIDE中选择"Upload" → "STLink"
3. 点击"Upload"按钮
4. 等待烧录完成

### 串口配置
- **波特率**: 115200
- **数据位**: 8
- **停止位**: 1
- **校验位**: None

## 配置说明

### WiFi配置
在`Core/Src/freertos.c`中修改WiFi配置：
```c
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWD "your_wifi_password"
```

### 天气API配置

在`Core/Src/freertos.c`中修改天气API URL：
```c
static const char *weather_url = "http://api.seniverse.com/v3/weather/now.json?key=YOUR_API_KEY&location=YOUR_CITY&language=en&unit=c";
```

### LCD分辨率配置

在`APP/lvgl_port/lv_port_disp.c`中修改：
```c
#define MY_DISP_HOR_RES    240
#define MY_DISP_VER_RES    320
```

## 调试

### 串口调试
系统通过串口输出详细的调试信息，包括：
- 系统初始化状态
- 任务创建和运行状态
- WiFi连接状态
- 传感器数据
- LVGL渲染信息
- 页面切换信息

### LCD测试
系统启动时会自动执行LCD硬件测试，依次显示：
- 黑色屏幕
- 白色屏幕
- 红色屏幕
- 绿色屏幕
- 蓝色屏幕

### LVGL测试
系统默认启动LVGL测试页面，包含：
- 按钮（带点击计数）
- 滑块（带数值显示）
- 开关
- 进度条
- 复选框

## 已知问题

1. **中文字体支持**: 当前使用的Montserrat字体不支持中文字符，中文会显示为方框
2. **触摸功能**: 当前使用按键模拟触摸，尚未实现真正的触摸功能
3. **天气API**: 使用免费API，可能有调用次数限制

## 未来改进

1. **中文字体**: 添加中文字体支持
2. **触摸功能**: 实现真正的触摸屏支持
3. **天气预报**: 添加多天天气预报功能
4. **闹钟功能**: 添加闹钟和提醒功能
5. **数据存储**: 添加历史数据存储功能
6. **低功耗**: 优化功耗，支持电池供电

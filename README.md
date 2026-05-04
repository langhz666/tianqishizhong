# 智能天气时钟 (Weather Clock)

基于 STM32F407 + LVGL 的智能天气时钟显示系统，支持 WiFi 连接、室内温湿度监测、网络时间同步和实时天气信息显示。

## 功能演示

> **[点击观看演示视频](Project/asset/demo.mp4)**

## 硬件平台

### 主控芯片

| 参数 | 规格 |
|------|------|
| MCU | STM32F407ZGT6 |
| 主频 | 168 MHz |
| Flash | 1 MB |
| RAM | 192 KB (128KB + 64KB CCM) |
| FPU | 单精度浮点单元 |

### 外设模块

| 模块 | 型号/规格 | 用途 |
|------|----------|------|
| LCD 屏幕 | ST7789, 240x320, RGB565 | 图形显示 |
| WiFi 模块 | ESP32-C3 (AT 固件) | 网络通信 |
| 温湿度传感器 | DHT11 | 室内环境监测 |
| 实时时钟 | STM32 内部 RTC (LSE) | 时间保持 |
| LED | PF9/PF10 | 状态指示 |
| 蜂鸣器 | PF8 | 声音提示 |
| 按键 | KEY0/KEY1/KEY2/KEY_UP | 用户输入 |

## 软件架构

### 系统架构图

```
┌─────────────────────────────────────────────────────┐
│                    FreeRTOS 调度器                    │
├──────────┬──────────┬──────────┬──────────┬──────────┤
│ lvglTask │ wifiTask │ timeTask │sensorTask│weatherTask│
│ (5ms)    │ (监控)   │ (1s)     │ (3s)     │ (60s)    │
├──────────┴──────────┴──────────┴──────────┴──────────┤
│                    BSP 驱动层                         │
│  bsp_espat │ bsp_dht11 │ bsp_rtc │ lcd │ bsp_key    │
├─────────────────────────────────────────────────────┤
│                STM32 HAL 库 + LVGL                   │
├─────────────────────────────────────────────────────┤
│              STM32F407 硬件平台                       │
└─────────────────────────────────────────────────────┘
```

### 技术栈

| 组件 | 版本/说明 |
|------|----------|
| RTOS | FreeRTOS 10.3.1 (CMSIS-RTOS2 V2) |
| 图形库 | LVGL 8.3.11 (RGB565, 32KB 内存池) |
| HAL 库 | STM32F4xx HAL Driver |
| 编译器 | ARM GCC (arm-none-eabi-gcc) |
| 构建系统 | Makefile + EIDE |

### FreeRTOS 任务

| 任务名称 | 优先级 | 栈大小 | 功能描述 |
|---------|--------|--------|---------|
| lvglTask | Normal | 4KB | LVGL 图形渲染，每 5ms 调用一次 |
| wifiTask | Normal | 4KB | WiFi 连接管理，断线自动重连 |
| timeTask | Normal | 1KB | SNTP 时间同步，每秒更新显示 |
| sensorTask | Normal | 1KB | DHT11 读取，每 3 秒更新温湿度 |
| weatherTask | BelowNormal | 2KB | 天气 API 请求，每 60 秒更新 |
| defaultTask | Normal | 512B | 空闲任务 |

### 同步机制

| 同步对象 | 类型 | 用途 |
|---------|------|------|
| lcd_mutex | 互斥锁 | 保护 LVGL 线程安全 |
| esp_mutex | 互斥锁 | 保护 ESP 模块访问 |
| EVENT_WIFI_CONNECTED | 事件标志 | WiFi 连接完成通知 |
| EVENT_TIME_SYNCED | 事件标志 | 时间同步完成通知 |
| EVENT_MAIN_PAGE_READY | 事件标志 | 主页面就绪通知 |

## 项目结构

```
Project/
├── Core/                          # STM32 核心代码
│   ├── Inc/                       # 头文件
│   │   ├── main.h                 # 主程序头文件（GPIO引脚定义）
│   │   ├── FreeRTOSConfig.h       # FreeRTOS 配置
│   │   └── ...
│   └── Src/                       # 源文件
│       ├── main.c                 # 主程序入口，系统初始化
│       ├── freertos.c             # FreeRTOS 任务实现（核心逻辑）
│       ├── gpio.c                 # GPIO 初始化（CubeMX 生成）
│       ├── usart.c                # UART 初始化（CubeMX 生成）
│       ├── rtc.c                  # RTC 初始化（CubeMX 生成）
│       └── fsmc.c                 # FSMC 初始化（CubeMX 生成）
├── Drivers/                       # 驱动代码
│   └── BSP/                       # 板级支持包
│       ├── bsp_espat.c/h          # ESP AT 指令驱动
│       ├── bsp_dht11.c/h          # DHT11 温湿度传感器驱动
│       ├── bsp_rtc.c/h            # RTC 读写驱动（带验证）
│       ├── bsp_key.c/h            # 按键驱动
│       ├── bsp_delay.c/h          # DWT 微秒延时
│       ├── bsp_beep.c/h           # 蜂鸣器驱动
│       ├── bsp_usart.c/h          # printf 重定向
│       ├── led.c/h                # LED 驱动
│       ├── bsp_lcd/               # LCD 驱动（FSMC 16bit）
│       └── TOUCH/                 # 触摸屏驱动（预留）
├── APP/                           # 应用层代码
│   ├── app.h                      # 全局配置（WiFi SSID/密码）
│   ├── wifi.c                     # WiFi 初始化和连接
│   ├── mloop.c                    # 旧版主循环（已弃用）
│   ├── page/
│   │   ├── page.h                 # 页面函数声明
│   │   └── lvgl_pages.c           # LVGL UI 实现（全部页面）
│   ├── weather/
│   │   ├── weather.c/h            # 心知天气 API 解析
│   ├── font/
│   │   └── lcdfont.c/h            # 位图字体数据
│   └── imag/
│       ├── imag.h                 # 图片资源声明
│       ├── img_chengpingan.c      # 欢迎页 Logo
│       └── xiaozhang.c            # 主页面背景
├── LVGL/                          # LVGL 图形库（vendor）
├── Middlewares/                    # FreeRTOS 中间件（vendor）
├── Makefile                       # GCC ARM 构建脚本
└── STM32F407XX_FLASH.ld           # 链接脚本
```

## UI 界面

系统包含 4 个页面，支持滑动切换：

### 欢迎页 (Splash Screen)
- Logo 图片（弹性动画入场）
- "Weather Clock" 标题
- 进度条 + 百分比文字
- 状态提示文本

### WiFi 配网页面
- WiFi 图标动画
- SSID 名称显示
- 连接状态（Spinner 加载动画）

### 主界面（3 个 Tab 页）

**Tab 1 - 时间与环境**
- 大号时钟显示（Montserrat 38）
- 年月日 + 星期
- 室内卡片（温度 + 湿度，DHT11）
- 室外卡片（温度 + 天气图标，API 数据）

**Tab 2 - 天气详情**
- 城市定位
- 超大温度显示（Montserrat 48）
- 天气描述
- 湿度和风力详情

**Tab 3 - 系统设置**
- 定位城市
- MCU 型号
- WiFi 模块型号
- 固件版本

### 错误页
- 警告图标
- 错误信息文本

### 天气图标

系统使用 `lv_canvas` 自定义绘制天气图标：

| 天气 | 代码 | 绘制方式 |
|------|------|---------|
| 晴天 | 0, 2, 38 | 圆形 + 8 条光线 |
| 夜晚 | 1, 3 | 新月形 |
| 多云 | 4, 9, 30 | 3 个圆 + 圆角矩形 |
| 雨天 | 10-19 | 云朵 + 斜线 |
| 雷雨 | 11, 12 | 云朵 + 闪电折线 |
| 雪天 | 20-25 | 云朵 + 白色小点 |
| 未知 | 其他 | "??" 文字 |

## 编译和烧录

### 开发环境

- **IDE**: EIDE (Embedded IDE) 或 VSCode
- **编译器**: ARM GCC (arm-none-eabi-gcc)
- **调试器**: ST-Link V2 (SWD, 4MHz)
- **目标板**: STM32F407ZGT6

### 使用 Makefile 编译

```bash
cd Project
make          # 编译
make clean    # 清理
```

编译产物位于 `Project/build/` 目录：
- `Project.elf` - 可执行文件
- `Project.hex` - Intel HEX 格式
- `Project.bin` - 二进制格式

使用 EIDE 编译时，产物位于 `Project/build/Debug/` 目录：
- `lvgl.elf` - 可执行文件
- `lvgl.hex` - Intel HEX 格式

### 使用 EIDE 编译

1. 打开 `Project/Project.code-workspace`
2. 选择 Debug 配置
3. 点击 "Build" 按钮

### 烧录

1. 连接 ST-Link 调试器
2. 在 EIDE 中选择 "Upload" -> "STLink"
3. 点击 "Upload" 按钮

### 串口调试

- **波特率**: 115200
- **数据位**: 8
- **停止位**: 1
- **校验位**: 无
- **串口**: USART1 (PA9-TX, PA10-RX)

## 配置说明

### WiFi 配置

修改 `Project/APP/app.h` 中的宏定义：

```c
#define WIFI_SSID   "your_wifi_ssid"
#define WIFI_PASSWD "your_wifi_password"
```

### 天气 API 配置

修改 `Project/Core/Src/freertos.c` 中的 URL：

```c
static const char *weather_url =
    "http://api.seniverse.com/v3/weather/daily.json?key=YOUR_KEY&location=YOUR_CITY&language=en&unit=c&days=1";
```

API 来源：[心知天气 (Seniverse)](https://www.seniverse.com/)

### 系统时钟

系统时钟配置在 `Project/Core/Src/main.c` 的 `SystemClock_Config()` 中：

```
HSE (8MHz) -> PLL (M=4, N=168, P=2) -> SYSCLK = 168MHz
AHB = 168MHz, APB1 = 42MHz, APB2 = 84MHz
LSE (32.768kHz) -> RTC
```

## 代码规范

### 注释风格

- 使用 Doxygen 格式注释
- 中文注释，UTF-8 编码
- 文件头：`@file`、`@brief`、`@author`、`@version`
- 函数头：`@brief`、`@param`、`@return`、`@note`
- 行内注释：`/* */` 格式

### 命名规范

| 类型 | 规范 | 示例 |
|------|------|------|
| BSP 文件 | `bsp_` 前缀 | `bsp_espat.c` |
| 结构体 | `_t` 后缀 | `weather_info_t` |
| 宏定义 | 大写下划线 | `EVENT_WIFI_CONNECTED` |
| 函数 | 小写下划线 | `main_page_redraw_time()` |

### 线程安全

- 所有 LVGL 操作必须通过 `lcd_lock()`/`lcd_unlock()` 保护
- 所有 ESP AT 指令必须通过 `esp_lock()`/`esp_unlock()` 保护
- UI 更新函数内置变化检测，数据未变时跳过重绘

## 已知问题

1. **中文字体**：当前使用 Montserrat 字体，不支持中文字符
2. **触摸功能**：触摸屏驱动已存在但未启用，使用按键模拟
3. **天气 API**：免费版有调用次数限制
4. **WiFi 凭据**：硬编码在源文件中，未实现动态配网

## 许可证

本项目基于 STM32CubeMX 生成的代码框架开发。
LVGL 库遵循 MIT 许可证。
FreeRTOS 库遵循 MIT 许可证。

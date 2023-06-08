/*
  此程序用于实现 wooting 键盘中的 Rapid trigger 功能。
  
  主功能移植并优化自 Github 上的一个开源项目，添加了按键的独立参数和两端的死区，优化了初始化方式，增加控制器，模仿稚晖君 MonoUI 的超丝滑菜单。

  v3.1 更新内容：

    * 修复EC11旋钮使界面卡死的问题，感谢 GitHub 安红豆 提供的线索。
    
  v3 功能：

    * 此程序可视为 WouoUI v2.0 例程，WouoUI 是从此程序 v2.x 版本衍生出的 UI 架构。
    * UI 流畅度优化，实现了优雅的平滑动画，动画算法被优化到只有两行。
    * UI 架构优化，基本重构了 v2.x，实现了独立弹窗，白天和黑暗模式，单选和多选框，开关等功能。
    * 删除主界面列表菜单风格，增加主界面图标动画可选项，回到主界面时，图标可以从正上方当前位置向两边发牌，或者从左上方从头开始发牌。
    * 删除关于本机页配置查看功能，增加参数修改页面，参数直接显示在每行末尾功能。
    * 选择框跳转动画改为宽度和竖直方向位置都从当前位置到上一级选中位置平滑过渡。
    * 增加动画速度可调区间。10 ~ 100。
    * 修复循环模式选择框在跳转动画未结束前，选择项后退时，列表与屏幕错位的问题。
    * 修复选择框在选择滚动过快时，滚出屏幕外的问题（虽然我觉得这是个有趣的特性，只要往回滚动就好不影响正常使用）。
    * 增加独立弹窗动画，背景虚化开关功能（开启虚化会有轻微卡顿感，但或许有些人喜欢，因此保留可选项）。
    * 重新设计电压测试页，增加波形显示功能，现在可以同时观察数值和波形。
  
  测试用开发板为STM32 F103C8T6，以下是使用说明：

    硬件：

    * EC11：A -> PB13 ; B -> PB12 ; S -> PB14 ; 电源3.3v
    * OLED：四线，0.96 SSD1306 128*64，SDA -> PB7 ; SCL -> PB6 ; 电源3.3v
    * 板子上的跳线帽设置：BOOT0 -> 1 ; BOOT1 -> 0 
    * 串口模块与板子连线：GND -> GND ; 3V3 -> 3.3V ; RX -> PA9 ; TX -> PA10，其余引脚空着。
    * 串口与电脑连接，用数据线连接板子上的母口和电脑。
    * 点击IDE左上角向右箭头的按钮，等待编译和上传完成。
    * 上传完成后如要测试手感，无需断开串口模块，但需要拔插数据线，电脑才能识别到是USB设备。
    * 当BOOT在第一条所述设置时，上传完成后，第一次拔插数据线时电脑会认到USB设备，从第二次开始就认不到了。
    * 手感调试合适后修改跳线帽的设置为：BOOT0 -> 0 ; BOOT1 -> 0。设置后就能收起串口模块了。
  
    软件：
    
    * 文件 -> 首选项 -> 其他开发板管理器地址 -> 添加一行：http://dan.drown.org/stm32duino/package_STM32duino_index.json
    * 工具 -> 开发板 -> 开发板管理器 -> 搜索stm32F1，下载STM32F1xx开头的库
    * 工具 -> 开发板 -> stm32F1... -> Generic STM32F103C series
    * 工具 -> 端口：“COMX” -> COMX（X为设备对应端口号）
    * 工具 -> Upload method -> Serial
    * 进入目录C:\Users{username}\AppData\Local\Arduino15\packages\stm32duino\hardware\STM32F1{版本名称}\libraries\USBComposite下，打开usb_hid.c文件，搜索bInterval，将该值修改为0x01，即轮询间隔修改为1ms
    
    注意：

    * 上传程序前，先确认使用的轴的总键程是否为4mm，如果不是，需要修改 key 数组变量中代表 travel 一列的相关参数，单位0.1mm，即4mm的值为40。
    * 拔插USB后，正常情况下，屏幕会变黑，板子上的蓝灯会点亮一秒钟，点亮时不要动按键，这是在初始化。
    * 初始化后，每个按键都需要按下一次后才能正常使用，按下时要按到底。

  遇到困难的话参考以下资料：

    设置参考：
    
    * https://github.com/rogerclarkmelbourne/Arduino_STM32
    * https://baijiahao.baidu.com/s?id=1727012582049990775&wfr=spider&for=pc
    * https://blog.csdn.net/oChiTu1/article/details/108166233?depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-2-108166233-blog-119909968.pc_relevant_3mothn_strategy_and_data_recovery
    
    项目参考：
    
    * Rapid trigger功能：https://github.com/chent7/hall-2k-keypad-handmade
    * STM32触盘：https://mp.weixin.qq.com/s/o-8_3SQS2AGT_WTyTLYQnA
    * UI：https://www.bilibili.com/video/BV1HA411S7pv/ ; https://www.bilibili.com/video/BV1xd4y1C7BE/
    
  本项目使用Apache 2.0开源协议，如需商用或借鉴，请阅读此协议。
  
  欢迎关注我的B站账号，一个只分享osu!相关内容，很无聊的帐号。
  
  用户名：音游玩的人，B站主页：https://space.bilibili.com/9182439?spm_id_from=..0.0
*/

/************************************* 屏幕驱动 *************************************/

//分辨率128*64，可以使用硬件IIC接口

#include <U8g2lib.h>
#include <Wire.h>

#define   SCL   PB6
#define   SDA   PB7
#define   RST   U8X8_PIN_NONE

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, SCL, SDA, RST);     // 分辨率：128*64  驱动：SSD1306  接口：IIC（硬件）

/************************************* 定义页面 *************************************/

//总目录，缩进表示页面层级
enum 
{
  M_WINDOW,
  M_SLEEP,
    M_MAIN, 
      M_EDITOR,
        M_KNOB,
          M_KRF,
        M_KEYS,
          M_KCODE,
          M_KMODE,
      M_VOLT,
      M_SETTING,
        M_ABOUT,
};

//状态，初始化标签
enum
{
  S_FADE,       //转场动画
  S_WINDOW,     //弹窗初始化
  S_LAYER_IN,   //层级初始化
  S_LAYER_OUT,  //层级初始化
  S_NONE,       //直接选择页面
};

//菜单结构体
typedef struct MENU
{
  char *m_select;
} M_SELECT;

/************************************* 定义内容 *************************************/

/************************************* 文字内容 *************************************/

M_SELECT main_menu[]
{
  {"Sleep"},
  {"Editor"},
  {"Volt"},
  {"Setting"},
};

M_SELECT editor_menu[]
{
  {"[ Editor ]"},
  {"- Knob"},
  {"- Key 0"},
  {"- Key 1"},
  {"- Key 2"},
  {"- Key 3"},
  {"- Key 4"},
  {"- Key 5"},
  {"- Key 6"},
  {"- Key 7"},
  {"- Key 8"},
  {"- Key 9"},
};

M_SELECT knob_menu[]
{
  {"[ Knob ]"},
  {"# Rotate Func"},
  {"$ Press Func"},
};

M_SELECT krf_menu[]
{
  {"[ Rotate Function ]"},
  {"--------------------------"},
  {"= Disable"},
  {"--------------------------"},
  {"= Volume"},
  {"= Brightness"},
  {"--------------------------"},
};

M_SELECT keys_menu[]
{
  {"[ Keys ]"},
  {"~ Trigger"},
  {"~ Release"},
  {"~ Top"},
  {"~ Bottom"},
  {"~ Times"},
  {"$ Codes"},
  {"& Modes"},
};

M_SELECT kcode_menu[]
{
  {"[ Key Code ]"},
  {"--------------------------"},
  {"= Disable"},
  {"--------------------------"},
  {"= A"},
  {"= B"},
  {"= C"},
  {"= D"},
  {"= E"},
  {"= F"},
  {"= G"},
  {"= H"},
  {"= I"},
  {"= J"},
  {"= K"},
  {"= L"},
  {"= M"},
  {"= N"},
  {"= O"},
  {"= P"},
  {"= Q"},
  {"= R"},
  {"= S"},
  {"= T"},
  {"= U"},
  {"= V"},
  {"= W"},
  {"= X"},
  {"= Y"},
  {"= Z"},
  {"--------------------------"},
  {"= 0"},
  {"= 1"},
  {"= 2"},
  {"= 3"},
  {"= 4"},
  {"= 5"},
  {"= 6"},
  {"= 7"},
  {"= 8"},
  {"= 9"},
  {"--------------------------"},
  {"= Esc"},
  {"= F1"},
  {"= F2"},
  {"= F3"},
  {"= F4"},
  {"= F5"},
  {"= F6"},
  {"= F7"},
  {"= F8"},
  {"= F9"},
  {"= F10"},
  {"= F11"},
  {"= F12"},
  {"--------------------------"},
  {"= Left Ctrl"},
  {"= Left Shift"},
  {"= Left Alt"},
  {"= Left Win"},
  {"= Right Ctrl"},
  {"= Right Shift"},
  {"= Right Alt"},
  {"= Right Win"},
  {"--------------------------"},
  {"= Caps Lock"},
  {"= Backspace"},
  {"= Return"},
  {"= Insert"},
  {"= Delete"},
  {"= Tab"},
  {"--------------------------"},
  {"= Home"},
  {"= End"},
  {"= Page Up"},
  {"= Page Down"},
  {"--------------------------"},
  {"= Up Arrow"},
  {"= Down Arrow"},
  {"= Left Arrow"},
  {"= Right Arrow"},
  {"--------------------------"},
};

M_SELECT kmode_menu[]
{
  {"[ Mode ]"},
  {"--------------------------"},
  {"= Mode 1"},
  {"= Mode 2"},
  {"--------------------------"},
};

M_SELECT volt_menu[]
{
  {"A0"},
  {"A1"},
  {"A2"},
  {"A3"},
  {"A4"},
  {"A5"},
  {"A6"},
  {"A7"},
  {"B0"},
  {"B1"},
};

M_SELECT setting_menu[]
{
  {"[ Setting ]"},
  {"~ Disp Bri"},
  {"~ Tile Ani"},
  {"~ List Ani"},
  {"~ Win Ani"},
  {"~ Spot Ani"},
  {"~ Tag Ani"},
  {"~ Fade Ani"},
  {"~ Btn SPT"},
  {"~ Btn LPT"},
  {"+ T Ufd Fm Scr"},
  {"+ L Ufd Fm Scr"},
  {"+ T Loop Mode"},
  {"+ L Loop Mode"},
  {"+ Win Bokeh Bg"},
  {"+ Knob Rot Dir"},
  {"+ Dark Mode"},
  {"- [ About ]"},
};

M_SELECT about_menu[]
{
  {"[ WouoUI ]"},
  {"- Version: v2.1"},
  {"- Board: STM32F103"},
  {"- Ram: 20k"},
  {"- Flash: 64k"},
  {"- Freq: 72Mhz"},
  {"- Creator: RQNG"},
  {"- Billi UID: 9182439"},  
};

/************************************* 图片内容 *************************************/

PROGMEM const uint8_t main_icon_pic[][120]
{
  {
    0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xF1,0x3F,
    0xFF,0xFF,0xC3,0x3F,0xFF,0xFF,0x87,0x3F,0xFF,0xFF,0x07,0x3F,0xFF,0xFF,0x0F,0x3E,
    0xFF,0xFF,0x0F,0x3E,0xFF,0xFF,0x0F,0x3C,0xFF,0xFF,0x0F,0x3C,0xFF,0xFF,0x0F,0x38,
    0xFF,0xFF,0x0F,0x38,0xFF,0xFF,0x0F,0x38,0xFF,0xFF,0x07,0x38,0xFF,0xFF,0x07,0x38,
    0xFF,0xFF,0x03,0x38,0xF7,0xFF,0x01,0x38,0xE7,0xFF,0x00,0x3C,0x87,0x3F,0x00,0x3C,
    0x0F,0x00,0x00,0x3E,0x0F,0x00,0x00,0x3E,0x1F,0x00,0x00,0x3F,0x3F,0x00,0x80,0x3F,
    0x7F,0x00,0xC0,0x3F,0xFF,0x01,0xF0,0x3F,0xFF,0x07,0xFC,0x3F,0xFF,0xFF,0xFF,0x3F,
    0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F
  },
  {
    0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,0xFF,0xF9,0xE7,0x3F,
    0xFF,0xF9,0xE7,0x3F,0xFF,0xF9,0xE7,0x3F,0xFF,0xF0,0xE7,0x3F,0x7F,0xE0,0xE7,0x3F,
    0x7F,0xE0,0xC3,0x3F,0x7F,0xE0,0xC3,0x3F,0x7F,0xE0,0xC3,0x3F,0x7F,0xE0,0xE7,0x3F,
    0xFF,0xF0,0xE7,0x3F,0xFF,0xF9,0xE7,0x3F,0xFF,0xF9,0xE7,0x3F,0xFF,0xF9,0xE7,0x3F,
    0xFF,0xF9,0xE7,0x3F,0xFF,0xF9,0xC3,0x3F,0xFF,0xF9,0x81,0x3F,0xFF,0xF0,0x81,0x3F,
    0xFF,0xF0,0x81,0x3F,0xFF,0xF0,0x81,0x3F,0xFF,0xF9,0x81,0x3F,0xFF,0xF9,0xC3,0x3F,
    0xFF,0xF9,0xE7,0x3F,0xFF,0xF9,0xE7,0x3F,0xFF,0xF9,0xE7,0x3F,0xFF,0xFF,0xFF,0x3F,
    0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F
  },
  {
    0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,0xEF,0xFF,0xFF,0x3F,0xC7,0xFF,0xFF,0x3F,
    0xC7,0xF3,0xFF,0x3F,0x83,0xC0,0xFF,0x3F,0xEF,0xCC,0xFF,0x3F,0x6F,0x9E,0xFF,0x3F,
    0x6F,0x9E,0xFF,0x3F,0x2F,0x3F,0xFF,0x3F,0x2F,0x3F,0xFF,0x3F,0x8F,0x7F,0xFE,0x3F,
    0x8F,0x7F,0xFE,0x39,0x8F,0x7F,0xFE,0x39,0xCF,0xFF,0xFC,0x3C,0xCF,0xFF,0xFC,0x3C,
    0xEF,0xFF,0xFC,0x3C,0xEF,0xFF,0x79,0x3E,0xEF,0xFF,0x79,0x3E,0xEF,0xFF,0x33,0x3F,
    0xEF,0xFF,0x33,0x3F,0xEF,0xFF,0x87,0x3F,0xEF,0xFF,0xCF,0x3F,0xEF,0xFF,0x7F,0x3E,
    0xEF,0xFF,0x7F,0x38,0x0F,0x00,0x00,0x30,0xFF,0xFF,0x7F,0x38,0xFF,0xFF,0x7F,0x3E,
    0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,
  },
  {
    0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,
    0xFF,0x1F,0xFE,0x3F,0xFF,0x1F,0xFE,0x3F,0xFF,0x0C,0xCC,0x3F,0x7F,0x00,0x80,0x3F,
    0x3F,0x00,0x00,0x3F,0x3F,0xE0,0x01,0x3F,0x7F,0xF8,0x87,0x3F,0x7F,0xFC,0x8F,0x3F,
    0x3F,0xFC,0x0F,0x3F,0x0F,0x3E,0x1F,0x3C,0x0F,0x1E,0x1E,0x3C,0x0F,0x1E,0x1E,0x3C,
    0x0F,0x3E,0x1F,0x3C,0x3F,0xFC,0x0F,0x3F,0x7F,0xFC,0x8F,0x3F,0x7F,0xF8,0x87,0x3F,
    0x3F,0xE0,0x01,0x3F,0x3F,0x00,0x00,0x3F,0x7F,0x00,0x80,0x3F,0xFF,0x0C,0xCC,0x3F,
    0xFF,0x1F,0xFE,0x3F,0xFF,0x1F,0xFE,0x3F,0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F,
    0xFF,0xFF,0xFF,0x3F,0xFF,0xFF,0xFF,0x3F
  },
};

/************************************* 页面变量 *************************************/

//OLED变量
#define   DISP_H              64    //屏幕高度
#define   DISP_W              128   //屏幕宽度
uint8_t   *buf_ptr;                 //指向屏幕缓冲的指针
uint16_t  buf_len;                  //缓冲长度

//UI变量
#define   UI_DEPTH            20    //最深层级数
#define   UI_MNUMB            100   //菜单数量
#define   UI_PARAM            16    //参数数量
enum 
{
  DISP_BRI,     //屏幕亮度
  TILE_ANI,     //磁贴动画速度
  LIST_ANI,     //列表动画速度
  WIN_ANI,      //弹窗动画速度
  SPOT_ANI,     //聚光动画速度
  TAG_ANI,      //标签动画速度
  FADE_ANI,     //消失动画速度
  BTN_SPT,      //按键短按时长
  BTN_LPT,      //按键长按时长
  TILE_UFD,     //磁贴图标从头展开开关
  LIST_UFD,     //菜单列表从头展开开关
  TILE_LOOP,    //磁贴图标循环模式开关
  LIST_LOOP,    //菜单列表循环模式开关
  WIN_BOK,      //弹窗背景虚化开关
  KNOB_DIR,     //旋钮方向切换开关
  DARK_MODE,    //黑暗模式开关
};
struct 
{
  bool      init;
  uint8_t   num[UI_MNUMB];
  uint8_t   select[UI_DEPTH];
  uint8_t   layer;
  uint8_t   index = M_SLEEP;
  uint8_t   state = S_NONE;
  bool      sleep = true;
  uint8_t   fade = 1;
  uint8_t   param[UI_PARAM];
} ui;

//磁贴变量
//所有磁贴页面都使用同一套参数
#define   TILE_B_FONT         u8g2_font_helvB18_tr        //磁贴大标题字体
#define   TILE_S_FONT         u8g2_font_HelvetiPixel_tr   //磁贴小标题字体
#define   TILE_B_TITLE_H      18                          //磁贴大标题字体高度
#define   TILE_ICON_H         30                          //磁贴图标高度
#define   TILE_ICON_W         30                          //磁贴图标宽度
#define   TILE_ICON_S         36                          //磁贴图标间距
#define   TILE_INDI_H         27                          //磁贴大标题指示器高度
#define   TILE_INDI_W         7                           //磁贴大标题指示器宽度
#define   TILE_INDI_S         36                          //磁贴大标题指示器上边距
struct 
{
  float   title_y_calc = TILE_INDI_S + (TILE_INDI_H - TILE_B_TITLE_H) / 2 + TILE_B_TITLE_H * 2;
  float   title_y_trg_calc = TILE_INDI_S + (TILE_INDI_H - TILE_B_TITLE_H) / 2 + TILE_B_TITLE_H;
  int16_t temp;
  bool    select_flag;
  float   icon_x;
  float   icon_x_trg;
  float   icon_y;
  float   icon_y_trg;
  float   indi_x; 
  float   indi_x_trg;
  float   title_y;
  float   title_y_trg;
} tile;

//列表变量

//默认参数
#define   LIST_FONT           u8g2_font_HelvetiPixel_tr   //列表字体
#define   LIST_TEXT_H         8                           //列表每行文字字体的高度
#define   LIST_LINE_H         16                          //列表单行高度
#define   LIST_TEXT_S         4                           //列表每行文字的上边距，左边距和右边距，下边距由它和字体高度和行高度决定
#define   LIST_BAR_W          5                           //列表进度条宽度，需要是奇数，因为正中间有1像素宽度的线
#define   LIST_BOX_R          0.5                         //列表选择框圆角

/*
//超窄行高度测试
#define   LIST_FONT           u8g2_font_4x6_tr            //列表字体
#define   LIST_TEXT_H         5                           //列表每行文字字体的高度
#define   LIST_LINE_H         7                           //列表单行高度
#define   LIST_TEXT_S         1                           //列表每行文字的上边距，左边距和右边距，下边距由它和字体高度和行高度决定
#define   LIST_BAR_W          7                           //列表进度条宽度，需要是奇数，因为正中间有1像素宽度的线
#define   LIST_BOX_R          0.5                         //列表选择框圆角
*/
struct
{
  uint8_t line_n = DISP_H / LIST_LINE_H;
  int16_t temp;
  bool    loop;
  float   y;
  float   y_trg;
  float   box_x;
  float   box_x_trg;
  float   box_y;
  float   box_y_trg[UI_DEPTH];
  float   bar_y;
  float   bar_y_trg;
} list;

//电压测量页面变量
//开发板模拟引脚
uint8_t   analog_pin[10] = { PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PB0, PB1 };
//曲线相关
#define   WAVE_SAMPLE         20                          //采集倍数
#define   WAVE_W              94                          //波形宽度
#define   WAVE_L              24                          //波形左边距
#define   WAVE_U              0                           //波形上边距
#define   WAVE_MAX            27                          //最大值
#define   WAVE_MIN            4                           //最小值
#define   WAVE_BOX_H          32                          //波形边框高度
#define   WAVE_BOX_W          94                          //波形边框宽度
#define   WAVE_BOX_L_S        24                          //波形边框左边距
//列表和文字背景框相关
#define   VOLT_FONT           u8g2_font_helvB18_tr        //电压数字字体
#define   VOLT_TEXT_BG_L_S    24                          //文字背景框左边距
#define   VOLT_TEXT_BG_W      94                          //文字背景框宽度
#define   VOLT_TEXT_BG_H      29                          //文字背景框高度
struct
{
  int     ch0_adc[WAVE_SAMPLE * WAVE_W];
  int     ch0_wave[WAVE_W];
  int     val;
  float   text_bg_r; 
  float   text_bg_r_trg; 
} volt;

//选择框变量

//默认参数
#define   CHECK_BOX_L_S       95                          //选择框在每行的左边距
#define   CHECK_BOX_U_S       2                           //选择框在每行的上边距
#define   CHECK_BOX_F_W       12                          //选择框外框宽度
#define   CHECK_BOX_F_H       12                          //选择框外框高度
#define   CHECK_BOX_D_S       2                           //选择框里面的点距离外框的边距

/*
//超窄行高度测试
#define   CHECK_BOX_L_S       99                          //选择框在每行的左边距
#define   CHECK_BOX_U_S       0                           //选择框在每行的上边距
#define   CHECK_BOX_F_W       5                           //选择框外框宽度
#define   CHECK_BOX_F_H       5                           //选择框外框高度
#define   CHECK_BOX_D_S       1                           //选择框里面的点距离外框的边距
*/
struct
{
  uint8_t *v;
  uint8_t *m;
  uint8_t *s;
  uint8_t *s_p;
} check_box;

//弹窗变量
#define   WIN_FONT            u8g2_font_HelvetiPixel_tr   //弹窗字体
#define   WIN_H               32                          //弹窗高度
#define   WIN_W               102                         //弹窗宽度
#define   WIN_BAR_W           92                          //弹窗进度条宽度
#define   WIN_BAR_H           7                           //弹窗进度条高度
#define   WIN_Y               - WIN_H - 2                 //弹窗竖直方向出场起始位置
#define   WIN_Y_TRG           - WIN_H - 2                 //弹窗竖直方向退场终止位置
struct
{
  //uint8_t
  uint8_t   *value;
  uint8_t   max;
  uint8_t   min;
  uint8_t   step;

  MENU      *bg;
  uint8_t   index;
  char      title[20];
  uint8_t   select;
  uint8_t   l = (DISP_W - WIN_W) / 2;
  uint8_t   u = (DISP_H - WIN_H) / 2;
  float     bar;
  float     bar_trg;
  float     y;
  float     y_trg;
} win;

//聚光灯变量
struct
{
  float   l; 
  float   l_trg; 
  float   r; 
  float   r_trg; 
  float   u; 
  float   u_trg; 
  float   d; 
  float   d_trg; 
} spot;

/************************************* USB 相关 *************************************/

#include <USBComposite.h>

USBHID HID;

const uint8_t reportDescription[] = 
{
   HID_CONSUMER_REPORT_DESCRIPTOR(),
   HID_KEYBOARD_REPORT_DESCRIPTOR()
};

HIDConsumer Consumer(HID);
HIDKeyboard Keyboard(HID);

void hid_init()
{
  HID.begin(reportDescription, sizeof(reportDescription));
  while (!USBComposite);
}

/********************************** 自定义功能变量 **********************************/

/********************************** 旋钮 HID 控制 **********************************/

//旋钮功能变量
#define   KNOB_PARAM          4
#define   KNOB_ROT_DIS        0
#define   KNOB_ROT_VOL        1
#define   KNOB_ROT_BRI        2
enum 
{
  KNOB_ROT,       //睡眠下旋转旋钮的功能，0 禁用，1 音量，2 亮度
  KNOB_COD,       //睡眠下短按旋钮输入的字符码，0 禁用
  KNOB_ROT_P,     //旋转旋钮功能在单选框中选择的位置
  KNOB_COD_P,     //字符码在单选框中选择的位置
};
struct 
{
  uint8_t param[KNOB_PARAM] = { 0, 0, 2, 2 }; //禁用在列表的第2个选项，第0个是标题，第1个是分界线
} knob;

/********************************** Rapid Trigger **********************************/

// 按键参数
#define         KEYS        10
#define         LIGHT       PC13
#define         KEY_DIS     0
#define         KEY_MODE1   1
#define         KEY_MODE2   2
enum 
{
  K0,
  K1,
  K2,
  K3,
  K4,
  K5,
  K6,
  K7,
  K8,
  K9,
};
enum 
{
  TRIGGER,
  RELEASE,
  TOP, 
  BOTTOM,
  TIMES,
  CODES,
  MODES,
  CODES_P,
  MODES_P,
  PINS,
  TRAVEL,
};

// 按键变量
uint8_t key [KEYS][11] =
{ 
//  trigger  release   top   bottom   times   codes     modes    codes_p  modes_p   pins  travel
  {    3,       3,      3,     3,      10,   KEY_DIS, KEY_MODE1,    2,      2,      PA0,    40 },  // K0
  {    3,       3,      3,     3,      10,   KEY_DIS, KEY_MODE1,    2,      2,      PA1,    40 },  // K1
  {    3,       3,      3,     3,      10,   KEY_DIS, KEY_MODE1,    2,      2,      PA2,    40 },  // K2
  {    3,       3,      3,     3,      10,   KEY_DIS, KEY_MODE1,    2,      2,      PA3,    40 },  // K3
  {    3,       3,      3,     3,      10,   KEY_DIS, KEY_MODE1,    2,      2,      PA4,    40 },  // K4
  {    3,       3,      3,     3,      10,   KEY_DIS, KEY_MODE1,    2,      2,      PA5,    40 },  // K5
  {    3,       3,      3,     3,      10,   KEY_DIS, KEY_MODE1,    2,      2,      PA6,    40 },  // K6
  {    3,       3,      3,     3,      10,   KEY_DIS, KEY_MODE1,    2,      2,      PA7,    40 },  // K7
  {    3,       3,      3,     3,      10,   KEY_DIS, KEY_MODE1,    2,      2,      PB0,    40 },  // K8
  {    3,       3,      3,     3,      10,   KEY_DIS, KEY_MODE1,    2,      2,      PB1,    40 },  // K9
};

/************************************* 按键扫描 *************************************/

// 按键参数结构体
struct 
{
  int   min;
  int   max;
  int   trigger;
  int   release;
  int   value;
  int   reference;
  int   unit;
  int   cushion;
  bool  init;  
  bool  pressed;
} keyx[KEYS];

/************************************* 不分模式 *************************************/

// 初始化
void key_init() 
{
  // 打开初始化指示灯
  pinMode(LIGHT, OUTPUT);
  digitalWrite(LIGHT, LOW);

  // 读模拟量，最大值和最小值先暂定为当前值
  for (uint8_t n = 0; n < KEYS; ++n) 
  {
    pinMode(key[n][PINS],INPUT_ANALOG);
    keyx[n].value = analogRead(key[n][PINS]);
    keyx[n].min = keyx[n].value;
    keyx[n].max = keyx[n].value;
  }

  // 并行扫描所有传感器，记录每个传感器的初始抖动范围，每1ms扫描一次，扫描1s
  for (int i = 0; i < 1000; ++i) 
  {
    for (uint8_t n = 0; n < KEYS; ++n) 
    {
      keyx[n].value = analogRead(key[n][PINS]);
      if (keyx[n].value < keyx[n].min) 
      {
        keyx[n].min = keyx[n].value;
      } 
      else if (keyx[n].value > keyx[n].max) 
      {
        keyx[n].max = keyx[n].value;
      }    
    }
    delay(1);
  }

  for (uint8_t n = 0; n < KEYS; ++n) 
  {
    keyx[n].cushion = (keyx[n].max - keyx[n].min) * key[n][TIMES];      // 计算缓冲值

    // 初始化一些参数
    keyx[n].init = false;                                               // 按键还没有被首次按下
    keyx[n].reference = keyx[n].value;                                  // 初始化参考值为当前值
    keyx[n].trigger = key[n][TRIGGER] * keyx[n].cushion;                // 初始化触发值，避免插上的瞬间有输入
    keyx[n].release = key[n][RELEASE] * keyx[n].cushion;                // 初始化释放值，避免插上的瞬间有输入

    if (key[n][MODES] == 1)
    {
      keyx[n].min = keyx[n].value;                                      // 初始化最小值，暂定为当前值
      keyx[n].max = keyx[n].value + keyx[n].cushion;                    // 初始化最大值，暂定为当前值 + 缓冲值
    }
    else if (key[n][MODES] == 2)
    {
      keyx[n].min = keyx[n].value - keyx[n].cushion;                    // 初始化最小值，暂定为当前值
      keyx[n].max = keyx[n].value;                                      // 初始化最大值，暂定为当前值 + 缓冲值
    }
  }

  // 关闭初始化指示灯
  digitalWrite(LIGHT, HIGH);
}

// 转换距离，将以毫米为单位的值转换成模拟量，假设磁铁外磁场强度均匀变化，则距离与模拟量线性相关
void convertDistance(uint8_t n) 
{
  keyx[n].unit = (keyx[n].max - keyx[n].min) / key[n][TRAVEL];
  keyx[n].trigger = keyx[n].unit * key[n][TRIGGER];
  keyx[n].release = keyx[n].unit * key[n][RELEASE];
}

/************************************* 区分模式 *************************************/

/*************************************  模式 1  *************************************/

// 按下按键，发送后改变按键状态，按键首次被按下
void pressKey_1(uint8_t n) 
{
  if (keyx[n].init)
  {
    Keyboard.press(key[n][CODES]);
  } 
  else if (keyx[n].value > keyx[n].min + keyx[n].cushion)
  {
    keyx[n].init = true;
  }
  keyx[n].pressed = true;
}

// 释放按键，发送后改变按键状态，按键首次被按下
void releaseKey_1(uint8_t n) 
{
  if (keyx[n].init)
  {
    Keyboard.release(key[n][CODES]);
  } 
  else if (keyx[n].value > keyx[n].min + keyx[n].cushion)
  {
    keyx[n].init = true;
  }
  keyx[n].pressed = false;
}

// 平衡按键，按下时探索更大的值，释放时探索更小的值，以免在使用时极值漂移使抖动能误触发
void balanceKey_1(uint8_t n) 
{
  if (!keyx[n].pressed && (keyx[n].value < keyx[n].min)) 
  {
    keyx[n].min = keyx[n].value;
    convertDistance(n);
  } 
  else if (keyx[n].pressed && (keyx[n].value > keyx[n].max)) 
  {
    keyx[n].max = keyx[n].value;
    convertDistance(n);
  }
}

// 按键移动时判断状态，同时动态改变参考值
void processKey_1(uint8_t n) 
{
  // 不在死区移动时的判断
  if ((keyx[n].value > keyx[n].min + keyx[n].unit * key[n][TOP]) && (keyx[n].value < keyx[n].max - keyx[n].unit * key[n][BOTTOM]))
  {
    if (!keyx[n].pressed) 
    {
      if (keyx[n].value > keyx[n].reference + keyx[n].trigger) 
      {
        pressKey_1(n);
      }
      else if (keyx[n].value < keyx[n].reference) 
      {
        keyx[n].reference = keyx[n].value;
      }
    } 
    else 
    {
      if (keyx[n].value <= keyx[n].reference - keyx[n].release) 
      {
        releaseKey_1(n);
      }
      else if (keyx[n].value > keyx[n].reference) 
      {
        keyx[n].reference = keyx[n].value;
      }
    }
  }

  // 在死区移动时的判断
  else if ((keyx[n].value < keyx[n].min + keyx[n].unit * key[n][TOP]) && keyx[n].pressed)
  {
    releaseKey_1(n);
  }
  else if ((keyx[n].value > keyx[n].max - keyx[n].unit * key[n][BOTTOM]) && !keyx[n].pressed)
  {
    pressKey_1(n);
  }
}

/*************************************  模式 2  *************************************/

// 按下按键，发送后改变按键状态，按键首次被按下
void pressKey_2(uint8_t n) 
{
  if (keyx[n].init)
  {
    Keyboard.press(key[n][CODES]);
  } 
  else if (keyx[n].value < keyx[n].max - keyx[n].cushion)
  {
    keyx[n].init = true;
  }
  keyx[n].pressed = true;
}

// 释放按键，发送后改变按键状态，按键首次被按下
void releaseKey_2(uint8_t n) 
{
  if (keyx[n].init)
  {
    Keyboard.release(key[n][CODES]);
  } 
  else if (keyx[n].value < keyx[n].min - keyx[n].cushion)
  {
    keyx[n].init = true;
  }
  keyx[n].pressed = false;
}

// 平衡按键，按下时探索更小的值，释放时探索更大的值，以免在使用时极值漂移使抖动能误触发
void balanceKey_2(uint8_t n) 
{
  if (keyx[n].pressed && (keyx[n].value < keyx[n].min)) 
  {
    keyx[n].min = keyx[n].value;
    convertDistance(n);
  } 
  else if (!keyx[n].pressed && (keyx[n].value > keyx[n].max)) 
  {
    keyx[n].max = keyx[n].value;
    convertDistance(n);
  }
}

// 按键移动时判断状态，同时动态改变参考值
void processKey_2(uint8_t n) 
{
  // 不在死区移动时的判断
  if ((keyx[n].value > keyx[n].min + keyx[n].unit * key[n][BOTTOM]) && (keyx[n].value < keyx[n].max - keyx[n].unit * key[n][TOP]))
  {
    if (!keyx[n].pressed) 
    {
      if (keyx[n].value < keyx[n].reference - keyx[n].trigger) 
      {
        pressKey_2(n);
      }
      else if (keyx[n].value > keyx[n].reference) 
      {
        keyx[n].reference = keyx[n].value;
      }
    } 
    else 
    {
      if (keyx[n].value >= keyx[n].reference + keyx[n].release) 
      {
        releaseKey_2(n);
      }
      else if (keyx[n].value < keyx[n].reference) 
      {
        keyx[n].reference = keyx[n].value;
      }
    }
  }

  // 在死区移动时的判断
  else if ((keyx[n].value < keyx[n].min + keyx[n].unit * key[n][BOTTOM]) && !keyx[n].pressed)
  {
    pressKey_2(n);
  }
  else if ((keyx[n].value > keyx[n].max - keyx[n].unit * key[n][TOP]) && keyx[n].pressed)
  {
    releaseKey_2(n);
  }
}

/************************************** 总扫描 **************************************/

// 扫描一次按键
void runKey(uint8_t n) 
{
  if (key[n][CODES])
  {
    switch (key[n][MODES])
    {
      case 1: keyx[n].value = analogRead(key[n][PINS]); balanceKey_1(n); processKey_1(n); break;
      case 2: keyx[n].value = analogRead(key[n][PINS]); balanceKey_2(n); processKey_2(n); break;
    }
  }
}

/************************************* 断电保存 *************************************/

#include <EEPROM.h>

//EEPROM变量
#define   EEPROM_CHECK        11
struct
{
  bool    init;
  bool    change;
  int     address;
  uint8_t check;
  uint8_t check_param[EEPROM_CHECK] = { 'a', 'b', 'c', 'd', 'e', 'f','g', 'h', 'i', 'j', 'k' }; 
} eeprom;

//EEPROM写数据，回到睡眠时执行一遍
void eeprom_write_all_data()
{
  eeprom.address = 0;
  for (uint8_t i = 0; i < EEPROM_CHECK; ++i)    EEPROM.write(eeprom.address + i, eeprom.check_param[i]);  eeprom.address += EEPROM_CHECK;
  for (uint8_t i = 0; i < UI_PARAM; ++i)        EEPROM.write(eeprom.address + i, ui.param[i]);            eeprom.address += UI_PARAM;
  for (uint8_t i = 0; i < KNOB_PARAM; ++i)      EEPROM.write(eeprom.address + i, knob.param[i]);          eeprom.address += KNOB_PARAM;

  for (uint8_t i = 0; i < KEYS; ++i)            EEPROM.write(eeprom.address + i, key[i][TRIGGER]);        eeprom.address += KEYS;
  for (uint8_t i = 0; i < KEYS; ++i)            EEPROM.write(eeprom.address + i, key[i][RELEASE]);        eeprom.address += KEYS;
  for (uint8_t i = 0; i < KEYS; ++i)            EEPROM.write(eeprom.address + i, key[i][TOP]);            eeprom.address += KEYS;
  for (uint8_t i = 0; i < KEYS; ++i)            EEPROM.write(eeprom.address + i, key[i][BOTTOM]);         eeprom.address += KEYS;
  for (uint8_t i = 0; i < KEYS; ++i)            EEPROM.write(eeprom.address + i, key[i][TIMES]);          eeprom.address += KEYS;
  for (uint8_t i = 0; i < KEYS; ++i)            EEPROM.write(eeprom.address + i, key[i][CODES]);          eeprom.address += KEYS;
  for (uint8_t i = 0; i < KEYS; ++i)            EEPROM.write(eeprom.address + i, key[i][MODES]);          eeprom.address += KEYS;
  for (uint8_t i = 0; i < KEYS; ++i)            EEPROM.write(eeprom.address + i, key[i][CODES_P]);        eeprom.address += KEYS;
  for (uint8_t i = 0; i < KEYS; ++i)            EEPROM.write(eeprom.address + i, key[i][MODES_P]);        eeprom.address += KEYS;
  for (uint8_t i = 0; i < KEYS; ++i)            EEPROM.write(eeprom.address + i, key[i][PINS]);           eeprom.address += KEYS;
  for (uint8_t i = 0; i < KEYS; ++i)            EEPROM.write(eeprom.address + i, key[i][TRAVEL]);         eeprom.address += KEYS;
}

//EEPROM读数据，开机初始化时执行一遍
void eeprom_read_all_data()
{
  eeprom.address = EEPROM_CHECK;   
  for (uint8_t i = 0; i < UI_PARAM; ++i)        ui.param[i]   = EEPROM.read(eeprom.address + i);          eeprom.address += UI_PARAM;
  for (uint8_t i = 0; i < KNOB_PARAM; ++i)      knob.param[i] = EEPROM.read(eeprom.address + i);          eeprom.address += KNOB_PARAM;

  for (uint8_t i = 0; i < KEYS; ++i)            key[i][TRIGGER] = EEPROM.read(eeprom.address + i);        eeprom.address += KEYS;
  for (uint8_t i = 0; i < KEYS; ++i)            key[i][RELEASE] = EEPROM.read(eeprom.address + i);        eeprom.address += KEYS;
  for (uint8_t i = 0; i < KEYS; ++i)            key[i][TOP]     = EEPROM.read(eeprom.address + i);        eeprom.address += KEYS;
  for (uint8_t i = 0; i < KEYS; ++i)            key[i][BOTTOM]  = EEPROM.read(eeprom.address + i);        eeprom.address += KEYS;
  for (uint8_t i = 0; i < KEYS; ++i)            key[i][TIMES]   = EEPROM.read(eeprom.address + i);        eeprom.address += KEYS;
  for (uint8_t i = 0; i < KEYS; ++i)            key[i][CODES]   = EEPROM.read(eeprom.address + i);        eeprom.address += KEYS;
  for (uint8_t i = 0; i < KEYS; ++i)            key[i][MODES]   = EEPROM.read(eeprom.address + i);        eeprom.address += KEYS;
  for (uint8_t i = 0; i < KEYS; ++i)            key[i][CODES_P] = EEPROM.read(eeprom.address + i);        eeprom.address += KEYS;
  for (uint8_t i = 0; i < KEYS; ++i)            key[i][MODES_P] = EEPROM.read(eeprom.address + i);        eeprom.address += KEYS;
  for (uint8_t i = 0; i < KEYS; ++i)            key[i][PINS]    = EEPROM.read(eeprom.address + i);        eeprom.address += KEYS;
  for (uint8_t i = 0; i < KEYS; ++i)            key[i][TRAVEL]  = EEPROM.read(eeprom.address + i);        eeprom.address += KEYS;
}

//开机检查是否已经修改过，没修改过则跳过读配置步骤，用默认设置
void eeprom_init()
{
  eeprom.check = 0;
  eeprom.address = 0; for (uint8_t i = 0; i < EEPROM_CHECK; ++i)  if (EEPROM.read(eeprom.address + i) != eeprom.check_param[i])  eeprom.check ++;
  if (eeprom.check <= 1) eeprom_read_all_data();  //允许一位误码
  else ui_param_init();
}

/************************************* 旋钮相关 *************************************/

//可按下旋钮引脚
#define   AIO   PB12
#define   BIO   PB13
#define   SW    PB14

//按键ID
#define   BTN_ID_CC           0   //逆时针旋转
#define   BTN_ID_CW           1   //顺时针旋转
#define   BTN_ID_SP           2   //短按
#define   BTN_ID_LP           3   //长按

//按键变量
#define   BTN_PARAM_TIMES     2   //由于uint8_t最大值可能不够，但它存储起来方便，这里放大两倍使用
struct
{
  uint8_t   id;
  bool      flag;
  bool      pressed;
  bool      CW_1;
  bool      CW_2;
  bool      val;
  bool      val_last;  
  bool      alv;  
  bool      blv;
  long      count;
} volatile btn;

void knob_inter() 
{
  btn.alv = digitalRead(AIO);
  btn.blv = digitalRead(BIO);
  if (!btn.flag && btn.alv == LOW) 
  {
    btn.CW_1 = btn.blv;
    btn.flag = true;
  }
  if (btn.flag && btn.alv) 
  {
    btn.CW_2 = !btn.blv;
    if (btn.CW_1 && btn.CW_2)
     {
      btn.id = ui.param[KNOB_DIR];
      btn.pressed = true;
    }
    if (btn.CW_1 == false && btn.CW_2 == false) 
    {
      btn.id = !ui.param[KNOB_DIR];
      btn.pressed = true;
    }
    btn.flag = false;
  }
}

void btn_scan() 
{
  btn.val = digitalRead(SW);
  if (btn.val != btn.val_last)
  {
    btn.val_last = btn.val;
    delay(ui.param[BTN_SPT] * BTN_PARAM_TIMES);
    btn.val = digitalRead(SW);
    if (btn.val == LOW)
    {
      btn.pressed = true;
      btn.count = 0;
      while (!digitalRead(SW))
      {
        btn.count++;
        delay(1);
      }
      if (btn.count < ui.param[BTN_LPT] * BTN_PARAM_TIMES)  btn.id = BTN_ID_SP;
      else  btn.id = BTN_ID_LP;
    }
  }
}

void btn_init() 
{
  pinMode(AIO, INPUT);
  pinMode(BIO, INPUT);
  pinMode(SW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(AIO), knob_inter, CHANGE);
}

/************************************ 初始化函数 ***********************************/

/********************************* 初始化数据处理函数 *******************************/

//显示数值的初始化
void check_box_v_init(uint8_t *param)
{
  check_box.v = param;
}

//多选框的初始化
void check_box_m_init(uint8_t *param)
{
  check_box.m = param;
}

//单选框时的初始化
void check_box_s_init(uint8_t *param, uint8_t *param_p)
{
  check_box.s = param;
  check_box.s_p = param_p;
}

//多选框处理函数
void check_box_m_select(uint8_t param)
{
  check_box.m[param] = !check_box.m[param];
  eeprom.change = true;
}

//单选框处理函数
void check_box_s_select(uint8_t val, uint8_t pos)
{
  *check_box.s = val;
  *check_box.s_p = pos;
  eeprom.change = true;
}

//弹窗数值初始化
void window_value_init(char title[], uint8_t select, uint8_t *value, uint8_t max, uint8_t min, uint8_t step, MENU *bg, uint8_t index)
{
  strcpy(win.title, title);
  win.select = select;
  win.value = value;
  win.max = max;
  win.min = min;
  win.step = step;
  win.bg = bg;
  win.index = index;  
  ui.index = M_WINDOW;
  ui.state = S_WINDOW;
}

/*********************************** UI 初始化函数 *********************************/

//在初始化EEPROM时，选择性初始化的默认设置
void ui_param_init()
{
  ui.param[DISP_BRI]  = 255;      //屏幕亮度
  ui.param[TILE_ANI]  = 30;       //磁贴动画速度
  ui.param[LIST_ANI]  = 60;       //列表动画速度
  ui.param[WIN_ANI]   = 25;       //弹窗动画速度
  ui.param[SPOT_ANI]  = 50;       //聚光动画速度
  ui.param[TAG_ANI]   = 60;       //标签动画速度
  ui.param[FADE_ANI]  = 30;       //消失动画速度
  ui.param[BTN_SPT]   = 25;       //按键短按时长
  ui.param[BTN_LPT]   = 150;      //按键长按时长
  ui.param[TILE_UFD]  = 1;        //磁贴图标从头展开开关
  ui.param[LIST_UFD]  = 1;        //菜单列表从头展开开关
  ui.param[TILE_LOOP] = 0;        //磁贴图标循环模式开关
  ui.param[LIST_LOOP] = 0;        //菜单列表循环模式开关
  ui.param[WIN_BOK]   = 0;        //弹窗背景虚化开关
  ui.param[KNOB_DIR]  = 0;        //旋钮方向切换开关   
  ui.param[DARK_MODE] = 1;        //黑暗模式开关   
}

//列表类页面列表行数初始化，必须初始化的参数
void ui_init()
{
  ui.num[M_MAIN]      = sizeof( main_menu     )   / sizeof(M_SELECT);
  ui.num[M_EDITOR]    = sizeof( editor_menu   )   / sizeof(M_SELECT);
  ui.num[M_KNOB]      = sizeof( knob_menu     )   / sizeof(M_SELECT);
  ui.num[M_KRF]       = sizeof( krf_menu      )   / sizeof(M_SELECT);
  ui.num[M_KEYS]      = sizeof( keys_menu     )   / sizeof(M_SELECT);
  ui.num[M_KCODE]     = sizeof( kcode_menu    )   / sizeof(M_SELECT);
  ui.num[M_KMODE]     = sizeof( kmode_menu    )   / sizeof(M_SELECT);
  ui.num[M_VOLT]      = sizeof( volt_menu     )   / sizeof(M_SELECT);
  ui.num[M_SETTING]   = sizeof( setting_menu  )   / sizeof(M_SELECT);
  ui.num[M_ABOUT]     = sizeof( about_menu    )   / sizeof(M_SELECT);   
}

/********************************* 分页面初始化函数 ********************************/

//进入磁贴类时的初始化
void tile_param_init()
{
  ui.init = false;
  tile.icon_x = 0;
  tile.icon_x_trg = TILE_ICON_S;
  tile.icon_y = -TILE_ICON_H;
  tile.icon_y_trg = 0;
  tile.indi_x = 0;
  tile.indi_x_trg = TILE_INDI_W;
  tile.title_y = tile.title_y_calc;
  tile.title_y_trg = tile.title_y_trg_calc;
}

//进入睡眠时的初始化
void sleep_param_init()
{
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 0, DISP_W, DISP_H);
  u8g2.setPowerSave(1);
  ui.state = S_NONE;  
  ui.sleep = true;
  if (eeprom.change)
  {
    eeprom_write_all_data();
    eeprom.change = false;
  }
  key_init();
}

//电压测量页面初始化
void volt_param_init()
{
  volt.text_bg_r = 0;
  volt.text_bg_r_trg = VOLT_TEXT_BG_W; 
}

//设置页初始化
void setting_param_init()
{
  check_box_v_init(ui.param);
  check_box_m_init(ui.param);
}

/********************************** 通用初始化函数 *********************************/

/*
  页面层级管理逻辑是，把所有页面都先当作列表类初始化，不是列表类按需求再初始化对应函数
  这样做会浪费一些资源，但跳转页面时只需要考虑页面层级，逻辑上更清晰，减少出错
*/

//弹窗动画初始化
void window_param_init()
{
  win.bar = 0;
  win.y = WIN_Y;
  win.y_trg = win.u;
  ui.state = S_NONE;
}

//进入更深层级时的初始化
void layer_init_in()
{
  ui.layer ++;
  ui.init = 0;
  list.y = 0;
  list.y_trg = LIST_LINE_H;
  list.box_x = 0;
  list.box_y = 0;
  list.bar_y = 0;
  ui.state = S_FADE;
  switch (ui.index)
  {
    case M_MAIN:    tile_param_init();    break;  //睡眠进入主菜单，动画初始化   
    case M_VOLT:    volt_param_init();    break;  //主菜单进入电压测量页，动画初始化
    case M_SETTING: setting_param_init(); break;  //主菜单进入设置页，单选框初始化
  }
}

//进入更浅层级时的初始化
void layer_init_out()
{
  ui.select[ui.layer] = 0;
  list.box_y_trg[ui.layer] = 0;
  ui.layer --;
  ui.init = 0;
  list.y = 0;
  list.y_trg = LIST_LINE_H;
  list.bar_y = 0;
  ui.state = S_FADE;
  switch (ui.index)
  {
    case M_SLEEP: sleep_param_init(); break;    //主菜单进入睡眠页，检查是否需要写EEPROM
    case M_MAIN:  tile_param_init();  break;    //不管什么页面进入主菜单时，动画初始化
  }
}

/************************************* 动画函数 *************************************/

//动画函数
void animation(float *a, float *a_trg, uint8_t n)
{
  if (fabs(*a - *a_trg) < 0.15) *a = *a_trg;
  if (*a != *a_trg) *a += (*a_trg - *a) / (ui.param[n] / 10.0);
}

//消失函数
void fade()
{
  delay(ui.param[FADE_ANI]);
  if (ui.param[DARK_MODE])
  {
    switch (ui.fade)
    {
      case 1: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] & 0xAA; break;
      case 2: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] & 0x00; break;
      case 3: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] & 0x55; break;
      case 4: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] & 0x00; break;
      default: ui.state = S_NONE; ui.fade = 0; break;
    }
  }
  else
  {
    switch (ui.fade)
    {
      case 1: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] | 0xAA; break;
      case 2: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] | 0x00; break;
      case 3: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] | 0x55; break;
      case 4: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] | 0x00; break;
      default: ui.state = S_NONE; ui.fade = 0; break;
    }    
  }
  ui.fade++;
}

/************************************* 显示函数 *************************************/

//磁贴类页面通用显示函数
void tile_show(struct MENU arr_1[], const uint8_t icon_pic[][120])
{
  //计算动画过渡值
  animation(&tile.icon_x, &tile.icon_x_trg, TILE_ANI);
  animation(&tile.icon_y, &tile.icon_y_trg, TILE_ANI);
  animation(&tile.indi_x, &tile.indi_x_trg, TILE_ANI);
  animation(&tile.title_y, &tile.title_y_trg, TILE_ANI);

  //设置大标题的颜色，0透显，1实显，2反色，这里都用实显
  u8g2.setDrawColor(1);

  //绘制大标题
  u8g2.setFont(TILE_B_FONT); 
  u8g2.drawStr(((DISP_W - TILE_INDI_W) - u8g2.getStrWidth(arr_1[ui.select[ui.layer]].m_select)) / 2 + TILE_INDI_W, tile.title_y, arr_1[ui.select[ui.layer]].m_select);

  //绘制大标题指示器
  u8g2.drawBox(0, TILE_ICON_S, tile.indi_x, TILE_INDI_H);

  //绘制图标
  if (!ui.init)
  {
    for (uint8_t i = 0; i < ui.num[ui.index]; ++i)  
    {
      if (ui.param[TILE_UFD]) tile.temp = (DISP_W - TILE_ICON_W) / 2 + i * tile.icon_x - TILE_ICON_S * ui.select[ui.layer];
      else tile.temp = (DISP_W - TILE_ICON_W) / 2 + (i - ui.select[ui.layer]) * tile.icon_x;
      u8g2.drawXBMP(tile.temp, (int16_t)tile.icon_y, TILE_ICON_W, TILE_ICON_H, icon_pic[i]); 
    }
    if (tile.icon_x == tile.icon_x_trg) 
    {
      ui.init = true;
      tile.icon_x = tile.icon_x_trg = - ui.select[ui.layer] * TILE_ICON_S;
    }
  }
  else for (uint8_t i = 0; i < ui.num[ui.index]; ++i) u8g2.drawXBMP((DISP_W - TILE_ICON_W) / 2 + (int16_t)tile.icon_x + i * TILE_ICON_S, 0, TILE_ICON_W, TILE_ICON_H, icon_pic[i]);

  //反转屏幕内元素颜色，白天模式遮罩
  u8g2.setDrawColor(2);
  if (!ui.param[DARK_MODE]) u8g2.drawBox(0, 0, DISP_W, DISP_H);
}

/*************** 根据列表每行开头符号，判断每行尾部是否绘制以及绘制什么内容 *************/

//列表显示显示数值
void list_draw_value(int n) { u8g2.print(check_box.v[n - 1]); }

//绘制外框
void list_draw_check_box_frame() { u8g2.drawRFrame(CHECK_BOX_L_S, list.temp + CHECK_BOX_U_S, CHECK_BOX_F_W, CHECK_BOX_F_H, 1); }

//绘制框里面的点
void list_draw_check_box_dot() { u8g2.drawBox(CHECK_BOX_L_S + CHECK_BOX_D_S + 1, list.temp + CHECK_BOX_U_S + CHECK_BOX_D_S + 1, CHECK_BOX_F_W - (CHECK_BOX_D_S + 1) * 2, CHECK_BOX_F_H - (CHECK_BOX_D_S + 1) * 2); }

//列表显示旋钮功能
void list_draw_krf(int n) 
{ 
  switch (check_box.v[n - 1])
  {
    case 0: u8g2.print("OFF"); break;
    case 1: u8g2.print("VOL"); break;
    case 2: u8g2.print("BRI"); break;
  }
}

//列表显示按键模式
void list_draw_kmode(int n) 
{ 
  switch (check_box.v[n - 1])
  {
    case 1: u8g2.print("M 1"); break;
    case 2: u8g2.print("M 2"); break;
  }
}

//列表显示按键键值
void list_draw_kcode(int n) 
{ 
  if (check_box.v[n - 1] == 0) u8g2.print("OFF");
  else if (check_box.v[n - 1] <= 90) u8g2.print((char)check_box.v[n - 1]);
  else u8g2.print("?");
}

//判断列表尾部内容
void list_draw_text_and_check_box(struct MENU arr[], int i)
{
  u8g2.drawStr(LIST_TEXT_S, list.temp + LIST_TEXT_H + LIST_TEXT_S, arr[i].m_select);
  u8g2.setCursor(CHECK_BOX_L_S, list.temp + LIST_TEXT_H + LIST_TEXT_S);
  switch (arr[i].m_select[0])
  {
    case '~': list_draw_value(i); break;
    case '+': list_draw_check_box_frame(); if (check_box.m[i - 1] == 1)  list_draw_check_box_dot(); break;
    case '=': list_draw_check_box_frame(); if (*check_box.s_p == i)      list_draw_check_box_dot(); break;
    case '#': list_draw_krf(i);   break;
    case '&': list_draw_kmode(i); break;
    case '$': list_draw_kcode(i); break;
  }
}

/******************************** 列表显示函数 **************************************/

//列表类页面通用显示函数
void list_show(struct MENU arr[], uint8_t ui_index)
{
  //更新动画目标值
  u8g2.setFont(LIST_FONT);
  list.box_x_trg = u8g2.getStrWidth(arr[ui.select[ui.layer]].m_select) + LIST_TEXT_S * 2;
  list.bar_y_trg = ceil((ui.select[ui.layer]) * ((float)DISP_H / (ui.num[ui_index] - 1)));
  
  //计算动画过渡值
  animation(&list.y, &list.y_trg, LIST_ANI);
  animation(&list.box_x, &list.box_x_trg, LIST_ANI);
  animation(&list.box_y, &list.box_y_trg[ui.layer], LIST_ANI);
  animation(&list.bar_y, &list.bar_y_trg, LIST_ANI);

  //检查循环动画是否结束
  if (list.loop && list.box_y == list.box_y_trg[ui.layer]) list.loop = false;

  //设置文字和进度条颜色，0透显，1实显，2反色，这里都用实显
  u8g2.setDrawColor(1);
  
  //绘制进度条
  u8g2.drawHLine(DISP_W - LIST_BAR_W, 0, LIST_BAR_W);
  u8g2.drawHLine(DISP_W - LIST_BAR_W, DISP_H - 1, LIST_BAR_W);
  u8g2.drawVLine(DISP_W - ceil((float)LIST_BAR_W / 2), 0, DISP_H);
  u8g2.drawBox(DISP_W - LIST_BAR_W, 0, LIST_BAR_W, list.bar_y);

  //绘制列表文字  
  if (!ui.init)
  {
    for (int i = 0; i < ui.num[ui_index]; ++ i)
    {
      if (ui.param[LIST_UFD]) list.temp = i * list.y - LIST_LINE_H * ui.select[ui.layer] + list.box_y_trg[ui.layer];
      else list.temp = (i - ui.select[ui.layer]) * list.y + list.box_y_trg[ui.layer];
      list_draw_text_and_check_box(arr, i);
    }
    if (list.y == list.y_trg) 
    {
      ui.init = true;
      list.y = list.y_trg = - LIST_LINE_H * ui.select[ui.layer] + list.box_y_trg[ui.layer];
    }
  }
  else for (int i = 0; i < ui.num[ui_index]; ++ i)
  {
    list.temp = LIST_LINE_H * i + list.y;
    list_draw_text_and_check_box(arr, i);
  }

  //绘制文字选择框，0透显，1实显，2反色，这里用反色
  u8g2.setDrawColor(2);
  u8g2.drawRBox(0, list.box_y, list.box_x, LIST_LINE_H, LIST_BOX_R);

  //反转屏幕内元素颜色，白天模式遮罩，在这里屏蔽有列表参与的页面，使遮罩作用在那个页面上
  if (!ui.param[DARK_MODE])
  {
    u8g2.drawBox(0, 0, DISP_W, DISP_H);
    switch(ui.index)
    {
      case M_WINDOW: 
      case M_VOLT:
      u8g2.drawBox(0, 0, DISP_W, DISP_H);  
    }
  }
}

//电压页面显示函数
void volt_show()
{
  //使用列表类显示选项
  list_show(volt_menu, M_VOLT); 

  //计算动画过渡值  
  animation(&volt.text_bg_r, &volt.text_bg_r_trg, TAG_ANI);

  //设置曲线颜色，0透显，1实显，2反色，这里都用实显
  u8g2.setDrawColor(1);  

  //绘制电压曲线和外框
  volt.val = 0;
  u8g2.drawFrame(WAVE_BOX_L_S, 0, WAVE_BOX_W, WAVE_BOX_H);
  u8g2.drawFrame(WAVE_BOX_L_S + 1, 1, WAVE_BOX_W - 2, WAVE_BOX_H - 2);
  if (list.box_y == list.box_y_trg[ui.layer] && list.y == list.y_trg)
  {
    for (int i = 0; i < WAVE_SAMPLE * WAVE_W; i++) volt.ch0_adc[i] = volt.val = analogRead(analog_pin[ui.select[ui.layer]]);
    for (int i = 1; i < WAVE_W - 1; i++)
    { 
      volt.ch0_wave[i] = map(volt.ch0_adc[int(5 * i)], 0, 4095, WAVE_MAX, WAVE_MIN);   
      u8g2.drawLine(WAVE_L + i - 1, WAVE_U + volt.ch0_wave[i - 1], WAVE_L + i, WAVE_U + volt.ch0_wave[i]);
    }
  }

  //绘制电压值
  u8g2.setFontDirection(0);
  u8g2.setFont(VOLT_FONT); 
  u8g2.setCursor(39, DISP_H - 6);
  u8g2.print(volt.val / 4096.0 * 3.3);
  u8g2.print("V");

  //绘制列表选择框和电压文字背景
  u8g2.setDrawColor(2);
  u8g2.drawBox(VOLT_TEXT_BG_L_S, DISP_H - VOLT_TEXT_BG_H, volt.text_bg_r, VOLT_TEXT_BG_H);

  //反转屏幕内元素颜色，白天模式遮罩
  if (!ui.param[DARK_MODE]) u8g2.drawBox(0, 0, DISP_W, DISP_H);
}

//弹窗通用显示函数
void window_show()
{
  //绘制背景列表，根据开关判断背景是否要虚化
  list_show(win.bg, win.index);
  if (ui.param[WIN_BOK]) for (uint16_t i = 0; i < buf_len; ++i)  buf_ptr[i] = buf_ptr[i] & (i % 2 == 0 ? 0x55 : 0xAA);

  //更新动画目标值
  u8g2.setFont(WIN_FONT);
  win.bar_trg = (float)(*win.value - win.min) / (float)(win.max - win.min) * (WIN_BAR_W - 4);

  //计算动画过渡值
  animation(&win.bar, &win.bar_trg, WIN_ANI);
  animation(&win.y, &win.y_trg, WIN_ANI);

  //绘制窗口
  u8g2.setDrawColor(0); u8g2.drawRBox(win.l, (int16_t)win.y, WIN_W, WIN_H, 2);    //绘制外框背景
  u8g2.setDrawColor(1); u8g2.drawRFrame(win.l, (int16_t)win.y, WIN_W, WIN_H, 2);  //绘制外框描边
  u8g2.drawRFrame(win.l + 5, (int16_t)win.y + 20, WIN_BAR_W, WIN_BAR_H, 1);       //绘制进度条外框
  u8g2.drawBox(win.l + 7, (int16_t)win.y + 22, win.bar, WIN_BAR_H - 4);           //绘制进度条
  u8g2.setCursor(win.l + 5, (int16_t)win.y + 14); u8g2.print(win.title);          //绘制标题
  u8g2.setCursor(win.l + 78, (int16_t)win.y + 14); u8g2.print(*win.value);        //绘制当前值
  
  //需要在窗口修改参数时立即见效的函数
  if (!strcmp(win.title, "Disp Bri")) u8g2.setContrast(ui.param[DISP_BRI]);

  //反转屏幕内元素颜色，白天模式遮罩
  u8g2.setDrawColor(2);
  if (!ui.param[DARK_MODE]) u8g2.drawBox(0, 0, DISP_W, DISP_H);
}

/************************************* 处理函数 *************************************/

/*********************************** 通用处理函数 ***********************************/

//磁贴类页面旋转时判断通用函数
void tile_rotate_switch()
{
  switch (btn.id)
  { 
    case BTN_ID_CC:
      if (ui.init)
      {
        if (ui.select[ui.layer] > 0)
        {
          ui.select[ui.layer] -= 1;
          tile.icon_x_trg += TILE_ICON_S;
          tile.select_flag = false;
        }
        else 
        {
          if (ui.param[TILE_LOOP])
          {
            ui.select[ui.layer] = ui.num[ui.index] - 1;
            tile.icon_x_trg = - TILE_ICON_S * (ui.num[ui.index] - 1);
            break;
          }
          else tile.select_flag = true;
        }
      }
      break;

    case BTN_ID_CW:
      if (ui.init)
      {
        if (ui.select[ui.layer] < (ui.num[ui.index] - 1)) 
        {
          ui.select[ui.layer] += 1;
          tile.icon_x_trg -= TILE_ICON_S;
          tile.select_flag = false;
        }
        else 
        {
          if (ui.param[TILE_LOOP])
          {
            ui.select[ui.layer] = 0;
            tile.icon_x_trg = 0;
            break;
          }
          else tile.select_flag = true;
        }
      }
      break;
  }
}

//列表类页面旋转时判断通用函数
void list_rotate_switch()
{
  if (!list.loop)
  {
    switch (btn.id)
    {
      case BTN_ID_CC:
        if (ui.select[ui.layer] == 0)
        {
          if (ui.param[LIST_LOOP] && ui.init)
          {
            list.loop = true;
            ui.select[ui.layer] = ui.num[ui.index] - 1;
            if (ui.num[ui.index] > list.line_n) 
            {
              list.box_y_trg[ui.layer] = DISP_H - LIST_LINE_H;
              list.y_trg = DISP_H - ui.num[ui.index] * LIST_LINE_H;
            }
            else list.box_y_trg[ui.layer] = (ui.num[ui.index] - 1) * LIST_LINE_H;
            break;
          }
          else break;
        }
        if (ui.init)
        {
          ui.select[ui.layer] -= 1;
          if (ui.select[ui.layer] < - (list.y_trg / LIST_LINE_H)) 
          {
            if (!(DISP_H % LIST_LINE_H)) list.y_trg += LIST_LINE_H;
            else
            {
              if (list.box_y_trg[ui.layer] == DISP_H - LIST_LINE_H * list.line_n)
              {
                list.y_trg += (list.line_n + 1) * LIST_LINE_H - DISP_H;
                list.box_y_trg[ui.layer] = 0;
              }
              else if (list.box_y_trg[ui.layer] == LIST_LINE_H)
              {
                list.box_y_trg[ui.layer] = 0;
              }
              else list.y_trg += LIST_LINE_H;
            }
          }
          else list.box_y_trg[ui.layer] -= LIST_LINE_H;
          break;
        }

      case BTN_ID_CW:
        if (ui.select[ui.layer] == (ui.num[ui.index] - 1))
        {
          if (ui.param[LIST_LOOP] && ui.init)
          {
            list.loop = true;
            ui.select[ui.layer] = 0;
            list.y_trg = 0;
            list.box_y_trg[ui.layer] = 0;
            break;
          }
          else break;
        }
        if (ui.init)
        {
          ui.select[ui.layer] += 1;
          if ((ui.select[ui.layer] + 1) > (list.line_n - list.y_trg / LIST_LINE_H))
          {
            if (!(DISP_H % LIST_LINE_H)) list.y_trg -= LIST_LINE_H;
            else
            {
              if (list.box_y_trg[ui.layer] == LIST_LINE_H * (list.line_n - 1))
              {
                list.y_trg -= (list.line_n + 1) * LIST_LINE_H - DISP_H;
                list.box_y_trg[ui.layer] = DISP_H - LIST_LINE_H;
              }
              else if (list.box_y_trg[ui.layer] == DISP_H - LIST_LINE_H * 2)
              {
                list.box_y_trg[ui.layer] = DISP_H - LIST_LINE_H;
              }
              else list.y_trg -= LIST_LINE_H;
            }
          }
          else list.box_y_trg[ui.layer] += LIST_LINE_H;
          break;
        }
        break;
    }
  }
}

//弹窗通用处理函数
void window_proc()
{
  window_show();
  if (win.y == WIN_Y_TRG) ui.index = win.index;
  if (btn.pressed && win.y == win.y_trg && win.y != WIN_Y_TRG)
  {
    btn.pressed = false;
    switch (btn.id)
    {
      case BTN_ID_CW: if (*win.value < win.max)  *win.value += win.step;  eeprom.change = true;  break;
      case BTN_ID_CC: if (*win.value > win.min)  *win.value -= win.step;  eeprom.change = true;  break;  
      case BTN_ID_SP: case BTN_ID_LP: win.y_trg = WIN_Y_TRG; break;
    }
  }
}

/********************************** 分页面处理函数 **********************************/

//睡眠页面处理函数
void sleep_proc()
{
  while (ui.sleep)
  {
    //睡眠时循环执行的函数
    for (uint8_t n = 0; n < KEYS; n++) runKey(n);

    //睡眠时需要扫描旋钮才能退出睡眠
    btn_scan();

    //当旋钮有动作时
    if (btn.pressed) { btn.pressed = false; switch (btn.id) {    

        //睡眠时顺时针旋转执行的函数
        case BTN_ID_CW:
          switch (knob.param[KNOB_ROT])
          {
            case KNOB_ROT_VOL: Consumer.press(HIDConsumer::VOLUME_UP);       Consumer.release(); break;
            case KNOB_ROT_BRI: Consumer.press(HIDConsumer::BRIGHTNESS_UP);   Consumer.release(); break;
          }
          break;

        //睡眠时逆时针旋转执行的函数
        case BTN_ID_CC:
          switch (knob.param[KNOB_ROT])
          {
            case KNOB_ROT_VOL: Consumer.press(HIDConsumer::VOLUME_DOWN);     Consumer.release(); break;
            case KNOB_ROT_BRI: Consumer.press(HIDConsumer::BRIGHTNESS_DOWN); Consumer.release(); break;
          }
          break;

        //睡眠时短按执行的函数
        case BTN_ID_SP: Keyboard.press(knob.param[KNOB_COD]);  Keyboard.release(knob.param[KNOB_COD]); break;   
        
        //睡眠时长按执行的函数
        case BTN_ID_LP: ui.index = M_MAIN;  ui.state = S_LAYER_IN; u8g2.setPowerSave(0); ui.sleep = false; break;
      }
    }
  }
}

//主菜单处理函数，磁贴类模板
void main_proc()
{
  tile_show(main_menu, main_icon_pic);
  if (btn.pressed) { btn.pressed = false; switch (btn.id) { case BTN_ID_CW: case BTN_ID_CC: tile_rotate_switch(); break; case BTN_ID_SP: switch (ui.select[ui.layer]) {

        case 0: ui.index = M_SLEEP;   ui.state = S_LAYER_OUT; break;
        case 1: ui.index = M_EDITOR;  ui.state = S_LAYER_IN;  break;
        case 2: ui.index = M_VOLT;    ui.state = S_LAYER_IN;  break;
        case 3: ui.index = M_SETTING; ui.state = S_LAYER_IN;  break;
      }
    }
    if (!tile.select_flag && ui.init) { tile.indi_x = 0; tile.title_y = tile.title_y_calc; }
  }
}

//编辑器菜单处理函数
void editor_proc()
{
  list_show(editor_menu, M_EDITOR); 
  if (btn.pressed) { btn.pressed = false; switch (btn.id) { case BTN_ID_CW: case BTN_ID_CC: list_rotate_switch(); break; case BTN_ID_LP: ui.select[ui.layer] = 0; case BTN_ID_SP: switch (ui.select[ui.layer]) {
        
        case 0:   ui.index = M_MAIN;  ui.state = S_LAYER_OUT; break;
        case 1:   ui.index = M_KNOB;  ui.state = S_LAYER_IN;  check_box_v_init(knob.param); break;
        case 2:   ui.index = M_KEYS;  ui.state = S_LAYER_IN;  check_box_v_init(key[K0]);    break;
        case 3:   ui.index = M_KEYS;  ui.state = S_LAYER_IN;  check_box_v_init(key[K1]);    break;
        case 4:   ui.index = M_KEYS;  ui.state = S_LAYER_IN;  check_box_v_init(key[K2]);    break;
        case 5:   ui.index = M_KEYS;  ui.state = S_LAYER_IN;  check_box_v_init(key[K3]);    break;
        case 6:   ui.index = M_KEYS;  ui.state = S_LAYER_IN;  check_box_v_init(key[K4]);    break;
        case 7:   ui.index = M_KEYS;  ui.state = S_LAYER_IN;  check_box_v_init(key[K5]);    break;
        case 8:   ui.index = M_KEYS;  ui.state = S_LAYER_IN;  check_box_v_init(key[K6]);    break;
        case 9:   ui.index = M_KEYS;  ui.state = S_LAYER_IN;  check_box_v_init(key[K7]);    break;
        case 10:  ui.index = M_KEYS;  ui.state = S_LAYER_IN;  check_box_v_init(key[K8]);    break;
        case 11:  ui.index = M_KEYS;  ui.state = S_LAYER_IN;  check_box_v_init(key[K9]);    break;
      }
    }
  }
}

//旋钮编辑菜单处理函数
void knob_proc()
{
  list_show(knob_menu, M_KNOB);
  if (btn.pressed) { btn.pressed = false; switch (btn.id) { case BTN_ID_CW: case BTN_ID_CC: list_rotate_switch(); break; case BTN_ID_LP: ui.select[ui.layer] = 0; case BTN_ID_SP: switch (ui.select[ui.layer]) {
        
        case 0: ui.index = M_EDITOR;  ui.state = S_LAYER_OUT; break;
        case 1: ui.index = M_KRF;     ui.state = S_LAYER_IN;  check_box_s_init(&knob.param[KNOB_ROT], &knob.param[KNOB_ROT_P]); break;
        case 2: ui.index = M_KCODE;   ui.state = S_LAYER_IN;  check_box_s_init(&knob.param[KNOB_COD], &knob.param[KNOB_COD_P]); break;
      }
    }
  }
}

//旋钮旋转功能菜单处理函数
void krf_proc()
{
  list_show(krf_menu, M_KRF);
  if (btn.pressed) { btn.pressed = false; switch (btn.id) { case BTN_ID_CW: case BTN_ID_CC: list_rotate_switch(); break; case BTN_ID_LP: ui.select[ui.layer] = 0; case BTN_ID_SP: switch (ui.select[ui.layer]) {
        
        case 0: ui.index = M_KNOB;  ui.state = S_LAYER_OUT; break;
        case 1: break;
        case 2: check_box_s_select(KNOB_ROT_DIS, ui.select[ui.layer]); break;
        case 3: break;
        case 4: check_box_s_select(KNOB_ROT_VOL, ui.select[ui.layer]); break;
        case 5: check_box_s_select(KNOB_ROT_BRI, ui.select[ui.layer]); break;
        case 6: break;
      }
    }
  }
}


//按键编辑菜单处理函数
void keys_proc()
{
  list_show(keys_menu, M_KEYS);
  if (btn.pressed) { btn.pressed = false; switch (btn.id) { case BTN_ID_CW: case BTN_ID_CC: list_rotate_switch(); break; case BTN_ID_LP: ui.select[ui.layer] = 0; case BTN_ID_SP: switch (ui.select[ui.layer]) {
        
        case 0: ui.index = M_EDITOR;  ui.state = S_LAYER_OUT; break;
        case 1: window_value_init("Trigger", TRIGGER, &key[ui.select[ui.layer - 1] - 2][TRIGGER],  40,  0,  1, keys_menu, M_KEYS);  break;
        case 2: window_value_init("Release", RELEASE, &key[ui.select[ui.layer - 1] - 2][RELEASE],  40,  0,  1, keys_menu, M_KEYS);  break;
        case 3: window_value_init("Top",     TOP,     &key[ui.select[ui.layer - 1] - 2][TOP],      40,  0,  1, keys_menu, M_KEYS);  break;
        case 4: window_value_init("Bottom",  BOTTOM,  &key[ui.select[ui.layer - 1] - 2][BOTTOM],   40,  0,  1, keys_menu, M_KEYS);  break;
        case 5: window_value_init("Times",   TIMES,   &key[ui.select[ui.layer - 1] - 2][TIMES],    40,  0,  1, keys_menu, M_KEYS);  break;
        case 6: ui.index = M_KCODE; ui.state = S_LAYER_IN; check_box_s_init(&key[ui.select[ui.layer - 1] - 2][CODES], &key[ui.select[ui.layer - 1] - 2][CODES_P]); break;
        case 7: ui.index = M_KMODE; ui.state = S_LAYER_IN; check_box_s_init(&key[ui.select[ui.layer - 1] - 2][MODES], &key[ui.select[ui.layer - 1] - 2][MODES_P]); break;
      }
    }
  }
}

//点按功能菜单处理函数
void kcode_proc()
{
  list_show(kcode_menu, M_KCODE);
  if (btn.pressed) { btn.pressed = false; switch (btn.id) { case BTN_ID_CW: case BTN_ID_CC: list_rotate_switch(); break;  case BTN_ID_LP: ui.select[ui.layer] = 0; case BTN_ID_SP: switch (ui.select[ui.layer]) {
    
        case 0:   if (ui.select[ui.layer - 2] == 1) ui.index = M_KNOB; else ui.index = M_KEYS; ui.state = S_LAYER_OUT; break;
        case 1:   break;
        case 2:   check_box_s_select(KEY_DIS,           ui.select[ui.layer]); break;
        case 3:   break;
        case 4:   check_box_s_select('A',               ui.select[ui.layer]); break;
        case 5:   check_box_s_select('B',               ui.select[ui.layer]); break;
        case 6:   check_box_s_select('C',               ui.select[ui.layer]); break;
        case 7:   check_box_s_select('D',               ui.select[ui.layer]); break;
        case 8:   check_box_s_select('E',               ui.select[ui.layer]); break;
        case 9:   check_box_s_select('F',               ui.select[ui.layer]); break;
        case 10:  check_box_s_select('G',               ui.select[ui.layer]); break;
        case 11:  check_box_s_select('H',               ui.select[ui.layer]); break;
        case 12:  check_box_s_select('I',               ui.select[ui.layer]); break;
        case 13:  check_box_s_select('J',               ui.select[ui.layer]); break;
        case 14:  check_box_s_select('K',               ui.select[ui.layer]); break;
        case 15:  check_box_s_select('L',               ui.select[ui.layer]); break;
        case 16:  check_box_s_select('M',               ui.select[ui.layer]); break;
        case 17:  check_box_s_select('N',               ui.select[ui.layer]); break;
        case 18:  check_box_s_select('O',               ui.select[ui.layer]); break;
        case 19:  check_box_s_select('P',               ui.select[ui.layer]); break;
        case 20:  check_box_s_select('Q',               ui.select[ui.layer]); break;
        case 21:  check_box_s_select('R',               ui.select[ui.layer]); break;
        case 22:  check_box_s_select('S',               ui.select[ui.layer]); break;
        case 23:  check_box_s_select('T',               ui.select[ui.layer]); break;
        case 24:  check_box_s_select('U',               ui.select[ui.layer]); break;
        case 25:  check_box_s_select('V',               ui.select[ui.layer]); break;
        case 26:  check_box_s_select('W',               ui.select[ui.layer]); break;
        case 27:  check_box_s_select('X',               ui.select[ui.layer]); break;
        case 28:  check_box_s_select('Y',               ui.select[ui.layer]); break;
        case 29:  check_box_s_select('Z',               ui.select[ui.layer]); break;
        case 30:  break;
        case 31:  check_box_s_select('0',               ui.select[ui.layer]); break;
        case 32:  check_box_s_select('1',               ui.select[ui.layer]); break;
        case 33:  check_box_s_select('2',               ui.select[ui.layer]); break;
        case 34:  check_box_s_select('3',               ui.select[ui.layer]); break;
        case 35:  check_box_s_select('4',               ui.select[ui.layer]); break;
        case 36:  check_box_s_select('5',               ui.select[ui.layer]); break;
        case 37:  check_box_s_select('6',               ui.select[ui.layer]); break;
        case 38:  check_box_s_select('7',               ui.select[ui.layer]); break;
        case 39:  check_box_s_select('8',               ui.select[ui.layer]); break;
        case 40:  check_box_s_select('9',               ui.select[ui.layer]); break;
        case 41:  break;
        case 42:  check_box_s_select( KEY_ESC,          ui.select[ui.layer]); break;
        case 43:  check_box_s_select( KEY_F1,           ui.select[ui.layer]); break;
        case 44:  check_box_s_select( KEY_F2,           ui.select[ui.layer]); break;
        case 45:  check_box_s_select( KEY_F3,           ui.select[ui.layer]); break;
        case 46:  check_box_s_select( KEY_F4,           ui.select[ui.layer]); break;
        case 47:  check_box_s_select( KEY_F5,           ui.select[ui.layer]); break;
        case 48:  check_box_s_select( KEY_F6,           ui.select[ui.layer]); break;
        case 49:  check_box_s_select( KEY_F7,           ui.select[ui.layer]); break;
        case 50:  check_box_s_select( KEY_F8,           ui.select[ui.layer]); break;
        case 51:  check_box_s_select( KEY_F9,           ui.select[ui.layer]); break;
        case 52:  check_box_s_select( KEY_F10,          ui.select[ui.layer]); break;
        case 53:  check_box_s_select( KEY_F11,          ui.select[ui.layer]); break;
        case 54:  check_box_s_select( KEY_F12,          ui.select[ui.layer]); break;
        case 55:  break;
        case 56:  check_box_s_select( KEY_LEFT_CTRL,    ui.select[ui.layer]); break;
        case 57:  check_box_s_select( KEY_LEFT_SHIFT,   ui.select[ui.layer]); break;
        case 58:  check_box_s_select( KEY_LEFT_ALT,     ui.select[ui.layer]); break;
        case 59:  check_box_s_select( KEY_LEFT_GUI,     ui.select[ui.layer]); break;
        case 60:  check_box_s_select( KEY_RIGHT_CTRL,   ui.select[ui.layer]); break;
        case 61:  check_box_s_select( KEY_RIGHT_SHIFT,  ui.select[ui.layer]); break;
        case 62:  check_box_s_select( KEY_RIGHT_ALT,    ui.select[ui.layer]); break;
        case 63:  check_box_s_select( KEY_RIGHT_GUI,    ui.select[ui.layer]); break;
        case 64:  break;
        case 65:  check_box_s_select( KEY_CAPS_LOCK,    ui.select[ui.layer]); break;
        case 66:  check_box_s_select( KEY_BACKSPACE,    ui.select[ui.layer]); break;
        case 67:  check_box_s_select( KEY_RETURN,       ui.select[ui.layer]); break;
        case 68:  check_box_s_select( KEY_INSERT,       ui.select[ui.layer]); break;
        case 69:  check_box_s_select( KEY_DELETE,       ui.select[ui.layer]); break;
        case 70:  check_box_s_select( KEY_TAB,          ui.select[ui.layer]); break;
        case 71:  break;
        case 72:  check_box_s_select( KEY_HOME,         ui.select[ui.layer]); break;
        case 73:  check_box_s_select( KEY_END,          ui.select[ui.layer]); break;
        case 74:  check_box_s_select( KEY_PAGE_UP,      ui.select[ui.layer]); break;
        case 75:  check_box_s_select( KEY_PAGE_DOWN,    ui.select[ui.layer]); break;
        case 76:  break;
        case 77:  check_box_s_select( KEY_UP_ARROW,     ui.select[ui.layer]); break;
        case 78:  check_box_s_select( KEY_DOWN_ARROW,   ui.select[ui.layer]); break;
        case 79:  check_box_s_select( KEY_LEFT_ARROW,   ui.select[ui.layer]); break;
        case 80:  check_box_s_select( KEY_RIGHT_ARROW,  ui.select[ui.layer]); break;
        case 81:  break;
      }
    }
  }
}

//按键模式菜单
void kmode_proc()
{
  list_show(kmode_menu, M_KMODE);
  if (btn.pressed) { btn.pressed = false; switch (btn.id) { case BTN_ID_CW: case BTN_ID_CC: list_rotate_switch(); break;  case BTN_ID_LP: ui.select[ui.layer] = 0; case BTN_ID_SP: switch (ui.select[ui.layer]) {
    
        case 0:   ui.index = M_KEYS; ui.state = S_LAYER_OUT; break;
        case 1:   break;
        case 2:   check_box_s_select(KEY_MODE1, ui.select[ui.layer]); break;
        case 3:   check_box_s_select(KEY_MODE2, ui.select[ui.layer]); break;
        case 4:   break;
      }
    }
  }
}

//电压测量页处理函数
void volt_proc()
{
  volt_show();
  if (btn.pressed) { btn.pressed = false; switch (btn.id) { case BTN_ID_CW: case BTN_ID_CC: list_rotate_switch(); break;

      case BTN_ID_SP: case BTN_ID_LP: ui.index = M_MAIN;  ui.state = S_LAYER_OUT; break;
    }
  }
}

//设置菜单处理函数，多选框列表类模板，弹窗模板
void setting_proc()
{
  list_show(setting_menu, M_SETTING);
  if (btn.pressed) { btn.pressed = false; switch (btn.id) { case BTN_ID_CW: case BTN_ID_CC: list_rotate_switch(); break; case BTN_ID_LP: ui.select[ui.layer] = 0; case BTN_ID_SP: switch (ui.select[ui.layer]) {
        
        //返回更浅层级，长按被当作选择这一项，也是执行这一行
        case 0:   ui.index = M_MAIN;  ui.state = S_LAYER_OUT; break;
        
        //弹出窗口，参数初始化：标题，参数名，参数值，最大值，最小值，步长，背景列表名，背景列表标签
        case 1:   window_value_init("Disp Bri", DISP_BRI, &ui.param[DISP_BRI],  255,  0,  5, setting_menu, M_SETTING);  break;
        case 2:   window_value_init("Tile Ani", TILE_ANI, &ui.param[TILE_ANI],  100, 10,  1, setting_menu, M_SETTING);  break;
        case 3:   window_value_init("List Ani", LIST_ANI, &ui.param[LIST_ANI],  100, 10,  1, setting_menu, M_SETTING);  break;
        case 4:   window_value_init("Win Ani",  WIN_ANI,  &ui.param[WIN_ANI],   100, 10,  1, setting_menu, M_SETTING);  break;
        case 5:   window_value_init("Spot Ani", SPOT_ANI, &ui.param[SPOT_ANI],  100, 10,  1, setting_menu, M_SETTING);  break;
        case 6:   window_value_init("Tag Ani",  TAG_ANI,  &ui.param[TAG_ANI],   100, 10,  1, setting_menu, M_SETTING);  break;
        case 7:   window_value_init("Fade Ani", FADE_ANI, &ui.param[FADE_ANI],  255,  0,  1, setting_menu, M_SETTING);  break;
        case 8:   window_value_init("Btn SPT",  BTN_SPT,  &ui.param[BTN_SPT],   255,  0,  1, setting_menu, M_SETTING);  break;
        case 9:   window_value_init("Btn LPT",  BTN_LPT,  &ui.param[BTN_LPT],   255,  0,  1, setting_menu, M_SETTING);  break;

        //多选框
        case 10:  check_box_m_select( TILE_UFD  );  break;
        case 11:  check_box_m_select( LIST_UFD  );  break;
        case 12:  check_box_m_select( TILE_LOOP );  break;
        case 13:  check_box_m_select( LIST_LOOP );  break;
        case 14:  check_box_m_select( WIN_BOK   );  break;
        case 15:  check_box_m_select( KNOB_DIR  );  break;
        case 16:  check_box_m_select( DARK_MODE );  break;

        //关于本机
        case 17:  ui.index = M_ABOUT; ui.state = S_LAYER_IN; break;
      }
    }
  }
}

//关于本机页
void about_proc()
{
  list_show(about_menu, M_ABOUT);
  if (btn.pressed) { btn.pressed = false; switch (btn.id) { case BTN_ID_CW: case BTN_ID_CC: list_rotate_switch(); break; case BTN_ID_LP: ui.select[ui.layer] = 0; case BTN_ID_SP: switch (ui.select[ui.layer]) {

        case 0:   ui.index = M_SETTING;  ui.state = S_LAYER_OUT; break;
      }
    }
  }
}

//总的UI进程
void ui_proc()
{
  u8g2.sendBuffer();
  switch (ui.state)
  {
    case S_FADE:          fade();                   break;  //转场动画
    case S_WINDOW:        window_param_init();      break;  //弹窗初始化
    case S_LAYER_IN:      layer_init_in();          break;  //层级初始化
    case S_LAYER_OUT:     layer_init_out();         break;  //层级初始化
  
    case S_NONE: u8g2.clearBuffer(); switch (ui.index)      //直接选择页面
    {
      case M_WINDOW:      window_proc();            break;
      case M_SLEEP:       sleep_proc();             break;
      case M_MAIN:        main_proc();              break;
      case M_EDITOR:      editor_proc();            break;
      case M_KNOB:        knob_proc();              break;
      case M_KRF:         krf_proc();               break;
      case M_KEYS:        keys_proc();              break;
      case M_KCODE:       kcode_proc();             break;
      case M_KMODE:       kmode_proc();             break;
      case M_VOLT:        volt_proc();              break;
      case M_SETTING:     setting_proc();           break;
      case M_ABOUT:       about_proc();             break;
    }
  }
}

//OLED初始化函数
void oled_init()
{
  u8g2.setBusClock(1000000);  //硬件IIC接口使用
  u8g2.begin();
  u8g2.setContrast(ui.param[DISP_BRI]);
  buf_ptr = u8g2.getBufferPtr();
  buf_len = 8 * u8g2.getBufferTileHeight() * u8g2.getBufferTileWidth();
}

void setup() 
{
  eeprom_init();
  ui_init();
  oled_init();
  btn_init();
  hid_init();
  key_init();
}

void loop() 
{
  btn_scan();
  ui_proc();
}



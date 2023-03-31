/*
  此程序用于实现wooting键盘中的Rapid trigger功能。
  
  主功能移植并优化自Github上的一个开源项目，添加了按键的独立参数和两端的死区，优化了初始化方式，增加控制器，增加模仿稚晖君 MonoUI 的超丝滑菜单。

  当前版本：v2.0，增加控制器，实现了以下功能：

    * 丝滑菜单，可调节动画效果的速度，速度值 0 - 10 ，值 1 - 10 模拟临界阻尼的效果，以最大速度开始，逐渐减速，并刚好停在目标位置，值 0 模拟欠阻尼的效果，有类似果冻的回弹效果。
    * 旋钮功能，在休眠界面旋转可调节音量或亮度，点按后进入主菜单，在菜单界面旋转滚动菜单，点按选择菜单。
    * 三种模式，支持不同极性的磁轴和传感器的组合，还有禁用模式。
    * 参数设置，支持设置几乎所有参数，包括：触发值，释放值，上死区，下死区，键值，模式，初始化时放大抖动的倍数，每个按键都可以独立设置，也可以一起设置。
    * 断电保存，回到休眠模式即保存当前设置，断电后不会丢失。
    * 测试引脚，支持查看每一个引脚的电压变化情况，用于测试传感器和引脚是否正常。
  
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

    * 上传程序前，先确认使用的轴的总键程是否为4mm，如果不是，需要修改 key 数组变量中代表 travel 一行的相关参数，单位0.1mm，即4mm的值为40。
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

/************************************* 旋钮相关 *************************************/

#define   AIO   PB12
#define   BIO   PB13
#define   SW    PB14

#define   DEBOUNCE 50
uint8_t   btn_id = 0;
uint8_t   btn_flag = 0;
bool      btn_val = false;
bool      btn_val_last = false;
bool      btn_pressed = false;
bool      CW_1 = false;
bool      CW_2 = false;

void btn_inter() 
{
  bool alv = digitalRead(AIO);
  bool blv = digitalRead(BIO);
  if (btn_flag == 0 && alv == LOW) 
  {
    CW_1 = blv;
    btn_flag = 1;
  }
  if (btn_flag && alv) 
  {
    CW_2 = !blv;
    if (CW_1 && CW_2)
     {
      btn_id = 0;
      btn_pressed = true;
    }
    if (CW_1 == false && CW_2 == false) 
    {
      btn_id = 1;
      btn_pressed = true;
    }
    btn_flag = 0;
  }
}

void btn_init() 
{
  pinMode(AIO, INPUT);
  pinMode(BIO, INPUT);
  pinMode(SW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(AIO), btn_inter, CHANGE);
}

void btn_scan() 
{
  btn_val = digitalRead(SW);
  if (btn_val != btn_val_last)
  {
    delay(DEBOUNCE);
    btn_val_last = btn_val;
    if (btn_val == LOW)
    {
      btn_pressed = true;
      btn_id = 2;
    }
  }
}

/************************************* 按键相关 *************************************/

// 按键全局变量
#define         KEYS        10       // 按键数
#define         LIGHT       PC13     // 指示灯，插入后板子上的蓝色指示灯会亮起一秒，亮起时不能按下按键。如果需更改，注意是置低点亮，置高熄灭
#define         TRIGGER     0
#define         RELEASE     1
#define         TOP         2
#define         BOTTOM      3
#define         TIMES       4
#define         CODES       5
#define         MODES       6
#define         PINS        7
#define         TRAVEL      8

// 可以在控制器中修改的变量, 多出的一位用于调整全部参数
uint8_t         key        [9][KEYS + 1] =
{ 
  // key0  key1  key2  key3  key4  key5  key6  key7  key8  key9   all
  {    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3 }, // trigger
  {    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3 }, // release
  {    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3 }, // top
  {    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,    3 }, // bottom
  {   10,   10,   10,   10,   10,   10,   10,   10,   10,   10,   10 }, // times
  {  'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k' }, // codes
  {    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 }, // modes
  {  PA0,  PA1,  PA2,  PA3,  PA4,  PA5,  PA6,  PA7,  PB0,  PB1,  PB1 }, // pins
  {   40,   40,   40,   40,   40,   40,   40,   40,   40,   40,   40 }, // travel
}; 

/************************************* 按键扫描 *************************************/

// 按键参数结构体
struct 
{
  int min;
  int max;
  int trigger;
  int release;
  int value;
  int reference;
  int unit;
  int cushion;
  bool init;  
  bool pressed;
} keyx[KEYS];

/************************************* 不分模式 *************************************/

// 初始化
void key_init() 
{
  // 打开初始化指示灯
  pinMode(LIGHT, OUTPUT);
  digitalWrite(LIGHT, LOW);

  // 读模拟量，最大值和最小值先暂定为当前值
  for (int n = 0; n < KEYS; n++) 
  {
    pinMode(key[PINS][n],INPUT_ANALOG);
    keyx[n].value = analogRead(key[PINS][n]);
    keyx[n].min = keyx[n].value;
    keyx[n].max = keyx[n].value;
  }

  // 并行扫描所有传感器，记录每个传感器的初始抖动范围，每1ms扫描一次，扫描1s
  for (int i = 0; i < 1000; i++) 
  {
    for (int n = 0; n < KEYS; n++) 
    {
      keyx[n].value = analogRead(key[PINS][n]);
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

  for (int n = 0; n < KEYS; n++) 
  {
    keyx[n].cushion = (keyx[n].max - keyx[n].min) * key[TIMES][n];      // 计算缓冲值

    // 初始化一些参数
    keyx[n].init = false;                                               // 按键还没有被首次按下
    keyx[n].reference = keyx[n].value;                                  // 初始化参考值为当前值
    keyx[n].trigger = key[TRIGGER][n] * keyx[n].cushion;                // 初始化触发值，避免插上的瞬间有输入
    keyx[n].release = key[RELEASE][n] * keyx[n].cushion;                // 初始化释放值，避免插上的瞬间有输入

    if (key[MODES][n] == 1)
    {
      keyx[n].min = keyx[n].value;                                      // 初始化最小值，暂定为当前值
      keyx[n].max = keyx[n].value + keyx[n].cushion;                    // 初始化最大值，暂定为当前值 + 缓冲值
    }
    else if (key[MODES][n] == 2)
    {
      keyx[n].min = keyx[n].value - keyx[n].cushion;                    // 初始化最小值，暂定为当前值
      keyx[n].max = keyx[n].value;                                      // 初始化最大值，暂定为当前值 + 缓冲值
    }
  }

  // 关闭初始化指示灯
  digitalWrite(LIGHT, HIGH);
}

// 转换距离，将以毫米为单位的值转换成模拟量，假设磁铁外磁场强度均匀变化，则距离与模拟量线性相关
void convertDistance(int n) 
{
  keyx[n].unit = (keyx[n].max - keyx[n].min) / key[TRAVEL][n];
  keyx[n].trigger = keyx[n].unit * key[TRIGGER][n];
  keyx[n].release = keyx[n].unit * key[RELEASE][n];
}

/************************************* 区分模式 *************************************/

/*************************************  模式 1  *************************************/

// 按下按键，发送后改变按键状态，按键首次被按下
void pressKey_1(int n) 
{
  if (keyx[n].init)
  {
    Keyboard.press(key[CODES][n]);
  } 
  else if (keyx[n].value > keyx[n].min + keyx[n].cushion)
  {
    keyx[n].init = true;
  }
  keyx[n].pressed = true;
}

// 释放按键，发送后改变按键状态，按键首次被按下
void releaseKey_1(int n) 
{
  if (keyx[n].init)
  {
    Keyboard.release(key[CODES][n]);
  } 
  else if (keyx[n].value > keyx[n].min + keyx[n].cushion)
  {
    keyx[n].init = true;
  }
  keyx[n].pressed = false;
}

// 平衡按键，按下时探索更大的值，释放时探索更小的值，以免在使用时极值漂移使抖动能误触发
void balanceKey_1(int n) 
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
void processKey_1(int n) 
{
  // 不在死区移动时的判断
  if ((keyx[n].value > keyx[n].min + keyx[n].unit * key[TOP][n]) && (keyx[n].value < keyx[n].max - keyx[n].unit * key[BOTTOM][n]))
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
  else if ((keyx[n].value < keyx[n].min + keyx[n].unit * key[TOP][n]) && keyx[n].pressed)
  {
    releaseKey_1(n);
  }
  else if ((keyx[n].value > keyx[n].max - keyx[n].unit * key[BOTTOM][n]) && !keyx[n].pressed)
  {
    pressKey_1(n);
  }
}

/*************************************  模式 2  *************************************/

// 按下按键，发送后改变按键状态，按键首次被按下
void pressKey_2(int n) 
{
  if (keyx[n].init)
  {
    Keyboard.press(key[CODES][n]);
  } 
  else if (keyx[n].value < keyx[n].max - keyx[n].cushion)
  {
    keyx[n].init = true;
  }
  keyx[n].pressed = true;
}

// 释放按键，发送后改变按键状态，按键首次被按下
void releaseKey_2(int n) 
{
  if (keyx[n].init)
  {
    Keyboard.release(key[CODES][n]);
  } 
  else if (keyx[n].value < keyx[n].min - keyx[n].cushion)
  {
    keyx[n].init = true;
  }
  keyx[n].pressed = false;
}

// 平衡按键，按下时探索更小的值，释放时探索更大的值，以免在使用时极值漂移使抖动能误触发
void balanceKey_2(int n) 
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
void processKey_2(int n) 
{
  // 不在死区移动时的判断
  if ((keyx[n].value > keyx[n].min + keyx[n].unit * key[BOTTOM][n]) && (keyx[n].value < keyx[n].max - keyx[n].unit * key[TOP][n]))
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
  else if ((keyx[n].value < keyx[n].min + keyx[n].unit * key[BOTTOM][n]) && !keyx[n].pressed)
  {
    pressKey_2(n);
  }
  else if ((keyx[n].value > keyx[n].max - keyx[n].unit * key[TOP][n]) && keyx[n].pressed)
  {
    releaseKey_2(n);
  }
}

// 扫描一次按键
void runKey(int n) 
{
  if (key[MODES][n] == 1)
  {
    keyx[n].value = analogRead(key[PINS][n]);
    balanceKey_1(n);
    processKey_1(n);
  }
  else if (key[MODES][n] == 2)
  {
    keyx[n].value = analogRead(key[PINS][n]);
    balanceKey_2(n);
    processKey_2(n);
  }
}

/************************************* 屏幕和UI *************************************/

#include <U8g2lib.h>
#include <Wire.h>

#define   SCL   PB6
#define   SDA   PB7
#define   RST   U8X8_PIN_NONE

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, SCL, SDA, RST);

/************************************* 定义页面 *************************************/

//目录
enum
{
  M_SLEEP,              //休眠页面
  M_LIST,               //主页面
  M_KEYS,               //按键选择页面
  M_EDIT,               //按键编辑页面
  M_EDITING,            //编辑进行页面
  M_CODE,               //键值选择页面
  M_ALPH,               //字母键选择页面
  M_NUMB,               //数字键选择页面
  M_MORE,               //更多键选择页面
  M_FUNC,               //功能键选择页面
  M_MODE,               //模式选择页面
  M_TEST,               //按键测试页面
  M_CHART,              //按键绘图页面
  M_CONTROLLER,         //控制器设置页面
  M_CONTROLLER_EDITING, //控制器正在设置页面
  M_CONTROL,            //控制页面
  M_ABOUT,              //关于本机
};

//状态
enum
{
  S_NONE,
  S_DISAPPEAR,
};

/************************************* 定义内容 *************************************/

//菜单结构体
typedef struct MENU
{
  char *select;
} SELECT_LIST;

//主菜单
SELECT_LIST list[]
{
  {"<< Sleep"},
  {"- Editor"},
  {"- Tester"},
  {"- Controller"},
};

//按键选择菜单
SELECT_LIST keys[]
{
  {"<< Main"},
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
  {"- All"},
};

//按键编辑菜单
SELECT_LIST edit[]
{
  {"<< Keys"},
  {"- Trigger"},
  {"- Release"},
  {"- Top"},
  {"- Bottom"},
  {"- Times"},
  {"- Codes"},
  {"- Modes"},
};

//键值选择菜单
SELECT_LIST code[]
{
  {"<< Editor"},
  {"- Alphabet"},
  {"- Number"},
  {"- Function"},
  {"- More"},
};

//字母键选择菜单
SELECT_LIST alph[]
{
  {"<< Code"},
  {"- a"},
  {"- b"},
  {"- c"},
  {"- d"},
  {"- e"},
  {"- f"},
  {"- g"},
  {"- h"},
  {"- i"},
  {"- j"},
  {"- k"},
  {"- l"},
  {"- m"},
  {"- n"},
  {"- o"},
  {"- p"},
  {"- q"},
  {"- r"},
  {"- s"},
  {"- t"},
  {"- u"},
  {"- v"},
  {"- w"},
  {"- x"},
  {"- y"},
  {"- z"},
};

//数字键选择菜单
SELECT_LIST numb[]
{
  {"<< Code"},
  {"- 0"},
  {"- 1"},
  {"- 2"},
  {"- 3"},
  {"- 4"},
  {"- 5"},
  {"- 6"},
  {"- 7"},
  {"- 8"},
  {"- 9"},
};

//功能键选择菜单
SELECT_LIST func[]
{
  {"<< Code"},
  {"- F1"},
  {"- F2"},
  {"- F3"},
  {"- F4"},
  {"- F5"},
  {"- F6"},
  {"- F7"},
  {"- F8"},
  {"- F9"},
  {"- F10"},
  {"- F11"},
  {"- F12"},
};

//其它键选择菜单
SELECT_LIST more[]
{
  {"<< Code"},
  {"- LEFT CTRL"},
  {"- RIGHT CTRL"},
  {"- LEFT SHIFT"},
  {"- RIGHT SHIFT"},
  {"- LEFT ALT"},
  {"- RIGHT ALT"},
  {"- LEFT GUI"},
  {"- RIGHT GUI"},
  {"- UP ARROW"},
  {"- DOWN ARROW"},
  {"- LEFT ARROW"},
  {"- RIGHT ARROW"},
  {"- BACKSPACE"},
  {"- TAB"},
  {"- RETURN"},
  {"- ESC"},
  {"- INSERT"},
  {"- DELETE"},
  {"- PAGE UP"},
  {"- PAGE DOWN"},
  {"- HOME"},
  {"- END"},
  {"- CAPS LOCK"},
};

//模式选择菜单
SELECT_LIST mode[]
{
  {"<< Editor"},
  {"- Mode 1"},
  {"- Mode 2"},
  {"- Disable"},
};

//按键测试菜单
SELECT_LIST test[]
{
  {"<< Main"},
  {"- Key 0 : PA 0"},
  {"- Key 1 : PA 1"},
  {"- Key 2 : PA 2"},
  {"- Key 3 : PA 3"},
  {"- Key 4 : PA 4"},
  {"- Key 5 : PA 5"},
  {"- Key 6 : PA 6"},
  {"- Key 7 : PA 7"},
  {"- Key 8 : PB 0"},
  {"- Key 9 : PB 1"},
};

//控制器设置菜单
SELECT_LIST controller[]
{
  {"<< Main"},
  {"- OLED Brightness"},
  {"- Animation Speed"},
  {"- Knob Function"},
  {"- About"},
};

//控制菜单
SELECT_LIST control[]
{
  {"<< Controller"},
  {"- Volume"},
  {"- Brightness"},
  {"- Disable"},
};
/************************************* 页面变量 *************************************/

//UI变量
uint8_t   ui_index;                 //目录变量
uint8_t   ui_state;                 //状态变量
uint8_t   *buf_ptr;                 //指向buf首地址的指针
uint16_t  buf_len;                  //buf的长度
bool      sleep_flag = true;        //休眠标志
uint8_t   disappear_step = 1;       //消失步数

float STEP[4]     = { 5.167, 5.167, 7.167, 6.167 };
float step[4]     = { STEP[0], STEP[1], STEP[2], STEP[3] };
float damping[4]  = { 0.573, 0.573, 0.673, 0.673 };
bool  flag[4]     = { false, false, false, false };

//主页面变量
int8_t    list_ui_select = 0;
uint8_t   list_x = 4;
int16_t   list_y = 0;
int16_t   list_y_trg = 1;
int16_t   list_box_width;
int16_t   list_box_width_trg;
int16_t   list_box_y;
int16_t   list_box_y_trg;
uint8_t   list_num = sizeof(list) / sizeof(SELECT_LIST);
int16_t   list_line_y;
int16_t   list_line_y_trg;

//按键选择页面变量
int8_t    keys_ui_select = 0;
uint8_t   keys_x = 4;
int16_t   keys_y = 0;
int16_t   keys_y_trg = 0;
int16_t   keys_box_width;
int16_t   keys_box_width_trg;
int16_t   keys_box_y;
int16_t   keys_box_y_trg;
uint8_t   keys_num = sizeof(keys) / sizeof(SELECT_LIST);
int16_t   keys_line_y;
int16_t   keys_line_y_trg;

//按键编辑页面变量
int8_t    edit_ui_select = 0;
uint8_t   edit_x = 4;
int16_t   edit_y = 0;
int16_t   edit_y_trg = 0;
int16_t   edit_box_width;
int16_t   edit_box_width_trg;
int16_t   edit_box_y;
int16_t   edit_box_y_trg;
uint8_t   edit_num = sizeof(edit) / sizeof(SELECT_LIST);
int16_t   edit_line_y;
int16_t   edit_line_y_trg;

//键值选择页面变量
int8_t    code_ui_select = 0;
uint8_t   code_x = 4;
int16_t   code_y = 0;
int16_t   code_y_trg = 0;
int16_t   code_box_width;
int16_t   code_box_width_trg;
int16_t   code_box_y;
int16_t   code_box_y_trg;
uint8_t   code_num = sizeof(code) / sizeof(SELECT_LIST);
int16_t   code_line_y;
int16_t   code_line_y_trg;

//字母键选择页面变量
int8_t    alph_ui_select = 0;
uint8_t   alph_x = 4;
int16_t   alph_y = 0;
int16_t   alph_y_trg = 0;
int16_t   alph_box_width;
int16_t   alph_box_width_trg;
int16_t   alph_box_y;
int16_t   alph_box_y_trg;
uint8_t   alph_num = sizeof(alph) / sizeof(SELECT_LIST);
int16_t   alph_line_y;
int16_t   alph_line_y_trg;

//数字键选择页面变量
int8_t    numb_ui_select = 0;
uint8_t   numb_x = 4;
int16_t   numb_y = 0;
int16_t   numb_y_trg = 0;
int16_t   numb_box_width;
int16_t   numb_box_width_trg;
int16_t   numb_box_y;
int16_t   numb_box_y_trg;
uint8_t   numb_num = sizeof(numb) / sizeof(SELECT_LIST);
int16_t   numb_line_y;
int16_t   numb_line_y_trg;

//功能键选择页面变量
int8_t    func_ui_select = 0;
uint8_t   func_x = 4;
int16_t   func_y = 0;
int16_t   func_y_trg = 0;
int16_t   func_box_width;
int16_t   func_box_width_trg;
int16_t   func_box_y;
int16_t   func_box_y_trg;
uint8_t   func_num = sizeof(func) / sizeof(SELECT_LIST);
int16_t   func_line_y;
int16_t   func_line_y_trg;

//其它键选择页面变量
int8_t    more_ui_select = 0;
uint8_t   more_x = 4;
int16_t   more_y = 0;
int16_t   more_y_trg = 0;
int16_t   more_box_width;
int16_t   more_box_width_trg;
int16_t   more_box_y;
int16_t   more_box_y_trg;
uint8_t   more_num = sizeof(more) / sizeof(SELECT_LIST);
int16_t   more_line_y;
int16_t   more_line_y_trg;

//模式选择页面变量
int8_t    mode_ui_select = 0;
uint8_t   mode_x = 4;
int16_t   mode_y = 0;
int16_t   mode_y_trg = 0;
int16_t   mode_box_width;
int16_t   mode_box_width_trg;
int16_t   mode_box_y;
int16_t   mode_box_y_trg;
uint8_t   mode_num = sizeof(mode) / sizeof(SELECT_LIST);
int16_t   mode_line_y;
int16_t   mode_line_y_trg;

//按键测试页面变量
int8_t    test_ui_select = 0;
uint8_t   test_x = 4;
int16_t   test_y = 0;
int16_t   test_y_trg = 0;
int16_t   test_box_width;
int16_t   test_box_width_trg;
int16_t   test_box_y;
int16_t   test_box_y_trg;
uint8_t   test_num = sizeof(test) / sizeof(SELECT_LIST);
int16_t   test_line_y;
int16_t   test_line_y_trg;

//控制器设置页面变量
int8_t    controller_ui_select = 0;
uint8_t   controller_x = 4;
int16_t   controller_y = 0;
int16_t   controller_y_trg = 0;
int16_t   controller_box_width;
int16_t   controller_box_width_trg;
int16_t   controller_box_y;
int16_t   controller_box_y_trg;
uint8_t   controller_num = sizeof(controller) / sizeof(SELECT_LIST);
int16_t   controller_line_y;
int16_t   controller_line_y_trg;
uint8_t   controller_value[4] = { 255, 6, 0, 0 }; //OLED亮度，动画速度，旋钮功能，选择标志

//控制页面变量
int8_t    control_ui_select = 0;
uint8_t   control_x = 4;
int16_t   control_y = 0;
int16_t   control_y_trg = 1;
int16_t   control_box_width;
int16_t   control_box_width_trg;
int16_t   control_box_y;
int16_t   control_box_y_trg;
uint8_t   control_num = sizeof(control) / sizeof(SELECT_LIST);
int16_t   control_line_y;
int16_t   control_line_y_trg;

//按键绘图页面变量
float     angle;                      //角度
float     angle_last;                 //角度
uint8_t   chart_x;                    //实时坐标
bool      frame_is_drawed = false;    //绘画标志

/************************************* 断电保存 *************************************/
#include <EEPROM.h>

uint8_t   initCheck[6] = { 'a', 'b', 'c', 'd', 'e', 'f' }; 
bool      eeprom_change = false;

//EEPROM写数据
void eeprom_write(uint8_t n, int address)
{
  for (uint8_t i = 0; i < KEYS + 1; i++ )
  {
    EEPROM.write(address + i, key[n][i]);
  }
}

//EEPROM写数据，回到主菜单时执行一遍
void eeprom_write_all_data()
{
  eeprom_write(0,  0);    //写入第一组数据，11个uint8_t数据，每个数据占1个字节，用到1个地址，共11位，地址： 0 - 10，地址初始化为0
  eeprom_write(1, 11);    //写入第二组数据，11个uint8_t数据，每个数据占1个字节，用到1个地址，共11位，地址：11 - 21，地址初始化为11
  eeprom_write(2, 22);    //写入第三组数据，11个uint8_t数据，每个数据占1个字节，用到1个地址，共11位，地址：22 - 32，地址初始化为22
  eeprom_write(3, 33);    //写入第四组数据，11个uint8_t数据，每个数据占1个字节，用到1个地址，共11位，地址：33 - 43，地址初始化为33 
  eeprom_write(4, 44);    //写入第五组数据，11个uint8_t数据，每个数据占1个字节，用到1个地址，共11位，地址：44 - 54，地址初始化为44
  eeprom_write(5, 55);    //写入第六组数据，11个uint8_t数据，每个数据占1个字节，用到1个地址，共11位，地址：55 - 65，地址初始化为55
  eeprom_write(6, 66);    //写入第七组数据，11个uint8_t数据，每个数据占1个字节，用到1个地址，共11位，地址：66 - 76，地址初始化为66

  int address;

  //写入控制器参数，3个uint8_t数据，每个数据占1个字节，用到1个地址，共2位，地址：77 - 79，地址初始化为77
  address = 77;
  for (uint8_t i = 0; i < 3; i++ ) EEPROM.write(address + i, controller_value[i]);

  //写入初始化标志位
  address = 80;
  for (uint8_t i = 0; i < 6; i++ ) EEPROM.write(address + i, initCheck[i]);

}

//EEPROM读数据
void eeprom_read(uint8_t n, int address)
{
  for (uint8_t i = 0; i < KEYS + 1; i++ )
  {
    key[n][i] = EEPROM.read(address + i);
  }
}

//EEPROM读数据，开机初始化时执行一遍
void eeprom_read_all_data()
{
  eeprom_read(0,  0);    //读出第一组数据，11个uint8_t数据，每个数据占1个字节，用到1个地址，共11位，地址： 0 - 10，地址初始化为0
  eeprom_read(1, 11);    //读出第二组数据，11个uint8_t数据，每个数据占1个字节，用到1个地址，共11位，地址：11 - 21，地址初始化为11
  eeprom_read(2, 22);    //读出第三组数据，11个uint8_t数据，每个数据占1个字节，用到1个地址，共11位，地址：22 - 32，地址初始化为22
  eeprom_read(3, 33);    //读出第四组数据，11个uint8_t数据，每个数据占1个字节，用到1个地址，共11位，地址：33 - 43，地址初始化为33 
  eeprom_read(4, 44);    //读出第五组数据，11个uint8_t数据，每个数据占1个字节，用到1个地址，共11位，地址：44 - 54，地址初始化为44
  eeprom_read(5, 55);    //读出第六组数据，11个uint8_t数据，每个数据占1个字节，用到1个地址，共11位，地址：55 - 65，地址初始化为55
  eeprom_read(6, 66);    //读出第七组数据，11个uint8_t数据，每个数据占1个字节，用到1个地址，共11位，地址：66 - 76，地址初始化为66

  //读出控制器设置
  int address = 77;
  for (uint8_t i = 0; i < 3; i++ ) controller_value[i] = EEPROM.read(address + i);
}

//开机检查是否已经修改过，没修改过则跳过读配置步骤，用默认设置
void eeprom_init()
{
  uint8_t check = 0;
  int address = 80;
  for (uint8_t i = 0; i < 6; i++ )
  {
    if (EEPROM.read(address + i) != initCheck[i]) 
    {
      check ++;
    }
  }
  if (check <= 1) eeprom_read_all_data(); //允许1位误码
}

/************************************* 动画函数 *************************************/

void movex(int16_t *a, int16_t *a_trg, uint8_t n)
{
  if (*a < *a_trg)
	{
		if (flag[n] == true)
		{
			flag[n] = false;
      step[n] *= damping[n];
		}
		*a += ceil(step[n]);
	}
	else if (*a > *a_trg)
	{
		if (flag[n] == false)
		{
			flag[n] = true;
			step[n] *= damping[n];
		}
		*a -= ceil(step[n]);
	}
  else step[n] = STEP[n];
}

//移动函数
void move(int16_t *a, int16_t *a_trg)
{
  if (*a < *a_trg) *a += ceil(fabs((float)(*a_trg - *a) / controller_value[1]));
  else if (*a > *a_trg) *a -= ceil(fabs((float)(*a_trg - *a) / controller_value[1]));
}

//消失函数
void disappear()
{
  switch (disappear_step)
  {
    case 1:
      for (uint16_t i = 0; i < buf_len; ++i)
      {
        if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] & 0x55;
      }
      break;
    case 2:
      for (uint16_t i = 0; i < buf_len; ++i)
      {
        if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] & 0xAA;
      }
      break;
    case 3:
      for (uint16_t i = 0; i < buf_len; ++i)
      {
        if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] & 0x00;
      }
      break;
    case 4:
      for (uint16_t i = 0; i < buf_len; ++i)
      {
        if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] & 0x00;
      }
      break;
    default:
      ui_state = S_NONE;
      disappear_step = 0;
      break;
  }
  disappear_step++;
}

/************************************* 显示函数 *************************************/

//列表类页面通用显示函数
void menu_ui_show
(
  struct  MENU arr[],
  int8_t  *ui_select,
  uint8_t *x,
  int16_t *y,
  int16_t *y_trg,
  int16_t *box_width,
  int16_t *box_width_trg,
  int16_t *box_y,
  int16_t *box_y_trg,
  uint8_t *num,
  int16_t *line_y,
  int16_t *line_y_trg
)
{
  *line_y_trg = ceil((*ui_select) * ((float)64 / (*num - 1)));
  *box_width_trg = u8g2.getStrWidth(arr[*ui_select].select) + 8;
  
  if (controller_value[1] == 0)
  {
    movex(y, y_trg, 0);
    movex(box_y, box_y_trg, 1);
    movex(box_width, box_width_trg, 2);
    movex(line_y, line_y_trg, 3);
    if (*line_y < 0) *line_y = 0;
  }
  else
  {
    move(y, y_trg);
    move(box_y, box_y_trg);
    move(box_width, box_width_trg);
    move(line_y, line_y_trg);
  }  

  for(uint8_t i = 0 ; i < *num ; ++i)
  {
    u8g2.drawStr(*x, 16 * i + *y + 12, arr[i].select);
  }
  u8g2.drawBox(125, 0, 3, *line_y);
  u8g2.setDrawColor(2);
  u8g2.drawRBox(0, *box_y, *box_width, 16, 1);
  u8g2.setDrawColor(1);
}

//正在编辑界面显示函数
void editing_ui_show()
{
  u8g2.setDrawColor(0);
  u8g2.drawRBox(12, 14, 104, 35, 8);
  u8g2.setDrawColor(1);
  u8g2.drawRFrame(12, 14, 104, 35, 8);
  u8g2.setCursor(25, 35);

  u8g2.drawRFrame(18, 36, 92, 8, 3);
  float width = (float)(key[edit_ui_select - 1][keys_ui_select - 1]) / (float)(key[TRAVEL][keys_ui_select - 1]) * 86;
  if (width) u8g2.drawRBox(20, 38, width + 2, 4, 1);
  
  u8g2.setCursor(22, 30);
  switch (edit_ui_select)
  {
    case 1:
      u8g2.print("Trigger");
      break;

    case 2:
      u8g2.print("Release");
      break;

    case 3:
      u8g2.print("Top");
      break;

    case 4:
      u8g2.print("Bottom");
      break;

    case 5:
      u8g2.print("Times");
      break;

    default:
      break;
  }
  u8g2.setCursor(94, 30);
  u8g2.print(key[edit_ui_select - 1][keys_ui_select - 1]);
}

//测试页面显示函数1
void chart_ui_show()
{
  if (!frame_is_drawed) //框架只画一遍
  {
    u8g2.clearBuffer();
    chart_draw_frame();
    angle_last = 10.00 + (float)analogRead(key[PINS][test_ui_select - 1]) / 100.00;
    frame_is_drawed = true;
  }

  u8g2.drawBox(96, 0, 30, 14);
  u8g2.drawVLine(chart_x + 10, 59, 3);
  if (chart_x == 100) chart_x = 0;
  u8g2.drawVLine(chart_x + 11, 24, 34);
  u8g2.drawVLine(chart_x + 12, 24, 34);
  u8g2.drawVLine(chart_x + 13, 24, 34);
  u8g2.drawVLine(chart_x + 14, 24, 34);

  //异或绘制
  u8g2.setDrawColor(2);
  angle = 10.00 + (float)analogRead(key[PINS][test_ui_select - 1]) / 100.00;
  u8g2.drawLine(chart_x + 11, 58 - (int)angle_last / 2, chart_x + 12, 58 - (int)angle / 2);
  u8g2.drawVLine(chart_x + 12, 59, 3);
  angle_last = angle;
  chart_x += 2;
  u8g2.drawBox(96, 0, 30, 14);
  u8g2.setDrawColor(1);
}

//测试页面显示函数2
void chart_draw_frame()
{
  u8g2.drawStr(34, 12, "Pin Voltage");
  u8g2.drawRBox(4, 18, 120, 46, 8);
  u8g2.setDrawColor(2);
  u8g2.drawHLine(10, 58, 108);
  u8g2.drawVLine(10, 24, 34);

  u8g2.drawPixel(7, 27);
  u8g2.drawPixel(8, 26);
  u8g2.drawPixel(9, 25);

  u8g2.drawPixel(116, 59);
  u8g2.drawPixel(115, 60);
  u8g2.drawPixel(114, 61);
  u8g2.setDrawColor(1);
}

//控制器编辑界面显示函数
void controller_editing_ui_show()
{
  uint8_t controller_max;
  if (controller_value[3] == 0) controller_max = 255;
  else if (controller_value[3] == 1) controller_max = 10;

  u8g2.setDrawColor(0);
  u8g2.drawRBox(12, 14, 104, 35, 8);
  u8g2.setDrawColor(1);
  u8g2.drawRFrame(12, 14, 104, 35, 8);
  u8g2.setCursor(25, 35);
  u8g2.drawRFrame(18, 36, 92, 8, 3);
  
  float width = (float)(controller_value[controller_value[3]]) / (float)(controller_max) * 86;
  if (width) u8g2.drawRBox(20, 38, width + 2, 4, 1);
  
  u8g2.setCursor(22, 30);
  switch (controller_ui_select)
  {
    case 1:
      u8g2.print("Brightness");
      break;

    case 2:
      u8g2.print("Speed");
      break;

    default:
      break;
  }
  if (controller_value[3] == 0) u8g2.setCursor(90, 30);
  else if (controller_value[3] == 1) u8g2.setCursor(94, 30);
  u8g2.print(controller_value[controller_value[3]]);
}

//关于本机界面
void about_ui_show()
{
  u8g2.drawStr(2, 12, "MCU : STM32F103C8T6");
  u8g2.drawStr(2, 28, "FREQ : 72 MHz");
  u8g2.drawStr(2, 44, "SRAM : 20 KB");
  u8g2.drawStr(2, 60, "FLASH : 64 KB");
}

/************************************* 处理函数 *************************************/

//列表类页面旋转时判断通用函数
void rotate_switch
(
  int8_t  *ui_select,
  int16_t *y,
  int16_t *y_trg,
  int16_t *box_y_trg,
  uint8_t *num
)
{
  switch (btn_id)
  {
    case 0:
      if (*ui_select < 1) break;
      *ui_select -= 1;
      if (*ui_select < -(*y / 16)) *y_trg += 16;
      else *box_y_trg -= 16;
      break;

    case 1:
      if ((*ui_select + 2) > *num) break;
      *ui_select += 1;
      if ((*ui_select + 1) > (4 - *y / 16)) *y_trg -= 16;
      else *box_y_trg += 16;
      break;
  }
}

//睡眠页面处理函数
void sleep_proc()
{
  while (sleep_flag)
  {
    //按键扫描
    for (int n = 0; n < KEYS; n++) runKey(n);

    //旋钮扫描
    btn_scan();
    if (btn_pressed)
    {
      btn_pressed = false;

      switch (btn_id)
      {
        case 0:
          //顺时针旋转
          if (controller_value[2] == 1)
          {  
            Consumer.press(HIDConsumer::VOLUME_UP);
            Consumer.release();               
          }
          else if (controller_value[2] == 2)
          {
            Consumer.press(HIDConsumer::BRIGHTNESS_UP);
            Consumer.release();
          }
          break;

        case 1:
          //逆时针旋转
          if (controller_value[2] == 1)
          { 
            Consumer.press(HIDConsumer::VOLUME_DOWN);
            Consumer.release();   
          }
          else if (controller_value[2] == 2)
          {
            Consumer.press(HIDConsumer::BRIGHTNESS_DOWN);
            Consumer.release();
          }
          break;  

        case 2:
          ui_index = M_LIST;
          ui_state = S_NONE;
          sleep_flag = false;
          u8g2.setPowerSave(0);       
          break;
      }
    }    
  }
}

//主菜单处理函数
void list_proc()
{
  if (btn_pressed)
  {
    btn_pressed = false;
    switch (btn_id)
    {
      case 0:
      case 1:
        rotate_switch
        (
          & list_ui_select,
          & list_y,
          & list_y_trg,
          & list_box_y_trg,
          & list_num
        );
        break;      

      case 2:
        switch (list_ui_select)
        {
          case 0: 
            ui_index = M_SLEEP;
            ui_state = S_NONE;
            u8g2.setPowerSave(1);
            key_init();
            if (eeprom_change == true)
            {
              eeprom_write_all_data();
              eeprom_change == false;
            }
            sleep_flag = true;
            break;

          case 1:
            ui_index = M_KEYS;
            ui_state = S_DISAPPEAR;
            break;

          case 2:
            ui_index = M_TEST;
            ui_state = S_DISAPPEAR;
            break;
          
          case 3:
            ui_index = M_CONTROLLER;
            ui_state = S_DISAPPEAR;
            break;

          default:
            break;
        }
      default:
        break;
    }
    list_box_width_trg = u8g2.getStrWidth(list[list_ui_select].select) + list_x * 2;
  }
  menu_ui_show
  (
    list,
    & list_ui_select,
    & list_x,
    & list_y,
    & list_y_trg,
    & list_box_width,
    & list_box_width_trg,
    & list_box_y,
    & list_box_y_trg,
    & list_num,
    & list_line_y,
    & list_line_y_trg
  );
}

void keys_proc()
{
  if (btn_pressed)
  {
    btn_pressed = false;
    switch (btn_id)
    {
      case 0:
      case 1:
        rotate_switch
        (
          & keys_ui_select,
          & keys_y,
          & keys_y_trg,
          & keys_box_y_trg,
          & keys_num
        );
        break;      

      case 2:
        switch (keys_ui_select)
        {
          case 0:
            ui_index = M_LIST;
            ui_state = S_DISAPPEAR;
            break;

          case 1:
          case 2:
          case 3:
          case 4:
          case 5:
          case 6:
          case 7:
          case 8:
          case 9:
          case 10:
          case 11:
            ui_state = S_DISAPPEAR;
            ui_index = M_EDIT;
            break;

          default:
            break;
        }
      default:
        break;
    }
    keys_box_width_trg = u8g2.getStrWidth(keys[keys_ui_select].select) + keys_x * 2;
  }
  menu_ui_show
  (
    keys,
    & keys_ui_select,
    & keys_x,
    & keys_y,
    & keys_y_trg,
    & keys_box_width,
    & keys_box_width_trg,
    & keys_box_y,
    & keys_box_y_trg,
    & keys_num,
    & keys_line_y,
    & keys_line_y_trg
  );
}

//按键编辑页面处理函数
void edit_proc()
{
  if (btn_pressed)
  {
    btn_pressed = false;
    switch (btn_id)
    {
      case 0:
      case 1:
        rotate_switch
        (
          & edit_ui_select,
          & edit_y,
          & edit_y_trg,
          & edit_box_y_trg,
          & edit_num
        );
        break;      

      case 2:
        switch (edit_ui_select)
        {
          case 0:
            ui_index = M_KEYS;
            ui_state = S_DISAPPEAR;
            break;

          case 1: 
          case 2: 
          case 3: 
          case 4: 
          case 5:
            ui_index = M_EDITING;
            ui_state = S_NONE;
            break;

          case 6:
            ui_index = M_CODE;
            ui_state = S_DISAPPEAR;
            break;

          case 7: 
            ui_index = M_MODE;
            ui_state = S_DISAPPEAR;
            break;  

          default:
            break;
        }
      
      default:
        break;
    }
    edit_box_width_trg = u8g2.getStrWidth(edit[edit_ui_select].select) + edit_x * 2;
  }
  menu_ui_show
  (
    edit,
    & edit_ui_select,
    & edit_x,
    & edit_y,
    & edit_y_trg,
    & edit_box_width,
    & edit_box_width_trg,
    & edit_box_y,
    & edit_box_y_trg,
    & edit_num,
    & edit_line_y,
    & edit_line_y_trg
  );
}

//正在编辑页面处理函数
void editing_proc()
{
  if (btn_pressed)
  {
    btn_pressed = false;
    switch (btn_id)
    {
      case 0:
        if (key[edit_ui_select - 1][keys_ui_select - 1] < key[TRAVEL][keys_ui_select - 1] )
        {
          key[edit_ui_select - 1][keys_ui_select - 1] += 1;
          eeprom_change = true;
        } 
        break;

      case 1:
        if (key[edit_ui_select - 1][keys_ui_select - 1] > 0)
        {
          key[edit_ui_select - 1][keys_ui_select - 1] -= 1;
          eeprom_change = true;
        }
        break;  

      case 2:
        ui_index = M_EDIT;
        break;
      default:
        break;
    }

    if (keys_ui_select == keys_num - 1)
    {
      for (int i = 0 ; i < KEYS + 1 ; i++)
      {
        key[edit_ui_select - 1][i] = key[edit_ui_select - 1][keys_ui_select - 1];
      }
    }
  }
  menu_ui_show
  (
    edit,
    & edit_ui_select,
    & edit_x,
    & edit_y,
    & edit_y_trg,
    & edit_box_width,
    & edit_box_width_trg,
    & edit_box_y,
    & edit_box_y_trg,
    & edit_num,
    & edit_line_y,
    & edit_line_y_trg
  );
  for (uint16_t i = 0; i < buf_len; ++i)
  {
    buf_ptr[i] = buf_ptr[i] & (i % 2 == 0 ? 0x55 : 0xAA);
  }
  editing_ui_show();
}

//键值种类选择页面处理函数
void code_proc(void)
{
  if (btn_pressed)
  {
    btn_pressed = false;
    switch (btn_id)
    {
      case 0:
      case 1:
        rotate_switch
        (
          & code_ui_select,
          & code_y,
          & code_y_trg,
          & code_box_y_trg,
          & code_num
        );
        break;      

      case 2:
        switch (code_ui_select)
        {
          case 0: 
            ui_index = M_EDIT;
            ui_state = S_DISAPPEAR;
            break;

          case 1:
            ui_index = M_ALPH;
            ui_state = S_DISAPPEAR;
            break;

          case 2:
            ui_index = M_NUMB;
            ui_state = S_DISAPPEAR;
            break;

          case 3:
            ui_index = M_FUNC;
            ui_state = S_DISAPPEAR;
            break;
          
          case 4:
            ui_index = M_MORE;
            ui_state = S_DISAPPEAR;
            break;

          default:
            break;
        }
      default:
        break;
    }
    code_box_width_trg = u8g2.getStrWidth(code[code_ui_select].select) + code_x * 2;
  }
  menu_ui_show
  (
    code,
    & code_ui_select,
    & code_x,
    & code_y,
    & code_y_trg,
    & code_box_width,
    & code_box_width_trg,
    & code_box_y,
    & code_box_y_trg,
    & code_num,
    & code_line_y,
    & code_line_y_trg
  );
}

//字母按键选择页面处理函数
void alph_proc()
{
  if (btn_pressed)
  {
    btn_pressed = false;
    switch (btn_id)
    {
      case 0:
      case 1:
        rotate_switch
        (
          & alph_ui_select,
          & alph_y,
          & alph_y_trg,
          & alph_box_y_trg,
          & alph_num
        );
        break;      

      case 2:
        switch (alph_ui_select)
        {
          case 0: ui_index = M_CODE; break;

          case 1:  key[CODES][keys_ui_select - 1] = 'a'; ui_index = M_ALPH; eeprom_change = true; break;
          case 2:  key[CODES][keys_ui_select - 1] = 'b'; ui_index = M_ALPH; eeprom_change = true; break;
          case 3:  key[CODES][keys_ui_select - 1] = 'c'; ui_index = M_ALPH; eeprom_change = true; break;
          case 4:  key[CODES][keys_ui_select - 1] = 'd'; ui_index = M_ALPH; eeprom_change = true; break;
          case 5:  key[CODES][keys_ui_select - 1] = 'e'; ui_index = M_ALPH; eeprom_change = true; break;
          case 6:  key[CODES][keys_ui_select - 1] = 'f'; ui_index = M_ALPH; eeprom_change = true; break;
          case 7:  key[CODES][keys_ui_select - 1] = 'g'; ui_index = M_ALPH; eeprom_change = true; break;
          case 8:  key[CODES][keys_ui_select - 1] = 'h'; ui_index = M_ALPH; eeprom_change = true; break;
          case 9:  key[CODES][keys_ui_select - 1] = 'i'; ui_index = M_ALPH; eeprom_change = true; break;
          case 10: key[CODES][keys_ui_select - 1] = 'j'; ui_index = M_ALPH; eeprom_change = true; break;
          case 11: key[CODES][keys_ui_select - 1] = 'k'; ui_index = M_ALPH; eeprom_change = true; break;
          case 12: key[CODES][keys_ui_select - 1] = 'l'; ui_index = M_ALPH; eeprom_change = true; break;
          case 13: key[CODES][keys_ui_select - 1] = 'm'; ui_index = M_ALPH; eeprom_change = true; break;
          case 14: key[CODES][keys_ui_select - 1] = 'n'; ui_index = M_ALPH; eeprom_change = true; break;
          case 15: key[CODES][keys_ui_select - 1] = 'o'; ui_index = M_ALPH; eeprom_change = true; break;
          case 16: key[CODES][keys_ui_select - 1] = 'p'; ui_index = M_ALPH; eeprom_change = true; break;
          case 17: key[CODES][keys_ui_select - 1] = 'q'; ui_index = M_ALPH; eeprom_change = true; break;
          case 18: key[CODES][keys_ui_select - 1] = 'r'; ui_index = M_ALPH; eeprom_change = true; break;
          case 19: key[CODES][keys_ui_select - 1] = 's'; ui_index = M_ALPH; eeprom_change = true; break;
          case 20: key[CODES][keys_ui_select - 1] = 't'; ui_index = M_ALPH; eeprom_change = true; break;
          case 21: key[CODES][keys_ui_select - 1] = 'u'; ui_index = M_ALPH; eeprom_change = true; break;
          case 22: key[CODES][keys_ui_select - 1] = 'v'; ui_index = M_ALPH; eeprom_change = true; break;
          case 23: key[CODES][keys_ui_select - 1] = 'w'; ui_index = M_ALPH; eeprom_change = true; break;
          case 24: key[CODES][keys_ui_select - 1] = 'x'; ui_index = M_ALPH; eeprom_change = true; break;
          case 25: key[CODES][keys_ui_select - 1] = 'y'; ui_index = M_ALPH; eeprom_change = true; break;
          case 26: key[CODES][keys_ui_select - 1] = 'z'; ui_index = M_ALPH; eeprom_change = true; break;

          default: break;
        }
        ui_state = S_DISAPPEAR;

      if (keys_ui_select == keys_num - 1)
      {
        for (int i = 0 ; i < KEYS + 1 ; i++)
        {
          key[CODES][i] = key[CODES][keys_ui_select - 1];
        }
      }

      default:
        break;
    }
    alph_box_width_trg = u8g2.getStrWidth(alph[alph_ui_select].select) + alph_x * 2;
  }
  menu_ui_show
  (
    alph,
    & alph_ui_select,
    & alph_x,
    & alph_y,
    & alph_y_trg,
    & alph_box_width,
    & alph_box_width_trg,
    & alph_box_y,
    & alph_box_y_trg,
    & alph_num,
    & alph_line_y,
    & alph_line_y_trg
  );
}

//数字按键选择页面处理函数
void numb_proc()
{
  if (btn_pressed)
  {
    btn_pressed = false;
    switch (btn_id)
    {
      case 0:
      case 1:
        rotate_switch
        (
          & numb_ui_select,
          & numb_y,
          & numb_y_trg,
          & numb_box_y_trg,
          & numb_num
        );
        break;      

      case 2:
        switch (numb_ui_select)
        {
          case 0: ui_index = M_CODE; break;

          case 1:  key[CODES][keys_ui_select - 1] = 0; ui_index = M_NUMB; eeprom_change = true; break;
          case 2:  key[CODES][keys_ui_select - 1] = 1; ui_index = M_NUMB; eeprom_change = true; break;
          case 3:  key[CODES][keys_ui_select - 1] = 2; ui_index = M_NUMB; eeprom_change = true; break;
          case 4:  key[CODES][keys_ui_select - 1] = 3; ui_index = M_NUMB; eeprom_change = true; break;
          case 5:  key[CODES][keys_ui_select - 1] = 4; ui_index = M_NUMB; eeprom_change = true; break;
          case 6:  key[CODES][keys_ui_select - 1] = 5; ui_index = M_NUMB; eeprom_change = true; break;
          case 7:  key[CODES][keys_ui_select - 1] = 6; ui_index = M_NUMB; eeprom_change = true; break;
          case 8:  key[CODES][keys_ui_select - 1] = 7; ui_index = M_NUMB; eeprom_change = true; break;
          case 9:  key[CODES][keys_ui_select - 1] = 8; ui_index = M_NUMB; eeprom_change = true; break;
          case 10: key[CODES][keys_ui_select - 1] = 9; ui_index = M_NUMB; eeprom_change = true; break;
          
          default: break;
        }
        ui_state = S_DISAPPEAR;

      if (keys_ui_select == keys_num - 1)
      {
        for (int i = 0 ; i < KEYS + 1 ; i++)
        {
          key[CODES][i] = key[CODES][keys_ui_select - 1];
        }
      }

      default:
        break;
    }
    numb_box_width_trg = u8g2.getStrWidth(numb[numb_ui_select].select) + numb_x * 2;
  }
  menu_ui_show
  (
    numb,
    & numb_ui_select,
    & numb_x,
    & numb_y,
    & numb_y_trg,
    & numb_box_width,
    & numb_box_width_trg,
    & numb_box_y,
    & numb_box_y_trg,
    & numb_num,
    & numb_line_y,
    & numb_line_y_trg
  );
}

//功能按键选择页面处理函数
void func_proc()
{
  if (btn_pressed)
  {
    btn_pressed = false;
    switch (btn_id)
    {
      case 0:
      case 1:
        rotate_switch
        (
          & func_ui_select,
          & func_y,
          & func_y_trg,
          & func_box_y_trg,
          & func_num
        );
        break;    

      case 2:
        switch (func_ui_select)
        {
          case 0: ui_index = M_CODE; break;

          case 1:  key[CODES][keys_ui_select - 1] = KEY_F1;  ui_index = M_FUNC; eeprom_change = true; break;
          case 2:  key[CODES][keys_ui_select - 1] = KEY_F2;  ui_index = M_FUNC; eeprom_change = true; break;
          case 3:  key[CODES][keys_ui_select - 1] = KEY_F3;  ui_index = M_FUNC; eeprom_change = true; break;
          case 4:  key[CODES][keys_ui_select - 1] = KEY_F4;  ui_index = M_FUNC; eeprom_change = true; break;
          case 5:  key[CODES][keys_ui_select - 1] = KEY_F5;  ui_index = M_FUNC; eeprom_change = true; break;
          case 6:  key[CODES][keys_ui_select - 1] = KEY_F6;  ui_index = M_FUNC; eeprom_change = true; break;
          case 7:  key[CODES][keys_ui_select - 1] = KEY_F7;  ui_index = M_FUNC; eeprom_change = true; break;
          case 8:  key[CODES][keys_ui_select - 1] = KEY_F8;  ui_index = M_FUNC; eeprom_change = true; break;
          case 9:  key[CODES][keys_ui_select - 1] = KEY_F9;  ui_index = M_FUNC; eeprom_change = true; break;
          case 10: key[CODES][keys_ui_select - 1] = KEY_F10; ui_index = M_FUNC; eeprom_change = true; break;
          case 11: key[CODES][keys_ui_select - 1] = KEY_F11; ui_index = M_FUNC; eeprom_change = true; break;
          case 12: key[CODES][keys_ui_select - 1] = KEY_F12; ui_index = M_FUNC; eeprom_change = true; break;

          default: break;
        }
        ui_state = S_DISAPPEAR;

      if (keys_ui_select == keys_num - 1)
      {
        for (int i = 0 ; i < KEYS + 1 ; i++)
        {
          key[CODES][i] = key[CODES][keys_ui_select - 1];
        }
      }

      default:
        break;
    }
    func_box_width_trg = u8g2.getStrWidth(func[func_ui_select].select) + func_x * 2;
  }
  menu_ui_show
  (
    func,
    & func_ui_select,
    & func_x,
    & func_y,
    & func_y_trg,
    & func_box_width,
    & func_box_width_trg,
    & func_box_y,
    & func_box_y_trg,
    & func_num,
    & func_line_y,
    & func_line_y_trg
  );
}

//其它按键选择页面处理函数
void more_proc()
{
  if (btn_pressed)
  {
    btn_pressed = false;
    switch (btn_id)
    {
      case 0:
      case 1:
        rotate_switch
        (
          & more_ui_select,
          & more_y,
          & more_y_trg,
          & more_box_y_trg,
          & more_num
        );
        break;    

      case 2:
        switch (more_ui_select)
        {
          case 0: ui_index = M_CODE; break;

          case 1:  key[CODES][keys_ui_select - 1] = KEY_LEFT_CTRL;     ui_index = M_MORE; eeprom_change = true; break;
          case 2:  key[CODES][keys_ui_select - 1] = KEY_RIGHT_CTRL;    ui_index = M_MORE; eeprom_change = true; break;
          case 3:  key[CODES][keys_ui_select - 1] = KEY_LEFT_SHIFT;    ui_index = M_MORE; eeprom_change = true; break;
          case 4:  key[CODES][keys_ui_select - 1] = KEY_RIGHT_SHIFT;   ui_index = M_MORE; eeprom_change = true; break;
          case 5:  key[CODES][keys_ui_select - 1] = KEY_LEFT_ALT;      ui_index = M_MORE; eeprom_change = true; break;
          case 6:  key[CODES][keys_ui_select - 1] = KEY_RIGHT_ALT;     ui_index = M_MORE; eeprom_change = true; break;
          case 7:  key[CODES][keys_ui_select - 1] = KEY_LEFT_GUI;      ui_index = M_MORE; eeprom_change = true; break;
          case 8:  key[CODES][keys_ui_select - 1] = KEY_RIGHT_GUI;     ui_index = M_MORE; eeprom_change = true; break;
          case 9:  key[CODES][keys_ui_select - 1] = KEY_UP_ARROW;      ui_index = M_MORE; eeprom_change = true; break;
          case 10: key[CODES][keys_ui_select - 1] = KEY_DOWN_ARROW;    ui_index = M_MORE; eeprom_change = true; break;
          case 11: key[CODES][keys_ui_select - 1] = KEY_LEFT_ARROW;    ui_index = M_MORE; eeprom_change = true; break;
          case 12: key[CODES][keys_ui_select - 1] = KEY_RIGHT_ARROW;   ui_index = M_MORE; eeprom_change = true; break;
          case 13: key[CODES][keys_ui_select - 1] = KEY_BACKSPACE;     ui_index = M_MORE; eeprom_change = true; break;
          case 14: key[CODES][keys_ui_select - 1] = KEY_TAB;           ui_index = M_MORE; eeprom_change = true; break;
          case 15: key[CODES][keys_ui_select - 1] = KEY_RETURN;        ui_index = M_MORE; eeprom_change = true; break;
          case 16: key[CODES][keys_ui_select - 1] = KEY_ESC;           ui_index = M_MORE; eeprom_change = true; break;
          case 17: key[CODES][keys_ui_select - 1] = KEY_INSERT;        ui_index = M_MORE; eeprom_change = true; break;
          case 18: key[CODES][keys_ui_select - 1] = KEY_DELETE;        ui_index = M_MORE; eeprom_change = true; break;
          case 19: key[CODES][keys_ui_select - 1] = KEY_PAGE_UP;       ui_index = M_MORE; eeprom_change = true; break;
          case 21: key[CODES][keys_ui_select - 1] = KEY_PAGE_DOWN;     ui_index = M_MORE; eeprom_change = true; break;
          case 22: key[CODES][keys_ui_select - 1] = KEY_HOME;          ui_index = M_MORE; eeprom_change = true; break;
          case 23: key[CODES][keys_ui_select - 1] = KEY_END;           ui_index = M_MORE; eeprom_change = true; break;
          case 24: key[CODES][keys_ui_select - 1] = KEY_CAPS_LOCK;     ui_index = M_MORE; eeprom_change = true; break;

          default: break;
        }
        ui_state = S_DISAPPEAR;

      if (keys_ui_select == keys_num - 1)
      {
        for (int i = 0 ; i < KEYS + 1 ; i++)
        {
          key[CODES][i] = key[CODES][keys_ui_select - 1];
        }
      }

      default:
        break;
    }
    more_box_width_trg = u8g2.getStrWidth(more[more_ui_select].select) + more_x * 2;
  }
  menu_ui_show
  (
    more,
    & more_ui_select,
    & more_x,
    & more_y,
    & more_y_trg,
    & more_box_width,
    & more_box_width_trg,
    & more_box_y,
    & more_box_y_trg,
    & more_num,
    & more_line_y,
    & more_line_y_trg
  );
}

//模式选择页面处理函数
void mode_proc(void)
{
  if (btn_pressed)
  {
    btn_pressed = false;
    switch (btn_id)
    {
      case 0:
      case 1:
        rotate_switch
        (
          & mode_ui_select,
          & mode_y,
          & mode_y_trg,
          & mode_box_y_trg,
          & mode_num
        );
        break;    

      case 2:
        switch (mode_ui_select)
        {
          case 0: ui_index = M_EDIT; break;
          
          case 1: key[MODES][keys_ui_select - 1] = 1; ui_index = M_MODE; eeprom_change = true; break;
          case 2: key[MODES][keys_ui_select - 1] = 2; ui_index = M_MODE; eeprom_change = true; break;
          case 3: key[MODES][keys_ui_select - 1] = 0; ui_index = M_MODE; eeprom_change = true; break;
          
          default:
            break;
        }
        ui_state = S_DISAPPEAR;

      if (keys_ui_select == keys_num - 1)
      {
        for (int i = 0 ; i < KEYS + 1 ; i++)
        {
          key[MODES][i] = key[MODES][keys_ui_select - 1];
        }
      }      

      default:
        break;
    }
    mode_box_width_trg = u8g2.getStrWidth(mode[mode_ui_select].select) + mode_x * 2;
  }
  menu_ui_show
  (
    mode,
    & mode_ui_select,
    & mode_x,
    & mode_y,
    & mode_y_trg,
    & mode_box_width,
    & mode_box_width_trg,
    & mode_box_y,
    & mode_box_y_trg,
    & mode_num,
    & mode_line_y,
    & mode_line_y_trg
  );
}

//按键测试页面处理函数
void test_proc()
{
  if (btn_pressed)
  {
    btn_pressed = false;
    switch (btn_id)
    {
      case 0:
      case 1:
        rotate_switch
        (
          & test_ui_select,
          & test_y,
          & test_y_trg,
          & test_box_y_trg,
          & test_num
        );
        break; 

      case 2:
        switch (test_ui_select)
        {
          case 0:
            ui_index = M_LIST;
            ui_state = S_DISAPPEAR;
            break;

          case 1:
          case 2:
          case 3:
          case 4:
          case 5:
          case 6:
          case 7:
          case 8:
          case 9:
          case 10:
            ui_index = M_CHART;
            ui_state = S_DISAPPEAR;
            break;

          default:
            break;
        }
      default:
        break;
    }
    test_box_width_trg = u8g2.getStrWidth(test[test_ui_select].select) + test_x * 2;
  }
  menu_ui_show
  (
    test,
    & test_ui_select,
    & test_x,
    & test_y,
    & test_y_trg,
    & test_box_width,
    & test_box_width_trg,
    & test_box_y,
    & test_box_y_trg,
    & test_num,
    & test_line_y,
    & test_line_y_trg
  );
}

//按键测试选择页面处理函数
void chart_proc()
{
  chart_ui_show();
  if (btn_pressed)
  {
    btn_pressed = false;
    if (btn_id == 2)
    {
      ui_index = M_TEST;
      ui_state = S_DISAPPEAR;
      frame_is_drawed = false; //退出后框架为未画状态
      chart_x = 0;
    }
  }
}

//键值种类选择页面处理函数
void controller_proc(void)
{
  if (btn_pressed)
  {
    btn_pressed = false;
    switch (btn_id)
    {
      case 0:
      case 1:
        rotate_switch
        (
          & controller_ui_select,
          & controller_y,
          & controller_y_trg,
          & controller_box_y_trg,
          & controller_num
        );
        break;      

      case 2:
        switch (controller_ui_select)
        {
          case 0: 
            ui_index = M_LIST;
            ui_state = S_DISAPPEAR;
            break;

          case 1:
            ui_index = M_CONTROLLER_EDITING;
            ui_state = S_NONE;
            controller_value[3] = 0;
            eeprom_change = true;
            break;

          case 2:
            ui_index = M_CONTROLLER_EDITING;
            ui_state = S_NONE;
            controller_value[3] = 1;
            eeprom_change = true;
            break;

          case 3:
            ui_index = M_CONTROL;
            ui_state = S_DISAPPEAR;
            break;

          case 4:
            ui_index = M_ABOUT;
            ui_state = S_DISAPPEAR;
            break;

          default:
            break;
        }
      default:
        break;
    }
    controller_box_width_trg = u8g2.getStrWidth(controller[controller_ui_select].select) + controller_x * 2;
  }
  menu_ui_show
  (
    controller,
    & controller_ui_select,
    & controller_x,
    & controller_y,
    & controller_y_trg,
    & controller_box_width,
    & controller_box_width_trg,
    & controller_box_y,
    & controller_box_y_trg,
    & controller_num,
    & controller_line_y,
    & controller_line_y_trg
  );
}

//正在编辑页面处理函数
void controller_editing_proc()
{
  uint8_t controller_max;
  if (controller_value[3] == 0) controller_max = 255;
  else if (controller_value[3] == 1) controller_max = 10;

  if (btn_pressed)
  {
    btn_pressed = false;
    switch (btn_id)
    {
      case 0:
        if (controller_value[controller_value[3]] < controller_max )
        {
          controller_value[controller_value[3]] += 1;
          eeprom_change = true;
        } 
        break;

      case 1:
        if (controller_value[controller_value[3]] > 0)
        {
          controller_value[controller_value[3]] -= 1;
          eeprom_change = true;
        } 
        break;  

      case 2:
        ui_index = M_CONTROLLER;
        break;

      default:
        break;
    }
    if (controller_value[3] == 0) u8g2.setContrast(controller_value[0]);
  }
  menu_ui_show
  (
    controller,
    & controller_ui_select,
    & controller_x,
    & controller_y,
    & controller_y_trg,
    & controller_box_width,
    & controller_box_width_trg,
    & controller_box_y,
    & controller_box_y_trg,
    & controller_num,
    & controller_line_y,
    & controller_line_y_trg
  );
  for (uint16_t i = 0; i < buf_len; ++i)
  {
    buf_ptr[i] = buf_ptr[i] & (i % 2 == 0 ? 0x55 : 0xAA);
  }
  controller_editing_ui_show();
}

//控制菜单处理函数
void control_proc()
{
  if (btn_pressed)
  {
    btn_pressed = false;
    switch (btn_id)
    {
      case 0:
      case 1:
        rotate_switch
        (
          & control_ui_select,
          & control_y,
          & control_y_trg,
          & control_box_y_trg,
          & control_num
        );
        break;      

      case 2:
        switch (control_ui_select)
        {
          case 0: 
            ui_index = M_CONTROLLER;
            ui_state = S_DISAPPEAR;
            break;

          case 1:
            ui_index = M_CONTROL;
            ui_state = S_DISAPPEAR;
            controller_value[2] = 1;
            break;

          case 2:
            ui_index = M_CONTROL;
            ui_state = S_DISAPPEAR;
            controller_value[2] = 2;
            break;

          case 3:
            ui_index = M_CONTROL;
            ui_state = S_DISAPPEAR;
            controller_value[2] = 0;
            break;

          default:
            break;
        }
      default:
        break;
    }
    control_box_width_trg = u8g2.getStrWidth(control[control_ui_select].select) + control_x * 2;
  }
  menu_ui_show
  (
    control,
    & control_ui_select,
    & control_x,
    & control_y,
    & control_y_trg,
    & control_box_width,
    & control_box_width_trg,
    & control_box_y,
    & control_box_y_trg,
    & control_num,
    & control_line_y,
    & control_line_y_trg
  );
}

//关于本机页面处理函数
void about_proc()
{
  if (btn_pressed)
  {
    btn_pressed = false;
    ui_index = M_CONTROLLER;
    ui_state = S_DISAPPEAR;
  }
  about_ui_show();
}

//总的UI进程
void ui_proc()
{
  switch (ui_state)
  {
    case S_NONE:
      if (ui_index != M_CHART) u8g2.clearBuffer();
      switch (ui_index)
      {
        case M_SLEEP:
          sleep_proc();
          break;
        
        case M_LIST:
          list_proc();
          break;
        
        case M_KEYS:
          keys_proc();
          break;

        case M_EDIT:
          edit_proc();
          break;

        case M_EDITING:
          editing_proc();
          break;

        case M_CODE:
          code_proc();
          break;

        case M_ALPH:
          alph_proc();
          break;

        case M_NUMB:
          numb_proc();
          break;

        case M_FUNC:
          func_proc();
          break;

        case M_MORE:
          more_proc();
          break;

        case M_MODE:
          mode_proc();
          break;

        case M_TEST:
          test_proc();
          break;

        case M_CHART:
          chart_proc();
          break;

        case M_CONTROLLER:
          controller_proc();
          break;

        case M_CONTROLLER_EDITING:
          controller_editing_proc();
          break;

        case M_CONTROL:
          control_proc();
          break;

        case M_ABOUT:
          about_proc();
          break;

        default:
          break;
      }
      break;
    case S_DISAPPEAR:
      disappear();
      break;
    default:
      break;
  }
  u8g2.sendBuffer();
}

//OLED初始化函数
void oled_init()
{
  u8g2.setBusClock(800000);
  u8g2.begin();
  u8g2.setFont(u8g2_font_wqy12_t_chinese1);
  u8g2.setContrast(controller_value[0]);

  buf_ptr = u8g2.getBufferPtr(); //拿到buffer首地址
  buf_len = 8 * u8g2.getBufferTileHeight() * u8g2.getBufferTileWidth();
}

//ui初始化
void ui_init()
{
  list_box_width = list_box_width_trg = u8g2.getStrWidth(list[list_ui_select].select) + list_x * 2; //两边各多2
  keys_box_width = keys_box_width_trg = u8g2.getStrWidth(keys[keys_ui_select].select) + keys_x * 2; //两边各多2
  edit_box_width = edit_box_width_trg = u8g2.getStrWidth(edit[edit_ui_select].select) + edit_x * 2; //两边各多2
  code_box_width = code_box_width_trg = u8g2.getStrWidth(code[code_ui_select].select) + code_x * 2; //两边各多2
  alph_box_width = alph_box_width_trg = u8g2.getStrWidth(alph[alph_ui_select].select) + alph_x * 2; //两边各多2
  numb_box_width = numb_box_width_trg = u8g2.getStrWidth(numb[numb_ui_select].select) + numb_x * 2; //两边各多2
  func_box_width = func_box_width_trg = u8g2.getStrWidth(func[func_ui_select].select) + func_x * 2; //两边各多2
  more_box_width = more_box_width_trg = u8g2.getStrWidth(more[more_ui_select].select) + more_x * 2; //两边各多2
  mode_box_width = mode_box_width_trg = u8g2.getStrWidth(mode[mode_ui_select].select) + mode_x * 2; //两边各多2
  test_box_width = test_box_width_trg = u8g2.getStrWidth(test[test_ui_select].select) + test_x * 2; //两边各多2
  controller_box_width = controller_box_width_trg = u8g2.getStrWidth(controller[controller_ui_select].select) + controller_x * 2; //两边各多2
  control_box_width = control_box_width_trg = u8g2.getStrWidth(control[control_ui_select].select) + control_x * 2; //两边各多2

  ui_index = M_SLEEP;
  ui_state = S_NONE;
}

void setup() 
{
  hid_init();
  key_init();
  eeprom_init();
  oled_init();
  ui_init();
  btn_init();
}

void loop() 
{
  btn_scan();
  ui_proc();
}


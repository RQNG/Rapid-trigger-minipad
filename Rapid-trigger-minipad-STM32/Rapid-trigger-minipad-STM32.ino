/*
  此程序用于实现wooting键盘中的Rapid trigger功能，移植并优化自Github上的一个开源项目，添加了按键的独立参数和两端的死区，优化了初始化方式。

  当前版本：v1.2，测试用开发板为STM32 F103C8T6，上传程序前请确认以下设置是正确的：

    硬件：

    板子上的跳线帽设置：BOOT0 -> 1 ; BOOT1 -> 0 
    串口模块与板子连线：GND -> GND ; 3V3 -> 3.3V ; RX -> PA9 ; TX -> PA10，其余引脚空着。
    串口与电脑连接，用数据线连接板子上的母口和电脑。
    每次修改参数后都需要点击IDE左上角向右箭头的按钮，等待编译和上传完成。
    上传完成后如要测试手感，无需断开串口模块，但需要拔插数据线，电脑才能识别到是USB设备。
    当BOOT在第一条所述设置时，上传完成后，第一次拔插数据线时电脑会认到USB设备，从第二次开始就认不到了。
    每次初始化时，需要等待几秒按下按键，每个按键都需要按下一次后才能正常使用，按下时要按到底。
    手感调试合适后修改跳线帽的设置为：BOOT0 -> 0 ; BOOT1 -> 0。设置后就能收起串口模块了。
  
    软件：
    
    文件 -> 首选项 -> 其他开发板管理器地址 -> 添加一行：http://dan.drown.org/stm32duino/package_STM32duino_index.json
    工具 -> 开发板 -> 开发板管理器 -> 搜索stm32F1，下载STM32F1xx开头的库
    工具 -> 开发板 -> stm32F1... -> Generic STM32F103C series
    工具 -> 端口：“COMX” -> COMX（X为设备对应端口号）
    工具 -> Upload method -> Serial
    进入目录C:\Users{username}\AppData\Local\Arduino15\packages\stm32duino\hardware\STM32F1{版本名称}\libraries\USBComposite下，打开usb_hid.c文件，搜索bInterval，将该值修改为0x01，即轮询间隔修改为1ms

  为提高易读性，此程序并未严格遵守编写规范，请在开头处“需要自定义的参数”中更改和添加相关量以适应手感和实现功能，若不想了解运行原理，则不必理会下面所有的参数和函数。

    设置参考：https://github.com/rogerclarkmelbourne/Arduino_STM32 ; https://baijiahao.baidu.com/s?id=1727012582049990775&wfr=spider&for=pc ; https://blog.csdn.net/oChiTu1/article/details/108166233?depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-2-108166233-blog-119909968.pc_relevant_3mothn_strategy_and_data_recovery

    项目参考：https://github.com/chent7/hall-2k-keypad-handmade ; https://mp.weixin.qq.com/s/o-8_3SQS2AGT_WTyTLYQnA

  本项目使用Apache 2.0开源协议，如需商用或借鉴，请阅读此协议。
  
  欢迎关注我的B站账号，一个只分享osu!相关内容，很无聊的帐号。
  
  用户名：音游玩的人，B站主页：https://space.bilibili.com/9182439?spm_id_from=..0.0
*/

/***************** 需要查看和自定义的参数 *****************/

// 除字母以外的键值，仅查看，请勿修改
#define LEFT_CTRL		  0x80
#define RIGHT_CTRL		0x84

#define LEFT_SHIFT		0x81
#define RIGHT_SHIFT		0x85

#define LEFT_ALT	  	0x82
#define RIGHT_ALT		  0x86

#define LEFT_GUI		  0x83
#define RIGHT_GUI		  0x87

#define UP_ARROW		  0xDA
#define DOWN_ARROW		0xD9
#define LEFT_ARROW		0xD8
#define RIGHT_ARROW		0xD7

#define BACKSPACE		  0xB2
#define TAB				    0xB3
#define RETURN			  0xB0
#define ESC				    0xB1
#define INSERT			  0xD1
#define DELETE			  0xD4
#define PAGE_UP			  0xD3
#define PAGE_DOWN		  0xD6
#define HOME			    0xD2
#define END				    0xD5
#define CAPS_LOCK		  0xC1

#define F1				    0xC2
#define F2				    0xC3
#define F3				    0xC4
#define F4				    0xC5
#define F5				    0xC6
#define F6				    0xC7
#define F7				    0xC8
#define F8				    0xC9
#define F9				    0xCA
#define F10				    0xCB
#define F11				    0xCC
#define F12				    0xCD

//需要自定义的参数，除第一个外，每一列对应一个按键
#define         KEYS                           4                                          // 按键数
const int       keyPins[KEYS]    = {         PA0,        PA1,        PA2,        PA3 };   // 引脚号，只能使用有ADC功能的引脚，有10个，分别是：PA0 - PA7，PB0，PB1，如有疑问，看设置参考的链接
const uint8_t   keyCodes[KEYS]   = {         'z',        'x',        ESC,   LEFT_GUI };   // 按键值，字母要用单引号括住键值
const float     trigger_mm[KEYS] = {         0.2,        0.2,        0.2,        0.2 };   // 触发值，向下移动多远能触发，单位mm
const float     release_mm[KEYS] = {         0.2,        0.2,        0.2,        0.2 };   // 释放值，向上移动多远能释放，单位mm
const float     top_mm[KEYS]     = {         0.0,        0.0,        0.0,        0.0 };   // 上死区，顶部抖动多少不触发，单位mm
const float     bottom_mm[KEYS]  = {         0.0,        0.0,        0.0,        0.0 };   // 下死区，底部抖动多少不释放，单位mm
const int       travel[KEYS]     = {           4,          4,          4,          4 };   // 总行程，轴体本身的物理参数，单位mm
const int       times[KEYS]      = {          30,         30,         30,         30 };   // 初始化时放大抖动的倍数，如果插入后有自动输入则增大，按下后还无法输入则减小值，完全相同的按键这个值可以是一样的

/***************** 不需要修改的参数和函数 *****************/

// 头文件和USB设置
#include <USBComposite.h>
USBHID HID;
HIDKeyboard Keyboard(HID);

// 按键参数结构体
struct {
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
} keys[KEYS];

// 按下按键，发送后改变按键状态，按键首次被按下
void pressKey(int n) {
  if (keys[n].init){
    Keyboard.press(keyCodes[n]);
  } 
  else if (keys[n].value > keys[n].min + keys[n].cushion){
    keys[n].init = true;
  }
  keys[n].pressed = true;
}

// 释放按键，发送后改变按键状态，按键首次被按下
void releaseKey(int n) {
  if (keys[n].init){
    Keyboard.release(keyCodes[n]);
  } 
  else if (keys[n].value > keys[n].min + keys[n].cushion){
    keys[n].init = true;
  }
  keys[n].pressed = false;
}

// 转换距离，将以毫米为单位的值转换成模拟量，假设磁铁外磁场强度均匀变化，则距离与模拟量线性相关
void convertDistance(int n) {
  keys[n].unit = (keys[n].max - keys[n].min) / travel[n];
  keys[n].trigger = keys[n].unit * trigger_mm[n];
  keys[n].release = keys[n].unit * release_mm[n];
}

// 按键初始化
void initKey(int n) {

  // 读模拟量，最大值和最小值先暂定为当前值
  pinMode(keyPins[n],INPUT_ANALOG);
  keys[n].value = analogRead(keyPins[n]);
  keys[n].min = keys[n].value;
  keys[n].max = keys[n].value;

  // 给每个按键1s的时间，记录抖动范围，此时应保证按键不受力
  for (int i = 0; i < 500; i++) {
    keys[n].value = analogRead(keyPins[n]);
    if (keys[n].value < keys[n].min) {
      keys[n].min = keys[n].value;
    } else if (keys[n].value > keys[n].max) {
      keys[n].max = keys[n].value;
    }
    delay(1);
  }

  // 计算缓冲值
  keys[n].cushion = (keys[n].max - keys[n].min) * times[n];   

  // 初始化一些参数
  keys[n].init = false;                               // 按键还没有被首次按下
  keys[n].min = keys[n].value;                        // 初始化最小值，暂定为当前值
  keys[n].max = keys[n].value + keys[n].cushion;      // 初始化最大值，暂定为当前值 + 缓冲值
  keys[n].reference = keys[n].value;                  // 初始化参考值为当前值
  keys[n].trigger = trigger_mm[n] * keys[n].cushion;  // 初始化触发值，避免插上的瞬间有输入
  keys[n].release = release_mm[n] * keys[n].cushion;  // 初始化释放值，避免插上的瞬间有输入
}

// 平衡按键，按下时探索更大的值，释放时探索更小的值，以免在使用时极值漂移使抖动能误触发
void balanceKey(int n) {
  if (!keys[n].pressed && (keys[n].value < keys[n].min)) {
    keys[n].min = keys[n].value;
    convertDistance(n);
  } else if (keys[n].pressed && (keys[n].value > keys[n].max)) {
    keys[n].max = keys[n].value;
    convertDistance(n);
  }
}

// 按键移动时判断状态，同时动态改变参考值
void processKey(int n) {

  // 不在死区移动时的判断
  if ((keys[n].value > keys[n].min + keys[n].unit * top_mm[n]) && (keys[n].value < keys[n].max - keys[n].unit * bottom_mm[n])){
    if (!keys[n].pressed) {
      if (keys[n].value > keys[n].reference + keys[n].trigger) {
        pressKey(n);
      }
      if (keys[n].value < keys[n].reference) {
        keys[n].reference = keys[n].value;
      }
    } else {
      if (keys[n].value <= keys[n].reference - keys[n].release) {
        releaseKey(n);
      }
      if (keys[n].value > keys[n].reference) {
        keys[n].reference = keys[n].value;
      }
    }
  }

  // 在死区移动时的判断
  else if ((keys[n].value < keys[n].min + keys[n].unit * top_mm[n]) && keys[n].pressed){
    releaseKey(n);
  }
  else if ((keys[n].value > keys[n].max - keys[n].unit * bottom_mm[n]) && !keys[n].pressed){
    pressKey(n);
  }
}

// 扫描一次按键
void runKey(int n) {
  keys[n].value = analogRead(keyPins[n]);
  balanceKey(n);
  processKey(n);
}

// 初始化
void setup() {
  HID.begin(HID_KEYBOARD);
  while (!USBComposite);
  Keyboard.begin();
  for (int i = 0; i < KEYS; i++) {
    initKey(i);
  }
}

// 循环体
void loop() {
  for (int i = 0; i < KEYS; i++) {
    runKey(i);
  }
}

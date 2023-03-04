/*
  此程序用于实现wooting键盘中的Rapid trigger功能，移植并优化自Github上的一个开源项目，添加了按键的独立参数，优化了确定缓冲值的方式。

  为提高易读性，此程序并未严格遵守编写规范，请在开头处“需要自定义的参数”中更改和添加相关量以适应手感和实现功能，若不想了解运行原理，则不必理会下面所有的参数和函数。

  测试用开发板为CH552G，注意，当它用作USB设备时，每次上传程序都需要按住板子上的按钮同时插入电脑，再松开按钮，电脑才能识别到。
  
  上传程序前请确认以下设置是正确的：

    文件 -> 首选项 -> 其他开发板管理器地址 -> 添加一行：https://github.com/DeqingSun/ch55xduino
    工具 -> 开发板 -> 搜索ch552，安装以CH55xduino开头的环境
    工具 -> 开发板 -> CH55xduino...  -> CH552 Board
    工具 -> Clock Source -> 24MHz(Internal),5V;
    工具 -> Upload method -> USB
    工具 -> USB Settings -> USER CODE w/148B USB RAM

    设置参考：https://github.com/DeqingSun/ch55xduino ； 

    原始项目：https://github.com/chent7/hall-2k-keypad-handmade

  本项目使用Apache 2.0开源协议，如需商用或借鉴，请阅读此协议。
  
  欢迎关注我的B站账号，一个只分享osu!相关内容，很无聊的帐号。
  
  用户名：音游玩的人，B站主页：https://space.bilibili.com/9182439?spm_id_from=..0.0
*/

/******************** 需要自定义的参数 ********************/

#define     KEYS  1                      // 按键数，下面的参数表是故意对齐的，每一列对应一个按键
const int   keyPins[KEYS]    = {  11 };  // 引脚号，只能使用有ADC功能的引脚，详情看引脚图
const char  keyCodes[KEYS]   = { 'z' };  // 按键值，注意要用单引号括住键值
const float trigger_mm[KEYS] = { 0.9 };  // 触发值，向下移动多远能触发，单位mm
const float release_mm[KEYS] = { 0.8 };  // 释放值，向上移动多远能释放，单位mm
const int   travel[KEYS]     = {   4 };  // 总行程，轴体本身的物理参数，单位mm

/***************** 不需要修改的参数和函数 *****************/

// 头文件和USB设置
#ifndef USER_USB_RAM
#error "This example needs to be compiled with a USER USB setting"
#endif
#include "src/userUsbHidKeyboardMouse/USBHIDKeyboardMouse.h"

// 按键参数结构体
struct {
  int min;
  int max;
  int trigger;
  int release;
  int value;
  int reference;
  bool pressed;
} keys[KEYS];

// 按下按键，发送后改变按键状态
void pressKey(int n) {
  Keyboard_press(keyCodes[n]);
  keys[n].pressed = true;
}

// 释放按键，发送后改变按键状态
void releaseKey(int n) {
  Keyboard_release(keyCodes[n]);
  keys[n].pressed = false;
}

// 转换距离，将以毫米为单位的值转换成模拟量，假设磁铁外磁场强度均匀变化，则距离与模拟量线性相关
void convertDistance(int n) {
  keys[n].trigger = ((keys[n].max - keys[n].min) / travel[n]) * trigger_mm[n];
  keys[n].release = ((keys[n].max - keys[n].min) / travel[n]) * release_mm[n];
}

// 按键初始化
void initKey(int n) {
  
  // 读模拟量，最大值和最小值先暂定为当前值
  pinMode(keyPins[n],INPUT);
  keys[n].value = analogRead(keyPins[n]);
  keys[n].min = keys[n].value;
  keys[n].max = keys[n].value;

  // 给每个按键1s的时间，记录抖动范围，此时应保证按键不受力
  for (int i = 0; i < 1000; i++) {
    keys[n].value = analogRead(keyPins[n]);
    if (keys[n].value < keys[n].min) {
      keys[n].min = keys[n].value;
    } else if (keys[n].value > keys[n].max) {
      keys[n].max = keys[n].value;
    }
    delay(1);
  }

  // 计算缓冲值，为抖动范围的10倍，如果插上后一直有输入，需要按一下按键才停止，尝试增大这个值
  int cushion = (keys[n].max - keys[n].min) * 30;

  // 设置最值，最小值不应该减去缓冲值，以避免扩大模拟量的范围
  keys[n].min = keys[n].value;
  keys[n].max = keys[n].value + cushion;

  // 初始化触发值和释放值，避免插上的瞬间有输入
  keys[n].trigger = trigger_mm[n] * cushion;
  keys[n].release = release_mm[n] * cushion;

  // 初始化参考值
  keys[n].reference = keys[n].value;
}

// 平衡按键，按下时探索更大的值，释放时探索更小的值，完整按下一次后能精确记录模拟量范围
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

// 扫描一次按键
void runKey(int n) {
  keys[n].value = analogRead(keyPins[n]);
  balanceKey(n);
  processKey(n);
}

// 初始化
void setup() {
  for (int i = 0; i < KEYS; i++) {
    initKey(i);
  }
  USBInit();
}

// 循环体
void loop() {
  for (int i = 0; i < KEYS; i++) {
    runKey(i);
  }
}
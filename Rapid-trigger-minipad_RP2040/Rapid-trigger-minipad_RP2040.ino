/*
  此程序用于实现wooting键盘中的Rapid trigger功能，移植并优化自Github上的一个开源项目，添加了按键的独立参数，优化了确定缓冲值的方式。

  为提高易读性，此程序并未严格遵守编写规范，请在开头处“需要自定义的参数”中更改和添加相关量以适应手感和实现功能，若不想了解运行原理，则不必理会下面所有的参数和函数。

  测试用开发板为微雪RP2040 Zero，上传程序前请确认以下设置是正确的：

    文件 -> 首选项 -> 其他开发板管理器地址 -> 添加一行：https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
    工具 -> 开发板 -> 搜索RP2040，看到描述非常多的那一项，安装最新版本
    工具 -> 开发板 -> Raspberry Pi Pico/RP2040 -> Waveshare RP2040 Zero，如果没有说明上一步装错了
    工具 -> 端口：“COMX” -> COMX（X为设备对应端口号）
    工具 -> Upload method -> Default(UF2)
    工具 -> USB Stack -> Pico SDK

    参考：https://github.com/earlephilhower/arduino-pico ; http://bbs.eeworld.com.cn/thread-1230450-1-1.html    

    原始项目：https://github.com/chent7/hall-2k-keypad-handmade    

  本项目使用Apache 2.0开源协议，如需商用或借鉴，请阅读此协议。
  
  欢迎关注我的B站账号，一个只分享osu!相关内容，很无聊的帐号。
  
  用户名：音游玩的人，B站主页：https://space.bilibili.com/9182439?spm_id_from=..0.0
*/

/******************** 需要自定义的参数 ********************/

#define         KEYS  4                                     // 按键数，下面的参数表是故意对齐的，每一列对应一个按键
const uint8_t   keyPins[KEYS]    = {  A0,  A1,  A2,  A3,};  // 引脚号，只能使用有ADC功能的引脚，对应引脚26，27，28，29
const char      keyCodes[KEYS]   = { 'z', 'x', 'c', 'v' };  // 按键值，注意要用单引号括住键值
const float     trigger_mm[KEYS] = { 0.9, 0.9, 0.9, 0.9 };  // 触发值，向下移动多远能触发，单位mm
const float     release_mm[KEYS] = { 0.8, 0.8, 0.8, 0.8 };  // 释放值，向上移动多远能释放，单位mm
const int       travel[KEYS]     = {   4,   4,   4,   4 };  // 总行程，轴体本身的物理参数，单位mm

/***************** 不需要修改的参数和函数 *****************/

// 头文件
#include <Keyboard.h>   // 注意，要删除IDE自带的USB库，留下通过管理器安装的库，否则会因为找到多个库而报错

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
  Keyboard.press(keyCodes[n]);
  keys[n].pressed = true;
}

// 释放按键，发送后改变按键状态
void releaseKey(int n) {
  Keyboard.release(keyCodes[n]);
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
  pinMode(keyPins[n], INPUT);
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
  int cushion = (keys[n].max - keys[n].min) * 10;

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
  Keyboard.begin();
}

// 循环体
void loop() {
  for (int i = 0; i < KEYS; i++) {
    runKey(i);
  }
}
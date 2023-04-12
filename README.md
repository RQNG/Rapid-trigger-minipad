# Rapid-trigger-minipad
Rapid trigger是Wooting键盘的特色功能，是指没有固定键程，靠上下移动判定触发的功能，能解决切指卡手的大难题，传言能破解osu!这款游戏。

我本来想尝试一下wooting，但其价格昂贵，没有渠道，二手抢钱等等问题让我很不爽，于是决定自己做，我想这也是许多玩家想做的，所以我将它分享出来，现在，它属于每一名玩家了。

本项目提供三种主控方案，其中以STM32为主，出于性能和性价比考虑，CH552和RP2040版本将不再更新，但仍然保留代码给有需要的人。

按键为独立模块并且全部使用杜邦线连接，按键间距可调，STM32版本最多支持10个模拟输入按键，这应该能满足大部分玩家在按键数量和个性化上的需求了。

本项目移植并改进自另一个项目，虽然我也用自己的方法实现过，但发现已经有更好的方法了，就移植过来并优化。

这并不是很厉害的技术，但确实还算有趣，感兴趣的话欢迎尝试制作。我希望这个项目越多人关注越好，最好在玩家群体里流行开来，我认为这种功能被说是走捷径的根源就是不够普及，所以都来用吧。

欢迎对此项目进行改造和发布新作品，请在描述中注明参考本项目的地方和本项目GitHub网址即可。

![13](https://user-images.githubusercontent.com/115459678/220674378-8d570217-f2c0-463d-8ff0-872b27b706b0.jpg)

# 项目特点
* 制作简单，无需任何专业操作（如焊接等）和专业知识就能制作。
* 无法垄断，所有材料在国内都有渠道购买并且供货充足。
* 本地控制，增加控制器，除按键总键程外，所有设置都可以在本地完成。
* 丝滑菜单，控制器使用模仿稚晖君MonoUI的多级菜单，动画被优化到非常丝滑，可以当解压玩具（真的）。
* 按键数多，最多能添加10个模拟按键，多于市面上其它方案，并且每个按键都可以不一样。
* 三种模式，支持不同极性按键，不同极性传感器的所有组合，组合对应两个模式，还有禁用模式。
* 键距可调，每个按键为独立制作并且可以在外壳内自由摆放，提供多种宽度的填充模块。
* 成本较低，使用stm32作为主控，价格比arduino更低，按键数量可以自行选择以降低成本。
* 性能更好，经测试，ADC精度比频率等因素更影响表现，而stm32有精度更高的ADC，因此是最优选择。

# 全功能控制器
控制器使用0.96寸OLED显示，EC11旋钮控制，使用模仿稚晖MonoUI的多级菜单，具有以下功能：

* 丝滑菜单，可调节动画效果的速度，速度值 0 - 10 ，值 1 - 10 模拟临界阻尼的效果，以最大速度开始，逐渐减速，并刚好停在目标位置，值 0 模拟欠阻尼的效果，有类似果冻的回弹。
* 旋钮功能，在休眠界面旋转可调节音量或亮度，点按后进入主菜单，在菜单界面旋转滚动菜单，点按选择菜单。
* 三种模式，支持不同极性的磁轴和传感器的组合，还有禁用模式。
* 参数设置，支持设置几乎所有参数，包括：触发值，释放值，上死区，下死区，键值，模式，初始化时放大抖动的倍数，每个按键都可以独立设置，也可以一起设置。
* 断电保存，回到休眠模式即保存当前设置，断电后不会丢失。
* 测试引脚，支持查看每一个引脚的电压变化情况，用于测试传感器和引脚是否正常。

主界面：
![屏幕截图 2023-03-31 232407](https://user-images.githubusercontent.com/115459678/229164865-9ca36938-1b3c-413d-baa1-37ebc37f25a1.png)

控制器演示视频：https://www.bilibili.com/video/BV1Hv4y1V7Zu/#reply569309422

# 内部结构
四按键版本：

![Notes_230225_033130_2](https://user-images.githubusercontent.com/115459678/221276253-6802f2ba-7774-4be8-8dec-03acbd14931b.jpg)

十按键版本：

![Notes_230225_033130_2 (1)](https://user-images.githubusercontent.com/115459678/221276353-b42a8cb6-5055-44fe-86a3-1e9330aefd2a.jpg)

# 制作教程
1. 轴体改造篇：https://www.bilibili.com/read/cv21948435
2. 软件设置篇：https://www.bilibili.com/read/cv21969155
3. 硬件组装篇：https://www.bilibili.com/read/cv21985212

# 参考项目
1. hall-2k-keypad-handmade：https://github.com/chent7/hall-2k-keypad-handmade
2. 如何DIY一款属于自己的HID键盘：https://mp.weixin.qq.com/s/o-8_3SQS2AGT_WTyTLYQnA
3. 【三键侧透光小键盘】CH552G：https://oshwhub.com/phantomr/styudy-xiao-jian-pan-ch552g

# 推荐项目
下列项目都有Rapid trigger功能，它们各有优势，对我的项目有建议的，在给出建议之外，也可以尝试在这些项目中寻找解决方案。

* hall-2k-keypad-handmade: https://github.com/chent7/hall-2k-keypad-handmade
* minipad-firmware-old: https://github.com/minipadKB/minipad-firmware-old
* minipad-firmware: https://github.com/minipadKB/minipad-firmware
* fluxpad: https://github.com/sssata/fluxpad
* the-poor-mans-keypad-and-future-wooting: https://www.reddit.com/r/osugame/comments/zpcvkw/the_poor_mans_keypad_and_future_wooting/
* making-a-diy-wooting-keypad: https://www.reddit.com/r/osugame/comments/117v6xd/making_a_diy_wooting_keypad/
* DIY under 20$ WOOTING like Keypad for Osu! (analog switches): https://www.youtube.com/watch?v=4rrDy9KakRI&t=10s&ab_channel=prAlex
* BexPad: https://github.com/Bexin3/BexPad

# 改进空间
目前我的方案在通用性和表现上都已经很不错了，动手能力不强的普通玩家完全可以放心尝试，不会令你们失望的。
对于不惜代价追求极致的人来说，我的方案可能显得太过廉价，因此在这里提出一些改进空间，欢迎尝试，如果愿意分享的话，请合并在第三方设计项目中供他人参考。

* 使用stm32作为主控，不是因为廉价，而是因为它拥有最高的ADC分辨率，这个最影响体验。
* 使用stm32标准库或者hal库实现通信功能，因为有成熟的方案可用，但因为IDE劝退新手而弃用。
* 使用AH491线性霍尔传感器，这种灵敏度很高为15mv/Gs，但相对的会放大抖动，因为不好调试而弃用。
* 使用pcb，能降低干扰并且有助于批量化生产，但因为无法实现可调键距和需要焊接而弃用。
* 使用成品磁轴，轴芯有个小磁铁跟着上下移动的那种，因为有垄断风险而弃用，目前已知的有三种：
  1. wooting的lekker轴，这个要海淘。
  2. 佳达隆的ks-20，这个买不到。
  3. 东莞市益谦电子有限公司，1688搜索“磁轴”能找到，应该能买到。

# 建议收集
可以直接在GitHub上提建议，也可以在B站专门的动态下提。

* 初版方案建议收集动态：https://t.bilibili.com/757193959862697993

# 更新
## 软件

### v 2.1
* 重构代码，只保存路径和选择框所在位置，分页面类型定义变量。
* 增加图标和列表两种主界面风格。
* 支持本地查看所有配置，在关于本机页面。
* 支持128×64，128×32两种主流OLED屏幕分辨率，只需要替换驱动即可。
* 增加列表展开动画效果。
* 增加选择框展开和跳转动画效果。
* 增强动画速度的可调精度。
* 增加循环模式。
* 增加旋钮方向反转选项。

### v 2.0
* 增加模仿MonoUI的丝滑菜单。
* 支持不同极性的轴和不同极性的传感器，使用对应模式即可。
* 支持在本地设置所有参数。
* 支持断电保存功能。
* 支持引脚测试，可以在本地查看引脚电压变化曲线。

### v 1.3
* 初始化改为并行扫描，按键数增加不会明显延长初始化时间。
* 增加初始化指示灯，在板子上的蓝灯未熄灭前，不要按下按键。

### v 1.2 
* 解决了插入后微小干扰下误触发的bug。
* 增加了上下死区。
* 增加除字母外其它按键的支持。

### v 1.1 
* 解决了插入后不断输入的bug。
* 优化了初始化方式。

### v 1.0 
* 简单移植hall-2k-keypad-handmade。

## 硬件：

### v 1.2
* 增加OLED显示屏。
* 增加EC11旋钮。

### v 1.1 
* 增加十按键版本外壳模型。
* 增加几种填充模块模型。

### v 1.0 
* 四按键版本外壳模型。
* 独立按键所需模型。
* 材料清单。

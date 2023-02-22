# Rapid-trigger-minipad
Rapid trigger是Wooting键盘的特色功能，是指没有固定键程，靠上下移动判定触发的功能，能解决切指卡手的大难题，传言能破解osu!这款游戏。

我本来想尝试一下wooting，但其价格昂贵，没有渠道，二手抢钱等等问题让我很不爽，于是决定自己做，我想这也是许多玩家想做的，所以我将它分享出来，现在，它属于每一名玩家了。

本项目提供三种主控方案，其中以STM32为主，出于性能和性价比考虑，CH552和RP2040版本将不再更新，但仍然保留代码给有需要的人。

按键为独立模块并且全部使用杜邦线连接，按键间距可调，STM32版本最多支持10个模拟输入按键，这应该能满足大部分玩家在按键数量和个性化上的需求了。

本项目移植并改进自另一个项目，虽然我也用自己的方法实现过，但发现已经有更好的方法了，就移植过来并优化。

这并不是很厉害的技术，但确实还算有趣，感兴趣的话欢迎尝试制作。我希望这个项目越多人关注越好，最好在玩家群体里流行开来，我认为这种功能被说是走捷径的根源就是不够普及，所以都来用吧。

欢迎对此项目进行改造和发布新作品，请在描述中注明参考本项目的地方和本项目GitHub网址即可。

# 制作教程
![13](https://user-images.githubusercontent.com/115459678/220674378-8d570217-f2c0-463d-8ff0-872b27b706b0.jpg)

1. 轴体改造篇：https://www.bilibili.com/read/cv21948435
2. 软件设置篇：https://www.bilibili.com/read/cv21969155
3. 硬件组装篇：https://www.bilibili.com/read/cv21985212

# 参考项目
1. hall-2k-keypad-handmade：https://github.com/chent7/hall-2k-keypad-handmade
2. 如何DIY一款属于自己的HID键盘：https://mp.weixin.qq.com/s/o-8_3SQS2AGT_WTyTLYQnA
3. 【三键侧透光小键盘】CH552G：https://oshwhub.com/phantomr/styudy-xiao-jian-pan-ch552g

# 建议收集
初版方案建议收集动态：https://t.bilibili.com/757193959862697993

# 更新
v 1.2
解决了插入后微小干扰下误触发的bug，增加了上下死区，增加除字母外其它按键的支持。

v 1.1
解决了插入后不断输入的bug，优化了初始化方式。

v 1.0 
简单移植hall-2k-keypad-handmade。

# homeassistant_zinguo_mqtt

峥果浴霸开关-ESP8285 自制固件接入开源智能家居平台

前言：
	前期本人与其他大佬共同编写了python插件https://github.com/gexing147/homeassistant_zinguo。

	由于ha版本不停迭代，造成维护不及时，造成无法使用等一些问题。本人也由于时间原因也暂时搁置。

	Hassbian 论坛也有大佬写了node-red 插件 挽救了个别用户 在此表示感谢https://github.com/yaming116/node-red-contrib-zinguo 

新的开始：

	上次与我一起上车的童鞋都知道，硬件是ESP8266方案。（官方解释升级硬件是因为某些原因会出现触摸失灵）
    注：插件是否支持新版本ESP8285未验证。

硬件分支：

	ESP8266版：暂时放弃完善代码，会编程的大佬可以联系我提供DEMO代码与图纸。
	新款ESP8285 版本现在已经基本完善，后期我们会持续更新该固件。

已支持接入的开源智能家居平台

1.Home Assistant：
		Home Assistant 是一款基于 Python 的智能家居开源系统，支持众多品牌的智能家居设备，可以轻松实现设备的语音控制、自动化等。
		
		官方网站：https://www.home-assistant.io/
		
		国内论坛：https://bbs.hassbian.com/forum.php

	接入方法：
		1.按照TTL接线方法接线（具体查看file目录）
		2.修改固件信息并编译
		3.配置Home Assistant（具体查看file目录yaml）

    变更日志：
		1.0 
			初始版本
		1.1
			单电机本地版本，联动，等
		1.2
			新增mqtt 修复MQTT控制与当前的按钮开关不一致问题，MQTTCallback增加了对MQTT的payload的信息判断 
		1.2.1
			新增 MQTT设备名配置（支持多设备）
			新增 单双电机配置 
			新增 新增联动配置
			新增 新增风铃开关配置
		
	在线升级：
		HTTPWebUpdater
		输入IP登陆在线升级网站，账号密码与mqtt一致
2、ioBroker
	ioBroker是基于nodejs的物联网的集成平台，为物联网设备提供核心服务，系统管理和统一操作方式。
	
	ioBroker中国 https://doc.iobroker.cn/#/_zh-cn/

	接入方法
		待补充

3、其他支持mqtt的平台
	理论上来说，只要是支持mqtt的平台都可以实现接入。

	接入方法
		待补充


主要硬件清单：
  SC09A：* 1
  ESP8285：* 1
  数码管：* 1
  74HC595D：* 2
 
致谢：
  以下排名不分先后，为随机。
  老妖：SC09A驱动编写，SC09A 测试DEMO https://github.com/smarthomefans/zinguo_smart_switch
  
  楓づ回憶：提供硬件与后期代码测试与更改
  
  快乐的猪：修复代码bug与mqtt部分
  
  NoThing ：前期画制原理图、测试引脚走向、协议分析、代码编写

免责申明：
  以上纯属个人爱好，因为使用上述方法造成的任何问题，不承担任何责任。
  部分图片来源于网络，如果涉及版权，请通知删除。 

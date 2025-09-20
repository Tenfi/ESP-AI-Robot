
# B 站粉丝显示

本例程可以实时显示当前的 B 站粉丝数，并在粉丝增长后播放欢呼的音效

<div align="center">
<img src="../../_static/bilibili-fans.jpg" width="50%">
</div>

# 配置工程

在 [bilibili_fans/main/Kconfig.projbuild](main/Kconfig.projbuild) 中配置：

1. **BILIBILI_ACCOUNT_NUMBER**
   - B 站账号 UID，在个人资料中可以获得，如：3493296718482287

2. **BILIBILI_ACCOUNT_NAME**
   - B 站账号名称，用于显示在屏幕上。**注意：若包含中文字符需自己配置字体文件**

3. **EXAMPLE_WIFI_SSID**
   - 连接 Wi-Fi 的 SSID

4. **EXAMPLE_WIFI_PASSWORD**
   - 连接 Wi-Fi 的密码

可以通过 ``idf.py menuconfig`` 手动配置上方参数，也可以直接在 Kconfig.projbuild 中修改 default 值

注意：如果 sdkconfig 文件已经生成，则 Kconfig.projbuild 不再生效。可以删除 sdkconfig 或运行 ``idf.py menuconfig`` 指令修改。

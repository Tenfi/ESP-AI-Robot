# **ESP32-S3 大模型 AI 桌面机器人**

本实例可以实现桌面钟、大模型语音对话、敲木鱼、摇色子、2048、摄像头等功能。

# **大模型对话**

- 本例程使用百度大语言模型：[ERNIE-Lite-8K](https://cloud.baidu.com/doc/WENXINWORKSHOP/s/dltgsna1o) 进行对话，通过[百度的语音服务](https://cloud.baidu.com/doc/SPEECH/s/mlbxh7xie) **进行语音转文字**和**文字转语音**。以上功能个人开发者完成身份认证后可以[免费领取使用额度](https://console.bce.baidu.com/ai/?fromai=389#/ai/speech/overview/resource/getFree)。**[获取 access token](https://cloud.baidu.com/doc/WENXINWORKSHOP/s/Ilkkrb0i5) 后便可快速体验大模型对话功能。**

- 若想对接其他大语言模型，可替换 ``components`` 文件夹中的 [baidu_llm](components/baidu_llm)，并修改 [main/app/app_audio_record.c](main/app/app_audio_record.c) 中对应的内容。

- [Demo 视频](https://www.bilibili.com/video/BV1MezzYaEfP/)中展示了接入小度 APP 并使用对应功能（包含对接文心一言大模型），使用该功能需要联系百度商务以获取其特殊提供的 **DuerOS profile、设备名前缀和密钥**。当前**个人开发者无法自行通过网页获取**。也可以**联系[乐鑫商务](https://www.espressif.com/zh-hans/contact-us/sales-questions)获取更多信息**

- 若已经获取了 DuerOS profile、设备名前缀和密钥，可以先体验 ESP-ADF 中的 [DuerOS 方案](https://github.com/espressif/esp-adf/tree/master/examples/dueros)。若想结合本工程，可以将 [baidu_llm](components/baidu_llm) 组件替换为 [dueros_service](https://github.com/espressif/esp-adf/tree/master/components/dueros_service) 组件。注意：[DuerOS 方案](https://github.com/espressif/esp-adf/tree/master/examples/dueros) 开发需要搭建 [ESP-ADF](https://github.com/espressif/esp-adf/tree/master) 开发环境。

# 开发环境

本例程推荐在 ``ESP-IDF v5.2.3`` 下进行开发
- Windows 用户可以使用 [一键安装工具](https://dl.espressif.com/dl/esp-idf/)。参考文档：[Windows 平台工具链的标准设置](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/windows-setup.html)，也可以参考视频教程：[使用一键安装工具快速搭建 ESP-IDF 开发环境 (Windows)](https://www.bilibili.com/video/BV1to4y177ko/)
- Linux 和 macOS 用户可以参考文档：[Linux 和 macOS 平台工具链的标准设置](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/linux-macos-setup.html)。
  - 若 Github 网络不稳定，可以尝试 Gitee 镜像：https://gitee.com/EspressifSystems/esp-idf.git

# 配置工程

在 [main/Kconfig.projbuild](main/Kconfig.projbuild) 中配置：

1. **BAIDU_AUDIO_ACCESS_TOKEN**
   - 百度音频服务的 ACCESS_TOKEN，通过百度控制台中的[语音技术]((https://cloud.baidu.com/doc/SPEECH/s/mlbxh7xie))的 API Key 和 Secret Key 申请，详情参考：[获取 access_token](https://cloud.baidu.com/doc/WENXINWORKSHOP/s/Ilkkrb0i5)

2. **BAIDU_LLM_ACCESS_TOKEN**
   - 大语言模型服务的 ACCESS_TOKEN，通过百度控制台中的[大语言模型应用](https://cloud.baidu.com/doc/WENXINWORKSHOP/s/dltgsna1o)的 API Key 和 Secret Key 申请，详情参考：[获取 access_token](https://cloud.baidu.com/doc/WENXINWORKSHOP/s/Ilkkrb0i5)。
   - **注意：大语言模型和语音服务使用的是两种不同的 access_token**

3. **QWEATHER_REQUEST_KEY**
   - [和风天气的 request key](https://dev.qweather.com/), 用于在桌面钟界面显示天气和温度信息

4. **EXAMPLE_WIFI_SSID**
   - 连接 Wi-Fi 的 SSID

5. **EXAMPLE_WIFI_PASSWORD**
   - 连接 Wi-Fi 的密码

可以通过 ``idf.py menuconfig`` 手动配置上方参数，也可以直接在 Kconfig.projbuild 中修改 default 值

注意：如果 sdkconfig 文件已经生成，则 Kconfig.projbuild 不再生效。可以删除 sdkconfig 或运行 ``idf.py menuconfig`` 指令修改。

# 编译和烧录

完成上方的配置后，运行下方指令进行编译和烧录
```
idf.py flash monitor
```
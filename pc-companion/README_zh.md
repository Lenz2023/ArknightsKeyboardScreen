# GK4 屏幕音乐桥接（GK4ScreenMusicBridge）

把网易云音乐「正在播放」的歌名、歌手、同步歌词实时推到 **SKYLOONG GK4 / GK87 键盘副屏**。

这是一个 [BetterNCM](https://github.com/BetterNCM/BetterNCM) 插件，不会额外启动常驻程序，也不创建任务栏窗口——它只在网易云音乐的渲染进程里跑，每 250 ms 采一次播放状态，用 250 ms～5 s 的自适应节流 POST 到键盘屏幕的 `/music` 接口。

## 运行条件

- 网易云音乐 PC 版 + [BetterNCM](https://github.com/BetterNCM/BetterNCM)
- BetterNCM 插件：**LibEAPIRequest**（EAPI 请求）、**LibLyric**（歌词数据）——两者缺失时歌名仍可显示，但不会有同步歌词
- 电脑与键盘屏幕在**同一个 Wi-Fi**
- 键盘已开机、已配网、屏幕停在主界面（固件启动后会开一个网页服务供插件推送）
- 屏幕地址：默认用 mDNS 名 `http://skyloong-screen.local`；Windows 解析不了这个名称时，在**插件设置页**里填屏幕的局域网 IP 即可（见下）

## 安装

**方式一（插件商店）**：在 BetterNCM 的插件商店里搜 `GK4ScreenMusicBridge` 安装。

**方式二（手动）**：把 `GK4ScreenMusicBridge.plugin` 放进 BetterNCM 目录下的 `plugins/`，重启网易云或在 BetterNCM 里重载插件。

安装后确认 LibEAPIRequest、LibLyric 已启用。

## 设置

在 BetterNCM 插件列表里点本插件的**设置**按钮，可以：

- 看到当前的屏幕地址与连接测试结果
- 手动填写屏幕地址 →「保存并测试」（会自动补 `http://` 并探测 `/info`）
- 「自动发现」：先试 mDNS 名，再用 WebRTC 拿到本机网段、按 /24 分段扫一遍局域网找屏幕
- 「清除记录」：忘掉缓存地址，回到默认 mDNS 名
- 直接点开**源代码仓库**和**问题反馈（GitHub Issues）**

> 屏幕地址会缓存在 `localStorage` 的 `gk4-screen-base-url` 里。

## 行为说明

- 播放时屏幕自动切到音乐页；没有当前曲目时屏幕回主界面；暂停时保留曲目信息
- 歌词按「当前句 + 下一句」推送，并做了 400 ms 的提前量（`LYRIC_LEAD_MS`），让换行听感对上节拍
- 进度条用本地插值：DOM 只暴露整秒，插件用 `performance.now()` 补足毫秒
- 每行歌词限长（`MAX_LINE_BYTES = 180`），超长按屏幕宽度截断
- 不读 PCM 音频、不跑额外服务、不做任何上传（只发到局域网里的键盘屏幕）

## 源代码与反馈

- 仓库：<https://github.com/Lenz2023/ArknightsKeyboardScreen/tree/main/pc-companion>
- 反馈：<https://github.com/Lenz2023/ArknightsKeyboardScreen/issues>

插件没有构建步骤，`main.js` 就是最终运行的代码（`manifest.json` 里 `injects.Main` 直接指向它），从仓库源码可直接复现商店里的包。

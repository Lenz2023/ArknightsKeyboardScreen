#pragma once

#include <stdint.h>

// 背景音乐：6 个场景槽位，与 Atmosphere 的背景槽位一一对应
//   0 clear(视野) / 1 cloudy(阴云) / 2 rain(秘海) / 3 snow(严冬) / 4 dusk(商业) / 5 night(夜幕)
namespace bgm
{

constexpr int slotCount = 6;

// 启动后台播放任务（只创建一次）。音量 0 时不发声。
void init();

// 请求切换到某个槽位；-1 表示停止。切曲会先淡出再淡入。
void setSlot(int slot);

// 当前正在播的槽位（-1 表示没在播）
int currentSlot();

// 音量变化后调用，重新应用（内部会按音量决定要不要真的往 I2S 写）
void onVolumeChanged();

} // namespace bgm

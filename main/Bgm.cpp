#include "Bgm.h"

#include "A_Config.h"
#include "hal.h"
#include "esp_heap_caps.h"
#include "esp_system.h"

#include <math.h>
#include <string.h>

// minimp3 的实现在 main/minimp3_impl.c（C 文件），这里只引用声明
extern "C" {
#define MINIMP3_NO_SIMD
#define MINIMP3_ONLY_MP3
#include "minimp3.h"
}

// mp3 由 main/bgm_data.c 内嵌（数组 + 长度）
extern "C" {
extern const unsigned char bgm_clear_mp3[];  extern const unsigned int bgm_clear_mp3_len;
extern const unsigned char bgm_cloudy_mp3[]; extern const unsigned int bgm_cloudy_mp3_len;
extern const unsigned char bgm_rain_mp3[];   extern const unsigned int bgm_rain_mp3_len;
extern const unsigned char bgm_snow_mp3[];   extern const unsigned int bgm_snow_mp3_len;
extern const unsigned char bgm_dusk_mp3[];   extern const unsigned int bgm_dusk_mp3_len;
extern const unsigned char bgm_night_mp3[];  extern const unsigned int bgm_night_mp3_len;
}
namespace
{

const uint8_t *const kTrackBegin[bgm::slotCount] = {
    bgm_clear_mp3, bgm_cloudy_mp3, bgm_rain_mp3,
    bgm_snow_mp3, bgm_dusk_mp3, bgm_night_mp3};

const uint8_t *const kTrackEnd[bgm::slotCount] = {
    bgm_clear_mp3 + bgm_clear_mp3_len,
    bgm_cloudy_mp3 + bgm_cloudy_mp3_len,
    bgm_rain_mp3 + bgm_rain_mp3_len,
    bgm_snow_mp3 + bgm_snow_mp3_len,
    bgm_dusk_mp3 + bgm_dusk_mp3_len,
    bgm_night_mp3 + bgm_night_mp3_len,
};

constexpr int kFadeInMs = 1200;
constexpr int kFadeOutMs = 700;
constexpr int kPcmFrames = 2304; // minimp3 一帧最大样本数（每声道）

__attribute__((aligned(16))) mp3dec_t decoder;
mp3dec_frame_info_t frameInfo;
__attribute__((aligned(16))) int16_t pcm[kPcmFrames * 2];
__attribute__((aligned(16))) int16_t stereo[kPcmFrames * 2 * 2];
__attribute__((aligned(16))) uint8_t stage[8192 + 8];

volatile int requestedSlot = -1;
volatile int playingSlot = -1;
volatile bool workerStarted = false;
TaskHandle_t workerTask = nullptr;

// 播放位置（当前曲目内的字节偏移）与当前曲目的淡入淡出状态
struct PlayState
{
    const uint8_t *begin = nullptr;
    const uint8_t *end = nullptr;
    const uint8_t *cursor = nullptr;
    size_t totalBytes = 0;
    size_t playedBytes = 0;
    float gain = 0.0f;   // 0..1，按淡入淡出包络推进
    int samplesPlayed = 0;
    int totalSamples = 0; // 估算，用于淡出
};

void resetDecoder(PlayState &st, const uint8_t *begin, const uint8_t *end)
{
    st.begin = begin;
    st.end = end;
    st.cursor = begin;
    st.totalBytes = (size_t)(end - begin);
    st.playedBytes = 0;
    st.gain = 0.0f;
    st.samplesPlayed = 0;
    // 48kbps 单声道 44.1kHz：总样本数 ≈ 文件字节数 / 6（粗略），只用于计算淡出位置
    st.totalSamples = (int)(st.totalBytes / 6);
    mp3dec_init(&decoder);
    frameInfo = {};
}

// 解码并送出一帧；返回实际写入的字节数，0 表示这一帧没有数据
size_t decodeAndWrite(PlayState &st)
{
    int bytesLeft = (int)(st.end - st.cursor);
    if (bytesLeft <= 0)
        return 0;

    if (bytesLeft > 8192)
        bytesLeft = 8192;

    memcpy(stage, st.cursor, (size_t)bytesLeft);   // 拷到对齐的内部缓冲，避免闪存非对齐读取
    int samples = mp3dec_decode_frame(&decoder, stage, bytesLeft, pcm, &frameInfo);
    if (frameInfo.frame_bytes <= 0)
        return 0;

    st.cursor += frameInfo.frame_bytes;
    st.playedBytes += (size_t)frameInfo.frame_bytes;

    if (samples <= 0 || samples > kPcmFrames)
        return 0; // 头信息或空帧

    const int channels = frameInfo.channels;
    const int count = samples;
    int16_t *out = stereo;
    int totalSamples = count * 2; // 输出固定立体声

    // 淡入 / 淡出包络
    const int fadeInSamples = (AUDIO_SAMPLE_RATE * kFadeInMs) / 1000;
    const int fadeOutStart = st.totalSamples > 0 ? st.totalSamples - (AUDIO_SAMPLE_RATE * kFadeOutMs) / 1000 : INT32_MAX;

    for (int i = 0; i < count; ++i)
    {
        int pos = st.samplesPlayed + i;
        float env = 1.0f;
        if (pos < fadeInSamples)
            env = (float)pos / (float)fadeInSamples;
        if (pos > fadeOutStart)
        {
            float tail = (float)(st.totalSamples - pos) / (float)((AUDIO_SAMPLE_RATE * kFadeOutMs) / 1000);
            if (tail < env)
                env = tail;
        }
        if (env < 0.0f)
            env = 0.0f;

        int16_t left = channels == 2 ? pcm[i * 2] : pcm[i];
        int16_t right = channels == 2 ? pcm[i * 2 + 1] : pcm[i];
        out[i * 2] = (int16_t)(left * env);
        out[i * 2 + 1] = (int16_t)(right * env);
    }
    st.samplesPlayed += count;

    return hal.audio_write((const uint8_t *)out, (size_t)totalSamples * sizeof(int16_t));
}

void worker(void *)
{
    PlayState st;
    bool haveTrack = false;

    for (;;)
    {
        int want = requestedSlot;
        if (want < 0 || want >= bgm::slotCount)
        {
            if (haveTrack)
            {
                mp3dec_init(&decoder);
                haveTrack = false;
                playingSlot = -1;
            }
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        if (!haveTrack || want != playingSlot)
        {
            resetDecoder(st, kTrackBegin[want], kTrackEnd[want]);
            playingSlot = want;
            haveTrack = true;
            ESP_LOGI("BGM", "slot %d start (%u bytes)", want, (unsigned)st.totalBytes);
        }

        if (hal._volume <= 0)
        {
            // 静音时不解码，省 CPU
            vTaskDelay(pdMS_TO_TICKS(300));
            continue;
        }

        size_t wrote = decodeAndWrite(st);
        if (wrote == 0)
        {
            if (st.cursor >= st.end)
            {
                // 播完一遍，回到开头继续循环
                resetDecoder(st, st.begin, st.end);
                ESP_LOGI("BGM", "slot %d loop", playingSlot);
            }
            else
            {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }
    }
}

} // namespace

namespace bgm
{

void init()
{
    if (workerStarted)
        return;
    workerStarted = true;
    BaseType_t ok = xTaskCreatePinnedToCore(worker, "bgm", 32768, nullptr, 2, &workerTask, 0);
    if (ok != pdPASS)
        ESP_LOGE("BGM", "worker create failed");
    else
        ESP_LOGI("BGM", "worker started (%d tracks embedded)", slotCount);
}

void setSlot(int slot)
{
    if (slot >= slotCount)
        slot = slotCount - 1;
    if (slot < -1)
        slot = -1;
    requestedSlot = slot;
}

int currentSlot()
{
    return playingSlot;
}

void onVolumeChanged()
{
    // 音量变化不需要重启任务：worker 每次循环都会读 hal._volume
}

} // namespace bgm

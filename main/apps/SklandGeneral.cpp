#include "A_Config.h"
#include "Atmosphere.h"
#include "SklandApi.h"
#include <stdlib.h>

extern "C" const lv_font_t font_rajdhani_32;
extern "C" const lv_font_t font_rajdhani_16;
#define LODEPNG_NO_COMPILE_CPP
extern "C" {
#include "src/extra/libs/png/lodepng.h"
}
namespace {
#include "DoctorIcons.inc"
#include "DoctorGlyphs.inc"
#include "OperatorNames.inc"
lv_obj_t *pages[2];

lv_obj_t *root = nullptr;
lv_obj_t *labels[24] = {};
lv_obj_t *portraitImage = nullptr;
lv_obj_t *taskHeader = nullptr;
lv_obj_t *avatarImage = nullptr;
lv_obj_t *profileAvatarImage = nullptr;
lv_obj_t *profileName = nullptr;
lv_obj_t *profileLevel = nullptr;
lv_obj_t *profileUid = nullptr;
lv_obj_t *profileStatus = nullptr;
lv_obj_t *secretaryName = nullptr;
// Column furniture, swapped for the wide-name marker when the name is long.
lv_obj_t *heroNameRule = nullptr;
lv_obj_t *heroNameTickTop = nullptr;
lv_obj_t *heroNameTickBottom = nullptr;
lv_obj_t *heroNameMark = nullptr;
lv_obj_t *heroNameWideRule = nullptr;

lv_obj_t *meters[5] = {};
constexpr int missionMeterWidth = 70;
lv_img_dsc_t portraitDesc = {};
lv_img_dsc_t avatarDesc = {};
uint8_t *portraitBytes = nullptr;
uint8_t *avatarBytes = nullptr;
uint32_t lastFetch = 0;
constexpr uint32_t kProfileFetchIntervalMs = 180000u;
bool fetched = false;
String cachedUID;
String values[24];
bool attempted = false;
bool retry = false;
uint8_t page = 0;
String portraitUrl;
String avatarUrl;

// --- Async profile fetch (mirrors the garrison worker) ---
// Network + PNG decode run on a worker task; the UI loop only applies a
// finished, self-contained result under the LVGL lock. Page switching stays
// instant because the森空岛 round-trips never block AppManagerLite::loop().
portMUX_TYPE profileMux = portMUX_INITIALIZER_UNLOCKED;
struct ProfileRequest { char uid[32]; uint32_t gen; char avCached[256]; char ptCached[256]; };
struct ProfileResult {
    char uid[32]; uint32_t gen; bool ok;
    char vals[24][64];
    bool avReady; uint8_t *avPixels; char avUrl[256];
    bool ptReady; uint8_t *ptPixels; char ptUrl[256];
};
ProfileRequest profileReq = {};
ProfileResult pendingProfile = {};
volatile bool profileFetchWanted = false;
volatile bool profileResultReady = false;
volatile bool profileWorkerBusy = false;
volatile uint32_t profileGeneration = 0;
TaskHandle_t profileWorker = nullptr;

const lv_color_t bg = lv_color_hex(0x171819);
const lv_color_t panel = lv_color_hex(0x1D2935);
const lv_color_t cyan = lv_color_hex(0x57D8E8);
const lv_color_t pale = lv_color_hex(0xF5F0E8);
const lv_color_t muted = lv_color_hex(0xAAA9A4);
const lv_color_t lime = lv_color_hex(0xD7E778);

// Hero portrait geometry, shared by setup() and the edge dissolve below.
constexpr int portraitWidth = 176;
constexpr int portraitHeight = 226;
constexpr int portraitX = 152;
constexpr int portraitY = -6;
constexpr int portraitZoom = 256;
// Bitmap columns melted into the scene so the hero has no cut-away left shoulder.
// The bottom of the source art is dropped so the half-body crop reads larger;
// whatever is left over is swallowed by the dissolve into the stats strip.
constexpr float heroSourceCrop = 0.78f;
constexpr unsigned heroLeftFade = 16;
// Shared by loadArt() and setup(): where the rule, its end ticks and the column sit.
constexpr int heroNameColumnX = 152;
constexpr int heroNameColumnWidth = 18;
constexpr int heroNameRuleX = 150;
constexpr int heroNameBandTop = 46;
constexpr int heroNameBandBottom = 168;
constexpr int heroNamePitch = 13;
// Names longer than this stop working as a letter column (the pitch would have
// to drop below 12px), so they are set as a right aligned block instead.
constexpr unsigned heroNameVerticalMax = 10;
constexpr int heroNameWideX = 10;
constexpr int heroNameWideWidth = 137;
constexpr int heroNameWideTop = 136;
constexpr unsigned heroNameWideChars = 14;

lv_obj_t *label(const char *text, int x, int y, int w, lv_color_t color, const lv_font_t *font = &lv_font_chinese_16) {
    lv_obj_t *o = lv_label_create(root);
    lv_label_set_text(o, text);
    lv_obj_set_style_text_font(o, font, 0);
    lv_obj_set_style_text_color(o, color, 0);
    lv_obj_set_width(o, w);
    lv_label_set_long_mode(o, LV_LABEL_LONG_DOT);
    lv_obj_set_pos(o, x, y);
    return o;
}

lv_obj_t *rect(int x, int y, int w, int h, lv_color_t color, uint8_t opa = LV_OPA_COVER) {
    lv_obj_t *o = lv_obj_create(root);
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, w, h);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_style_bg_color(o, color, 0);
    lv_obj_set_style_bg_opa(o, opa, 0);
    lv_obj_set_style_radius(o, 4, 0);
    return o;
}

String valueText(cJSON *obj, const char *key, const char *fallback = "—") {
    cJSON *v = obj ? cJSON_GetObjectItem(obj, key) : nullptr;
    if (cJSON_IsString(v)) return v->valuestring;
    if (cJSON_IsNumber(v)) return String(static_cast<int>(v->valuedouble));
    return fallback;
}
int numberValue(cJSON *obj, const char *key, int fallback = 0) {
    cJSON *v = obj ? cJSON_GetObjectItem(obj, key) : nullptr;
    if (cJSON_IsNumber(v)) return static_cast<int>(v->valuedouble);
    if (cJSON_IsString(v) && v->valuestring[0]) return atoi(v->valuestring);
    return fallback;
}
int64_t timestampValue(cJSON *obj, const char *key, int64_t fallback = 0) {
    cJSON *v = obj ? cJSON_GetObjectItem(obj, key) : nullptr;
    int64_t value = fallback;
    if (cJSON_IsNumber(v)) value = static_cast<int64_t>(v->valuedouble);
    else if (cJSON_IsString(v) && v->valuestring[0]) value = strtoll(v->valuestring, nullptr, 10);
    if (value > 1000000000000LL) value /= 1000;
    return value;
}

void paintValue(uint8_t index, const String &value) {
    if(index==1 && profileName) lv_label_set_text(profileName,value.c_str());
    if(index==2 && profileLevel) lv_label_set_text(profileLevel,value.c_str());
    if(index==3 && profileUid) lv_label_set_text(profileUid,value.c_str());
    if(index==4 && profileStatus) lv_label_set_text(profileStatus,value.c_str());
    if(index==8 && secretaryName){
        String name=value;
        name.trim();
        const unsigned length=name.length();
        if(length<=heroNameVerticalMax){
            // One glyph per line: no transform buffer needed.
            String column;
            for(unsigned i=0;i<length;++i){if(i)column+='\n';column+=name[i];}
            lv_label_set_text(secretaryName,column.c_str());
            const int n=static_cast<int>(length?length:1);
            const int band=heroNameBandBottom-heroNameBandTop;
            const int pitch=std::min(heroNamePitch,band/n);
            lv_obj_set_width(secretaryName,heroNameColumnWidth);
            lv_obj_set_pos(secretaryName,heroNameColumnX,heroNameBandTop+(band-n*pitch)/2);
            lv_label_set_long_mode(secretaryName,LV_LABEL_LONG_CLIP);
            lv_obj_set_style_text_align(secretaryName,LV_TEXT_ALIGN_CENTER,0);
            lv_obj_set_style_text_line_space(secretaryName,pitch-15,0);
            if(heroNameRule) lv_obj_clear_flag(heroNameRule,LV_OBJ_FLAG_HIDDEN);
            if(heroNameTickTop) lv_obj_clear_flag(heroNameTickTop,LV_OBJ_FLAG_HIDDEN);
            if(heroNameTickBottom) lv_obj_clear_flag(heroNameTickBottom,LV_OBJ_FLAG_HIDDEN);
            if(heroNameMark) lv_obj_add_flag(heroNameMark,LV_OBJ_FLAG_HIDDEN);
            if(heroNameWideRule) lv_obj_add_flag(heroNameWideRule,LV_OBJ_FLAG_HIDDEN);
        } else {
            // A long appellation reads badly as a letter column, so set it as a
            // right aligned block that still hugs the hero's left edge.
            String lines;
            if(length<=heroNameWideChars) lines=name;
            else {
                int cut=name.indexOf(' ',static_cast<int>(length/2));
                if(cut<0) cut=static_cast<int>(length/2);
                String head=name.substring(0,cut),tail=name.substring(cut);
                head.trim();
                tail.trim();
                lines=head;
                if(tail.length()){lines+='\n';lines+=tail;}
            }
            lv_label_set_text(secretaryName,lines.c_str());
            const int count=lines.indexOf('\n')>=0?2:1;
            lv_obj_set_width(secretaryName,heroNameWideWidth);
            lv_obj_set_pos(secretaryName,heroNameWideX,heroNameWideTop);
            lv_label_set_long_mode(secretaryName,LV_LABEL_LONG_CLIP);
            lv_obj_set_style_text_align(secretaryName,LV_TEXT_ALIGN_RIGHT,0);
            lv_obj_set_style_text_line_space(secretaryName,-2,0);
            if(heroNameRule) lv_obj_add_flag(heroNameRule,LV_OBJ_FLAG_HIDDEN);
            if(heroNameTickTop) lv_obj_add_flag(heroNameTickTop,LV_OBJ_FLAG_HIDDEN);
            if(heroNameTickBottom) lv_obj_add_flag(heroNameTickBottom,LV_OBJ_FLAG_HIDDEN);
            // Same tab language as the column, only laid on its side: a rule
            // under the block with a lime segment marking its right end.
            const int underline=heroNameWideTop+count*15+3;
            if(heroNameWideRule){
                lv_obj_clear_flag(heroNameWideRule,LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_y(heroNameWideRule,underline);
            }
            if(heroNameMark){
                lv_obj_clear_flag(heroNameMark,LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_pos(heroNameMark,heroNameWideX+heroNameWideWidth-16,underline);
            }
        }
    }
    if (index == 11 || index == 13) {
        const int slash = value.indexOf('/');
        const String current = slash < 0 ? value : value.substring(0, slash);
        lv_label_set_text(labels[index], current.c_str());
        lv_label_set_text(labels[index-1], slash < 0 ? "" : value.substring(slash).c_str());
    } else if (labels[index]) {
        lv_label_set_text(labels[index], value.c_str());
    }
    if (index >= 11 && index <= 19 && index % 2) {
        int current, maximum;
        int n=(index-11)/2;
        int length=n<2?132:missionMeterWidth;
        int fill=0;
        if(sscanf(value.c_str(),"%d/%d",&current,&maximum)==2 && maximum>0)
            fill=length*std::clamp(current,0,maximum)/maximum;
        lv_obj_set_width(meters[n],fill);
        if(n>=2) lv_obj_set_style_bg_color(meters[n],fill==length ? lime : cyan,0);
    }
}
void setValue(uint8_t index,const String &value) {
    hal.LOCKLV();
    values[index]=value;
    paintValue(index,value);
    hal.UNLOCKLV();
}
String clockText(int64_t ts){
    if(ts<=0)return "--:--";
    time_t local=ts+8*3600;
    tm t={};gmtime_r(&local,&t);
    char out[12];strftime(out,sizeof(out),"%H:%M",&t);
    return String(out);
}

int nonnegativeValue(cJSON *obj, const char *key, int fallback = -1) {
    int value = numberValue(obj, key, fallback);
    return value < 0 ? fallback : value;
}

int droneCount(cJSON *labor, int64_t now) {
    const int current = nonnegativeValue(labor, "value");
    const int maximum = nonnegativeValue(labor, "maxValue");
    const int64_t last = timestampValue(labor, "lastUpdateTime", 0);
    const int remaining = nonnegativeValue(labor, "remainSecs");
    if (current < 0 || maximum < 0 || current >= maximum || last <= 0 || remaining <= 0 || now < last)
        return current;
    const int64_t elapsed = now - last;
    const int64_t recovered = std::min<int64_t>(maximum - current, (maximum - current) * elapsed / remaining);
    return current + static_cast<int>(recovered);
}

void setPage(uint8_t next) {
    page = next % 2;
    lv_obj_add_flag(pages[1-page], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(pages[page], LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(pages[page]);
    if(page==0){
        lv_obj_add_flag(taskHeader,LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(avatarImage,LV_OBJ_FLAG_HIDDEN);
        for(int i=1;i<=4;i++)if(labels[i])lv_obj_add_flag(labels[i],LV_OBJ_FLAG_HIDDEN);
    }else{
        lv_obj_clear_flag(taskHeader,LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(avatarImage,LV_OBJ_FLAG_HIDDEN);
        for(int i=1;i<=4;i++)if(labels[i])lv_obj_clear_flag(labels[i],LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(taskHeader);
        lv_obj_move_foreground(avatarImage);
        for(int i=1;i<=4;i++)if(labels[i])lv_obj_move_foreground(labels[i]);
    }
}

// Only display fields are stored here. Authentication stays in SklandApi.
bool profileSnapshot(bool write) {
    const String uid=SklandApi::uid();
    if(uid.isEmpty()) return false;
    Preferences prefs;
    if(!prefs.begin("doctor-view",!write)) return false;
    bool ok=false;
    if(write){
        cJSON *doc=cJSON_CreateObject();
        if(doc){
            cJSON_AddStringToObject(doc,"uid",uid.c_str());
            auto *rows=cJSON_AddArrayToObject(doc,"rows");
            if(rows) for(const auto &v:values)cJSON_AddItemToArray(rows,cJSON_CreateString(v.c_str()));
            char *json=cJSON_PrintUnformatted(doc);
            if(json){ok=prefs.putString("snapshot",json)>0;cJSON_free(json);}
            cJSON_Delete(doc);
        }
    }else{
        String json=prefs.getString("snapshot","");
        cJSON *doc=cJSON_Parse(json.c_str());
        auto *rows=cJSON_GetObjectItem(doc,"rows");
        ok=valueText(doc,"uid","")==uid && cJSON_GetArraySize(rows)==24;
        for(int i=0;ok && i<24;i++)ok=cJSON_IsString(cJSON_GetArrayItem(rows,i));
        if(ok)for(int i=0;i<24;i++)values[i]=cJSON_GetArrayItem(rows,i)->valuestring;
        cJSON_Delete(doc);
    }
    prefs.end();
    return ok;
}

bool artFile(bool write,bool hero,String &url,lv_img_dsc_t &desc,uint8_t *&pixels){
    const char *path=hero?"/doctor-hero.bin":"/doctor-avatar.bin";
    const String uid=SklandApi::uid();
    if(uid.isEmpty())return false;
    const unsigned w=hero?portraitWidth:70,h=hero?portraitHeight:70;
    const size_t bytes=w*h*3;
    // v10: closer half-body crop plus a wider edge melt, so old art must be redrawn.
    const String key=uid+" "+String(w)+"x"+String(h)+(hero?" v12":"");
    File f=LittleFS.open(write?"/doctor-art.tmp":path,write?"w":"r");
    if(!f)return false;
    if(write){
        String header=key+"\n"+url+"\n";
        bool ok=f.print(header)==header.length() && f.write(pixels,bytes)==bytes;
        f.close();
        return ok && LittleFS.rename("/doctor-art.tmp",path);
    }
    String storedKey=f.readStringUntil('\n');
    String stored=f.readStringUntil('\n');
    unsigned oldW=w,oldH=h;
    if(storedKey!=key){
        if(!hero)return false;
        if(storedKey==uid+" 319x156 v3" || storedKey==uid+" 319x156 v4"){oldW=319;oldH=156;}
        else if(storedKey==uid+" 168x184 v5"){oldW=168;oldH=184;}
        else if(storedKey==uid+" 204x202 v6"){oldW=204;oldH=202;}
        else if(storedKey==uid+" 204x157 v7"){oldW=204;oldH=157;}
        else if(storedKey==uid+" 171x180"){oldW=171;oldH=180;}
        else if(storedKey==uid+" 224x215"){oldW=224;oldH=215;}
        else return false;
    }
    const size_t oldBytes=size_t(oldW)*oldH*3;
    if(stored.isEmpty() || f.size()-f.position()!=oldBytes){f.close();return false;}
    auto *out=(uint8_t*)ps_malloc(bytes);
    if(!out){f.close();return false;}
    if(oldW==w && oldH==h){
        if(f.read(out,bytes)!=bytes){free(out);f.close();return false;}
    }else{
        auto *old=(uint8_t*)ps_malloc(oldBytes);
        if(!old){free(out);f.close();return false;}
        if(f.read(old,oldBytes)!=oldBytes){free(old);free(out);f.close();return false;}
        memset(out,0,bytes);
        const float scale=std::min(float(h)/oldH,float(w)/oldW);
        const unsigned nw=std::min(w,unsigned(oldW*scale)),nh=std::min(h,unsigned(oldH*scale));
        const unsigned ox=w-nw,oy=h-nh;
        for(unsigned y=0;y<nh;y++)for(unsigned x=0;x<nw;x++)
            memcpy(out+3*((oy+y)*w+ox+x),old+3*((y*oldH/nh)*oldW+(x*oldW/nw)),3);
        free(old);
    }
    f.close();
    pixels=out;url=(storedKey==key)?stored:String();
    desc.header.cf=LV_IMG_CF_TRUE_COLOR_ALPHA;
    desc.header.w=w;desc.header.h=h;desc.data_size=bytes;desc.data=out;
    return true;
}
String asset(const char *kind,String id){
    id.replace("%","%25");id.replace("#","%23");id.replace("@","%40");id.replace(" ","%20");
    return String("https://web.hycdn.cn/arknights/game/assets/")+kind+"/"+id+".png";
}

void putCell(char *dst, size_t n, const String &s) {
    if (n) { snprintf(dst, n, "%s", s.c_str()); dst[n - 1] = 0; }
}

// Worker-side decode: download + decode + fit + dissolve into a raw RGBX buffer.
// Pure function, no LVGL and no shared state; the caller owns the returned buffer.
bool decodeArt(const String &url, unsigned width, unsigned height, bool hero,
               uint8_t *&outPixels, unsigned &outDw, unsigned &outDh) {
    outPixels = nullptr; outDw = 0; outDh = 0;
    uint8_t *png = nullptr; size_t size = 0;
    if (!SklandApi::downloadPortrait(url, &png, &size)) return false;
    unsigned w = 0, h = 0; unsigned char *rgba = nullptr;
    LodePNGState state; lodepng_state_init(&state);
    unsigned err = lodepng_inspect(&w, &h, &state, png, size);
    lodepng_state_cleanup(&state);
    if (!err && w && h && uint64_t(w) * h <= 1500000)
        err = lodepng_decode32(&rgba, &w, &h, png, size);
    free(png);
    if (err || !rgba) { free(rgba); return false; }
    unsigned left = w, top = h, right = 0, bottom = 0;
    for (unsigned y = 0; y < h; y++) for (unsigned x = 0; x < w; x++) if (rgba[(y * w + x) * 4 + 3] > 24) {
        left = std::min(left, x); right = std::max(right, x);
        top = std::min(top, y); bottom = std::max(bottom, y);
    }
    if (right < left || bottom < top) { free(rgba); return false; }
    unsigned sw = right - left + 1, sh = bottom - top + 1;
    if (hero) sh = std::max(1u, unsigned(float(sh) * heroSourceCrop));
    float scale = std::min(float(height) / sh, float(width) / sw);
    unsigned dw = std::clamp(unsigned(sw * scale), 1u, width);
    unsigned dh = std::clamp(unsigned(sh * scale), 1u, height);
    uint8_t *out = (uint8_t *)calloc(width * height, 3);
    if (!out) { free(rgba); return false; }
    for (unsigned y = 0; y < dh; y++) for (unsigned x = 0; x < dw; x++) {
        unsigned alpha = 0, red = 0, green = 0, blue = 0;
        for (unsigned j : {1u, 3u}) for (unsigned i : {1u, 3u}) {
            unsigned sx = left + (4 * x + i) * sw / (4 * dw), sy = top + (4 * y + j) * sh / (4 * dh);
            const uint8_t *src = rgba + 4 * (sy * w + sx);
            alpha += src[3]; red += src[0] * src[3]; green += src[1] * src[3]; blue += src[2] * src[3];
        }
        if (!alpha) continue;
        unsigned ox = (width - dw) / 2;
        uint8_t *dst = out + 3 * ((height - dh + y) * width + ox + x);
        unsigned red8 = red / alpha, green8 = green / alpha, blue8 = blue / alpha;
        lv_color_t color = lv_color_make(red8, green8, blue8);
        memcpy(dst, &color, 2);
        // The source PNG is already a clean cut-out, so the portrait keeps its own
        // silhouette; only the left edge gets a light ramp so the shoulder does not
        // butt against the scenery on a hard vertical line.
        unsigned cover = alpha / 4;
        if (hero && x < heroLeftFade) cover = cover * x / heroLeftFade;
        dst[2] = cover;
    }
    free(rgba);
    outPixels = out; outDw = dw; outDh = dh;
    return true;
}

void runProfileFetch(const ProfileRequest &req, ProfileResult &res) {
    memset(&res, 0, sizeof(res));
    snprintf(res.uid, sizeof(res.uid), "%s", req.uid);
    res.gen = req.gen;
    if (WiFi.status() != WL_CONNECTED) WiFi.begin();
    if (WiFi.status() != WL_CONNECTED || !SklandApi::loadCredentials()) {
        res.ok = false;
        putCell(res.vals[4], sizeof(res.vals[4]), WiFi.status() == WL_CONNECTED ? "请在设置中重新登录森空岛" : "等待 Wi-Fi 连接…");
        return;
    }
    cJSON *response = SklandApi::get("api/v1/game/player/info", "uid=" + SklandApi::uid());
    cJSON *data = response ? cJSON_GetObjectItem(response, "data") : nullptr;
    cJSON *status = data ? cJSON_GetObjectItem(data, "status") : nullptr;
    if (!cJSON_IsObject(status)) {
        String error = SklandApi::lastError();
        res.ok = false;
        putCell(res.vals[4], sizeof(res.vals[4]), error.isEmpty() ? "博士数据同步失败" : error);
        if (response) cJSON_Delete(response);
        return;
    }
    putCell(res.vals[1], sizeof(res.vals[1]), valueText(status, "name", "博士"));
    putCell(res.vals[2], sizeof(res.vals[2]), "LV " + valueText(status, "level", "—"));
    putCell(res.vals[3], sizeof(res.vals[3]), "UID  " + SklandApi::uid());
    cJSON *secretary = cJSON_GetObjectItem(status, "secretary");
    const String charId = valueText(secretary, "charId", "");
    const char *known = operator_name::appellationFor(charId.c_str());
    String code = known ? String(known) : String();
    if (code.isEmpty() && charId.length()) {
        code = charId;
        const int split = code.lastIndexOf('_');
        if (split >= 0) code = code.substring(split + 1);
        code.toUpperCase();
    }
    if (code.length() > 10) {
        const int space = code.indexOf(' ');
        code = space > 0 ? code.substring(0, space) : code.substring(0, 10);
    }
    if (code.length() > 10) code = code.substring(0, 10);
    putCell(res.vals[8], sizeof(res.vals[8]), code);
    cJSON *apObj = cJSON_GetObjectItem(status, "ap");
    cJSON *routine = cJSON_GetObjectItem(data, "routine");
    cJSON *daily = cJSON_GetObjectItem(routine, "daily");
    cJSON *weekly = cJSON_GetObjectItem(routine, "weekly");
    cJSON *campaign = cJSON_GetObjectItem(data, "campaign");
    cJSON *reward = cJSON_GetObjectItem(campaign, "reward");
    cJSON *building = cJSON_GetObjectItem(data, "building");
    cJSON *labor = cJSON_GetObjectItem(building, "labor");
    const int64_t storedAt = timestampValue(status, "storeTs", 0);
    const int64_t now = SklandApi::serverTimestamp();
    const int64_t dailyStart = ((now + 4 * 3600) / 86400) * 86400 - 4 * 3600;
    const int weekDay = static_cast<int>((((now + 4 * 3600) / 86400 + 4) % 7 + 7) % 7);
    const int64_t weeklyStart = dailyStart - ((weekDay + 6) % 7) * 86400LL;
    const bool dailyExpired = storedAt > 0 && storedAt < dailyStart;
    const bool weeklyExpired = storedAt > 0 && storedAt < weeklyStart;
    int apCurrent = numberValue(apObj, "current", -1), apMax = numberValue(apObj, "max", 0);
    const int64_t lastAp = timestampValue(apObj, "lastApAddTime", 0);
    if (apCurrent >= 0 && apMax > 0 && lastAp > 0 && now >= lastAp && apCurrent < apMax)
        apCurrent = min(apMax, apCurrent + static_cast<int>((now - lastAp) / 360));
    putCell(res.vals[11], sizeof(res.vals[11]), apCurrent >= 0 ? String(apCurrent) + "/" + String(apMax) : "—/" + String(apMax));
    const int drones = droneCount(labor, now);
    const int droneMax = nonnegativeValue(labor, "maxValue");
    putCell(res.vals[13], sizeof(res.vals[13]), drones >= 0 && droneMax >= 0 ? String(drones) + "/" + String(droneMax) : "—/—");
    putCell(res.vals[15], sizeof(res.vals[15]), (dailyExpired ? "0" : valueText(daily, "current")) + "/" + valueText(daily, "total"));
    putCell(res.vals[17], sizeof(res.vals[17]), (weeklyExpired ? "0" : valueText(weekly, "current")) + "/" + valueText(weekly, "total"));
    putCell(res.vals[19], sizeof(res.vals[19]), (weeklyExpired ? "0" : valueText(reward, "current")) + "/" + valueText(reward, "total"));
    putCell(res.vals[5], sizeof(res.vals[5]), valueText(status, "charCnt"));
    String mainStage = valueText(status, "mainStageProgress");
    if (mainStage.startsWith("main_")) mainStage.remove(0, 5);
    if (mainStage.isEmpty() || cJSON_IsNull(cJSON_GetObjectItem(status, "mainStageProgress"))) mainStage = "END";
    putCell(res.vals[6], sizeof(res.vals[6]), mainStage);
    cJSON *rogue = cJSON_GetObjectItem(data, "rogue");
    cJSON *records = cJSON_GetObjectItem(rogue, "records");
    int total = 0, count = 0;
    cJSON *item = nullptr;
    cJSON_ArrayForEach(item, records) {
        int level = numberValue(item, "bpLevel", -1);
        if (level >= 0) { total += level; count++; }
    }
    putCell(res.vals[7], sizeof(res.vals[7]), count ? String(total) : String("—"));
    putCell(res.vals[4], sizeof(res.vals[4]), "更新 " + clockText(now));
    auto *av = cJSON_GetObjectItem(status, "avatar");
    String avId = valueText(av, "id", ""), avUrl = valueText(av, "url", "");
    String avTarget = avUrl.length() ? avUrl : (avId.length() ? asset(valueText(av, "type", "") == "ICON" ? "avatar" : "char_skin/avatar", avId) : "");
    if (avTarget.length() && avTarget != req.avCached) {
        uint8_t *px = nullptr; unsigned dw = 0, dh = 0;
        if (decodeArt(avTarget, 70, 70, false, px, dw, dh)) {
            res.avReady = true; res.avPixels = px; snprintf(res.avUrl, sizeof(res.avUrl), "%s", avTarget.c_str());
        }
    }
    String skin = valueText(secretary, "skinId", ""), secId = valueText(secretary, "charId", "");
    String ptTarget = skin.length() ? asset("char_skin/portrait", skin) : (secId.length() ? asset("char/portrait", secId) : "");
    if (ptTarget.length() && ptTarget != req.ptCached) {
        uint8_t *px = nullptr; unsigned dw = 0, dh = 0;
        if (decodeArt(ptTarget, portraitWidth, portraitHeight, true, px, dw, dh)) {
            res.ptReady = true; res.ptPixels = px; snprintf(res.ptUrl, sizeof(res.ptUrl), "%s", ptTarget.c_str());
        }
    }
    String logName = valueText(status, "name", "");
    res.ok = true;
    if (response) cJSON_Delete(response);
    ESP_LOGI("SklandGeneral", "profile fetched name=%s ap=%s drones=%s", logName.c_str(), res.vals[11], res.vals[13]);
}

void queueProfileFetch() {
    ProfileRequest req = {};
    snprintf(req.uid, sizeof(req.uid), "%s", SklandApi::uid().c_str());
    req.gen = profileGeneration;
    snprintf(req.avCached, sizeof(req.avCached), "%s", avatarUrl.c_str());
    snprintf(req.ptCached, sizeof(req.ptCached), "%s", portraitUrl.c_str());
    portENTER_CRITICAL(&profileMux);
    memcpy(&profileReq, &req, sizeof(req));
    profileFetchWanted = true;
    portEXIT_CRITICAL(&profileMux);
}

void profileWorkerEntry(void *) {
    while (true) {
        ProfileRequest req = {};
        bool wanted = false;
        portENTER_CRITICAL(&profileMux);
        if (profileFetchWanted) { memcpy(&req, &profileReq, sizeof(req)); profileFetchWanted = false; profileWorkerBusy = true; wanted = true; }
        portEXIT_CRITICAL(&profileMux);
        if (!wanted) { vTaskDelay(pdMS_TO_TICKS(100)); continue; }
        ProfileResult res;
        runProfileFetch(req, res);
        uint8_t *discardAv = nullptr, *discardPt = nullptr;
        portENTER_CRITICAL(&profileMux);
        if (res.gen == profileGeneration) { memcpy(&pendingProfile, &res, sizeof(res)); profileResultReady = true; }
        else { discardAv = res.avPixels; discardPt = res.ptPixels; }
        profileWorkerBusy = false;
        portEXIT_CRITICAL(&profileMux);
        free(discardAv); free(discardPt);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

bool takeProfile(ProfileResult &out) {
    bool ready = false;
    portENTER_CRITICAL(&profileMux);
    if (profileResultReady) {
        memcpy(&out, &pendingProfile, sizeof(out));
        memset(&pendingProfile, 0, sizeof(pendingProfile));
        profileResultReady = false;
        ready = true;
    }
    portEXIT_CRITICAL(&profileMux);
    return ready;
}

void applyProfile(ProfileResult &out) {
    if (out.uid[0] && strcmp(out.uid, SklandApi::uid().c_str()) != 0) {
        free(out.avPixels); free(out.ptPixels); out.avPixels = out.ptPixels = nullptr;
        return;
    }
    hal.LOCKLV();
    if (out.ok) {
        for (int i = 0; i < 24; i++)
            if (out.vals[i][0]) { values[i] = out.vals[i]; paintValue(i, values[i]); }
        if (out.ptReady && portraitImage) {
            free(portraitBytes); portraitBytes = out.ptPixels; out.ptPixels = nullptr;
            portraitUrl = out.ptUrl;
            lv_img_cache_invalidate_src(&portraitDesc);
            portraitDesc.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
            portraitDesc.header.w = portraitWidth; portraitDesc.header.h = portraitHeight;
            portraitDesc.data_size = portraitWidth * portraitHeight * 3; portraitDesc.data = portraitBytes;
            lv_img_set_src(portraitImage, &portraitDesc);
        }
        if (out.avReady && avatarImage) {
            free(avatarBytes); avatarBytes = out.avPixels; out.avPixels = nullptr;
            avatarUrl = out.avUrl;
            lv_img_cache_invalidate_src(&avatarDesc);
            avatarDesc.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
            avatarDesc.header.w = 70; avatarDesc.header.h = 70;
            avatarDesc.data_size = 70 * 70 * 3; avatarDesc.data = avatarBytes;
            lv_img_set_src(avatarImage, &avatarDesc);
            if (profileAvatarImage) lv_img_set_src(profileAvatarImage, &avatarDesc);
        }
        fetched = true; retry = false; cachedUID = SklandApi::uid();
    } else {
        if (out.vals[4][0]) { values[4] = out.vals[4]; paintValue(4, values[4]); }
        retry = true;
    }
    hal.UNLOCKLV();
    free(out.avPixels); free(out.ptPixels); out.avPixels = out.ptPixels = nullptr;
    if (out.ok) {
        artFile(true, true, portraitUrl, portraitDesc, portraitBytes);
        artFile(true, false, avatarUrl, avatarDesc, avatarBytes);
        profileSnapshot(true);
    }
}
}
void AppSklandGeneral::setup() {
    root = _appScreen; page = 0;
    bool logged=SklandApi::loadCredentials();
    bool same=logged && fetched && cachedUID==SklandApi::uid();
    if(!same){
        for(auto &v:values)v="";
        free(avatarBytes);free(portraitBytes);
        avatarBytes=portraitBytes=nullptr;avatarDesc={};portraitDesc={};
        avatarUrl="";portraitUrl="";fetched=false;attempted=false;
        if(logged && profileSnapshot(false)){
            cachedUID=SklandApi::uid(); same=true;fetched=true;
            artFile(false,false,avatarUrl,avatarDesc,avatarBytes);
            artFile(false,true,portraitUrl,portraitDesc,portraitBytes);
            ESP_LOGI("DoctorFile","restored persistent snapshot avatar=%d hero=%d",!!avatarBytes,!!portraitBytes);
        }
    }
    hal.LOCKLV();
    lv_obj_set_style_bg_color(root, bg, 0);
    lv_obj_set_style_bg_grad_color(root,lv_color_hex(0x294752),0);
    lv_obj_set_style_bg_grad_dir(root,LV_GRAD_DIR_VER,0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    // The dossier used to bring its own interior scene. It now shares the same
    // weather scenery as every other page, so the device reads as one shell.
    atmosphereAttach(root, 0x081C2A, 96);
    auto *screen=root;
    auto icon=[](const lv_img_dsc_t *src,int x,int y){
        auto *img=lv_img_create(root);lv_img_set_src(img,src);lv_obj_set_pos(img,x,y);return img;
    };
    auto glyph=[&](const lv_img_dsc_t *src,int x,int y){
        auto *img=icon(src,x,y);
        lv_obj_set_style_img_recolor(img,pale,0);
        lv_obj_set_style_img_recolor_opa(img,LV_OPA_COVER,0);
    };
    taskHeader=rect(0,0,320,70,lv_color_hex(0x182F3C),LV_OPA_COVER);
    lv_obj_set_style_bg_grad_color(taskHeader,lv_color_hex(0x304B54),0);
    lv_obj_set_style_bg_grad_dir(taskHeader,LV_GRAD_DIR_HOR,0);
    lv_obj_t *headerRule=rect(0,68,320,2,cyan,LV_OPA_70);
    lv_obj_t *headerTick=rect(0,68,43,2,lime,LV_OPA_COVER);
    // Keep the rule inside the header so it disappears with it on the dossier page.
    lv_obj_set_parent(headerRule,taskHeader);
    lv_obj_set_parent(headerTick,taskHeader);
    rect(5,4,62,62,lv_color_hex(0x1A4251),LV_OPA_COVER);
    avatarImage=icon(avatarBytes?&avatarDesc:&defaultDoctorIcon,1,0);
    lv_img_set_zoom(avatarImage,216);
    labels[1]=label("博士",74,9,154,pale);
    labels[3]=label("UID  —",74,37,125,muted);
    labels[2]=label("LV —",240,5,70,cyan);
    lv_obj_set_style_text_align(labels[2],LV_TEXT_ALIGN_RIGHT,0);
    labels[4]=label("同步中",201,37,111,muted);
    lv_obj_set_style_text_align(labels[4],LV_TEXT_ALIGN_RIGHT,0);
    
    for(int i=0;i<2;i++){
        // Transparent bodies: the shared backdrop has to stay visible between
        // the identity bar, the hero plate and the stats strip.
        auto *body=rect(0,i==0?0:70,320,i==0?240:170,bg,LV_OPA_TRANSP);
        pages[i]=body;
        lv_obj_clear_flag(body,LV_OBJ_FLAG_SCROLLABLE);
    }
    root=pages[0];
    // A soft scrim keeps the hero plate and the identity column readable on the
    // bright daytime scenes; the shared scenery still shows through everywhere.
    rect(0,0,320,174,lv_color_hex(0x0A1E2A),70);
    portraitImage=lv_img_create(root);
    if(portraitBytes) lv_img_set_src(portraitImage,&portraitDesc);
    lv_img_set_zoom(portraitImage,portraitZoom);
    lv_obj_set_pos(portraitImage,portraitX,portraitY);

    // The api avatar is a hard square; clip it inside a rounded frame so it reads
    // as a portrait chip instead of a pasted snapshot.
    lv_obj_t *avatarFrame=lv_obj_create(root);
    lv_obj_remove_style_all(avatarFrame);
    lv_obj_set_pos(avatarFrame,10,10);
    lv_obj_set_size(avatarFrame,70,70);
    lv_obj_set_style_radius(avatarFrame,10,0);
    lv_obj_set_style_clip_corner(avatarFrame,true,0);
    profileAvatarImage=lv_img_create(avatarFrame);
    lv_img_set_src(profileAvatarImage,avatarBytes?&avatarDesc:&defaultDoctorIcon);
    lv_img_set_zoom(profileAvatarImage,256);
    profileLevel=label("LV —",89,52,60,lv_color_hex(0x93A7AE),&font_rajdhani_16);
    rect(10,85,30,2,lime);
    // Player names run long ("安洁莉娜的创想#2673"); allow two lines and give
    // the level its own baseline instead of letting a wrapped name collide with it.
    profileName=label("博士",89,14,150,pale);
    lv_obj_set_height(profileName,30);
    lv_label_set_long_mode(profileName,LV_LABEL_LONG_DOT);
    profileUid=label("UID -",10,96,160,lv_color_hex(0xC3D3D8),&font_rajdhani_16);
    profileStatus=label("等待 Wi-Fi 连接",10,118,165,lv_color_hex(0x93A7AE));
    // Appellation column: the rule and its end ticks make it read as a label tab
    // hanging off the hero instead of loose text floating in the scene. The rule
    // stays desaturated so cyan keeps meaning "in progress" everywhere else.
    heroNameRule=rect(heroNameRuleX,heroNameBandTop,1,heroNameBandBottom-heroNameBandTop,lv_color_hex(0x567D8A),190);
    heroNameTickTop=rect(heroNameRuleX-2,heroNameBandTop,4,3,lime,215);
    heroNameTickBottom=rect(heroNameRuleX-2,heroNameBandBottom-3,4,3,lime,120);
    secretaryName=label("",heroNameColumnX,heroNameBandTop,heroNameColumnWidth,lv_color_hex(0xE8F4F7),&font_rajdhani_16);
    lv_label_set_long_mode(secretaryName,LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(secretaryName,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_line_space(secretaryName,heroNamePitch-15,0);
    lv_obj_set_style_text_opa(secretaryName,190,0);
    // Shown only when the appellation is set as a wide block (see paintValue).
    heroNameWideRule=rect(heroNameWideX,heroNameWideTop+18,heroNameWideWidth,1,lv_color_hex(0x567D8A),190);
    lv_obj_add_flag(heroNameWideRule,LV_OBJ_FLAG_HIDDEN);
    heroNameMark=rect(heroNameWideX+heroNameWideWidth-16,heroNameWideTop+18,16,2,lime,200);
    lv_obj_add_flag(heroNameMark,LV_OBJ_FLAG_HIDDEN);

    // The stats strip is a printed block laid over the photo: a hard dark band
    // with a bright hairline on top, instead of fading into the scenery.
    for(int i=0;i<4;++i)
        rect(0,162+3*i,320,3,lv_color_hex(0xBCBCB5),static_cast<uint8_t>(46+32*i));
    // The printed strip is a straight band, not a rounded card.
    lv_obj_set_style_radius(rect(0,174,320,66,lv_color_hex(0xBCBCB5),LV_OPA_COVER),0,0);
    const char *titles[]={"干员总数","主线进度","集成战略"};
    for(int i=0;i<3;++i){
        const int x=13+105*i;
        if(i) rect(x-9,185,1,43,lv_color_hex(0x93AFB3));
        label(titles[i],x,180,94,lv_color_hex(0x365763));
        labels[i+5]=label("—",x,199,94,lv_color_hex(0x132F40),&font_rajdhani_32);
    }
    root=pages[1];
    lv_obj_set_style_bg_color(pages[1],lv_color_hex(0x1B3542),0);
    lv_obj_set_style_bg_grad_color(pages[1],lv_color_hex(0x2B4854),0);
    lv_obj_set_style_bg_grad_dir(pages[1],LV_GRAD_DIR_VER,0);
    lv_obj_set_style_bg_opa(pages[1],LV_OPA_60,0);
    const char *fields[]={"理智","无人机","每日任务","每周任务","每周剿灭"};
    for(int i=0;i<2;i++){
        int x=7+158*i;
        auto *card=rect(x,4,151,62,lv_color_hex(0x2C4954),LV_OPA_90);
        lv_obj_set_style_border_width(card,1,0);
        lv_obj_set_style_border_color(card,lv_color_hex(0x587A82),0);
        rect(x,4,151,2,i==0?cyan:lime,LV_OPA_COVER);
        glyph(i==0?&boltGlyph:&droneGlyph,x+7,21);
        label(fields[i],x+44,7,99,muted);
        labels[11+2*i]=label("—",x+39,23,54,pale,&font_rajdhani_32);
        lv_obj_set_style_text_align(labels[11+2*i],LV_TEXT_ALIGN_RIGHT,0);
        labels[10+2*i]=label("",x+94,33,46,muted);
        rect(x+7,60,132,3,lv_color_hex(0x607175));
        meters[i]=rect(x+7,60,0,3,cyan);
    }
    const lv_img_dsc_t *missionIcons[] = {&dayGlyph, &weekGlyph, &raidGlyph};
    rect(7,68,306,100,lv_color_hex(0x263D49));
    for(int i=2;i<5;++i){
        const int row = i-2;
        const int y = 68+33*row;
        if(row) rect(17,y,286,1,lv_color_hex(0x47606B));
        // All three masks are 32x32 alpha glyphs: native size keeps the stroke
        // weight identical, so no per-row scale or offset tweaks are needed.
        glyph(missionIcons[row],12,y+1);
        label(fields[i],50,y+7,79,pale);
        rect(129,y+15,missionMeterWidth,4,lv_color_hex(0x58717B));
        meters[i]=rect(129,y+15,0,4,cyan);
        labels[11+2*i]=label("—",204,y+7,100,pale);
        lv_obj_set_style_text_align(labels[11+2*i],LV_TEXT_ALIGN_RIGHT,0);
    }
    root=screen;
    // Page order is arranged in setPage() so the identity bar stays above it.
    if(same){
        for(unsigned i=0;i<24;i++)if(values[i].length())paintValue(i,values[i]);
        ESP_LOGI("DoctorFile","restored cached dossier and artwork");
    }
    setPage(appManagerLite.startDaily ? 1 : 0);
    appManagerLite.startDaily = false;
    hal.UNLOCKLV();
}

void AppSklandGeneral::loop() {
    ProfileResult out;
    if (takeProfile(out)) {
        applyProfile(out);
        lastFetch = millis();
    }
    if (!attempted || millis() - lastFetch >= (retry ? 30000u : kProfileFetchIntervalMs)) {
        if (!profileWorkerBusy) {
            attempted = true;
            retry = true;
            lastFetch = millis();
            if (!profileWorker)
                xTaskCreatePinnedToCore(profileWorkerEntry, "doctor_net", 10240, nullptr, 2, &profileWorker, 0);
            queueProfileFetch();
        }
    }
    if (root) {
        hal.LOCKLV();
        atmosphereRefresh();
        hal.UNLOCKLV();
    }
    vTaskDelay(pdMS_TO_TICKS(40));
}

bool AppSklandGeneral::nextPage() {
    if (page == 0) { hal.LOCKLV(); setPage(1); hal.UNLOCKLV(); return true; }
    return false;
}

void AppSklandGeneral::destroy() {
    atmosphereDetach();
    uint8_t *staleAv = nullptr, *stalePt = nullptr;
    portENTER_CRITICAL(&profileMux);
    profileGeneration = profileGeneration + 1;
    profileFetchWanted = false;
    if (profileResultReady) {
        staleAv = pendingProfile.avPixels;
        stalePt = pendingProfile.ptPixels;
        memset(&pendingProfile, 0, sizeof(pendingProfile));
        profileResultReady = false;
    }
    portEXIT_CRITICAL(&profileMux);
    free(staleAv); free(stalePt);
    root = nullptr; portraitImage = nullptr; avatarImage = nullptr; 
    secretaryName = nullptr;
    heroNameRule = nullptr;
    heroNameTickTop = nullptr;
    heroNameTickBottom = nullptr;
    heroNameMark = nullptr;
    heroNameWideRule = nullptr;
    for (auto &item : labels) item = nullptr;
    hal.LOCKLV(); lv_img_cache_invalidate_src(&portraitDesc);lv_img_cache_invalidate_src(&avatarDesc);
    hal.UNLOCKLV();
}

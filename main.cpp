// 雷霆战机 (Thunder Fighter) - Win32 Game
// 编译: g++ -o ThunderFighter.exe main.cpp -lgdi32 -lwinmm -static
// 或使用 MSVC: cl main.cpp user32.lib gdi32.lib winmm.lib

#define UNICODE
#define _UNICODE
#include <windows.h>
#include <mmsystem.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <cwchar>
#include <fstream>

// ===================== 游戏常量 =====================
const int SCREEN_WIDTH = 480;
const int SCREEN_HEIGHT = 720;
const int PLAYER_WIDTH = 40;
const int PLAYER_HEIGHT = 50;
const int BULLET_WIDTH = 6;
const int BULLET_HEIGHT = 16;
const int ENEMY_SMALL_W = 32;
const int ENEMY_SMALL_H = 32;
const int ENEMY_MEDIUM_W = 44;
const int ENEMY_MEDIUM_H = 44;
const int ENEMY_BOSS_W = 80;
const int ENEMY_BOSS_H = 64;
const int MAX_BULLETS = 80;
const int MAX_ENEMIES = 32;
const int MAX_PARTICLES = 220;
const int MAX_POWERUPS = 6;
const int MAX_FLOAT_TEXTS = 40;
const int MAX_CHARGE = 120;      // 满蓄力帧数 (约2秒)
const int CHARGE_THRESHOLD = 75; // 超过该值松开空格释放激光

// 武器类型
const int WEAPON_MG = 0;      // 机枪（默认）
const int WEAPON_SCATTER = 1; // 散射
const int WEAPON_HOMING = 2;  // 追踪导弹
const int WEAPON_LASER = 3;   // 激光

// 游戏模式
const int GAMEMODE_STORY = 0;    // 剧情模式
const int GAMEMODE_SURVIVAL = 1; // 生存模式
const int GAMEMODE_BOSSRUSH = 2; // Boss Rush

// 机体（达成成就解锁新机体）
const int MAX_SHIPS = 3;
const int SHIP_STANDARD = 0; // 标准战机（默认）
const int SHIP_HEAVY = 1;    // 重装战机（解锁：初出茅庐）
const int SHIP_NIMBLE = 2;   // 疾风战机（解锁：弹幕舞者）

// 成就
const int MAX_ACHIEVEMENTS = 3;
const int ACH_KILLER = 0; // 初出茅庐：累计击毁100架敌机
const int ACH_CLEAN = 1;  // 无伤通关：某关不受伤
const int ACH_DANCER = 2; // 弹幕舞者：50发敌弹中存活10秒

// 环形护盾弹（限时道具）
const int ORBIT_TIME = 480;  // 8秒
const int ORBIT_COUNT = 3;   // 环绕弹数量

// 生存模式难度递增速度（帧）
const int SURVIVAL_DIFF = 450;

// 玩家移动参数（加速度 + 惯性）
const float PLAYER_ACCEL = 0.55f;
const float PLAYER_MAX_SPEED = 6.0f;

// ===================== 游戏状态枚举 =====================
enum GameState {
    STATE_MENU,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAMEOVER
};

// ===================== 游戏对象结构 =====================
struct Vector2 {
    float x, y;
    Vector2() : x(0), y(0) {}
    Vector2(float _x, float _y) : x(_x), y(_y) {}
};

struct Bullet {
    Vector2 pos;
    Vector2 vel;
    bool active;
    bool isEnemy;
    int width, height;
    int dmg;         // 伤害
    bool pierce;     // 穿透（激光）
    bool homing;     // 追踪
    int pierceTimer; // 穿透冷却（防止同一敌人每帧被扣血）
    
    Bullet() : pos(0,0), vel(0,0), active(false), isEnemy(false), width(BULLET_WIDTH), height(BULLET_HEIGHT),
               dmg(1), pierce(false), homing(false), pierceTimer(0) {}
};

struct Enemy {
    Vector2 pos;
    int type; // 0=small, 1=medium, 2=boss, 3=kamikaze, 4=shield(护盾机), 5=summoner(召唤机)
    int hp;
    int maxHp;
    int width, height;
    bool active;
    int fireTimer; // 敌机射击计时器
    int movePattern; // 移动模式
    float startX;
    int pattern;    // Boss 弹幕模式 0=散射 1=螺旋 2=召唤 3=多阶段 4=突击
    float angle;    // 螺旋弹幕角度
    int pierceCooldown; // 被穿透激光命中后的个体冷却
    int hitFlash;   // 受击闪白剩余帧数
    int shieldHp;   // 护盾机能量盾血量（0=无盾）
    int summonTimer;// 召唤机/召唤Boss召唤计时
    bool phase2;    // 多阶段Boss进入第二形态
    int formId;     // 编队组ID（-1=非编队）
    int formSlot;   // 编队槽位 0-4（-1=非编队）
    float formOffsetX, formOffsetY; // 编队相对偏移
    
    Enemy() : pos(0,0), type(0), hp(1), maxHp(1), width(0), height(0), active(false),
              fireTimer(0), movePattern(0), startX(0), pattern(0), angle(0), pierceCooldown(0),
              hitFlash(0), shieldHp(0), summonTimer(0), phase2(false), formId(-1), formSlot(-1),
              formOffsetX(0), formOffsetY(0) {}
};

struct Particle {
    Vector2 pos;
    Vector2 vel;
    int life;
    int maxLife;
    int color;
    bool active;
    int size;
    int type; // 0=粒子点 1=环形冲击波
    
    Particle() : pos(0,0), vel(0,0), life(0), maxLife(0), color(RGB(255,255,0)), active(false),
                 size(3), type(0) {}
};

struct PowerUp {
    Vector2 pos;
    int type; // 0=火力提升, 1=护盾, 2=炸弹, 3=更换武器, 4=轨道盾
    bool active;
    
    PowerUp() : pos(0,0), type(0), active(false) {}
};

struct FloatingText {
    Vector2 pos;
    wchar_t text[32];
    int life, maxLife;
    COLORREF color;
    int size;
    bool active;
    
    FloatingText() : pos(0,0), life(0), maxLife(0), color(RGB(255,255,255)), size(18), active(false) {}
};

// ===================== 音频合成工具 =====================
// 所有音效与 BGM 均由代码实时合成，经软件混音器(44.1kHz 立体声)播放：
// 多音轨可叠加不再互相打断，支持扫频、包络与左右声像定位
static const double SND_PI2 = 6.28318530718;
static const int AUD_SR = 44100;       // 采样率
static const int AUD_FRAMES = 1024;    // 每块缓冲帧数 (~23ms，低延迟)
static const int AUD_BLOCKS = 3;       // 循环缓冲块数
static const int AUD_MAX_VOICES = 24;  // 最大同时音效层数

static inline unsigned int SndXorshift(unsigned int* s) {
    unsigned int x = *s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *s = x;
    return x;
}
static inline double SndNoise(unsigned int* s) {
    return (double)SndXorshift(s) / 2147483648.0 - 1.0;   // [-1, 1)
}
static inline double MidiFreq(int m) {
    return 440.0 * std::pow(2.0, (m - 69) / 12.0);
}

// 往 BGM 浮点缓冲写入一个音符（支持指数扫频/颤音）
static void BakeNote(std::vector<float>& buf, double startSec, double durSec,
                     double f0, double f1, double vol, int wave,
                     double atk, double dpow, double vibHz = 0.0, double vibAmt = 0.0) {
    int start = (int)(startSec * AUD_SR);
    int n = (int)(durSec * AUD_SR);
    for (int i = 0; i < n; i++) {
        int idx = start + i;
        if (idx < 0 || idx >= (int)buf.size()) break;
        double t = (double)i / AUD_SR;
        double env;
        if (t < atk) env = t / atk;
        else {
            double d = (t - atk) / std::max(1e-4, durSec - atk);
            env = std::pow(std::max(0.0, 1.0 - d), dpow);
        }
        double u = (durSec > 0) ? t / durSec : 0.0;
        double freq = (f1 == f0) ? f0 : f0 * std::pow(f1 / f0, u);
        double s;
        if (wave == 0) {
            double ph = SND_PI2 * freq * t;
            if (vibAmt > 0) ph += vibAmt * std::sin(SND_PI2 * vibHz * t);
            s = std::sin(ph);
        } else if (wave == 1) {
            s = (std::sin(SND_PI2 * freq * t) > 0.0) ? 1.0 : -1.0;
        } else if (wave == 2) {   // 锯齿
            double frac = freq * t - std::floor(freq * t);
            s = 2.0 * frac - 1.0;
        } else {                  // 三角波
            double frac = freq * t - std::floor(freq * t);
            s = 4.0 * std::fabs(frac - 0.5) - 1.0;
        }
        buf[idx] += (float)(s * env * vol);
    }
}

static void BakeNoise(std::vector<float>& buf, double startSec, double durSec, double vol, double dpow = 2.0) {
    int start = (int)(startSec * AUD_SR);
    int n = (int)(durSec * AUD_SR);
    unsigned int seed = 0x9e3779b9u;
    for (int i = 0; i < n; i++) {
        int idx = start + i;
        if (idx < 0 || idx >= (int)buf.size()) break;
        double t = (double)i / AUD_SR;
        double d = t / std::max(1e-4, durSec);
        double env = std::pow(std::max(0.0, 1.0 - d), dpow);
        buf[idx] += (float)(SndNoise(&seed) * env * vol);
    }
}

// 底鼓：正弦下扫 + 极短噪声起音
static void BakeKick(std::vector<float>& buf, double t, double vol) {
    BakeNote(buf, t, 0.10, 160.0, 40.0, vol, 0, 0.001, 2.2);
    BakeNoise(buf, t, 0.012, vol * 0.25, 1.0);
}
// 军鼓：噪声 + 短音身
static void BakeSnare(std::vector<float>& buf, double t, double vol) {
    BakeNoise(buf, t, 0.09, vol, 1.8);
    BakeNote(buf, t, 0.06, 195.0, 140.0, vol * 0.5, 0, 0.001, 1.5);
}

// 实时音效的"发声层"配方
struct SfxLayer {
    int   wave;     // 0正弦 1方波 2锯齿 3三角 4噪声
    double f0, f1;  // 起止频率（指数扫频，噪声忽略）
    double dur;     // 时长(秒)
    double vol;     // 音量
    double atk;     // 起音(秒)
    double dpow;    // 衰减指数（越大收得越快）
    double delay;   // 延迟触发(秒)
};

// 音效编号
enum {
    SFX_SHOOT = 0, SFX_LASER, SFX_HOMING, SFX_EXPLO, SFX_BIGEXP, SFX_HIT,
    SFX_POWERUP, SFX_BOMB, SFX_BOSS, SFX_CHARGEFULL, SFX_ZAP, SFX_COMBO,
    SFX_ACHV, SFX_SHIELD, SFX_CLEAR, SFX_COUNT
};

// 每个音效最多 5 层叠加（未填的层全 0 自动跳过）
static const SfxLayer SFX_TABLE[SFX_COUNT][5] = {
    // SHOOT 机枪/散射：短促方波
    { { 1, 950.0, 420.0, 0.06, 0.10, 0.002, 1.6 } },
    // LASER 激光
    { { 2, 1600.0, 240.0, 0.09, 0.13, 0.002, 1.8 } },
    // HOMING 导弹发射
    { { 0, 480.0, 980.0, 0.09, 0.09, 0.004, 1.5 } },
    // EXPLO 小爆炸：噪声 + 低音砰
    { { 4, 0, 0, 0.28, 0.32, 0.001, 2.2 },
      { 0, 210.0, 52.0, 0.24, 0.34, 0.001, 1.8 } },
    // BIGEXP 大爆炸
    { { 4, 0, 0, 0.55, 0.42, 0.001, 2.0 },
      { 0, 130.0, 34.0, 0.50, 0.50, 0.001, 1.8 },
      { 1, 55.0, 40.0, 0.30, 0.14, 0.001, 1.5 } },
    // HIT 受伤
    { { 2, 330.0, 70.0, 0.22, 0.36, 0.002, 1.4 },
      { 4, 0, 0, 0.14, 0.16, 0.001, 2.0 } },
    // POWERUP 道具三连音
    { { 0, 660.0, 660.0, 0.09, 0.20, 0.004, 1.5 },
      { 0, 880.0, 880.0, 0.09, 0.20, 0.004, 1.5, 0.07 },
      { 0, 1320.0, 1320.0, 0.14, 0.20, 0.004, 1.6, 0.14 } },
    // BOMB 清屏炸弹
    { { 4, 0, 0, 0.90, 0.52, 0.001, 1.6 },
      { 0, 90.0, 26.0, 0.85, 0.55, 0.001, 1.7 },
      { 2, 60.0, 30.0, 0.50, 0.18, 0.001, 1.5 } },
    // BOSS 警报（双音 klaxon 两组）
    { { 1, 196.0, 196.0, 0.22, 0.24, 0.004, 0.8 },
      { 1, 147.0, 147.0, 0.22, 0.24, 0.004, 0.8, 0.24 },
      { 1, 196.0, 196.0, 0.22, 0.18, 0.004, 0.8, 0.90 },
      { 1, 147.0, 147.0, 0.22, 0.18, 0.004, 0.8, 1.14 } },
    // CHARGEFULL 蓄力完成
    { { 0, 620.0, 1650.0, 0.22, 0.24, 0.004, 1.4 } },
    // ZAP 蓄力激光释放
    { { 2, 1900.0, 140.0, 0.30, 0.30, 0.002, 2.0 },
      { 4, 0, 0, 0.18, 0.16, 0.001, 2.0 } },
    // COMBO 连击
    { { 0, 1046.5, 1046.5, 0.09, 0.20, 0.003, 1.4 },
      { 0, 1318.5, 1318.5, 0.16, 0.20, 0.003, 1.6, 0.08 } },
    // ACHV 成就解锁小号角
    { { 0, 523.25, 523.25, 0.10, 0.18, 0.004, 1.3 },
      { 0, 659.25, 659.25, 0.10, 0.18, 0.004, 1.3, 0.10 },
      { 0, 783.99, 783.99, 0.10, 0.18, 0.004, 1.3, 0.20 },
      { 0, 1046.5, 1046.5, 0.22, 0.20, 0.004, 1.5, 0.30 } },
    // SHIELD 护盾击破
    { { 0, 1500.0, 420.0, 0.18, 0.22, 0.002, 1.8 },
      { 0, 2200.0, 900.0, 0.12, 0.12, 0.002, 2.0, 0.02 } },
    // CLEAR 过关
    { { 0, 783.99, 783.99, 0.10, 0.16, 0.004, 1.4 },
      { 0, 1174.7, 1174.7, 0.20, 0.16, 0.004, 1.6, 0.10 } },
};

// 实时混音的单个发声体
struct SndVoice {
    bool active;
    int wave;
    double f0, f1, t, dur, vol, atk, dpow, phase;
    double delay;      // 剩余延迟(秒)
    double gl, gr;     // 左右声道增益
    unsigned int seed;
    SndVoice() : active(false), wave(0), f0(0), f1(0), t(0), dur(0), vol(0),
                 atk(0), dpow(1), phase(0), delay(0), gl(1), gr(1), seed(1) {}
};

// ===================== 游戏主类 =====================
class ThunderFighter {
private:
    // 玩家属性
    Vector2 playerPos;
    int playerLives;
    int playerLevel; // 火力等级 0-3
    int score;
    int combo;
    bool shieldActive;
    int shieldTimer;
    int invincibleTimer;
    int bombCount;
    int shootTimer;
    
    // 游戏对象
    std::vector<Bullet> playerBullets;
    std::vector<Bullet> enemyBullets;
    std::vector<Enemy> enemies;
    std::vector<Particle> particles;
    std::vector<PowerUp> powerups;
    
    // 背景星星
    struct Star {
        int x, y;
        int speed;
        int brightness;
        int layer; // 0=远景 1=中景 2=近景
        float phase; // 闪烁相位
    };
    std::vector<Star> stars;
    
    // 游戏状态
    GameState state;
    int frameCount;
    int uiTick;   // UI 动画计数（菜单/结算界面也递增）
    int enemySpawnTimer;
    int spawnRate;
    int difficulty;
    int stage;
    int enemiesSpawnedThisStage;
    int enemiesKilledThisStage;
    int enemiesToSpawnThisStage;
    int stageClearTimer;
    bool bossStage;
    bool bossSpawned;
    int highScore;
    HDC hdcBuffer;
    HBITMAP hbmBuffer;
    HDC hdcMem;              // 共享精灵DC（辉光贴图用）
    HDC hdcBg;               // 预渲染背景
    HBITMAP hbmBg;
    HINSTANCE hInst;
    HWND hWnd;
    RECT clientRect;
    
    // 按键状态
    bool keyLeft, keyRight, keyUp, keyDown, keySpace;
    
    // 新增：惯性移动
    float pvx, pvy;
    
    // 新增：武器与蓄力
    int weapon;
    int chargeAmount;
    
    // 新增：打击感
    int slowmoTimer;   // 慢动作（炸弹/击杀Boss）
    int shakeTimer, shakePower; // 屏幕震动
    int flashTimer, flashMax;   // 白屏闪烁
    int bossWarningTimer;       // Boss 警告横幅
    std::vector<FloatingText> floatTexts; // 伤害/得分飘字
    
    // 新增：固定时间步长（QueryPerformanceCounter）
    LARGE_INTEGER tickFreq, lastTick;
    bool tickInit;
    double acc;
    
    // 音频：waveOut 软件混音器
    HWAVEOUT hWaveOut;
    WAVEHDR waveHdrs[AUD_BLOCKS];
    short* mixData[AUD_BLOCKS];
    SndVoice voices[AUD_MAX_VOICES];
    CRITICAL_SECTION audioCS;
    bool audioOpen;
    std::vector<float> bgmBuf;  // 预合成的 BGM 采样
    int bgmCursor;
    bool bgmOn;

    // 视觉：缓存资源（字体/辉光贴图，避免每帧创建 GDI 对象）
    std::vector<std::pair<int, HFONT>> fontCache;
    std::vector<std::pair<COLORREF, HBITMAP>> glowCache;
    
    // 新增：模式/机体/成就
    int gameMode;           // 当前游戏模式
    int selectedMode;       // 菜单中选中的模式
    int selectedShip;       // 菜单中选中的机体
    int shipLives[MAX_SHIPS], shipBombs[MAX_SHIPS], shipLevel[MAX_SHIPS];
    float shipSpeed[MAX_SHIPS];
    bool shipUnlocked[MAX_SHIPS];
    const wchar_t* shipNames[MAX_SHIPS];
    COLORREF shipColor[MAX_SHIPS], shipColor2[MAX_SHIPS];
    COLORREF pColor, pColor2; // 当前机体配色
    float pSpeedMul;          // 当前机体速度倍率
    bool achUnlocked[MAX_ACHIEVEMENTS];
    int totalKills;         // 累计击毁数（跨局累计）
    bool stageNoDamage;     // 本关无伤标记
    int bulletDancerTimer;  // 弹幕舞者计数
    int achPopupTimer;      // 成就弹窗计时
    int achPopupIndex;      // 弹窗成就ID
    int orbitTimer;         // 轨道盾剩余时间
    int orbitCount;         // 轨道弹数量
    int survivalFrames;     // 生存模式存活帧数
    int bossRushIndex;      // BossRush 已击败数
    int stageIntroTimer;    // 关卡剧情文字计时
    wchar_t stageIntroText[64];
    
    // 颜色常量
    static constexpr COLORREF COLOR_PLAYER = RGB(0, 180, 255);
    static constexpr COLORREF COLOR_PLAYER2 = RGB(0, 120, 255);
    static constexpr COLORREF COLOR_SHIELD = RGB(0, 255, 200);
    
public:
    ThunderFighter(HINSTANCE hInstance, HWND hwnd) : hInst(hInstance), hWnd(hwnd) {
        Init();
        InitAudio();
    }
    
    ~ThunderFighter() {
        ShutdownAudio();
        for (auto& f : fontCache) DeleteObject(f.second);
        for (auto& g : glowCache) if (g.second) DeleteObject(g.second);
        if (hbmBg) DeleteObject(hbmBg);
        if (hdcBg) DeleteDC(hdcBg);
        if (hbmBuffer) DeleteObject(hbmBuffer);
        if (hdcBuffer) DeleteDC(hdcBuffer);
        if (hdcMem) DeleteDC(hdcMem);
    }
    
    void Init() {
        srand((unsigned int)time(NULL));
        highScore = 0;
        LoadHighScore();
        
        // 新增成员初始化
        pvx = pvy = 0.0f;
        weapon = WEAPON_MG;
        chargeAmount = 0;
        slowmoTimer = 0;
        shakeTimer = shakePower = 0;
        flashTimer = flashMax = 0;
        bossWarningTimer = 0;
        tickInit = false;
        acc = 0.0;
        hWaveOut = NULL;
        audioOpen = false;
        bgmOn = false;
        bgmCursor = 0;
        for (int i = 0; i < AUD_BLOCKS; i++) mixData[i] = NULL;
        hdcBg = NULL;
        hbmBg = NULL;
        
        // 模式/机体/成就初始化
        selectedMode = GAMEMODE_STORY;
        selectedShip = SHIP_STANDARD;
        gameMode = GAMEMODE_STORY;
        totalKills = 0;
        bulletDancerTimer = 0;
        achPopupTimer = 0;
        achPopupIndex = 0;
        orbitTimer = 0;
        orbitCount = 0;
        survivalFrames = 0;
        bossRushIndex = 0;
        stageIntroTimer = 0;
        stageNoDamage = true;
        LoadSave();
        pColor = shipColor[0];
        pColor2 = shipColor2[0];
        pSpeedMul = 1.0f;
        
        GetClientRect(hWnd, &clientRect);

        // 创建双缓冲
        HDC hdc = GetDC(hWnd);
        hdcBuffer = CreateCompatibleDC(hdc);
        hbmBuffer = CreateCompatibleBitmap(hdc, SCREEN_WIDTH, SCREEN_HEIGHT);
        SelectObject(hdcBuffer, hbmBuffer);
        SetStretchBltMode(hdcBuffer, COLORONCOLOR);   // 辉光贴图拉伸入缓冲时用最近邻，避免抖动

        // 共享精灵DC（辉光贴图临时选入用）
        hdcMem = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);

        // 预渲染静态背景（渐变星空 + 星云）
        BuildBackground();

        ResetGame();
    }
    
    void ResetGame() {
        playerPos = Vector2(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 80);
        playerLives = 3;
        playerLevel = 0;
        score = 0;
        combo = 0;
        shieldActive = false;
        shieldTimer = 0;
        invincibleTimer = 0;
        bombCount = 2;
        shootTimer = 0;
        frameCount = 0;
        uiTick = 0;
        enemySpawnTimer = 0;
        spawnRate = 60;
        difficulty = 0;
        stage = 1;
        keyLeft = keyRight = keyUp = keyDown = keySpace = false;
        
        // 新增成员重置
        pvx = pvy = 0.0f;
        weapon = WEAPON_MG;
        chargeAmount = 0;
        slowmoTimer = 0;
        shakeTimer = shakePower = 0;
        flashTimer = flashMax = 0;
        bossWarningTimer = 0;
        orbitTimer = 0;
        orbitCount = 0;
        survivalFrames = 0;
        bossRushIndex = 0;
        achPopupTimer = 0;
        bulletDancerTimer = 0;
        stageNoDamage = true;
        
        playerBullets.clear();
        enemyBullets.clear();
        enemies.clear();
        particles.clear();
        powerups.clear();
        floatTexts.clear();
        
        StartStage();
        
        // 初始化星星（三层视差）
        stars.clear();
        for (int i = 0; i < 90; i++) {
            Star s;
            s.x = rand() % SCREEN_WIDTH;
            s.y = rand() % SCREEN_HEIGHT;
            s.layer = rand() % 3;
            s.speed = (s.layer == 0) ? 1 : (s.layer == 1) ? 2 + rand() % 2 : 3 + rand() % 3;
            s.brightness = (s.layer == 0) ? 60 + rand() % 60 : (s.layer == 1) ? 100 + rand() % 80 : 160 + rand() % 96;
            s.phase = (float)(rand() % 628) / 100.0f;
            stars.push_back(s);
        }
        
        state = STATE_MENU;
    }
    
    void StartGame() {
        gameMode = selectedMode;
        ResetGame();
        // 应用机体属性（不同初始属性）
        int ship = selectedShip;
        playerLives = shipLives[ship];
        bombCount = shipBombs[ship];
        playerLevel = shipLevel[ship];
        pColor = shipColor[ship];
        pColor2 = shipColor2[ship];
        pSpeedMul = shipSpeed[ship];
        state = STATE_PLAYING;
        StartBGM();
    }

    void LoadHighScore() {
        std::ifstream ifs("highscore.txt");
        if (ifs) {
            ifs >> highScore;
        }
    }

    void SaveHighScore() {
        std::ofstream ofs("highscore.txt");
        if (ofs) {
            ofs << highScore;
        }
    }

    // ===================== 存档/机体/成就 =====================
    void DefineShips() {
        shipNames[0] = L"标准战机"; shipLives[0] = 3; shipBombs[0] = 2; shipLevel[0] = 0; shipSpeed[0] = 1.0f;
        shipColor[0] = RGB(0, 180, 255); shipColor2[0] = RGB(0, 120, 255);
        shipNames[1] = L"重装战机"; shipLives[1] = 4; shipBombs[1] = 3; shipLevel[1] = 0; shipSpeed[1] = 0.85f;
        shipColor[1] = RGB(255, 140, 80); shipColor2[1] = RGB(190, 90, 60);
        shipNames[2] = L"疾风战机"; shipLives[2] = 2; shipBombs[2] = 1; shipLevel[2] = 1; shipSpeed[2] = 1.25f;
        shipColor[2] = RGB(90, 255, 150); shipColor2[2] = RGB(50, 190, 110);
        shipUnlocked[0] = true;
        shipUnlocked[1] = false;
        shipUnlocked[2] = false;
    }

    void LoadSave() {
        for (int i = 0; i < MAX_ACHIEVEMENTS; i++) achUnlocked[i] = false;
        DefineShips();
        int achMask = 0, shipMask = 0;
        std::ifstream ifs("save.dat");
        if (ifs) {
            ifs >> achMask >> shipMask;
            int tk = 0;
            if (ifs >> tk) totalKills = tk;
        }
        for (int i = 0; i < MAX_ACHIEVEMENTS; i++) if (achMask & (1 << i)) achUnlocked[i] = true;
        for (int i = 0; i < MAX_SHIPS; i++) if (shipMask & (1 << i)) shipUnlocked[i] = true;
        if (achUnlocked[ACH_KILLER]) shipUnlocked[SHIP_HEAVY] = true;
        if (achUnlocked[ACH_DANCER]) shipUnlocked[SHIP_NIMBLE] = true;
    }

    void SaveGame() {
        std::ofstream ofs("save.dat");
        if (ofs) {
            int achMask = 0, shipMask = 0;
            for (int i = 0; i < MAX_ACHIEVEMENTS; i++) if (achUnlocked[i]) achMask |= (1 << i);
            for (int i = 0; i < MAX_SHIPS; i++) if (shipUnlocked[i]) shipMask |= (1 << i);
            ofs << achMask << " " << shipMask << " " << totalKills;
        }
    }

    const wchar_t* AchName(int id) {
        switch (id) {
            case ACH_KILLER: return L"初出茅庐";
            case ACH_CLEAN: return L"无伤通关";
            default: return L"弹幕舞者";
        }
    }
    const wchar_t* AchDesc(int id) {
        switch (id) {
            case ACH_KILLER: return L"累计击毁100架敌机";
            case ACH_CLEAN: return L"某关全程不受伤";
            default: return L"50发敌弹中存活10秒";
        }
    }

    void UnlockAchievement(int id) {
        if (id < 0 || id >= MAX_ACHIEVEMENTS || achUnlocked[id]) return;
        achUnlocked[id] = true;
        achPopupIndex = id;
        achPopupTimer = 240;
        if (id == ACH_KILLER) shipUnlocked[SHIP_HEAVY] = true;
        if (id == ACH_DANCER) shipUnlocked[SHIP_NIMBLE] = true;
        SaveGame();
        PlaySfx(SFX_ACHV);
        AddFlash(6);
        SpawnRing(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 120, RGB(255, 220, 80));
        SpawnExplosion(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 15, RGB(255, 220, 80));
    }

    // ===================== 音频系统 =====================
    // 预合成 8 小节循环 BGM（132 BPM，A 小调）：
    // 底鼓/军鼓/踩镲 + 三角波贝斯 + 方波琶音 + 正弦主旋律(带颤音与回声)
    void GenerateBgm() {
        const double BPM = 132.0;
        const double beat = 60.0 / BPM;
        const double step = beat / 4.0;   // 16 分音符
        const int BARS = 8;
        double total = BARS * 4 * beat;
        bgmBuf.assign((size_t)(total * AUD_SR) + 8, 0.0f);

        static const double roots[8] = { 110.00, 87.31, 130.81, 98.00, 110.00, 87.31, 130.81, 98.00 };
        static const int chords[8][3] = {
            { 57, 60, 64 }, { 53, 57, 60 }, { 55, 60, 64 }, { 55, 59, 62 },
            { 57, 60, 64 }, { 53, 57, 60 }, { 55, 60, 64 }, { 55, 59, 62 }
        };
        static const int mel[64] = {
            69, 72, 76, 72,  69, 76, 74, 72,    69, 65, 69, 72,  74, 72, 69, 65,
            76, 67, 72, 76,  79, 76, 72, 67,    74, 71, 67, 71,  74, 79, 71, 74,
            69, 72, 76, 81,  79, 76, 72, 76,    77, 72, 69, 72,  74, 77, 74, 72,
            76, 72, 67, 72,  76, 79, 76, 72,    74, 71, 74, 79,  81, 79, 76, 74
        };
        static const int arpPat[16] = { 0, 1, 2, 1, 0, 1, 2, 1, 0, 1, 2, 1, 0, 1, 2, 1 };
        static const int bassPat[8] = { 0, 0, 12, 0, 0, 7, 0, 12 };

        for (int bar = 0; bar < BARS; bar++) {
            double t0 = bar * 4 * beat;
            // 鼓组
            for (int b = 0; b < 4; b++) BakeKick(bgmBuf, t0 + b * beat, 0.40);
            if (bar % 2 == 1) BakeKick(bgmBuf, t0 + 3.5 * beat, 0.28);
            BakeSnare(bgmBuf, t0 + beat, 0.18);
            BakeSnare(bgmBuf, t0 + 3 * beat, 0.18);
            for (int h = 1; h < 8; h += 2) BakeNoise(bgmBuf, t0 + h * beat / 2, 0.03, 0.06, 2.5);
            // 贝斯（三角波，八分音符律动）
            for (int e = 0; e < 8; e++) {
                double f = roots[bar] * std::pow(2.0, bassPat[e] / 12.0);
                BakeNote(bgmBuf, t0 + e * beat / 2, beat / 2 * 0.9, f, f, 0.24, 3, 0.004, 1.3);
            }
            // 琶音（方波 16 分音符）
            for (int s16 = 0; s16 < 16; s16++) {
                double f = MidiFreq(chords[bar][arpPat[s16]] + 12);
                BakeNote(bgmBuf, t0 + s16 * step, step * 0.85, f, f, 0.055, 1, 0.002, 2.0);
            }
            // 主旋律（正弦 + 颤音 + 3/4 拍延迟回声）
            for (int e = 0; e < 8; e++) {
                int m = mel[bar * 8 + e];
                if (m < 0) continue;
                double f = MidiFreq(m);
                BakeNote(bgmBuf, t0 + e * beat / 2, beat / 2 * 0.95, f, f, 0.14, 0, 0.006, 1.25, 5.5, 0.30);
                BakeNote(bgmBuf, t0 + e * beat / 2 + beat * 0.75, beat / 2 * 0.6, f, f, 0.04, 0, 0.006, 2.2, 5.5, 0.30);
            }
        }
    }

    void InitAudio() {
        InitializeCriticalSection(&audioCS);
        audioOpen = false;
        hWaveOut = NULL;
        bgmCursor = 0;
        bgmOn = false;
        for (int i = 0; i < AUD_BLOCKS; i++) mixData[i] = NULL;

        GenerateBgm();

        WAVEFORMATEX wf;
        memset(&wf, 0, sizeof(wf));
        wf.wFormatTag = WAVE_FORMAT_PCM;
        wf.nChannels = 2;                 // 立体声（用于声像定位）
        wf.nSamplesPerSec = AUD_SR;
        wf.wBitsPerSample = 16;
        wf.nBlockAlign = 4;
        wf.nAvgBytesPerSec = AUD_SR * 4;
        if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wf, (DWORD_PTR)&ThunderFighter::WaveCallback,
                        (DWORD_PTR)this, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
            hWaveOut = NULL;
            return;
        }
        // 预填满所有循环缓冲块
        for (int i = 0; i < AUD_BLOCKS; i++) {
            mixData[i] = new short[AUD_FRAMES * 2];
            memset(mixData[i], 0, AUD_FRAMES * 4);
            WAVEHDR* h = &waveHdrs[i];
            memset(h, 0, sizeof(WAVEHDR));
            h->lpData = (LPSTR)mixData[i];
            h->dwBufferLength = AUD_FRAMES * 4;
            if (waveOutPrepareHeader(hWaveOut, h, sizeof(WAVEHDR)) == MMSYSERR_NOERROR) {
                MixInto(h);
                waveOutWrite(hWaveOut, h, sizeof(WAVEHDR));
            }
        }
        audioOpen = true;
        StartBGM();   // 主菜单也有音乐
    }

    // 播放音效：panX 为 -1(左)..1(右)，按屏幕坐标传入可定位声像
    void PlaySfxPan(int id, float panX) {
        if (!audioOpen || id < 0 || id >= SFX_COUNT) return;
        float pan = panX;
        if (pan > 1.0f) pan = 1.0f;
        if (pan < -1.0f) pan = -1.0f;
        double p = (pan + 1.0) * 0.25 * 3.14159265358979;
        double gl = std::cos(p), gr = std::sin(p);
        EnterCriticalSection(&audioCS);
        for (int L = 0; L < 5; L++) {
            const SfxLayer& d = SFX_TABLE[id][L];
            if (d.vol <= 0.0 || d.dur <= 0.0) continue;
            SndVoice* v = NULL;
            for (int i = 0; i < AUD_MAX_VOICES; i++) {
                if (!voices[i].active) { v = &voices[i]; break; }
            }
            if (!v) break;   // 声部满则丢弃
            v->active = true;
            v->wave = d.wave;
            v->f0 = d.f0;
            v->f1 = (d.f1 > 0.0) ? d.f1 : d.f0;
            v->t = 0.0;
            v->dur = d.dur;
            v->vol = d.vol;
            v->atk = d.atk;
            v->dpow = d.dpow;
            v->delay = d.delay;
            v->phase = 0.0;
            v->gl = gl;
            v->gr = gr;
            v->seed = (unsigned int)(rand() * 2654435761u) | 1u;
        }
        LeaveCriticalSection(&audioCS);
    }
    void PlaySfx(int id) { PlaySfxPan(id, 0.0f); }

    // 把一块缓冲混满：BGM 循环流 + 所有活动声部实时合成
    void MixInto(WAVEHDR* h) {
        EnterCriticalSection(&audioCS);
        short* out = (short*)h->lpData;
        int frames = (int)(h->dwBufferLength / 4);
        int bgmLen = (int)bgmBuf.size();
        for (int i = 0; i < frames; i++) {
            double l = 0.0, r = 0.0;
            if (bgmOn && bgmLen > 0) {
                float b = bgmBuf[bgmCursor];
                bgmCursor++;
                if (bgmCursor >= bgmLen) bgmCursor = 0;
                l += b * 0.30;
                r += b * 0.30;
            }
            for (int vi = 0; vi < AUD_MAX_VOICES; vi++) {
                SndVoice& s = voices[vi];
                if (!s.active) continue;
                if (s.delay > 0.0) { s.delay -= 1.0 / AUD_SR; continue; }
                if (s.t >= s.dur) { s.active = false; continue; }
                double env;
                if (s.t < s.atk) env = s.t / s.atk;
                else env = std::pow(std::max(0.0, 1.0 - (s.t - s.atk) / std::max(1e-4, s.dur - s.atk)), s.dpow);
                double u = s.t / s.dur;
                double f = (s.f1 == s.f0) ? s.f0 : s.f0 * std::pow(s.f1 / s.f0, u);
                s.phase += SND_PI2 * f / AUD_SR;
                if (s.phase >= SND_PI2) s.phase -= SND_PI2;
                double w;
                if (s.wave == 4) w = SndNoise(&s.seed);
                else if (s.wave == 1) w = (std::sin(s.phase) > 0.0) ? 1.0 : -1.0;
                else if (s.wave == 2) w = 2.0 * (s.phase / SND_PI2 - std::floor(0.5 + s.phase / SND_PI2));
                else if (s.wave == 3) w = 4.0 * std::fabs(s.phase / SND_PI2 - std::floor(0.5 + s.phase / SND_PI2)) - 1.0;
                else w = std::sin(s.phase);
                l += w * env * s.vol * s.gl;
                r += w * env * s.vol * s.gr;
                s.t += 1.0 / AUD_SR;
            }
            l *= 0.9; r *= 0.9;
            if (l > 1.0) l = 1.0;
            if (l < -1.0) l = -1.0;
            if (r > 1.0) r = 1.0;
            if (r < -1.0) r = -1.0;
            out[i * 2] = (short)(l * 32767.0);
            out[i * 2 + 1] = (short)(r * 32767.0);
        }
        LeaveCriticalSection(&audioCS);
    }

    static void CALLBACK WaveCallback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
        (void)hwo; (void)dwParam2;
        if (uMsg != WOM_DONE) return;
        ThunderFighter* g = (ThunderFighter*)dwInstance;
        if (g && g->hWaveOut) g->OnBufferDone((WAVEHDR*)dwParam1);
    }

    void OnBufferDone(WAVEHDR* h) {
        if (!hWaveOut) return;
        MixInto(h);
        waveOutWrite(hWaveOut, h, sizeof(WAVEHDR));
    }

    void StartBGM() {
        if (!audioOpen) return;
        EnterCriticalSection(&audioCS);
        bgmOn = true;
        LeaveCriticalSection(&audioCS);
    }

    void StopBGM() {
        if (!audioOpen) return;
        EnterCriticalSection(&audioCS);
        bgmOn = false;
        LeaveCriticalSection(&audioCS);
    }

    void ShutdownAudio() {
        if (hWaveOut) {
            waveOutReset(hWaveOut);
            for (int i = 0; i < AUD_BLOCKS; i++) {
                if (mixData[i]) {
                    waveOutUnprepareHeader(hWaveOut, &waveHdrs[i], sizeof(WAVEHDR));
                    delete[] mixData[i];
                    mixData[i] = NULL;
                }
            }
            waveOutClose(hWaveOut);
            hWaveOut = NULL;
        }
        audioOpen = false;
        DeleteCriticalSection(&audioCS);
    }

    // ===================== 固定时间步长（QueryPerformanceCounter）=====================
    void Tick() {
        if (!tickInit) {
            QueryPerformanceFrequency(&tickFreq);
            QueryPerformanceCounter(&lastTick);
            tickInit = true;
        }
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double dt = (double)(now.QuadPart - lastTick.QuadPart) / tickFreq.QuadPart;
        lastTick = now;
        if (dt > 0.1) dt = 0.1;
        uiTick++;
        if (state != STATE_PLAYING) {
            acc = 0;
            // 菜单/暂停/结算时星空仍缓慢流动，画面不死板
            for (auto& s : stars) {
                s.y += s.speed * 0.4f;
                if (s.y > SCREEN_HEIGHT) {
                    s.y = 0;
                    s.x = rand() % SCREEN_WIDTH;
                }
            }
            return;
        }
        acc += dt;
        const double STEP = 1.0 / 60.0;
        double scale = (slowmoTimer > 0) ? 0.45 : 1.0; // 慢动作
        int steps = 0;
        while (acc >= STEP && steps < 5) {
            Update((float)scale);
            acc -= STEP;
            steps++;
        }
        if (steps >= 5) acc = 0;
    }

    // ===================== 视觉辅助（缓存 GDI 资源，杜绝每帧泄漏） =====================
    static COLORREF ScaleColor(COLORREF c, float k) {
        if (k < 0) k = 0;
        if (k > 1) k = 1;
        return RGB((int)(GetRValue(c) * k), (int)(GetGValue(c) * k), (int)(GetBValue(c) * k));
    }

    void FillRectC(HDC hdc, int l, int t, int r, int b, COLORREF c) {
        RECT rc = { l, t, r, b };
        HBRUSH br = CreateSolidBrush(c);
        FillRect(hdc, &rc, br);
        DeleteObject(br);
    }

    void FilledPoly(HDC hdc, const POINT* pts, int n, COLORREF fill, COLORREF line, int lw) {
        HBRUSH b = CreateSolidBrush(fill);
        HBRUSH ob = (HBRUSH)SelectObject(hdc, b);
        HPEN p = CreatePen(PS_SOLID, lw, line);
        HPEN op = (HPEN)SelectObject(hdc, p);
        Polygon(hdc, pts, n);
        SelectObject(hdc, ob);
        SelectObject(hdc, op);
        DeleteObject(b);
        DeleteObject(p);
    }

    void FilledEllipse(HDC hdc, int l, int t, int r, int b, COLORREF fill, COLORREF line, int lw) {
        HBRUSH br = CreateSolidBrush(fill);
        HBRUSH ob = (HBRUSH)SelectObject(hdc, br);
        HPEN p = CreatePen(PS_SOLID, lw, line);
        HPEN op = (HPEN)SelectObject(hdc, p);
        Ellipse(hdc, l, t, r, b);
        SelectObject(hdc, ob);
        SelectObject(hdc, op);
        DeleteObject(br);
        DeleteObject(p);
    }

    // 小三角指示器：dir -1=左 1=右 0=下 2=上
    void DrawTri(HDC hdc, int cx, int cy, int r, int dir, COLORREF c) {
        POINT p[3];
        if (dir == 1)      { p[0] = { cx + r, cy }; p[1] = { cx - r, cy - r }; p[2] = { cx - r, cy + r }; }
        else if (dir == -1){ p[0] = { cx - r, cy }; p[1] = { cx + r, cy - r }; p[2] = { cx + r, cy + r }; }
        else if (dir == 0) { p[0] = { cx, cy + r }; p[1] = { cx - r, cy - r }; p[2] = { cx + r, cy - r }; }
        else               { p[0] = { cx, cy - r }; p[1] = { cx - r, cy + r }; p[2] = { cx + r, cy + r }; }
        FilledPoly(hdc, p, 3, c, c, 1);
    }

    // 按像素高缓存字体（每尺寸只创建一次）
    HFONT FontPx(int px) {
        for (auto& f : fontCache)
            if (f.first == px) return f.second;
        HFONT f = CreateFont(px, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, DEFAULT_PITCH, L"Arial");
        fontCache.push_back(std::make_pair(px, f));
        return f;
    }

    // 辉光贴图：64x64 径向衰减亮度图，SRCPAINT(OR) 叠加=加法发光
    HBITMAP GetGlow(COLORREF c) {
        for (auto& g : glowCache)
            if (g.first == c) return g.second;
        HDC sdc = GetDC(hWnd);
        HBITMAP bm = CreateCompatibleBitmap(sdc, 64, 64);
        HDC mdc = CreateCompatibleDC(sdc);
        ReleaseDC(hWnd, sdc);
        HBITMAP old = (HBITMAP)SelectObject(mdc, bm);
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 64; x++) {
                float dx = (x - 31.5f) / 31.5f, dy = (y - 31.5f) / 31.5f;
                float d = std::sqrt(dx * dx + dy * dy);
                float k = (d < 1.0f) ? std::pow(1.0f - d, 1.9f) : 0.0f;
                if (k > 0.02f) SetPixel(mdc, x, y, ScaleColor(c, k));
            }
        }
        SelectObject(mdc, old);
        DeleteDC(mdc);
        glowCache.push_back(std::make_pair(c, bm));
        return bm;
    }

    void DrawGlow(HDC hdc, float x, float y, int radius, COLORREF c) {
        HBITMAP bm = GetGlow(c);
        if (!bm) return;
        HBITMAP old = (HBITMAP)SelectObject(hdcMem, bm);
        StretchBlt(hdc, (int)x - radius, (int)y - radius, radius * 2, radius * 2,
                   hdcMem, 0, 0, 64, 64, SRCPAINT);
        SelectObject(hdcMem, old);
    }

    // 预渲染静态背景：垂直渐变深空 + 柔和星云 + 远景小星（只画一次）
    void BuildBackground() {
        HDC sdc = GetDC(hWnd);
        hdcBg = CreateCompatibleDC(sdc);
        hbmBg = CreateCompatibleBitmap(sdc, SCREEN_WIDTH, SCREEN_HEIGHT);
        SelectObject(hdcBg, hbmBg);
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            float t = y / (float)SCREEN_HEIGHT;
            int r = (int)(9 * (1 - t) + 2 * t);
            int g = (int)(12 * (1 - t) + 3 * t);
            int b = (int)(42 * (1 - t) + 12 * t);
            RECT row = { 0, y, SCREEN_WIDTH, y + 1 };
            HBRUSH rb = CreateSolidBrush(RGB(r, g, b));
            FillRect(hdcBg, &row, rb);
            DeleteObject(rb);
        }
        BakeNebula(hdcBg, 250, 150, 150, RGB(46, 20, 90));
        BakeNebula(hdcBg, 110, 470, 130, RGB(16, 52, 84));
        BakeNebula(hdcBg, 395, 350, 115, RGB(40, 16, 70));
        BakeNebula(hdcBg, 240, 620, 120, RGB(30, 18, 66));
        for (int i = 0; i < 80; i++)
            SetPixel(hdcBg, rand() % SCREEN_WIDTH, rand() % SCREEN_HEIGHT, RGB(30, 34, 58));
        ReleaseDC(hWnd, sdc);
    }

    // 多层同心椭圆叠加出柔和星云
    void BakeNebula(HDC hdc, int cx, int cy, int r, COLORREF c) {
        for (int k = 7; k >= 1; k--) {
            float t = k / 7.0f;
            int rr = (int)(r * t);
            HBRUSH b = CreateSolidBrush(ScaleColor(c, 0.10f + (1.0f - t) * 0.55f));
            HBRUSH ob = (HBRUSH)SelectObject(hdc, b);
            Ellipse(hdc, cx - rr, cy - rr * 3 / 4, cx + rr, cy + rr * 3 / 4);
            SelectObject(hdc, ob);
            DeleteObject(b);
        }
    }

    // ===================== 打击感辅助 =====================
    void AddShake(int power, int frames) {
        if (power > shakePower) shakePower = power;
        shakeTimer = std::max(shakeTimer, frames);
    }

    void AddFlash(int frames) {
        flashTimer = frames;
        flashMax = frames;
    }

    void AddSlowmo(int frames) {
        slowmoTimer = std::max(slowmoTimer, frames);
    }

    void SpawnFloatingText(float x, float y, const wchar_t* txt, COLORREF color, int size = 18) {
        FloatingText ft;
        ft.active = true;
        ft.pos = Vector2(x, y);
        ft.life = 50;
        ft.maxLife = 50;
        ft.color = color;
        ft.size = size;
        size_t k = 0;
        while (txt[k] && k < 31) { ft.text[k] = txt[k]; k++; }
        ft.text[k] = 0;
        bool placed = false;
        for (auto& t : floatTexts) {
            if (!t.active) { t = ft; placed = true; break; }
        }
        if (!placed && floatTexts.size() < MAX_FLOAT_TEXTS) floatTexts.push_back(ft);
    }

    void AddParticle(const Particle& p) {
        bool placed = false;
        for (auto& pt : particles) {
            if (!pt.active) { pt = p; placed = true; break; }
        }
        if (!placed && particles.size() < MAX_PARTICLES) particles.push_back(p);
    }

    void SpawnRing(float x, float y, int maxRadius, COLORREF color) {
        Particle p;
        p.active = true;
        p.type = 1; // 环形冲击波
        p.pos = Vector2(x, y);
        p.size = maxRadius;
        p.life = 18;
        p.maxLife = 18;
        p.color = color;
        p.vel = Vector2(0, 0);
        AddParticle(p);
    }

    // 敌人被击毁统一处理
    void OnEnemyKilled(Enemy& e, bool countCombo) {
        e.active = false;
        enemiesKilledThisStage++;
        totalKills++;
        int expSize = (e.type == 0) ? 14 : (e.type == 1) ? 26 : (e.type == 2) ? 48 : 16;
        COLORREF expColor = (e.type == 0) ? RGB(255, 100, 100) :
                            (e.type == 1) ? RGB(255, 80, 200) :
                            (e.type == 2) ? RGB(255, 100, 255) :
                            (e.type == 3) ? RGB(255, 180, 60) :
                            (e.type == 4) ? RGB(80, 220, 200) : RGB(200, 100, 255);
        SpawnExplosion(e.pos.x + e.width / 2, e.pos.y + e.height / 2, expSize, expColor);
        SpawnExplosion(e.pos.x + e.width / 2, e.pos.y + e.height / 2, expSize / 3, RGB(255, 255, 220)); // 白热内核
        if (e.type == 1) SpawnRing(e.pos.x + e.width / 2, e.pos.y + e.height / 2, 42, RGB(255, 120, 220));
        if (e.type == 2) {
            SpawnRing(e.pos.x + e.width / 2, e.pos.y + e.height / 2, 60, RGB(255, 120, 255));
            SpawnRing(e.pos.x + e.width / 2, e.pos.y + e.height / 2, 90, RGB(255, 200, 255));
            for (int i = 0; i < 3; i++)
                SpawnRing(e.pos.x + e.width / 2 + (rand() % 30 - 15),
                          e.pos.y + e.height / 2 + (rand() % 20 - 10), 35, RGB(255, 150, 200));
        }
        if (e.type == 3) SpawnRing(e.pos.x + e.width / 2, e.pos.y + e.height / 2, 30, RGB(255, 180, 80));
        if (e.type == 4) SpawnRing(e.pos.x + e.width / 2, e.pos.y + e.height / 2, 34, RGB(80, 220, 200));
        if (e.type == 5) SpawnRing(e.pos.x + e.width / 2, e.pos.y + e.height / 2, 36, RGB(200, 100, 255));
        
        // 得分
        int pts = (e.type == 0) ? 100 : (e.type == 1) ? 300 : (e.type == 2) ? 1000 :
                  (e.type == 3) ? 200 : (e.type == 4) ? 300 : 300;
        if (countCombo) {
            combo++;
            if (combo > 1) pts = (int)(pts * (1.0f + combo * 0.1f));
        }
        score += pts;
        
        wchar_t scoreBuf[32];
        wsprintfW(scoreBuf, L"+%d", pts);
        SpawnFloatingText(e.pos.x + e.width / 2 - 12, e.pos.y - 10, scoreBuf, RGB(255, 230, 80), 18);
        
        // 掉落道具
        if (rand() % 100 < (e.type == 0 ? 6 : e.type == 1 ? 22 : e.type == 2 ? 70 : 14)) {
            SpawnPowerUp(e.pos.x + e.width / 2 - 10, e.pos.y + e.height / 2);
        }
        
        // 音效与反馈（按位置定位声像）
        {
            float panX = (e.pos.x + e.width / 2.0f) / SCREEN_WIDTH * 2.0f - 1.0f;
            if (e.type == 2) {
                PlaySfxPan(SFX_BIGEXP, panX);
                AddShake(14, 40);
                AddFlash(10);
                AddSlowmo(40);
            } else if (e.type == 1) {
                PlaySfxPan(SFX_EXPLO, panX);
                AddShake(5, 10);
            } else if (e.type == 3) {
                PlaySfxPan(SFX_EXPLO, panX);
                AddShake(6, 12);
            } else {
                PlaySfxPan(SFX_EXPLO, panX);
            }
        }

        // 连击里程碑奖励
        if (combo == 10 || combo == 20 || combo == 50 || combo == 100) {
            int bonus = combo * 10;
            score += bonus;
            wchar_t comboBuf[48];
            wsprintfW(comboBuf, L"连击 x%d!  +%d", combo, bonus);
            SpawnFloatingText(SCREEN_WIDTH / 2 - 80, 200, comboBuf, RGB(255, 80, 200), 26);
            SpawnRing(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 120, RGB(255, 100, 220));
            SpawnExplosion(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 20, RGB(255, 120, 255));
            AddFlash(6);
            PlaySfx(SFX_COMBO);
        }
        
        // 成就：初出茅庐（累计击毁100架）
        if (totalKills >= 100) UnlockAchievement(ACH_KILLER);
    }
    
    // ===================== 关卡流程 =====================
    void StartStage() {
        enemiesSpawnedThisStage = 0;
        enemiesKilledThisStage = 0;
        stageClearTimer = 0;
        bossStage = (stage % 5 == 0);
        bossSpawned = false;
        stageNoDamage = true;
        if (gameMode == GAMEMODE_BOSSRUSH) {
            bossStage = true;
            enemiesToSpawnThisStage = 1;
            enemySpawnTimer = 0;
        } else if (gameMode == GAMEMODE_SURVIVAL) {
            bossStage = false;
            enemiesToSpawnThisStage = 999999;
        } else if (bossStage) {
            enemiesToSpawnThisStage = 1;
            enemySpawnTimer = 0;
        } else {
            enemiesToSpawnThisStage = 12 + stage * 2;
        }
        // 关卡剧情文字
        stageIntroTimer = 0;
        if (gameMode == GAMEMODE_STORY) {
            if (bossStage) wsprintfW(stageIntroText, L"第%d关  BOSS 来袭!", stage);
            else wsprintfW(stageIntroText, L"第%d关 - 敌军来袭", stage);
            stageIntroTimer = 90;
        } else if (gameMode == GAMEMODE_BOSSRUSH) {
            wsprintfW(stageIntroText, L"BOSS RUSH  #%d", bossRushIndex + 1);
            stageIntroTimer = 60;
        }
    }

    void AdvanceStage() {
        if (gameMode == GAMEMODE_STORY && stageNoDamage) {
            UnlockAchievement(ACH_CLEAN);
        }
        score += 500 + stage * 50;
        stage++;
        StartStage();
        PlaySfx(SFX_CLEAR);
    }

    bool ActiveEnemyExists() {
        for (auto& e : enemies) {
            if (e.active) return true;
        }
        return false;
    }

    // ===================== 爆炸粒子 =====================
    void SpawnExplosion(float x, float y, int count, COLORREF color) {
        for (int i = 0; i < count; i++) {
            Particle p;
            p.active = true;
            p.type = 0;
            p.pos = Vector2(x, y);
            float angle = (rand() % 360) * 3.14159f / 180.0f;
            float speed = 1.0f + (rand() % 5) * 0.5f;
            p.vel = Vector2(std::cos(angle) * speed, std::sin(angle) * speed);
            p.life = 15 + rand() % 20;
            p.maxLife = p.life;
            p.color = color;
            p.size = 2 + rand() % 4;
            AddParticle(p);
        }
    }

    // ===================== 生成敌人 =====================
    bool PlaceEnemy(const Enemy& e, bool countSpawn = true) {
        bool placed = false;
        for (auto& en : enemies) {
            if (!en.active) { en = e; placed = true; break; }
        }
        if (!placed && enemies.size() < MAX_ENEMIES) {
            enemies.push_back(e);
            placed = true;
        }
        if (placed && countSpawn) enemiesSpawnedThisStage++;
        return placed;
    }

    // 编队机：5架一组 V字形编队
    void SpawnFormation() {
        int cx = 60 + rand() % (SCREEN_WIDTH - 120);
        int formId = rand() % 1000;
        for (int slot = 0; slot < 5; slot++) {
            Enemy e;
            e.type = 0;
            e.hp = 1; e.maxHp = 1;
            e.width = ENEMY_SMALL_W; e.height = ENEMY_SMALL_H;
            e.formId = formId;
            e.formSlot = slot;
            e.formOffsetX = (float)(slot - 2) * 36;
            e.formOffsetY = (float)(abs(slot - 2)) * 14;
            e.startX = (float)cx;
            e.pos = Vector2((float)cx + e.formOffsetX, -e.height - e.formOffsetY);
            e.movePattern = 0;
            e.fireTimer = 1000; // 编队机不射击
            e.pattern = 0; e.angle = 0;
            e.active = true;
            PlaceEnemy(e, true);
        }
    }

    // 召唤小兵（召唤机 / 召唤Boss 使用）
    void SummonMinion(Enemy& from) {
        Enemy s;
        s.type = 0; s.hp = 1; s.maxHp = 1;
        s.width = ENEMY_SMALL_W; s.height = ENEMY_SMALL_H;
        s.startX = from.pos.x + from.width / 2 - 10;
        s.pos = Vector2(s.startX, from.pos.y + 20);
        s.movePattern = rand() % 3;
        s.fireTimer = 60;
        s.pattern = 0; s.angle = 0;
        s.active = true;
        PlaceEnemy(s, false);
    }

    void SpawnEnemy() {
        // 编队机：V字形编队（5架一组）
        if (difficulty >= 1 && !bossStage && rand() % 100 < 15) {
            SpawnFormation();
            return;
        }
        Enemy e;
        int r = rand() % 100;
        bool special = (difficulty >= 3 && !bossStage && r < 20);
        if (special && rand() % 2 == 0) {
            // 护盾机：前方有能量盾，需先打盾
            e.type = 4;
            e.hp = 3; e.maxHp = 3;
            e.shieldHp = 3;
            e.width = 40; e.height = 40;
        } else if (special) {
            // 召唤机：定期召唤小型机
            e.type = 5;
            e.hp = 5 + difficulty / 2; e.maxHp = e.hp;
            e.summonTimer = 90;
            e.width = 42; e.height = 42;
        } else if (r < 60 || difficulty < 3) {
            // 小型敌机
            e.type = 0;
            e.hp = 1; e.maxHp = 1;
            e.width = ENEMY_SMALL_W; e.height = ENEMY_SMALL_H;
        } else if (r < 85 || difficulty < 8) {
            // 中型敌机
            e.type = 1;
            e.hp = 3 + difficulty / 3; e.maxHp = e.hp;
            e.width = ENEMY_MEDIUM_W; e.height = ENEMY_MEDIUM_H;
        } else {
            // 大型敌机(Boss级)
            e.type = 2;
            e.hp = 8 + difficulty / 2; e.maxHp = e.hp;
            e.width = ENEMY_BOSS_W; e.height = ENEMY_BOSS_H;
        }

        // 自爆机：随机混入（不射击，直冲玩家）
        if (difficulty >= 2 && !bossStage && e.type <= 1 && rand() % 100 < 14) {
            e.type = 3;
            e.hp = 1; e.maxHp = 1;
            e.width = 26; e.height = 26;
        }

        e.startX = 30 + rand() % (SCREEN_WIDTH - 60 - e.width);
        e.pos = Vector2((float)e.startX, (float)(-e.height));
        e.movePattern = rand() % 3;
        e.fireTimer = 30 + rand() % 60;
        e.pattern = 0;
        e.angle = (float)(rand() % 360) * 3.14159f / 180.0f;
        e.active = true;
        PlaceEnemy(e, true);
    }

    // 生成 Boss（多模式弹幕 + 警告横幅）
    void SpawnBoss() {
        Enemy e;
        e.type = 2;
        int idx = stage;
        int bossHp = 20 + stage * 6;
        if (gameMode == GAMEMODE_BOSSRUSH) {
            idx = bossRushIndex + 1;
            bossHp = 20 + idx * 10;
        }
        e.hp = bossHp; e.maxHp = bossHp;
        e.width = ENEMY_BOSS_W; e.height = ENEMY_BOSS_H;
        e.startX = 40 + rand() % (SCREEN_WIDTH - 80 - e.width);
        e.pos = Vector2((float)e.startX, (float)(-e.height));
        e.movePattern = 2;
        e.fireTimer = 20 + rand() % 40;
        e.angle = 0.0f;
        // 弹幕模式选择
        if (gameMode == GAMEMODE_BOSSRUSH) {
            int p = bossRushIndex % 5;
            if (p == 0) e.pattern = 4;      // 突击型
            else if (p == 1) e.pattern = 1; // 弹幕型
            else if (p == 2) e.pattern = 2; // 召唤型
            else if (p == 3) e.pattern = 3; // 多阶段
            else e.pattern = 0;             // 散射
        } else {
            if (idx % 20 == 0) e.pattern = 3;      // 多阶段（第20关）
            else if (idx % 15 == 0) e.pattern = 2; // 召唤型（第15关）
            else if (idx % 10 == 0) e.pattern = 1; // 弹幕型（第10关）
            else e.pattern = 4;                    // 突击型（第5/25关等）
        }
        e.active = true;
        if (PlaceEnemy(e, true)) {
            bossWarningTimer = 120;
            PlaySfx(SFX_BOSS);
            AddShake(8, 20);
        }
    }

    // ===================== 生成道具 =====================
    void SpawnPowerUp(float x, float y) {
        PowerUp p;
        p.active = true;
        p.pos = Vector2(x, y);
        // 0=P火力 40%  1=S护盾 15%  2=B炸弹 12%  3=W武器 18%  4=O轨道盾 15%
        int r = rand() % 100;
        if (r < 40) p.type = 0;
        else if (r < 55) p.type = 1;
        else if (r < 67) p.type = 2;
        else if (r < 85) p.type = 3;
        else p.type = 4;

        bool placed = false;
        for (auto& pw : powerups) {
            if (!pw.active) {
                pw = p;
                placed = true;
                break;
            }
        }
        if (!placed && powerups.size() < MAX_POWERUPS) {
            powerups.push_back(p);
        }
    }

    // ===================== 玩家射击（武器系统） =====================
    void AddPlayerBullet(Bullet& b) {
        bool placed = false;
        for (auto& bl : playerBullets) {
            if (!bl.active) {
                bl = b;
                placed = true;
                break;
            }
        }
        if (!placed && playerBullets.size() < MAX_BULLETS) {
            playerBullets.push_back(b);
        }
    }

    void PlayerShoot() {
        if (shootTimer > 0) return;
        int lv = playerLevel;

        if (weapon == WEAPON_SCATTER) {
            // 散射：5方向扇形
            int n = 3 + lv;
            for (int i = 0; i < n; i++) {
                float t = (n == 1) ? 0.0f : (float)i / (n - 1);
                float angle = -0.55f + t * 1.1f;
                Bullet b;
                b.active = true;
                b.isEnemy = false;
                b.dmg = 1;
                b.pos = Vector2(playerPos.x + PLAYER_WIDTH / 2 - 2, playerPos.y - 6);
                b.vel = Vector2(std::sin(angle) * 6.5f, -6.5f);
                b.width = 5; b.height = 12;
                AddPlayerBullet(b);
            }
            shootTimer = 13 - lv;
            PlaySfxPan(SFX_SHOOT, -0.15f);
        } else if (weapon == WEAPON_HOMING) {
            // 追踪导弹：自动锁定最近敌人
            int n = 1 + lv;
            for (int i = 0; i < n; i++) {
                Bullet b;
                b.active = true;
                b.isEnemy = false;
                b.homing = true;
                b.dmg = 2;
                float off = (float)(i - (n - 1) / 2) * 12.0f;
                b.pos = Vector2(playerPos.x + PLAYER_WIDTH / 2 - 4 + off, playerPos.y - 8);
                b.vel = Vector2(off * 0.1f, -6.0f);
                b.width = 8; b.height = 16;
                AddPlayerBullet(b);
            }
            shootTimer = 22 - lv * 2;
            PlaySfxPan(SFX_HOMING, 0.15f);
        } else if (weapon == WEAPON_LASER) {
            // 激光：穿透持续伤害
            int n = 1 + lv / 2;
            for (int i = 0; i < n; i++) {
                Bullet b;
                b.active = true;
                b.isEnemy = false;
                b.pierce = true;
                b.dmg = 1;
                float off = (float)(i - (n - 1) / 2) * 16.0f;
                b.pos = Vector2(playerPos.x + PLAYER_WIDTH / 2 - 5 + off, playerPos.y - 20);
                b.vel = Vector2(0, -12.0f);
                b.width = 10; b.height = 32;
                AddPlayerBullet(b);
            }
            shootTimer = 8 - lv / 2;
            PlaySfx(SFX_LASER);
        } else {
            // 默认机枪（按火力等级 0~3）
            PlaySfx(SFX_SHOOT);
            Bullet b;
            b.active = true;
            b.isEnemy = false;
            b.dmg = 1;
            b.vel = Vector2(0.0f, -8.0f - lv * 0.3f);

            if (lv == 0) {
                // 单发
                b.pos = Vector2(playerPos.x + PLAYER_WIDTH/2 - BULLET_WIDTH/2, playerPos.y - 10);
                b.width = BULLET_WIDTH; b.height = BULLET_HEIGHT;
                AddPlayerBullet(b);
                shootTimer = 12;
            } else if (lv == 1) {
                // 双发
                b.pos = Vector2(playerPos.x + 6, playerPos.y - 5);
                AddPlayerBullet(b);
                b.pos = Vector2(playerPos.x + PLAYER_WIDTH - 6 - BULLET_WIDTH, playerPos.y - 5);
                AddPlayerBullet(b);
                shootTimer = 11;
            } else if (lv == 2) {
                // 三发
                b.pos = Vector2(playerPos.x + PLAYER_WIDTH/2 - BULLET_WIDTH/2, playerPos.y - 10);
                AddPlayerBullet(b);
                b.pos = Vector2(playerPos.x + 2, playerPos.y);
                b.width = 4; b.height = 12;
                AddPlayerBullet(b);
                b.pos = Vector2(playerPos.x + PLAYER_WIDTH - 6, playerPos.y);
                AddPlayerBullet(b);
                b.width = BULLET_WIDTH; b.height = BULLET_HEIGHT;
                shootTimer = 10;
            } else {
                // 四发 + 散射
                b.pos = Vector2(playerPos.x + PLAYER_WIDTH/2 - BULLET_WIDTH/2, playerPos.y - 12);
                AddPlayerBullet(b);
                b.pos = Vector2(playerPos.x + 4, playerPos.y - 4);
                b.width = 4; b.height = 14;
                AddPlayerBullet(b);
                b.pos = Vector2(playerPos.x + PLAYER_WIDTH - 8, playerPos.y - 4);
                AddPlayerBullet(b);
                b.width = BULLET_WIDTH; b.height = BULLET_HEIGHT;
                b.pos = Vector2(playerPos.x - 4, playerPos.y + 8);
                b.width = 4; b.height = 10;
                AddPlayerBullet(b);
                b.pos = Vector2(playerPos.x + PLAYER_WIDTH, playerPos.y + 8);
                AddPlayerBullet(b);
                b.width = BULLET_WIDTH; b.height = BULLET_HEIGHT;
                shootTimer = 8;
            }
        }

        // 枪口闪光
        Particle m;
        m.active = true;
        m.type = 0;
        m.pos = Vector2(playerPos.x + PLAYER_WIDTH / 2, playerPos.y - 4);
        m.vel = Vector2(0, -1);
        m.life = 3;
        m.maxLife = 3;
        m.color = RGB(255, 240, 160);
        m.size = 3;
        AddParticle(m);
    }

    // 蓄力激光释放（长按空格后松开）
    void FireChargeLaser() {
        Bullet b;
        b.active = true;
        b.isEnemy = false;
        b.pierce = true;
        b.dmg = 5;
        b.pos = Vector2(playerPos.x + PLAYER_WIDTH / 2 - 7, playerPos.y - 180);
        b.vel = Vector2(0, -16.0f);
        b.width = 14;
        b.height = 240;
        b.pierceTimer = 0;
        AddPlayerBullet(b);
        // 反馈
        SpawnExplosion(playerPos.x + PLAYER_WIDTH / 2, playerPos.y, 12, RGB(0, 255, 255));
        SpawnRing(playerPos.x + PLAYER_WIDTH / 2, playerPos.y - 10, 30, RGB(0, 220, 255));
        AddShake(6, 12);
        PlaySfx(SFX_ZAP);
    }

    // ===================== 敌人射击 =====================
    void EnemyShoot(Enemy& e) {
        if (e.type == 3) return; // 自爆机不射击
        if (e.type == 5) return; // 召唤机不射击（靠召唤）
        auto addBullet = [&](float offsetX, float offsetY, int w, int h, float vx, float vy) {
            Bullet b;
            b.active = true;
            b.isEnemy = true;
            b.width = w;
            b.height = h;
            b.pos = Vector2(e.pos.x + offsetX, e.pos.y + offsetY);
            b.vel = Vector2(vx, vy);
            bool placed = false;
            for (auto& bl : enemyBullets) {
                if (!bl.active) {
                    bl = b;
                    placed = true;
                    break;
                }
            }
            if (!placed && enemyBullets.size() < MAX_BULLETS) {
                enemyBullets.push_back(b);
            }
        };

        if (e.type == 2) {
            int centerX = e.width / 2 - 3;
            if (e.pattern == 1 || (e.pattern == 3 && e.phase2)) {
                // 螺旋弹幕（弹幕型 / 多阶段第二形态）
                for (int i = 0; i < 2; i++) {
                    e.angle += 0.25f;
                    addBullet(centerX, e.height, 6, 12,
                              (float)std::cos(e.angle) * 2.5f, 4.0f);
                }
            } else if (e.pattern == 2) {
                // 召唤型：瞄准玩家大弹 + 扇形 + 横向扫射
                float px = playerPos.x + PLAYER_WIDTH / 2;
                float py = playerPos.y + PLAYER_HEIGHT / 2;
                float dx = px - (e.pos.x + centerX);
                float dy = py - (e.pos.y + e.height);
                float len = std::sqrt(dx * dx + dy * dy) + 0.001f;
                float sp = 5.0f;
                addBullet(centerX, e.height, 10, 14, dx / len * sp, dy / len * sp);
                for (int i = -1; i <= 1; i++) {
                    addBullet(centerX + i * 8.0f, e.height, 6, 12, i * 1.2f, 4.5f);
                }
                // 激光扫射：一排横向扩散弹
                if (rand() % 3 == 0) {
                    for (int i = -3; i <= 3; i++) {
                        addBullet(centerX + i * 14.0f, e.height, 4, 8, i * 0.9f, 3.2f);
                    }
                }
            } else if (e.pattern == 4) {
                // 突击型：直线弹（瞄准玩家） + 侧向弹
                float px = playerPos.x + PLAYER_WIDTH / 2;
                float py = playerPos.y + PLAYER_HEIGHT / 2;
                float dx = px - (e.pos.x + centerX);
                float dy = py - (e.pos.y + e.height);
                float len = std::sqrt(dx * dx + dy * dy) + 0.001f;
                float sp = 6.0f;
                addBullet(centerX, e.height, 6, 12, dx / len * sp, dy / len * sp);
                addBullet(centerX - 10, e.height, 6, 12, -1.5f, 5.0f);
                addBullet(centerX + 10, e.height, 6, 12, 1.5f, 5.0f);
            } else {
                // 默认散射弹
                for (int i = -1; i <= 1; i++) {
                    addBullet(centerX + i * 8.0f, e.height, 6, 12, i * 0.8f, 4.5f);
                }
                if (rand() % 3 == 0) {
                    addBullet(centerX, e.height, 10, 14, 0.0f, 6.0f);
                }
            }
            // 半血狂暴：额外扇形弹
            if (e.hp * 2 <= e.maxHp) {
                for (int i = -2; i <= 2; i++) {
                    addBullet(centerX + i * 10.0f, e.height, 5, 10, i * 0.5f, 3.8f);
                }
            }
        } else if (e.type == 4) {
            // 护盾机：瞄准玩家的单发
            float px = playerPos.x + PLAYER_WIDTH / 2;
            float py = playerPos.y + PLAYER_HEIGHT / 2;
            float dx = px - (e.pos.x + e.width / 2);
            float dy = py - (e.pos.y + e.height);
            float len = std::sqrt(dx * dx + dy * dy) + 0.001f;
            float sp = 3.6f;
            addBullet(e.width / 2 - 3, e.height, 6, 12, dx / len * sp, dy / len * sp);
        } else if (e.type == 1) {
            // 中型敌机偶发两发
            addBullet(e.width / 2 - 3, e.height, 6, 12, 0.0f, 4.0f);
            if (rand() % 5 == 0) {
                addBullet(e.width / 2 - 3 + 12, e.height, 6, 12, -0.7f, 4.0f);
                addBullet(e.width / 2 - 3 - 12, e.height, 6, 12, 0.7f, 4.0f);
            }
        } else {
            // 小型敌机普通单发
            addBullet(e.width / 2 - 3, e.height, 6, 12, 0.0f, 4.0f);
        }
    }

    // ===================== 碰撞检测 =====================
    bool CheckCollision(float x1, float y1, int w1, int h1, float x2, float y2, int w2, int h2) {
        return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
    }

    // ===================== 更新逻辑 =====================
    void Update(float dt) {
        if (state != STATE_PLAYING) return;
        
        frameCount++;
        
        // ---- 更新背景星星（视差）----
        for (auto& s : stars) {
            s.y += s.speed * dt;
            if (s.y > SCREEN_HEIGHT) {
                s.y = 0;
                s.x = rand() % SCREEN_WIDTH;
            }
        }
        
        // ---- 难度递增 ----
        if (gameMode == GAMEMODE_SURVIVAL) difficulty = frameCount / SURVIVAL_DIFF;
        else difficulty = frameCount / 600; // 每10秒难度+1
        spawnRate = std::max(15, 60 - difficulty * 2);
        
        // ---- 更新玩家（加速度 + 惯性）----
        float maxSpeed = PLAYER_MAX_SPEED * pSpeedMul;
        float ax = (keyRight ? 1.0f : 0.0f) - (keyLeft ? 1.0f : 0.0f);
        float ay = (keyDown ? 1.0f : 0.0f) - (keyUp ? 1.0f : 0.0f);
        pvx += (ax * maxSpeed - pvx) * PLAYER_ACCEL * dt;
        pvy += (ay * maxSpeed - pvy) * PLAYER_ACCEL * dt;
        playerPos.x += pvx * dt;
        playerPos.y += pvy * dt;
        
        // 边界限制
        playerPos.x = std::max(0.0f, std::min((float)(SCREEN_WIDTH - PLAYER_WIDTH), playerPos.x));
        playerPos.y = std::max(0.0f, std::min((float)(SCREEN_HEIGHT - PLAYER_HEIGHT), playerPos.y));
        
        // ---- 射击与蓄力激光 ----
        if (keySpace) {
            chargeAmount = std::min(MAX_CHARGE, chargeAmount + 1);
            if (chargeAmount < CHARGE_THRESHOLD) PlayerShoot();
            else if (chargeAmount == CHARGE_THRESHOLD) PlaySfx(SFX_CHARGEFULL); // 蓄满提示
        } else {
            if (chargeAmount >= CHARGE_THRESHOLD) FireChargeLaser();
            chargeAmount = 0;
        }
        if (shootTimer > 0) shootTimer--;
        
        // 无敌/护盾计时器
        if (invincibleTimer > 0) invincibleTimer--;
        if (shieldTimer > 0) {
            shieldTimer--;
            if (shieldTimer == 0) shieldActive = false;
        }
        if (bossWarningTimer > 0) bossWarningTimer--;
        
        // 打击感计时器
        if (shakeTimer > 0) {
            shakeTimer--;
            if (shakeTimer == 0) shakePower = 0;
        }
        if (flashTimer > 0) flashTimer--;
        if (slowmoTimer > 0) slowmoTimer--;
        
        // ---- 生成敌人（按模式）----
        enemySpawnTimer++;
        int adjustedSpawn = std::max(5, spawnRate - difficulty);
        if (gameMode == GAMEMODE_BOSSRUSH) {
            if (!bossSpawned && enemySpawnTimer >= adjustedSpawn) {
                enemySpawnTimer = 0;
                SpawnBoss();
                bossSpawned = true;
            }
        } else if (gameMode == GAMEMODE_SURVIVAL) {
            if (enemySpawnTimer >= adjustedSpawn) {
                enemySpawnTimer = 0;
                SpawnEnemy();
            }
        } else if (!bossStage) {
            if (enemySpawnTimer >= adjustedSpawn && enemiesSpawnedThisStage < enemiesToSpawnThisStage) {
                enemySpawnTimer = 0;
                SpawnEnemy();
            }
        } else {
            if (!bossSpawned && enemySpawnTimer >= adjustedSpawn) {
                enemySpawnTimer = 0;
                SpawnBoss();
                bossSpawned = true;
            }
        }
        
        // ---- 更新玩家子弹（含追踪/穿透）----
        for (auto& b : playerBullets) {
            if (!b.active) continue;
            if (b.homing) {
                // 追踪最近敌人
                Enemy* t = NULL;
                float best = 1e9f;
                for (auto& e : enemies) {
                    if (!e.active) continue;
                    float cx = e.pos.x + e.width / 2, cy = e.pos.y + e.height / 2;
                    float dx = cx - b.pos.x, dy = cy - b.pos.y;
                    float d = dx * dx + dy * dy;
                    if (d < best) { best = d; t = &e; }
                }
                if (t) {
                    float cx = t->pos.x + t->width / 2, cy = t->pos.y + t->height / 2;
                    float dx = cx - b.pos.x, dy = cy - b.pos.y;
                    float len = std::sqrt(dx * dx + dy * dy) + 0.001f;
                    float sp = std::sqrt(b.vel.x * b.vel.x + b.vel.y * b.vel.y);
                    float tx = dx / len * sp, ty = dy / len * sp;
                    b.vel.x += (tx - b.vel.x) * 0.06f * dt;
                    b.vel.y += (ty - b.vel.y) * 0.06f * dt;
                }
            }
            b.pos.x += b.vel.x * dt;
            b.pos.y += b.vel.y * dt;
            if (b.pierceTimer > 0) b.pierceTimer--;
            if (b.pos.y + b.height < 0 || b.pos.x + b.width < 0 || b.pos.x > SCREEN_WIDTH) {
                b.active = false;
            }
        }
        
        // ---- 更新敌人子弹 ----
        for (auto& b : enemyBullets) {
            if (!b.active) continue;
            b.pos.x += b.vel.x * dt;
            b.pos.y += b.vel.y * dt;
            if (b.pos.y > SCREEN_HEIGHT || b.pos.x + b.width < 0 || b.pos.x > SCREEN_WIDTH) {
                b.active = false;
            }
        }
        
        // ---- 更新敌人 ----
        for (auto& e : enemies) {
            if (!e.active) continue;
            if (e.hitFlash > 0) e.hitFlash--;
            
            if (e.type == 3) {
                // 自爆机：直冲玩家
                float dx = (playerPos.x + PLAYER_WIDTH / 2) - (e.pos.x + e.width / 2);
                float dy = (playerPos.y + PLAYER_HEIGHT / 2) - (e.pos.y + e.height / 2);
                float len = std::sqrt(dx * dx + dy * dy) + 0.001f;
                float sp = 4.2f + difficulty * 0.15f;
                e.pos.x += dx / len * sp * dt;
                e.pos.y += dy / len * sp * dt;
                // 接近玩家即自爆
                if (len < 36) {
                    e.active = false;
                    SpawnExplosion(e.pos.x + e.width / 2, e.pos.y + e.height / 2, 18, RGB(255, 160, 40));
                    SpawnRing(e.pos.x + e.width / 2, e.pos.y + e.height / 2, 36, RGB(255, 180, 80));
                    AddShake(6, 12);
                    if (invincibleTimer <= 0) PlayerHit();
                }
                if (e.pos.y > SCREEN_HEIGHT + 30) e.active = false;
                continue;
            }
            
            if (e.formId >= 0) {
                // 编队机：V字形整体下降 + 集体摆动
                float sway = std::sin(frameCount * 0.03f + e.formId * 1.3f) * 20;
                e.pos.x = e.startX + e.formOffsetX + sway;
                e.pos.y += 1.2f * dt;
                if (e.pos.y > SCREEN_HEIGHT + 20) e.active = false;
                continue; // 编队机不射击
            }
            
            if (e.type == 5) {
                // 召唤机：缓慢下降并定期召唤小兵
                e.pos.y += 0.8f * dt;
                e.pos.x = e.startX + std::sin(frameCount * 0.03f + e.startX * 0.1f) * 30;
                if (e.summonTimer > 0) e.summonTimer--;
                if (e.summonTimer <= 0 && e.pos.y > 20) {
                    e.summonTimer = 150;
                    for (int i = 0; i < 2; i++) {
                        Enemy s;
                        s.type = 0; s.hp = 1; s.maxHp = 1;
                        s.width = ENEMY_SMALL_W; s.height = ENEMY_SMALL_H;
                        s.startX = e.pos.x + e.width / 2 - 10 + (i * 20 - 10);
                        s.pos = Vector2(s.startX, e.pos.y + 10);
                        s.movePattern = rand() % 3;
                        s.fireTimer = 60;
                        s.pattern = 0; s.angle = 0;
                        s.active = true;
                        PlaceEnemy(s, false);
                    }
                    SpawnExplosion(e.pos.x + e.width / 2, e.pos.y + 10, 5, RGB(255, 120, 200));
                }
                if (e.pos.y > SCREEN_HEIGHT + 20) e.active = false;
                continue;
            }
            
            // 移动
            float speed = (1.0f + difficulty * 0.1f) * dt;
            if (e.type == 0) speed *= 1.5f;
            else if (e.type == 2) speed *= 0.5f;
            else if (e.type == 4) speed *= 1.1f;
            
            e.pos.y += speed;
            
            // 横向摆动
            if (e.movePattern == 0) {
                e.pos.x = e.startX + std::sin(frameCount * 0.03f + e.startX * 0.1f) * 22;
            } else if (e.movePattern == 1) {
                e.pos.x = e.startX + std::sin(frameCount * 0.05f + e.startX * 0.12f) * 36;
            } else {
                e.pos.x += std::sin(frameCount * 0.02f + e.startX * 0.08f) * 0.7f * dt;
            }

            if (e.type == 2) {
                if (e.pattern == 4) {
                    // 突击型：垂直振荡 + 追踪玩家X
                    if (e.pos.y > 90) e.pos.y = 90;
                    e.pos.y = 90 + std::sin(frameCount * 0.05f) * 65;
                    float targetX = playerPos.x + PLAYER_WIDTH / 2 - e.width / 2;
                    e.pos.x += (targetX - e.pos.x) * 0.03f * dt;
                } else if (e.pattern == 3) {
                    // 多阶段：第一形态悬停，第二形态下压追踪
                    if (e.pos.y > 120) e.pos.y = 120;
                    if (e.phase2) {
                        e.pos.y = 90 + std::sin(frameCount * 0.04f) * 40;
                        e.pos.x += std::sin(frameCount * 0.03f) * 1.5f * dt;
                    } else {
                        e.pos.x += std::sin(frameCount * 0.03f) * 1.5f * dt;
                    }
                } else {
                    // 常规悬停巡逻
                    if (e.pos.y > 120) e.pos.y = 120;
                    e.pos.x += std::sin(frameCount * 0.03f) * 1.5f * dt;
                }
                // 召唤型 Boss：定期召唤小兵
                if (e.pattern == 2 && e.pos.y >= 100) {
                    if (e.summonTimer > 0) e.summonTimer--;
                    if (e.summonTimer <= 0) {
                        e.summonTimer = 200;
                        SummonMinion(e);
                    }
                }
                // 多阶段：半血切换第二形态
                if (e.pattern == 3 && !e.phase2 && e.hp * 2 <= e.maxHp) {
                    e.phase2 = true;
                    SpawnRing(e.pos.x + e.width / 2, e.pos.y + e.height / 2, 80, RGB(255, 60, 60));
                    SpawnExplosion(e.pos.x + e.width / 2, e.pos.y + e.height / 2, 20, RGB(255, 60, 60));
                    AddFlash(8);
                    AddShake(8, 16);
                    PlaySfx(SFX_BIGEXP);
                }
            }
            
            // 限制范围
            if (e.pos.x < 0) e.pos.x = 0;
            if (e.pos.x + e.width > SCREEN_WIDTH) e.pos.x = SCREEN_WIDTH - e.width;
            
            // 射击
            e.fireTimer--;
            if (e.pierceCooldown > 0) e.pierceCooldown--;
            if (e.fireTimer <= 0) {
                EnemyShoot(e);
                int minTimer = (e.type == 2) ? 35 : (e.type == 1 ? 45 : 50);
                int maxTimer = (e.type == 2) ? 70 : (e.type == 1 ? 85 : 120);
                // 半血 Boss 狂暴：射击间隔减半
                if (e.type == 2 && e.hp * 2 <= e.maxHp) { minTimer /= 2; maxTimer /= 2; }
                e.fireTimer = std::max(15, minTimer - difficulty * 2) + rand() % (maxTimer - minTimer + 1);
            }
            
            // 超出屏幕
            if (e.pos.y > SCREEN_HEIGHT + 20) {
                e.active = false;
            }
        }

        // ---- 关卡判断（按模式）----
        if (gameMode == GAMEMODE_BOSSRUSH) {
            if (bossSpawned && !ActiveEnemyExists()) {
                stageClearTimer++;
                if (stageClearTimer > 60) {
                    stageClearTimer = 0;
                    bossRushIndex++;
                    bossSpawned = false;
                    enemySpawnTimer = adjustedSpawn; // 立即生成下一个
                    score += 800 + bossRushIndex * 100;
                    PlaySfx(SFX_CLEAR);
                    wsprintfW(stageIntroText, L"BOSS RUSH  #%d", bossRushIndex + 1);
                    stageIntroTimer = 60;
                }
            } else {
                stageClearTimer = 0;
            }
        } else if (gameMode == GAMEMODE_SURVIVAL) {
            survivalFrames++;
        } else {
            if ((!bossStage && enemiesSpawnedThisStage >= enemiesToSpawnThisStage && !ActiveEnemyExists()) ||
                (bossStage && bossSpawned && !ActiveEnemyExists())) {
                stageClearTimer++;
            } else {
                stageClearTimer = 0;
            }
            if (stageClearTimer > 120) {
                AdvanceStage();
            }
        }
        
        // ---- 碰撞检测：玩家子弹 vs 敌人（支持穿透）----
        for (auto& b : playerBullets) {
            if (!b.active) continue;
            for (auto& e : enemies) {
                if (!e.active) continue;
                if (b.pierce && e.pierceCooldown > 0) continue; // 该敌人被激光命中后的冷却
                if (CheckCollision(b.pos.x, b.pos.y, b.width, b.height,
                                   e.pos.x, e.pos.y, e.width, e.height)) {
                    if (e.type == 4 && e.shieldHp > 0) {
                        // 护盾机：先打能量盾
                        e.shieldHp -= b.dmg;
                        e.hitFlash = 3;
                        SpawnExplosion(b.pos.x + b.width / 2, b.pos.y, 4, RGB(0, 255, 220));
                        if (b.dmg > 1) {
                            wchar_t dmgBuf[16];
                            wsprintfW(dmgBuf, L"-%d", b.dmg);
                            SpawnFloatingText(e.pos.x + e.width / 2 - 6, e.pos.y, dmgBuf, RGB(0, 255, 220), 14);
                        }
                        if (e.shieldHp <= 0) {
                            SpawnRing(e.pos.x + e.width / 2, e.pos.y - 4, 40, RGB(0, 255, 220));
                            SpawnExplosion(e.pos.x + e.width / 2, e.pos.y - 4, 12, RGB(0, 255, 220));
                            SpawnFloatingText(e.pos.x + e.width / 2 - 30, e.pos.y - 6, L"护盾击破!", RGB(0, 255, 220), 16);
                            PlaySfx(SFX_SHIELD);
                        }
                        if (!b.pierce) b.active = false;
                        if (!b.pierce) break;
                        continue;
                    }
                    e.hp -= b.dmg;
                    e.hitFlash = 3;
                    SpawnExplosion(b.pos.x + b.width / 2, b.pos.y, 4, RGB(255, 255, 200));
                    if (b.dmg > 1) {
                        wchar_t dmgBuf[16];
                        wsprintfW(dmgBuf, L"-%d", b.dmg);
                        SpawnFloatingText(e.pos.x + e.width / 2 - 6, e.pos.y, dmgBuf, RGB(255, 200, 120), 14);
                    }
                    if (b.pierce) {
                        e.pierceCooldown = 8; // 激光：该敌人短暂冷却后仍可被命中
                    } else {
                        b.active = false;
                    }
                    if (e.hp <= 0) {
                        OnEnemyKilled(e, true);
                    }
                    if (!b.pierce) break;
                }
            }
        }
        
        // ---- 碰撞检测：敌人子弹 vs 玩家 ----
        if (invincibleTimer <= 0) {
            for (auto& b : enemyBullets) {
                if (!b.active) continue;
                if (CheckCollision(b.pos.x, b.pos.y, b.width, b.height,
                                   playerPos.x, playerPos.y, PLAYER_WIDTH, PLAYER_HEIGHT)) {
                    b.active = false;
                    PlayerHit();
                    break;
                }
            }
            
            // 碰撞检测：敌人 vs 玩家
            if (invincibleTimer <= 0) {
                for (auto& e : enemies) {
                    if (!e.active) continue;
                    if (CheckCollision(e.pos.x, e.pos.y, e.width, e.height,
                                       playerPos.x, playerPos.y, PLAYER_WIDTH, PLAYER_HEIGHT)) {
                        e.active = false;
                        SpawnExplosion(e.pos.x + e.width/2, e.pos.y + e.height/2, 20, RGB(255,150,50));
                        PlayerHit();
                        break;
                    }
                }
            }
        }
        
        // ---- 道具收集 ----
        for (auto& p : powerups) {
            if (!p.active) continue;
            p.pos.y += 2.0f * dt;
            if (p.pos.y > SCREEN_HEIGHT) {
                p.active = false;
                continue;
            }
            if (CheckCollision(p.pos.x, p.pos.y, 20, 20,
                               playerPos.x, playerPos.y, PLAYER_WIDTH, PLAYER_HEIGHT)) {
                p.active = false;
                wchar_t pickBuf[48];
                if (p.type == 0) {
                    // 火力提升
                    playerLevel = std::min(3, playerLevel + 1);
                    wsprintfW(pickBuf, L"火力提升!");
                    SpawnFloatingText(playerPos.x - 25, playerPos.y - 16, pickBuf, RGB(255, 200, 50), 16);
                } else if (p.type == 1) {
                    // 护盾
                    shieldActive = true;
                    shieldTimer = 300; // 5秒
                    wsprintfW(pickBuf, L"获得护盾!");
                    SpawnFloatingText(playerPos.x - 25, playerPos.y - 16, pickBuf, RGB(0, 255, 200), 16);
                } else if (p.type == 2) {
                    // 炸弹 - 清屏
                    bombCount++;
                    UseBomb();
                } else if (p.type == 3) {
                    // 更换武器（1/2/3 循环）
                    weapon = (weapon % 3) + 1;
                    const wchar_t* wname = (weapon == WEAPON_SCATTER) ? L"散射" :
                                           (weapon == WEAPON_HOMING) ? L"追踪导弹" : L"激光";
                    wsprintfW(pickBuf, L"武器: %s", wname);
                    SpawnFloatingText(playerPos.x - 40, playerPos.y - 16, pickBuf, RGB(0, 220, 255), 16);
                } else if (p.type == 4) {
                    // 轨道盾：环绕弹阻挡敌弹（限时）
                    orbitTimer = ORBIT_TIME;
                    orbitCount = ORBIT_COUNT;
                    wsprintfW(pickBuf, L"获得轨道盾!");
                    SpawnFloatingText(playerPos.x - 35, playerPos.y - 16, pickBuf, RGB(0, 255, 220), 16);
                }
                PlaySfx(SFX_POWERUP);
            }
        }

        // ---- 更新粒子 ----
        for (auto& p : particles) {
            if (!p.active) continue;
            p.pos.x += p.vel.x * dt;
            p.pos.y += p.vel.y * dt;
            p.vel.x *= 0.95f;
            p.vel.y *= 0.95f;
            p.life--;
            if (p.life <= 0) p.active = false;
        }
        
        // ---- 更新飘字 ----
        for (auto& ft : floatTexts) {
            if (!ft.active) continue;
            ft.pos.y -= 1.0f * dt;
            ft.life--;
            if (ft.life <= 0) ft.active = false;
        }
        
        // ---- 环形护盾弹（限时）----
        if (orbitTimer > 0) {
            orbitTimer--;
            float pcx = playerPos.x + PLAYER_WIDTH / 2;
            float pcy = playerPos.y + PLAYER_HEIGHT / 2;
            for (int i = 0; i < orbitCount; i++) {
                float ang = frameCount * 0.08f + (float)i * (6.2831853f / orbitCount);
                float ox = pcx + std::cos(ang) * 36.0f;
                float oy = pcy + std::sin(ang) * 36.0f;
                // 阻挡敌弹
                for (auto& b : enemyBullets) {
                    if (!b.active) continue;
                    if (CheckCollision(ox - 9, oy - 9, 18, 18, b.pos.x, b.pos.y, b.width, b.height)) {
                        b.active = false;
                        SpawnExplosion(ox, oy, 4, RGB(0, 255, 220));
                        score += 5;
                    }
                }
            }
            if (orbitTimer <= 0) {
                for (int i = 0; i < orbitCount; i++)
                    SpawnExplosion(pcx, pcy, 3, RGB(0, 255, 220));
            }
        }
        
        // ---- 成就进度与计时器 ----
        int activeBullets = 0;
        for (auto& b : enemyBullets) if (b.active) activeBullets++;
        if (activeBullets >= 50) {
            bulletDancerTimer++;
            if (bulletDancerTimer >= 600) UnlockAchievement(ACH_DANCER);
        } else {
            bulletDancerTimer = 0;
        }
        if (achPopupTimer > 0) achPopupTimer--;
        if (stageIntroTimer > 0) stageIntroTimer--;
        
        // ---- 游戏结束检查 ----
        if (playerLives <= 0 && invincibleTimer <= -60) {
            if (score > highScore) {
                highScore = score;
                SaveHighScore();
            }
            StopBGM();
            state = STATE_GAMEOVER;
        }
    }
    
    void PlayerHit() {
        if (shieldActive) {
            shieldActive = false;
            shieldTimer = 0;
            SpawnExplosion(playerPos.x + PLAYER_WIDTH/2, playerPos.y + PLAYER_HEIGHT/2, 15, RGB(0,255,200));
            SpawnRing(playerPos.x + PLAYER_WIDTH/2, playerPos.y + PLAYER_HEIGHT/2, 40, RGB(0, 255, 200));
            PlaySfx(SFX_HIT);
            return;
        }
        
        playerLives--;
        combo = 0;
        stageNoDamage = false; // 本关受伤
        invincibleTimer = 90; // 1.5秒无敌
        if (playerLevel > 0) playerLevel--;
        
        SpawnExplosion(playerPos.x + PLAYER_WIDTH/2, playerPos.y + PLAYER_HEIGHT/2, 20, COLOR_PLAYER);
        SpawnRing(playerPos.x + PLAYER_WIDTH/2, playerPos.y + PLAYER_HEIGHT/2, 50, RGB(0, 180, 255));
        AddShake(10, 20);
        PlaySfx(SFX_HIT);
        
        if (playerLives > 0) {
            // 受伤提示飘字
            SpawnFloatingText(SCREEN_WIDTH / 2 - 40, 200, L"受到伤害!", RGB(255, 80, 80), 22);
        } else {
            invincibleTimer = -60; // 死亡动画时间
            // 大爆炸
            for (int i = 0; i < 5; i++) {
                SpawnExplosion(playerPos.x + PLAYER_WIDTH/2 + (rand()%20-10), 
                              playerPos.y + PLAYER_HEIGHT/2 + (rand()%20-10), 
                              15, RGB(255,100 + rand()%155,0));
            }
            SpawnRing(playerPos.x + PLAYER_WIDTH/2, playerPos.y + PLAYER_HEIGHT/2, 70, RGB(255, 120, 0));
            AddShake(12, 30);
            PlaySfx(SFX_BIGEXP);
        }
    }
    
    void UseBomb() {
        if (bombCount <= 0) return;
        bombCount--;
        // 清屏：清除所有敌人子弹
        for (auto& b : enemyBullets) b.active = false;
        // 伤害所有敌人
        for (auto& e : enemies) {
            if (!e.active) continue;
            e.hp -= 10;
            SpawnExplosion(e.pos.x + e.width/2, e.pos.y + e.height/2, 10, RGB(255,255,200));
            if (e.hp <= 0) {
                OnEnemyKilled(e, false);
            }
        }
        // 全屏冲击波 + 白屏闪烁 + 屏幕震动 + 慢动作
        for (int i = 0; i < 4; i++) {
            SpawnRing(rand() % SCREEN_WIDTH, rand() % SCREEN_HEIGHT, 60 + rand() % 60, RGB(255, 255, 255));
        }
        for (int i = 0; i < 40; i++) {
            SpawnExplosion(rand() % SCREEN_WIDTH, rand() % SCREEN_HEIGHT, 1, RGB(255,255,255));
        }
        AddShake(12, 30);
        AddFlash(12);
        AddSlowmo(35);
        PlaySfx(SFX_BOMB);
    }
    
    // ===================== 渲染 =====================
    void Render() {
        HDC hdc = GetDC(hWnd);
        RECT rect;
        GetClientRect(hWnd, &rect);

        HDC hdcDraw = hdcBuffer;   // hbmBuffer 常驻选择，无需每帧 SelectObject

        // 背景
        DrawBackground(hdcDraw);

        if (state == STATE_MENU) {
            DrawMenu(hdcDraw);
        } else if (state == STATE_PLAYING) {
            DrawGame(hdcDraw);
        } else if (state == STATE_PAUSED) {
            DrawGame(hdcDraw); // 保留当前画面
            DrawPause(hdcDraw);
        } else if (state == STATE_GAMEOVER) {
            DrawGame(hdcDraw); // 保留最后一帧
            DrawGameOver(hdcDraw);
        }

        // 复制到屏幕（保持宽高比，等比缩放 + 黑边）
        int cw = rect.right - rect.left, ch = rect.bottom - rect.top;
        if (cw <= 0 || ch <= 0) { cw = SCREEN_WIDTH; ch = SCREEN_HEIGHT; }
        float srcAspect = (float)SCREEN_WIDTH / SCREEN_HEIGHT;
        float dstAspect = (float)cw / ch;
        int dx = 0, dy = 0, dw = cw, dh = ch;
        if (dstAspect > srcAspect) {
            // 窗口偏宽：以高度为准
            dh = ch;
            dw = (int)(ch * srcAspect);
            dx = (cw - dw) / 2;
        } else {
            // 窗口偏高：以宽度为准
            dw = cw;
            dh = (int)(cw / srcAspect);
            dy = (ch - dh) / 2;
        }
        HBRUSH black = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &rect, black);
        DeleteObject(black);
        if (dw == SCREEN_WIDTH && dh == SCREEN_HEIGHT && dx == 0 && dy == 0) {
            // 客户区恰好 1:1 时直接拷贝，完全无插值
            BitBlt(hdc, 0, 0, cw, ch, hdcDraw, 0, 0, SRCCOPY);
        } else {
            // COLORONCOLOR 最近邻缩放：无 HALFTONE 抖动，动态画面不闪不花
            SetStretchBltMode(hdc, COLORONCOLOR);
            StretchBlt(hdc, dx, dy, dw, dh, hdcDraw, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SRCCOPY);
        }

        ReleaseDC(hWnd, hdc);
    }
    
    void DrawBackground(HDC hdc) {
        // 预渲染的渐变深空 + 星云，一次 BitBlt 完成
        BitBlt(hdc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hdcBg, 0, 0, SRCCOPY);

        // 三层视差星星（带正弦闪烁）
        for (auto& s : stars) {
            float tw = 0.55f + 0.45f * std::sin(uiTick * 0.05f * s.speed + s.phase);
            int a = (int)(s.brightness * tw);
            COLORREF starColor;
            if (s.layer == 0) {
                starColor = RGB(a / 3, a / 3, a / 2 + 20);
            } else if (s.layer == 1) {
                starColor = RGB(a, a, a);
            } else {
                starColor = RGB(a, a, 255);
            }
            SetPixel(hdc, s.x, s.y, starColor);
            if (s.layer == 2) {
                SetPixel(hdc, s.x + 1, s.y, starColor);
                SetPixel(hdc, s.x, s.y + 1, starColor);
                SetPixel(hdc, s.x + 1, s.y + 1, starColor);
            } else if (s.layer == 1) {
                SetPixel(hdc, s.x + 1, s.y, starColor);
            }
        }
    }
    
    void DrawMenu(HDC hdc) {
        SetBkMode(hdc, TRANSPARENT);

        // 标题辉光（加法叠加，柔光效果）
        DrawGlow(hdc, SCREEN_WIDTH / 2.0f, 188, 110, RGB(0, 70, 170));
        DrawGlow(hdc, SCREEN_WIDTH / 2.0f, 188, 55, RGB(0, 150, 255));

        // 标题（阴影 + 主体双层）
        HFONT oldFont = (HFONT)SelectObject(hdc, FontPx(56));
        SetTextColor(hdc, RGB(0, 60, 140));
        RECT titleRect = {5, 150, SCREEN_WIDTH, 230};
        DrawText(hdc, L"雷霆战机", -1, &titleRect, DT_CENTER);
        SetTextColor(hdc, RGB(0, 190, 255));
        titleRect = {0, 145, SCREEN_WIDTH, 225};
        DrawText(hdc, L"雷霆战机", -1, &titleRect, DT_CENTER);

        // 装饰线 + 两端亮点
        HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 150, 255));
        HPEN oldPen = (HPEN)SelectObject(hdc, hPen);
        MoveToEx(hdc, 90, 235, NULL);
        LineTo(hdc, SCREEN_WIDTH - 90, 235);
        SelectObject(hdc, oldPen);
        DeleteObject(hPen);
        FilledEllipse(hdc, 84, 231, 92, 239, RGB(0, 190, 255), RGB(140, 230, 255), 1);
        FilledEllipse(hdc, SCREEN_WIDTH - 92, 231, SCREEN_WIDTH - 84, 239, RGB(0, 190, 255), RGB(140, 230, 255), 1);

        // ---- 模式选择 ----
        SelectObject(hdc, FontPx(17));
        SetTextColor(hdc, RGB(150, 160, 180));
        RECT modeLabel = {40, 256, SCREEN_WIDTH - 40, 282};
        DrawText(hdc, L"选择模式 (← →)", -1, &modeLabel, DT_CENTER);
        SetTextColor(hdc, RGB(0, 220, 255));
        const wchar_t* modeNames[3] = { L"剧情模式", L"生存模式", L"Boss Rush" };
        wchar_t modeBuf[64];
        wsprintfW(modeBuf, L"[ %s ]", modeNames[selectedMode]);
        RECT modeSel = {40, 284, SCREEN_WIDTH - 40, 312};
        DrawText(hdc, modeBuf, -1, &modeSel, DT_CENTER);
        DrawTri(hdc, SCREEN_WIDTH / 2 - 105, 296, 6, -1, RGB(0, 200, 255));
        DrawTri(hdc, SCREEN_WIDTH / 2 + 105, 296, 6, 1, RGB(0, 200, 255));

        // ---- 机体选择 ----
        SetTextColor(hdc, RGB(150, 160, 180));
        RECT shipLabel = {40, 318, SCREEN_WIDTH - 40, 344};
        DrawText(hdc, L"选择机体 (↑ ↓)", -1, &shipLabel, DT_CENTER);
        wchar_t shipBuf[96];
        if (shipUnlocked[selectedShip]) {
            SetTextColor(hdc, RGB(120, 255, 180));
            wsprintfW(shipBuf, L"[ %s ]  生命%d  炸弹%d", shipNames[selectedShip],
                      shipLives[selectedShip], shipBombs[selectedShip]);
        } else {
            SetTextColor(hdc, RGB(110, 110, 120));
            wsprintfW(shipBuf, L"[ ??? ]  未解锁（达成成就解锁）");
        }
        RECT shipSel = {20, 346, SCREEN_WIDTH - 20, 374};
        DrawText(hdc, shipBuf, -1, &shipSel, DT_CENTER);
        DrawTri(hdc, SCREEN_WIDTH / 2 - 115, 358, 6, -1, RGB(0, 200, 255));
        DrawTri(hdc, SCREEN_WIDTH / 2 + 115, 358, 6, 1, RGB(0, 200, 255));

        // ---- 成就概览 ----
        int achCount = 0;
        for (int i = 0; i < MAX_ACHIEVEMENTS; i++) if (achUnlocked[i]) achCount++;
        SelectObject(hdc, FontPx(15));
        SetTextColor(hdc, RGB(230, 200, 90));
        wsprintfW(shipBuf, L"成就 %d/%d   %s · %s · %s", achCount, MAX_ACHIEVEMENTS,
                  achUnlocked[ACH_KILLER] ? L"初出茅庐" : L"？",
                  achUnlocked[ACH_CLEAN] ? L"无伤通关" : L"？",
                  achUnlocked[ACH_DANCER] ? L"弹幕舞者" : L"？");
        RECT achRect = {20, 380, SCREEN_WIDTH - 20, 406};
        DrawText(hdc, shipBuf, -1, &achRect, DT_CENTER);

        // 操作提示
        SelectObject(hdc, FontPx(16));
        SetTextColor(hdc, RGB(170, 180, 200));
        RECT infoRect = {20, 418, SCREEN_WIDTH - 20, 444};
        DrawText(hdc, L"↑ ↓ ← →  移动飞机    SPACE 射击 / 按住蓄力", -1, &infoRect, DT_CENTER);
        infoRect.top = 444; infoRect.bottom = 470;
        DrawText(hdc, L"B 炸弹    ESC 暂停", -1, &infoRect, DT_CENTER);

        // 开始提示（正弦呼吸脉冲，替代硬闪烁）
        float pulse = 0.55f + 0.45f * std::sin(uiTick * 0.09f);
        SelectObject(hdc, FontPx(30));
        SetTextColor(hdc, ScaleColor(RGB(255, 220, 50), pulse));
        RECT startRect = {20, 494, SCREEN_WIDTH - 20, 546};
        DrawText(hdc, L"按 ENTER 开始游戏", -1, &startRect, DT_CENTER);

        // 版权与最高分
        SelectObject(hdc, FontPx(15));
        SetTextColor(hdc, RGB(90, 95, 110));
        RECT creditRect = {20, SCREEN_HEIGHT - 66, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 42};
        DrawText(hdc, L"雷霆战机 v2.0  |  C++ Win32 Game", -1, &creditRect, DT_CENTER);
        SetTextColor(hdc, RGB(210, 210, 130));
        RECT highScoreRect = {20, SCREEN_HEIGHT - 38, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 14};
        wchar_t highScoreBuf[64];
        wsprintfW(highScoreBuf, L"最高分: %d", highScore);
        DrawText(hdc, highScoreBuf, -1, &highScoreRect, DT_CENTER);

        SelectObject(hdc, oldFont);
    }
    
    void DrawGame(HDC hdc) {
        SetBkMode(hdc, TRANSPARENT);
        
        // 屏幕震动（游戏对象整体偏移，HUD 不震）
        POINT oldOrg;
        int ox = 0, oy = 0;
        if (shakeTimer > 0 && shakePower > 0) {
            ox = (rand() % (shakePower * 2 + 1)) - shakePower;
            oy = (rand() % (shakePower * 2 + 1)) - shakePower;
        }
        SetViewportOrgEx(hdc, ox, oy, &oldOrg);
        
        // ---- 绘制道具（辉光 + 呼吸脉冲）----
        for (auto& p : powerups) {
            if (!p.active) continue;
            float pulse = 0.5f + 0.5f * std::sin(frameCount * 0.15f);
            int cx = (int)p.pos.x + 10, cy = (int)p.pos.y + 10;
            COLORREF col;
            wchar_t icon;
            if (p.type == 0)      { col = RGB(255, 200, 60);  icon = L'P'; }
            else if (p.type == 1) { col = RGB(60, 220, 255);  icon = L'S'; }
            else if (p.type == 2) { col = RGB(255, 90, 70);   icon = L'B'; }
            else if (p.type == 3) { col = RGB(120, 160, 255); icon = L'W'; }
            else                  { col = RGB(60, 255, 210);  icon = L'O'; }
            DrawGlow(hdc, (float)cx, (float)cy, 15 + (int)(pulse * 7), col);
            FilledEllipse(hdc, cx - 10, cy - 10, cx + 10, cy + 10, ScaleColor(col, 0.35f), col, 2);
            HFONT of = (HFONT)SelectObject(hdc, FontPx(13));
            SetTextColor(hdc, RGB(255, 255, 255));
            RECT ir = { cx - 10, cy - 10, cx + 10, cy + 10 };
            DrawText(hdc, &icon, 1, &ir, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(hdc, of);
        }
        
        // ---- 绘制敌人 ----
        for (auto& e : enemies) {
            if (!e.active) continue;

            bool flash = (e.hitFlash > 0);
            COLORREF color;
            if (e.type == 0) color = RGB(255, 90, 90);
            else if (e.type == 1) color = RGB(255, 70, 200);
            else if (e.type == 3) color = RGB(255, 150, 40); // 自爆机
            else if (e.type == 4) color = RGB(70, 210, 190); // 护盾机
            else if (e.type == 5) color = RGB(210, 90, 255); // 召唤机
            else color = (e.hp * 2 <= e.maxHp) ? RGB(255, 70, 70) : RGB(210, 70, 255); // Boss 半血变红
            if (flash) color = RGB(255, 255, 255);

            int ex = (int)e.pos.x, ey = (int)e.pos.y;

            if (e.type == 0) {
                // 小型敌机 - 菱形
                POINT pts[4] = {
                    {ex + e.width/2, ey},
                    {ex + e.width, ey + e.height/2},
                    {ex + e.width/2, ey + e.height},
                    {ex, ey + e.height/2}
                };
                FilledPoly(hdc, pts, 4, flash ? color : ScaleColor(color, 0.55f), color, 2);
            } else if (e.type == 1) {
                // 中型敌机 - 六边形
                POINT pts[6] = {
                    {ex + e.width/2, ey},
                    {ex + e.width, ey + e.height/4},
                    {ex + e.width, ey + e.height*3/4},
                    {ex + e.width/2, ey + e.height},
                    {ex, ey + e.height*3/4},
                    {ex, ey + e.height/4}
                };
                FilledPoly(hdc, pts, 6, flash ? color : ScaleColor(color, 0.55f), color, 2);

                // 血条（按血量变色）
                if (e.hp < e.maxHp) {
                    float hpRatio = (float)e.hp / e.maxHp;
                    COLORREF hpc = (hpRatio > 0.5f) ? RGB(80, 220, 90) : (hpRatio > 0.25f) ? RGB(255, 200, 60) : RGB(255, 70, 70);
                    FillRectC(hdc, ex, ey - 7, ex + e.width, ey - 3, RGB(60, 10, 14));
                    FillRectC(hdc, ex + 1, ey - 6, ex + 1 + (int)((e.width - 2) * hpRatio), ey - 4, hpc);
                }
            } else if (e.type == 3) {
                // 自爆机 - 小三角（朝下）+ 危险闪烁辉光
                POINT pts[3] = {
                    {ex + e.width/2, ey + e.height},
                    {ex, ey},
                    {ex + e.width, ey}
                };
                FilledPoly(hdc, pts, 3, flash ? color : ScaleColor(color, 0.6f), RGB(255, 220, 180), 1);
                if ((frameCount / 6) % 2 == 0)
                    DrawGlow(hdc, ex + e.width/2.0f, ey + e.height/2.0f, 11, RGB(255, 170, 60));
            } else if (e.type == 4) {
                // 护盾机：方形机身 + 前方能量盾
                bool shielded = (e.shieldHp > 0);
                POINT sq[4] = {
                    {ex + 4, ey + 4}, {ex + e.width - 4, ey + 4},
                    {ex + e.width - 4, ey + e.height - 4}, {ex + 4, ey + e.height - 4}
                };
                COLORREF body = shielded ? RGB(70, 210, 190) : RGB(130, 130, 130);
                if (flash) body = RGB(255, 255, 255);
                FilledPoly(hdc, sq, 4, ScaleColor(body, 0.6f), body, 2);
                if (shielded) {
                    // 前方能量盾（横带）
                    FillRectC(hdc, ex + 1, ey - 6, ex + e.width - 1, ey + 6, ScaleColor(RGB(0, 255, 220), 0.22f));
                    FillRectC(hdc, ex + 1, ey - 6, ex + e.width - 1, ey - 4, RGB(0, 255, 220));
                    FillRectC(hdc, ex + 1, ey + 4, ex + e.width - 1, ey + 6, RGB(0, 255, 220));
                    // 盾血量条
                    float shRatio = (float)e.shieldHp / 3.0f;
                    FillRectC(hdc, ex, ey - 12, ex + e.width, ey - 8, RGB(0, 60, 55));
                    FillRectC(hdc, ex, ey - 12, ex + (int)(e.width * shRatio), ey - 9, RGB(0, 255, 210));
                }
            } else if (e.type == 5) {
                // 召唤机：六边形 + 光环 + 呼吸辉光
                POINT pts[6] = {
                    {ex + e.width/2, ey},
                    {ex + e.width, ey + e.height/4},
                    {ex + e.width, ey + e.height*3/4},
                    {ex + e.width/2, ey + e.height},
                    {ex, ey + e.height*3/4},
                    {ex, ey + e.height/4}
                };
                FilledPoly(hdc, pts, 6, flash ? color : ScaleColor(color, 0.55f), color, 2);
                HPEN ringPen = CreatePen(PS_DOT, 1, RGB(255, 170, 255));
                HPEN oldRing = (HPEN)SelectObject(hdc, ringPen);
                SelectObject(hdc, GetStockObject(NULL_BRUSH));
                Ellipse(hdc, ex - 6, ey + e.height/2 - 6, ex + e.width + 6, ey + e.height/2 + 6);
                SelectObject(hdc, oldRing);
                DeleteObject(ringPen);
                DrawGlow(hdc, ex + e.width/2.0f, ey + e.height/2.0f,
                         13 + (int)(std::sin(frameCount * 0.15f) * 3), RGB(210, 90, 255));
            } else {
                // 大型敌机(Boss) - 八边形 + 底光 + 狂暴核心
                POINT pts[8] = {
                    {ex + e.width/2, ey},
                    {ex + e.width*3/4, ey + e.height/4},
                    {ex + e.width, ey + e.height/2},
                    {ex + e.width*3/4, ey + e.height*3/4},
                    {ex + e.width/2, ey + e.height},
                    {ex + e.width/4, ey + e.height*3/4},
                    {ex, ey + e.height/2},
                    {ex + e.width/4, ey + e.height/4}
                };
                bool rage = (e.hp * 2 <= e.maxHp);
                DrawGlow(hdc, ex + e.width/2.0f, ey + e.height/2.0f, 42,
                         rage ? RGB(255, 60, 60) : RGB(190, 80, 255));
                FilledPoly(hdc, pts, 8, flash ? color : ScaleColor(color, 0.6f), RGB(255, 220, 255), 2);
                if (rage) {
                    // 狂暴能量核心（脉冲）
                    int pr = 10 + (int)(std::sin(frameCount * 0.2f) * 3);
                    FilledEllipse(hdc, ex + e.width/2 - pr, ey + e.height/2 - pr,
                                  ex + e.width/2 + pr, ey + e.height/2 + pr,
                                  RGB(255, 240, 120), RGB(255, 255, 200), 1);
                }
                // 血条（按血量变色）
                float hpRatio = (float)e.hp / e.maxHp;
                COLORREF hpc = (hpRatio > 0.5f) ? RGB(80, 220, 90) : (hpRatio > 0.25f) ? RGB(255, 200, 60) : RGB(255, 70, 70);
                FillRectC(hdc, ex, ey - 10, ex + e.width, ey - 4, RGB(60, 8, 12));
                FillRectC(hdc, ex + 1, ey - 9, ex + 1 + (int)((e.width - 2) * hpRatio), ey - 5, hpc);
            }
        }
        
        // ---- 绘制玩家子弹（辉光 + 曳光拖尾）----
        for (auto& b : playerBullets) {
            if (!b.active) continue;
            int bx = (int)b.pos.x, by = (int)b.pos.y;
            if (b.pierce) {
                // 穿透激光 - 长光束：外辉光 + 亮核
                int bcx = bx + b.width / 2;
                if (b.height > 100) {
                    // 蓄力重激光：沿光束多点辉光
                    for (int gy = 40; gy < b.height; gy += 60)
                        DrawGlow(hdc, (float)bcx, by + (float)gy, 20, RGB(0, 190, 255));
                } else {
                    DrawGlow(hdc, (float)bcx, by + b.height / 2.0f, 14, RGB(0, 190, 255));
                }
                FillRectC(hdc, bx, by, bx + b.width, by + b.height, RGB(0, 170, 255));
                FillRectC(hdc, bcx - 1, by, bcx + 1, by + b.height, RGB(235, 255, 255));
            } else if (b.homing) {
                // 追踪导弹 - 弹体 + 尾焰 + 辉光
                int bcx = bx + b.width / 2, bcy = by + b.height / 2;
                DrawGlow(hdc, (float)bcx, (float)bcy, 10, RGB(255, 150, 40));
                FillRectC(hdc, bx + 1, by, bx + b.width - 1, by + b.height, RGB(255, 170, 60));
                FillRectC(hdc, bx + 2, by + 1, bx + b.width - 2, by + 6, RGB(255, 240, 200));
                FillRectC(hdc, bx + 2, by + b.height, bx + b.width - 2, by + b.height + 5,
                          ScaleColor(RGB(255, 120, 20), 0.65f));
            } else {
                // 常规弹：拖尾 + 辉光弹头 + 亮芯
                int bcx = bx + b.width / 2;
                FillRectC(hdc, bcx - 1, by + b.height, bcx + 1, by + b.height + 6, RGB(170, 135, 40));
                DrawGlow(hdc, (float)bcx, by + 4.0f, 8, RGB(255, 230, 120));
                FillRectC(hdc, bx, by, bx + b.width, by + b.height, RGB(255, 235, 140));
                FillRectC(hdc, bx + 1, by + 2, bx + b.width - 1, by + 6, RGB(255, 255, 230));
            }
        }
        
        // ---- 绘制敌人子弹（品红高辨识 + 白芯 + 辉光）----
        for (auto& b : enemyBullets) {
            if (!b.active) continue;
            int bx = (int)b.pos.x, by = (int)b.pos.y;
            int bcx = bx + b.width / 2, bcy = by + b.height / 2;
            DrawGlow(hdc, (float)bcx, (float)bcy, 9, RGB(255, 90, 140));
            FilledEllipse(hdc, bx, by, bx + b.width, by + b.height,
                          RGB(255, 110, 150), RGB(255, 200, 220), 1);
            FillRectC(hdc, bcx - 1, bcy - 1, bcx + 1, bcy + 1, RGB(255, 235, 245));
        }
        
        // ---- 绘制粒子（含环形冲击波）----
        for (auto& p : particles) {
            if (!p.active) continue;
            float alpha = (float)p.life / p.maxLife;
            COLORREF c = ScaleColor(p.color, alpha);

            if (p.type == 1) {
                // 环形冲击波：随时间扩散
                float t = 1.0f - alpha;
                int rad = 6 + (int)(t * p.size);
                HPEN ringPen = CreatePen(PS_SOLID, 1 + (int)(2 * alpha), c);
                HPEN oldRingPen = (HPEN)SelectObject(hdc, ringPen);
                SelectObject(hdc, GetStockObject(NULL_BRUSH));
                Ellipse(hdc, (int)p.pos.x - rad, (int)p.pos.y - rad,
                        (int)p.pos.x + rad, (int)p.pos.y + rad);
                SelectObject(hdc, oldRingPen);
                DeleteObject(ringPen);
            } else {
                int sz = (int)(p.size * alpha);
                if (sz < 1) sz = 1;
                FilledEllipse(hdc, (int)p.pos.x - sz, (int)p.pos.y - sz,
                              (int)p.pos.x + sz, (int)p.pos.y + sz, c, c, 1);
            }
        }
        
        // ---- 绘制玩家（无敌时半透明呼吸闪烁，不再硬性消失）----
        if (invincibleTimer > 0 && (invincibleTimer / 5) % 2 == 0) {
            DrawPlayer(hdc, 0.35f);
        } else {
            DrawPlayer(hdc, 1.0f);
        }

        // ---- 蓄力指示条 ----
        if (chargeAmount > 0) {
            int cw = 44, chh = 6;
            int cx = (int)playerPos.x + PLAYER_WIDTH / 2 - cw / 2;
            int cy = (int)playerPos.y + PLAYER_HEIGHT + 8;
            bool full = (chargeAmount >= CHARGE_THRESHOLD);
            FillRectC(hdc, cx, cy, cx + cw, cy + chh, RGB(45, 48, 66));
            float ratio = (float)chargeAmount / CHARGE_THRESHOLD;
            if (ratio > 1.0f) ratio = 1.0f;
            FillRectC(hdc, cx, cy, cx + (int)(cw * ratio), cy + chh,
                      full ? RGB(0, 255, 255) : RGB(0, 150, 255));
            if (full) {
                // 满蓄力：机头辉光 + 前方预示光束
                int nx = (int)playerPos.x + PLAYER_WIDTH / 2;
                int ny = (int)playerPos.y - 6;
                DrawGlow(hdc, (float)nx, (float)ny, 15 + (int)(std::sin(frameCount * 0.3f) * 4),
                         RGB(0, 220, 255));
                float flick = 0.5f + 0.5f * std::sin(frameCount * 0.8f);
                FillRectC(hdc, nx - 1, ny - 194, nx + 1, ny,
                          ScaleColor(RGB(0, 200, 255), 0.35f + 0.25f * flick));
            }
        }

        // ---- 环形护盾弹（辉光 + 拖尾）----
        if (orbitTimer > 0) {
            float pcx = playerPos.x + PLAYER_WIDTH / 2;
            float pcy = playerPos.y + PLAYER_HEIGHT / 2;
            for (int i = 0; i < orbitCount; i++) {
                for (int tr = 2; tr >= 0; tr--) {
                    float ang = (frameCount - tr * 3) * 0.08f + (float)i * (6.2831853f / orbitCount);
                    float gx = pcx + std::cos(ang) * 36.0f;
                    float gy = pcy + std::sin(ang) * 36.0f;
                    if (tr == 0) {
                        DrawGlow(hdc, gx, gy, 11, RGB(0, 255, 220));
                        FilledEllipse(hdc, (int)gx - 4, (int)gy - 4, (int)gx + 4, (int)gy + 4,
                                      RGB(190, 255, 246), RGB(0, 255, 220), 1);
                    } else {
                        COLORREF tc = ScaleColor(RGB(0, 255, 220), tr == 1 ? 0.45f : 0.2f);
                        FilledEllipse(hdc, (int)gx - 3, (int)gy - 3, (int)gx + 3, (int)gy + 3, tc, tc, 1);
                    }
                }
            }
        }
        
        // 恢复视口（结束震动），HUD 不震动
        SetViewportOrgEx(hdc, oldOrg.x, oldOrg.y, NULL);

        // ---- 飘字（带投影，随生命渐隐）----
        for (auto& ft : floatTexts) {
            if (!ft.active) continue;
            float a = (float)ft.life / ft.maxLife;
            HFONT oldFont = (HFONT)SelectObject(hdc, FontPx(ft.size));
            RECT tr = {(int)ft.pos.x - 70, (int)ft.pos.y, (int)ft.pos.x + 70, (int)ft.pos.y + 30};
            SetTextColor(hdc, RGB(15, 15, 25));
            RECT ts = tr;
            OffsetRect(&ts, 1, 1);
            DrawText(hdc, ft.text, -1, &ts, DT_CENTER | DT_TOP | DT_SINGLELINE);
            SetTextColor(hdc, ScaleColor(ft.color, a));
            DrawText(hdc, ft.text, -1, &tr, DT_CENTER | DT_TOP | DT_SINGLELINE);
            SelectObject(hdc, oldFont);
        }

        // ---- Boss 警告横幅（深红警示带 + 闪烁文字）----
        if (bossWarningTimer > 0) {
            FillRectC(hdc, 0, 52, SCREEN_WIDTH, 106, RGB(48, 0, 10));
            FillRectC(hdc, 0, 52, SCREEN_WIDTH, 55, RGB(130, 25, 35));
            FillRectC(hdc, 0, 103, SCREEN_WIDTH, 106, RGB(130, 25, 35));
            if ((frameCount / 6) % 2 == 0) {
                HFONT oldWarnFont = (HFONT)SelectObject(hdc, FontPx(32));
                SetTextColor(hdc, RGB(255, 70, 70));
                RECT warnRect = {0, 58, SCREEN_WIDTH, 100};
                DrawText(hdc, L"⚠ WARNING ⚠", -1, &warnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                SelectObject(hdc, oldWarnFont);
            }
        }

        // ---- 关卡剧情文字（上下细线 + 渐隐横幅）----
        if (stageIntroTimer > 0) {
            float a = (float)stageIntroTimer / 90.0f;
            if (a > 1.0f) a = 1.0f;
            FillRectC(hdc, 0, 158, SCREEN_WIDTH, 202, ScaleColor(RGB(10, 14, 30), a));
            FillRectC(hdc, 40, 158, SCREEN_WIDTH - 40, 160, ScaleColor(RGB(0, 160, 255), a * 0.8f));
            FillRectC(hdc, 40, 200, SCREEN_WIDTH - 40, 202, ScaleColor(RGB(0, 160, 255), a * 0.8f));
            HFONT oldIntro = (HFONT)SelectObject(hdc, FontPx(30));
            SetTextColor(hdc, ScaleColor(RGB(255, 220, 80), a));
            RECT ir = {0, 162, SCREEN_WIDTH, 198};
            DrawText(hdc, stageIntroText, -1, &ir, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(hdc, oldIntro);
        }

        // ---- 成就解锁弹窗（金色辉光横幅）----
        if (achPopupTimer > 0) {
            float a = (achPopupTimer < 60) ? achPopupTimer / 60.0f : 1.0f;
            FillRectC(hdc, 0, 294, SCREEN_WIDTH, 378, ScaleColor(RGB(24, 19, 5), a));
            DrawGlow(hdc, SCREEN_WIDTH / 2.0f, 336, 70, RGB(255, 220, 80));
            HFONT oldAch = (HFONT)SelectObject(hdc, FontPx(24));
            SetTextColor(hdc, ScaleColor(RGB(255, 220, 80), a));
            wchar_t achBuf[64];
            wsprintfW(achBuf, L"成就解锁: %s", AchName(achPopupIndex));
            RECT ar = {0, 300, SCREEN_WIDTH, 340};
            DrawText(hdc, achBuf, -1, &ar, DT_CENTER);
            SelectObject(hdc, FontPx(15));
            SetTextColor(hdc, ScaleColor(RGB(225, 205, 155), a));
            RECT ar2 = {0, 344, SCREEN_WIDTH, 372};
            DrawText(hdc, AchDesc(achPopupIndex), -1, &ar2, DT_CENTER);
            SelectObject(hdc, oldAch);
        }

        // ---- 绘制HUD ----
        DrawHUD(hdc);

        // ---- 白屏闪烁（炸弹/击杀Boss，柔和衰减）----
        if (flashTimer > 0 && flashMax > 0) {
            int v = (int)(190.0f * flashTimer / flashMax);
            HBRUSH fb = CreateSolidBrush(RGB(v, v, v < 210 ? v + 20 : 255));
            RECT fr = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
            FillRect(hdc, &fr, fb);
            DeleteObject(fb);
        }
    }
    
    void DrawPlayer(HDC hdc, float dim) {
        int px = (int)playerPos.x, py = (int)playerPos.y;
        int cx = px + PLAYER_WIDTH / 2;
        COLORREF cBody = ScaleColor(pColor2, dim);
        COLORREF cLine = ScaleColor(pColor, dim);
        COLORREF cCock = ScaleColor(RGB(230, 250, 255), dim);

        // 机身辉光
        if (dim > 0.5f)
            DrawGlow(hdc, (float)cx, py + PLAYER_HEIGHT / 2.0f, 30, RGB(0, 110, 220));

        // 引擎火焰（外焰 + 白热内焰 + 辉光，长度随机抖动）
        int flen = 10 + rand() % 7;
        POINT flameO[3] = {
            {cx - 5, py + PLAYER_HEIGHT - 8},
            {cx + 5, py + PLAYER_HEIGHT - 8},
            {cx, py + PLAYER_HEIGHT - 8 + flen}
        };
        FilledPoly(hdc, flameO, 3, ScaleColor(RGB(255, 120, 20), dim), ScaleColor(RGB(255, 120, 20), dim), 1);
        POINT flameI[3] = {
            {cx - 2, py + PLAYER_HEIGHT - 8},
            {cx + 2, py + PLAYER_HEIGHT - 8},
            {cx, py + PLAYER_HEIGHT - 8 + flen * 2 / 3}
        };
        FilledPoly(hdc, flameI, 3, ScaleColor(RGB(255, 230, 150), dim), ScaleColor(RGB(255, 230, 150), dim), 1);
        if (dim > 0.5f)
            DrawGlow(hdc, (float)cx, (float)(py + PLAYER_HEIGHT - 6), 12, RGB(255, 150, 40));

        // 机身（六点战机轮廓：机头-翼尖-翼根-尾锥）
        POINT body[6] = {
            {cx, py},                                          // 机头
            {px + PLAYER_WIDTH + 2, py + PLAYER_HEIGHT - 14},  // 右翼尖
            {px + PLAYER_WIDTH - 10, py + PLAYER_HEIGHT - 10}, // 右翼根
            {cx, py + PLAYER_HEIGHT - 4},                      // 尾部中点
            {px + 10, py + PLAYER_HEIGHT - 10},                // 左翼根
            {px - 2, py + PLAYER_HEIGHT - 14}                  // 左翼尖
        };
        FilledPoly(hdc, body, 6, cBody, cLine, 2);

        // 侧引擎喷口
        FillRectC(hdc, px + 8, py + PLAYER_HEIGHT - 8, px + 13, py + PLAYER_HEIGHT - 4, ScaleColor(RGB(90, 150, 220), dim));
        FillRectC(hdc, px + PLAYER_WIDTH - 13, py + PLAYER_HEIGHT - 8, px + PLAYER_WIDTH - 8, py + PLAYER_HEIGHT - 4, ScaleColor(RGB(90, 150, 220), dim));

        // 座舱
        FilledEllipse(hdc, cx - 5, py + 7, cx + 5, py + 18, cCock, ScaleColor(RGB(160, 230, 255), dim), 1);

        // 护盾（脉冲圆环 + 辉光）
        if (shieldActive) {
            int sr = (int)(30 + std::sin(frameCount * 0.2f) * 3);
            if (dim > 0.5f)
                DrawGlow(hdc, (float)cx, py + PLAYER_HEIGHT / 2.0f, sr + 8, RGB(0, 255, 200));
            HPEN shieldPen = CreatePen(PS_DOT, 1, ScaleColor(COLOR_SHIELD, dim));
            HPEN oldSp = (HPEN)SelectObject(hdc, shieldPen);
            SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Ellipse(hdc, cx - sr, py + PLAYER_HEIGHT/2 - sr, cx + sr, py + PLAYER_HEIGHT/2 + sr);
            SelectObject(hdc, oldSp);
            DeleteObject(shieldPen);
        }
    }
    
    void DrawHUD(HDC hdc) {
        HFONT oldFont = (HFONT)SelectObject(hdc, FontPx(15));
        wchar_t buf[64];

        // 带投影的文字（任何背景下都清晰，且不遮挡战场）
        auto hudText = [&](int x, int y, COLORREF c, const wchar_t* s) {
            SetTextColor(hdc, RGB(10, 12, 22));
            TextOut(hdc, x + 1, y + 1, s, (int)wcslen(s));
            SetTextColor(hdc, c);
            TextOut(hdc, x, y, s, (int)wcslen(s));
        };

        // 分数
        wsprintfW(buf, L"分数: %d", score);
        hudText(10, 8, RGB(255, 255, 255), buf);
        // 最高分
        wsprintfW(buf, L"最高分: %d", highScore);
        hudText(10, 28, RGB(215, 215, 135), buf);
        // 关卡 / Boss / 模式信息
        if (gameMode == GAMEMODE_SURVIVAL) {
            wsprintfW(buf, L"生存: %d秒  击杀: %d", survivalFrames / 60, totalKills);
        } else if (gameMode == GAMEMODE_BOSSRUSH) {
            wsprintfW(buf, L"BOSS RUSH  #%d", bossRushIndex + 1);
        } else if (bossStage && !bossSpawned) {
            wsprintfW(buf, L"关卡: %d  BOSS即将来袭", stage);
        } else {
            wsprintfW(buf, L"关卡: %d", stage);
        }
        hudText(10, 48, RGB(90, 215, 255), buf);
        // 连击
        if (combo > 1) {
            wsprintfW(buf, L"连击 x%d", combo);
            hudText(10, 68, RGB(255, 220, 50), buf);
        }
        // 火力等级
        wsprintfW(buf, L"火力 ");
        int base = 3;
        for (int i = 0; i < playerLevel + 1; i++) {
            buf[base + i] = L'★';
        }
        buf[base + playerLevel + 1] = 0;
        hudText(10, 88, RGB(90, 215, 255), buf);
        // 武器
        const wchar_t* weaponNames[4] = { L"机枪", L"散射", L"追踪导弹", L"激光" };
        wsprintfW(buf, L"武器: %s", weaponNames[weapon]);
        hudText(10, 108, RGB(100, 225, 255), buf);
        // 炸弹数量
        wsprintfW(buf, L"炸弹: %d", bombCount);
        hudText(10, 128, RGB(255, 185, 60), buf);
        // 护盾指示
        if (shieldActive) {
            wsprintfW(buf, L"护盾: %d秒", shieldTimer / 60 + 1);
            hudText(10, 148, RGB(0, 255, 200), buf);
        }
        // 轨道盾指示
        if (orbitTimer > 0) {
            wsprintfW(buf, L"轨道盾: %d秒", orbitTimer / 60 + 1);
            hudText(10, 168, RGB(0, 255, 220), buf);
        }

        // 生命（右上角小飞机图标 + 辉光）
        for (int i = 0; i < playerLives; i++) {
            int lx = SCREEN_WIDTH - 24 - i * 22;
            int ly = 12;
            DrawGlow(hdc, (float)lx + 7, (float)ly + 7, 9, RGB(255, 60, 60));
            POINT tri[3] = { {lx + 7, ly}, {lx, ly + 14}, {lx + 14, ly + 14} };
            FilledPoly(hdc, tri, 3, RGB(255, 70, 70), RGB(255, 165, 165), 1);
        }

        SelectObject(hdc, oldFont);
    }
    
    // 半透明暗化遮罩：50% 网纹模拟，保留背后战场画面
    void DimOverlay(HDC hdc) {
        SetBkMode(hdc, TRANSPARENT);
        HBRUSH overlay = CreateHatchBrush(HS_BDIAGONAL, RGB(0, 0, 0));
        RECT fullRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        FillRect(hdc, &fullRect, overlay);
        DeleteObject(overlay);
    }

    // 暂停菜单
    void DrawPause(HDC hdc) {
        DimOverlay(hdc);

        // 中央面板
        FillRectC(hdc, 90, 200, SCREEN_WIDTH - 90, 400, RGB(14, 18, 34));
        HPEN bp = CreatePen(PS_SOLID, 2, RGB(40, 90, 160));
        HPEN obp = (HPEN)SelectObject(hdc, bp);
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, 90, 200, SCREEN_WIDTH - 90, 400);
        SelectObject(hdc, obp);
        DeleteObject(bp);

        HFONT oldFont = (HFONT)SelectObject(hdc, FontPx(40));
        SetTextColor(hdc, RGB(0, 190, 255));
        RECT pauseRect = {0, 226, SCREEN_WIDTH, 282};
        DrawText(hdc, L"已暂停", -1, &pauseRect, DT_CENTER);

        SelectObject(hdc, FontPx(17));
        float pulse = 0.55f + 0.45f * std::sin(GetTickCount() * 0.006f);
        SetTextColor(hdc, ScaleColor(RGB(190, 200, 220), pulse));
        RECT hintRect = {0, 330, SCREEN_WIDTH, 358};
        DrawText(hdc, L"ESC 继续   |   R 重新开始", -1, &hintRect, DT_CENTER);

        SelectObject(hdc, oldFont);
    }

    void DrawGameOver(HDC hdc) {
        DimOverlay(hdc);

        // 中央面板
        FillRectC(hdc, 60, 170, SCREEN_WIDTH - 60, 500, RGB(18, 13, 22));
        HPEN bp = CreatePen(PS_SOLID, 2, RGB(150, 45, 65));
        HPEN obp = (HPEN)SelectObject(hdc, bp);
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, 60, 170, SCREEN_WIDTH - 60, 500);
        SelectObject(hdc, obp);
        DeleteObject(bp);

        HFONT oldFont = (HFONT)SelectObject(hdc, FontPx(44));
        SetTextColor(hdc, RGB(255, 70, 70));
        RECT goRect = {0, 198, SCREEN_WIDTH, 258};
        DrawText(hdc, L"游戏结束", -1, &goRect, DT_CENTER);

        // 分数
        SelectObject(hdc, FontPx(24));
        SetTextColor(hdc, RGB(255, 240, 130));
        wchar_t buf[64];
        wsprintfW(buf, L"最终得分: %d", score);
        RECT scoreRect = {0, 288, SCREEN_WIDTH, 330};
        DrawText(hdc, buf, -1, &scoreRect, DT_CENTER);

        // 模式结果
        SelectObject(hdc, FontPx(16));
        SetTextColor(hdc, RGB(150, 220, 255));
        if (gameMode == GAMEMODE_SURVIVAL) {
            wsprintfW(buf, L"生存时间: %d秒  击毁: %d", survivalFrames / 60, totalKills);
        } else if (gameMode == GAMEMODE_BOSSRUSH) {
            wsprintfW(buf, L"击败Boss: %d 个", bossRushIndex);
        } else {
            wsprintfW(buf, L"到达关卡: %d", stage);
        }
        RECT modeRect = {0, 342, SCREEN_WIDTH, 374};
        DrawText(hdc, buf, -1, &modeRect, DT_CENTER);

        // 提示（呼吸脉冲）
        float pulse = 0.55f + 0.45f * std::sin(uiTick * 0.09f);
        SetTextColor(hdc, ScaleColor(RGB(190, 190, 200), pulse));
        RECT restartRect = {0, 396, SCREEN_WIDTH, 426};
        DrawText(hdc, L"按 ENTER 重新开始", -1, &restartRect, DT_CENTER);
        restartRect.top = 426; restartRect.bottom = 456;
        DrawText(hdc, L"按 ESC 返回主菜单", -1, &restartRect, DT_CENTER);

        // 最高分
        SetTextColor(hdc, RGB(210, 210, 115));
        wchar_t hsBuf[64];
        wsprintfW(hsBuf, L"最高分: %d", highScore);
        RECT hsRect = {0, 462, SCREEN_WIDTH, 492};
        DrawText(hdc, hsBuf, -1, &hsRect, DT_CENTER);

        SelectObject(hdc, oldFont);
    }
    
    // ===================== 按键处理 =====================
    void KeyDown(int key) {
        // 主菜单：选择模式/机体
        if (state == STATE_MENU) {
            if (key == VK_LEFT || key == VK_RIGHT) {
                selectedMode = (selectedMode + (key == VK_RIGHT ? 1 : 2)) % 3;
                PlaySfx(SFX_POWERUP);
                return;
            }
            if (key == VK_UP || key == VK_DOWN) {
                int dir = (key == VK_DOWN) ? 1 : -1;
                for (int step = 0; step < MAX_SHIPS; step++) {
                    int s = (selectedShip + dir * (step + 1) + MAX_SHIPS) % MAX_SHIPS;
                    if (shipUnlocked[s]) { selectedShip = s; break; }
                }
                PlaySfx(SFX_POWERUP);
                return;
            }
        }
        switch (key) {
            case VK_LEFT: keyLeft = true; break;
            case VK_RIGHT: keyRight = true; break;
            case VK_UP: keyUp = true; break;
            case VK_DOWN: keyDown = true; break;
            case VK_SPACE: keySpace = true; break;
            case 'B': case 'b':
                if (state == STATE_PLAYING) UseBomb();
                break;
            case VK_ESCAPE:
                if (state == STATE_PLAYING) {
                    state = STATE_PAUSED;
                    StopBGM();
                } else if (state == STATE_PAUSED) {
                    state = STATE_PLAYING;
                    StartBGM();
                } else if (state == STATE_GAMEOVER) {
                    state = STATE_MENU;
                    StartBGM();   // 返回主菜单恢复音乐
                }
                break;
            case 'R': case 'r':
                if (state == STATE_PAUSED) {
                    StartGame();
                }
                break;
            case VK_RETURN:
                if (state == STATE_MENU || state == STATE_GAMEOVER) {
                    StartGame();
                } else if (state == STATE_PAUSED) {
                    state = STATE_PLAYING;
                    StartBGM();
                }
                break;
        }
    }
    
    void KeyUp(int key) {
        switch (key) {
            case VK_LEFT: keyLeft = false; break;
            case VK_RIGHT: keyRight = false; break;
            case VK_UP: keyUp = false; break;
            case VK_DOWN: keyDown = false; break;
            case VK_SPACE: keySpace = false; break;
        }
    }
};

// ===================== Windows 窗口过程 =====================
ThunderFighter* g_game = nullptr;
LONG_PTR g_frameTimer = 0;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            return 0;
        case WM_KEYDOWN:
            if (g_game) g_game->KeyDown((int)wParam);
            break;
        case WM_KEYUP:
            if (g_game) g_game->KeyUp((int)wParam);
            break;
        case WM_PAINT:
            if (g_game) g_game->Render();
            ValidateRect(hWnd, NULL);
            break;
        case WM_ERASEBKGND:
            return 1; // 避免闪烁（由 Render 全量绘制）
        case WM_GETMINMAXINFO:
            {
                MINMAXINFO* mmi = (MINMAXINFO*)lParam;
                mmi->ptMinTrackSize.x = 320;
                mmi->ptMinTrackSize.y = 480;
            }
            break;
        case WM_TIMER:
            if (g_game) {
                g_game->Tick();     // 固定时间步长更新
                g_game->Render();
            }
            break;
        case WM_DESTROY:
            KillTimer(hWnd, g_frameTimer);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// ===================== 程序入口 =====================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 注册窗口类
    const wchar_t CLASS_NAME[] = L"ThunderFighterWindow";
    
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;
    
    RegisterClass(&wc);
    
    // 计算窗口大小（客户区 480x720，允许缩放）
    RECT windowRect = {0, 0, 480, 720};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    
    int winWidth = windowRect.right - windowRect.left;
    int winHeight = windowRect.bottom - windowRect.top;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - winWidth) / 2;
    int y = (screenHeight - winHeight) / 2;
    
    // 创建窗口（可调整大小）
    HWND hWnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"雷霆战机 - Thunder Fighter",
        WS_OVERLAPPEDWINDOW,
        x, y, winWidth, winHeight,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    
    if (!hWnd) return 0;
    
    // 创建游戏实例
    g_game = new ThunderFighter(hInstance, hWnd);
    
    ShowWindow(hWnd, nCmdShow);
    
    // 设置游戏计时器 (60 FPS ≈ 16ms)
    g_frameTimer = SetTimer(hWnd, 1, 16, NULL);
    
    // 消息循环
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    delete g_game;
    return 0;
}

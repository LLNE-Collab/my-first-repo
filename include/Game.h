#pragma once

/**
 * @file Game.h
 * @brief 打砖块游戏实体、状态机与主控制器声明。
 *
 * 架构概要：
 * - 实体类（Ball / Paddle / Brick / Particle）负责更新与绘制；
 * - Game 类驱动菜单、关卡加载、存档、碰撞与性能 HUD；
 * - 关卡与存档通过 JSON 加载（见 JsonIO.h、ParseLevelJson、SaveProgress）。
 */

#include <raylib.h>
#include <nlohmann/json.hpp>

#include <future>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <memory>
#include <utility>

#include "JsonIO.h"

using json = nlohmann::json;

/** 游戏状态机：主菜单 → 选关 / 游玩 / 暂停 → 结束或排行榜。 */
enum class GameState {
    MENU,
    LEVEL_SELECT,
    PLAYING,
    PAUSED,
    LEVEL_CLEAR,
    GAME_OVER,
    LEADERBOARD
};

/** 战役通关庆祝：烟花转场 → 选关（Random 仅烟花后自动下一局）。 */
enum class LevelClearPhase {
    FIREWORKS,
    DIALOG
};

/** 下落道具种类，与 config.json 中 powerups 段对应。 */
enum class PowerUpType {
    PADDLE_EXTEND = 0,
    MULTI_BALL    = 1,
    SLOW_BALL     = 2
};

/** 砖块类型枚举，供 ScoreCalculator 计分（可与 layout 数字编码扩展）。 */
enum class BrickType {
    Normal = 1,
    Gold   = 2,
    Bomb   = 3
};

/** 异步「重载砖块颜色」演示用的加载状态（L 键触发）。 */
enum class LoadState {
    IDLE,
    LOADING,
    DONE
};

class Game;

/**
 * 得分计算器。
 * 是什么：按砖块类型返回基础分与连击加成。
 * 为什么：将计分规则集中，避免散落在碰撞代码中。
 * 怎么用：碰撞命中后调用 CalculateScore(brickType, combo)。
 */
class ScoreCalculator {
public:
    [[nodiscard]] int CalculateBaseScore(BrickType brickType) const noexcept;
    [[nodiscard]] int CalculateScore(BrickType brickType, int combo = 0) const noexcept;

private:
    static constexpr int comboBonus_ = 2;
};

/** 球：带定长环形缓冲尾迹，避免每帧堆分配。 */
class Ball {
public:
    Ball(Vector2 pos = {0, 0}, Vector2 vel = {0, 0}, float r = 0.0f, Color c = BLACK);
    ~Ball();

    void Update(float dt);
    void Draw() const;

    Vector2 position;
    Vector2 velocity;
    float radius;
    Color color;

private:
    static constexpr int MAX_TRAIL = 10;

    struct TrailPoint {
        Vector2 pos;
        float life;
    };

    TrailPoint trail_[MAX_TRAIL]{};
    int trailHead_{0};
    int trailCount_{0};
};

/** 挡板：支持限时加宽（道具），extendTimer 归零后恢复 originalWidth。 */
class Paddle {
public:
    Paddle(Vector2 pos = {0, 0}, float w = 100.0f, float h = 10.0f, Color c = BLUE);
    ~Paddle();

    void Update(float dt);
    void Draw() const;
    void Extend(float extraWidth, float duration);

    Vector2 position;
    float width;
    float height;
    float originalWidth;
    float extendTimer;
    Color color;
};

/** 单块砖：rect 为碰撞与绘制区域，active 表示是否仍在场上。 */
class Brick {
public:
    Brick(Vector2 pos = {0, 0}, float w = 0.0f, float h = 0.0f, Color c = GRAY);
    ~Brick();

    void Draw() const;

    Rectangle rect;
    bool active;
    Color color;
};

/**
 * 关卡加载结果（内存中的关卡快照）。
 * 由 BuildLevelData / ParseLevelJson 填充，再经 ApplyLoadedLevel 应用到 Game。
 */
struct LevelData {
    std::vector<Brick> bricks;
    std::string backgroundTexturePath;
    std::string brickTexturePath;
    std::string hitSoundPath;
};

/** 下落道具实例。 */
class PowerUp {
public:
    PowerUp(Vector2 pos = {0, 0}, PowerUpType t = PowerUpType::PADDLE_EXTEND);
    ~PowerUp();

    void Update(float dt, float screenHeight);
    void Draw() const;

    Vector2 position;
    PowerUpType type;
    bool active;
    float duration;
};

/** 单颗粒子；生命周期结束后由对象池标记为未激活，不 delete。 */
class Particle {
public:
    Particle(Vector2 p = {0, 0}, Vector2 v = {0, 0}, Color c = WHITE, float l = 0.0f);
    ~Particle();

    void Update(float dt);
    void Draw() const;

    Vector2 pos;
    Vector2 vel;
    Color color;
    float life;
};

/** 道具效果策略接口：接到道具时对 Game 施加状态变化。 */
class PowerUpEffect {
public:
    virtual ~PowerUpEffect() = default;
    virtual void Apply(Game& game) = 0;
};

class ExtendPaddleEffect : public PowerUpEffect {
public:
    ExtendPaddleEffect(float w, float d);
    ~ExtendPaddleEffect() override;
    void Apply(Game& game) override;

private:
    float extraWidth;
    float duration;
};

class MultiBallEffect : public PowerUpEffect {
public:
    MultiBallEffect(int b);
    ~MultiBallEffect() override;
    void Apply(Game& game) override;

private:
    int extraBalls;
};

class SlowBallEffect : public PowerUpEffect {
public:
    SlowBallEffect(float f, float d);
    ~SlowBallEffect() override;
    void Apply(Game& game) override;

private:
    float speedFactor;
    float duration;
};

/**
 * 游戏主控制器。
 *
 * 是什么：整合输入、物理、绘制、关卡、存档与性能统计的单例式主类。
 * 为什么：课程要求数据驱动 + 持久化 + 可测性能，需统一入口 Run()。
 * 怎么用：
 *   Game g; g.Run();
 *   关卡来自 levels/levelN.json；进度写入 save.json；
 *   菜单 CONTINUE 调用 ContinueFromSave()；编辑模式按 E，S 保存 custom.json。
 */
class Game {
public:
    Game();
    ~Game();

    /** 主循环：每帧 Update() + Draw()，直到关闭窗口。 */
    void Run();

    void ApplyPaddleExtend(float extraWidth, float duration);
    void ApplySlowBall(float speedFactor, float duration);
    void AddBall(const Ball& ball);
    Ball GetBall() const;

private:
    void Update();
    void UpdateMenu();
    void UpdateLevelSelect();
    void UpdatePlaying();
    void UpdatePaused();
    void UpdateLevelClear();
    void ResetGame();
    void SetupSessionDefaults();
    void StartRandomGame();
    void EnterLevelSelect();
    void ResumeCampaign();
    bool HasCampaignSave() const;
    void RefreshCampaignSaveFlag();
    void StartCampaignLevel(int level);
    void RefreshLevelPreviews();

    void InitBackgroundDemoFromLevel(int level);
    void InitBackgroundDemoFromLevelData(const LevelData& data);
    void ResetBackgroundDemoBricks();
    void UpdateBackgroundDemo(float dt);
    void ShuffleBackgroundDemoLevels();
    void AdvanceBackgroundDemoLevel();

    void StartLevelClearCelebration(bool randomMode, int nextLevel, bool isFinalVictory);
    void InitLevelClearDialogButtons();
    void SpawnFireworkBurst(Vector2 center);
    void UpdateFireworks(float dt);
    void DrawFireworks() const;

    void Draw();
    void DrawMenu();
    void DrawLevelSelect();
    void DrawLevelPreview(const LevelData& data, Rectangle bounds, float scale) const;
    void DrawTechBackground() const;
    void DrawPlayingBackground() const;
    void DrawPlayingEntities() const;
    void DrawBackgroundDemo() const;
    void DrawGameplaySnapshot() const;
    void PlayBrickHitSound(Color brickColor) const;
    [[nodiscard]] float PitchFromBrickColor(Color brickColor) const;
    void DrawLevelClearOverlay() const;
    void DrawUI();
    void DrawLoadingScreen();

    void LoadConfig();
    void LoadLeaderboard();
    void SaveScore();

    /**
     * 将当前进度序列化到 save.json。
     * @param levelOverride 若 >= 0，写入该关卡号（通关时写入下一关）；否则写 currentLevel_。
     */
    void SaveProgress(int levelOverride = -1);

    /** 从 save.json 反序列化；版本不兼容或 lives<1 时返回 false。 */
    bool LoadGame();

    void DeleteSave();

    /** 读档并 StartLevelLoad(currentLevel_)，供主菜单 CONTINUE 使用。 */
    /** 统计 levels/levelN.json 数量，确定 maxLevel_。 */
    int CountLevelFiles() const;

    /**
     * 解析关卡 JSON 为 LevelData。
     * 支持 pattern 字符串行，或 bricks.layout 二维数组 + color_map。
     */
    LevelData ParseLevelJson(const json& levelJson, int level) const;

    /** 文件缺失时 BuildLevelData 使用的内置默认关卡。 */
    json GetDefaultLevelJson(int level) const;

    /** 异步加载关卡：后台线程执行 BuildLevelData，主线程轮询 loadFuture_。 */
    void StartLevelLoad(int level);
    LevelData BuildLevelData(int level);
    LevelData BuildRandomLevelData();
    void StartRandomLevelLoad();

    /**
     * 应用关卡数据：清空并重建 bricks_、加载纹理音效、RebuildCollisionGrid、SaveProgress。
     */
    void ApplyLoadedLevel(const LevelData& data, int level);

    Color SimulateHeavyLoad();
    void ApplyLoadedBrickColor(Color c);

    bool IsButtonClicked(const Rectangle& btn);
    void ClampPaddle(Paddle& paddle);

    /** 记录 UpdatePlaying 墙钟耗时，每 60 帧 TraceLog 一次分段数据。 */
    void RecordUpdatePlayingLatency(double startSeconds);

    void ClearParticlePool();
    void EmitParticlesAtBrick(const Rectangle& brickRect, Color color, int count);
    void UpdateParticles(float dt);

    /**
     * 重建 8×6（可配置）碰撞网格：砖块索引按中心落入格子。
     * 为什么：将球-砖检测从 O(N) 降为只查邻域格。
     * 何时调用：关卡加载后、编辑模式增删砖后。
     */
    void RebuildCollisionGrid();
    void GetBallGridCell(int& gx, int& gy) const;
    bool ProcessBrickHit(size_t brickIndex);
    bool CheckBallBrickCollisionsSpatial();
    bool CheckBallBrickCollisionsNaive();
    void DrawCollisionGridDebug() const;
    int CountEstimatedDrawCalls() const;

    /** 编辑模式：鼠标增删砖，S 键调用 SaveLayoutToJson。 */
    void UpdateEditor();
    void DrawEditorOverlay();
    void SaveLayoutToJson(const std::string& path);
    Color ColorFromLayoutCode(int code, const json& colorMap) const;
    Color ColorFromPatternChar(char ch) const;

    static constexpr int MAX_PARTICLES = 1000;
    static constexpr int SAVE_VERSION = 2;
    static constexpr const char* SAVE_PATH = "save.json";
    static constexpr int kMaxLevels = 5;

    int screenWidth_;
    int screenHeight_;
    GameState state_;
    int score_;
    int lives_;
    int currentLevel_;
    int maxLevel_;

    Paddle paddle1_;
    Paddle paddle2_;
    Ball ball_;
    std::vector<Brick> bricks_;

    std::vector<PowerUp> powerUps_;
    Particle particlePool_[MAX_PARTICLES];
    bool particleActive_[MAX_PARTICLES]{};
    std::vector<std::pair<std::string, int>> leaderboardData_;
    std::vector<Vector2> originalVelocities_;

    json config_;
    std::string username_;
    bool isLoading_;
    std::future<LevelData> loadFuture_;
    std::atomic<bool> loadReady_;
    std::atomic<bool> loadFailed_;
    std::string loadingMessage_;
    LoadState loadState_;
    std::mutex loadMutex_;
    bool bricksColorChanged_;
    float loadCompleteTimer_;
    std::future<Color> loadColorFuture_;
    Color loadedBrickColor_;
    Texture2D backgroundTexture_{};
    Texture2D brickTexture_{};
    Sound hitSound_{};

    float slowBallTimer_;
    float ballRespawnTimer_;

    Rectangle btnRandomGame_;
    Rectangle btnSelectGame_;
    Rectangle btnContinue_;
    Rectangle btnSettings_;
    Rectangle btnQuit_;
    bool hasPendingSave_{false};
    bool isRandomMode_{false};
    std::vector<LevelData> levelPreviews_;
    std::string jsonStatusMessage_;
    int pendingLoadLevel_{1};

    bool editingMode_{false};
    float editorBrickWidth_{75.0f};
    float editorBrickHeight_{20.0f};
    float editorSpacing_{5.0f};

    float lastUpdatePlayingMs_{0.0f};
    float lastParticleUpdateMs_{0.0f};
    float lastBrickCollisionMs_{0.0f};
    int activeParticleCount_{0};

    int gridCols_{8};
    int gridRows_{6};
    float cellWidth_{100.0f};
    float cellHeight_{100.0f};
    std::vector<std::vector<std::vector<size_t>>> brickGrid_;
    bool useSpatialGrid_{true};
    bool showGridDebug_{false};
    int lastCollisionChecks_{0};
    int lastCollisionCandidates_{0};
    int activeBrickCount_{0};
    int estimatedDrawCalls_{0};

    // 主菜单 / 选关背景：模拟实机画面（暗色科技风）
    std::vector<Brick> bgDemoBricks_;
    std::vector<Brick> bgDemoBrickSnapshot_;
    std::vector<PowerUp> bgDemoPowerUps_;
    Ball bgDemoBall_;
    Paddle bgDemoPaddle1_;
    Paddle bgDemoPaddle2_;
    float bgDemoSimTime_{0.0f};
    float bgDemoPowerUpSpawnTimer_{2.0f};
    float bgDemoCycleTimer_{0.0f};
    static constexpr float kBgDemoCycleSeconds = 5.0f;
    std::vector<int> bgDemoLevelQueue_;
    int bgDemoQueueIndex_{0};

    // 通关庆祝：烟花 +（战役）下一关/退出
    LevelClearPhase levelClearPhase_{LevelClearPhase::FIREWORKS};
    float levelClearTimer_{0.0f};
    float fireworksBurstTimer_{0.0f};
    bool levelClearRandomMode_{false};
    bool levelClearIsFinal_{false};
    int levelClearPendingLevel_{1};
    Rectangle btnLevelClearNext_{};
    Rectangle btnLevelClearQuit_{};

    struct FireworkSpark {
        Vector2 pos{};
        Vector2 vel{};
        Color color{WHITE};
        float life{0.0f};
        float maxLife{1.0f};
    };
    std::vector<FireworkSpark> fireworks_;
};

// Includes and namespaces
#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <cmath>
#include "raylib.h"
#include "videoPlayer.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

using namespace std;

// Enums for game state, animation state, and difficulty
enum GameState {
    MAIN_MENU,
    CHAR_SELECT,
    GAMEPLAY,
    PAUSE,
    GAME_OVER,
    SETTINGS,
    DIFFICULTY_MODAL
};
enum AnimState { IDLE, WALK_LEFT, WALK_RIGHT, PUNCH, KICK, DYING };
enum Difficulty { EASY, MID, HARD };

// Particle effect for the main menu
struct MenuParticle {
    float x, y;
    float vx, vy;
    float radius;
    float alpha;
    Color color;
};

static vector<MenuParticle> menuParticles;

void InitMenuParticles(int screenW, int screenH) {
    menuParticles.clear();
    for (int i = 0; i < 60; i++) {
        MenuParticle p;
        p.x      = (float)GetRandomValue(0, screenW);
        p.y      = (float)GetRandomValue(0, screenH);
        p.vx     = ((float)GetRandomValue(-30, 30)) / 100.0f;
        p.vy     = -((float)GetRandomValue(20, 80)) / 100.0f;
        p.radius = (float)GetRandomValue(2, 6);
        p.alpha  = ((float)GetRandomValue(40, 100)) / 100.0f;
        int r    = GetRandomValue(0, 1);
        p.color  = r ? (Color){100,180,255,255} : (Color){220,50,50,255};
        menuParticles.push_back(p);
    }
}

void UpdateMenuParticles(float dt, int screenW, int screenH) {
    for (auto& p : menuParticles) {
        p.x += p.vx * dt * 60.0f;
        p.y += p.vy * dt * 60.0f;
        if (p.y < -10) p.y = (float)screenH + 5;
        if (p.x < -10) p.x = (float)screenW + 5;
        if (p.x > screenW + 10) p.x = -5;
    }
}

void DrawMenuParticles() {
    for (auto& p : menuParticles) {
        Color c = p.color;
        c.a = (unsigned char)(p.alpha * 180);
        DrawCircle((int)p.x, (int)p.y, p.radius, c);
    }
}

// Gojo character structure
struct Gojo {
    float x, y;
    int health = 200;
    int maxHealth = 200;
    float speed = 220.0f;
    int punchDamage = 8;
    int kickDamage = 5;

    AnimState state = IDLE;
    float actionTimer = 0.0f;
    int   comboCount = 0;
    float domainCooldown  = 0.0f;
    bool  facingLeft = false;
    bool  rctUsed = false;
    float animTimer = 0.0f;

    bool  infinityActive = false;
    float infinityTimer = 0.0f;

    float hitFlashTimer = 0.0f;
    float deathTimer = 0.0f;
    static constexpr float DEATH_DURATION = 1.2f;

    bool IsAlive()      { return health > 0 || deathTimer > 0.0f; }
    bool IsTrulyDead()  { return health <= 0 && deathTimer <= 0.0f; }

    void Reset() {
        health = maxHealth;
        state  = IDLE;
        actionTimer = 0.0f;
        comboCount  = 0;
        domainCooldown = 0.0f;
        facingLeft  = false;
        rctUsed     = false;
        infinityActive = false;
        infinityTimer  = 0.0f;
        hitFlashTimer = 0.0f;
        deathTimer    = 0.0f;
        animTimer = 0.0f;
    }

    void TakeHit(int dmg) {
        health -= dmg;
        hitFlashTimer = 0.25f;
        if (health <= 0) {
            health     = 0;
            state      = DYING;
            deathTimer = DEATH_DURATION;
        }
    }

    void Update(int screenWidth, float bossX, bool bossAlive, bool isTwoPlayer, Difficulty difficulty, float dt) {
        if (state == DYING) {
            deathTimer -= dt;
            if (deathTimer < 0.0f) deathTimer = 0.0f;
            return;
        }
        if (health <= 0) return;

        if (hitFlashTimer > 0.0f) hitFlashTimer -= dt;
        if (domainCooldown > 0.0f) { domainCooldown -= dt; if (domainCooldown < 0.0f) domainCooldown = 0.0f; }

        // Infinity ability for Hard difficulty in single player
        if (difficulty == HARD && !isTwoPlayer && IsKeyPressed(KEY_E) && !infinityActive && bossAlive) {
            infinityActive = true;
            infinityTimer  = 3.0f;
        }
        if (infinityActive) {
            infinityTimer -= dt;
            if (infinityTimer <= 0.0f) infinityActive = false;
        }

        // Teleport
        if (bossAlive && IsKeyPressed(KEY_SPACE)) {
            if (x < bossX) x = bossX + 150.0f;
            else           x = bossX - 150.0f;
            x = clamp(x, 50.0f, (float)(screenWidth - 50));
        }

        // Attack lock
        if (state == PUNCH || state == KICK) {
            actionTimer += dt;
            if (actionTimer >= 0.5f) { state = IDLE; actionTimer = 0.0f; }
        } else {
            state = IDLE;
            float nextX = x;
            if (IsKeyDown(KEY_D)) { nextX += speed * dt; state = WALK_RIGHT; }
            else if (IsKeyDown(KEY_A)) { nextX -= speed * dt; state = WALK_LEFT; }
            if (bossAlive && fabsf(nextX - bossX) < 60.0f) {
                if (fabsf(nextX - bossX) < fabsf(x - bossX)) nextX = x;
            }
            x = clamp(nextX, 50.0f, (float)(screenWidth - 50));
        }

        if (state == WALK_LEFT || state == WALK_RIGHT)
            animTimer += dt;
        else
            animTimer = 0.0f;
    }
};

// Sukuna character structure (boss / player 2)
struct Sukuna {
    float x, y;
    int health    = 200;
    int maxHealth = 200;
    float speed   = 130.0f;
    int punchDamage = 8;
    int kickDamage  = 9;

    AnimState state        = IDLE;
    float actionTimer      = 0.0f;
    float attackCooldown   = 0.0f;
    bool  facingLeft       = true;
    float animTimer        = 0.0f;

    float domainCooldown   = 0.0f;
    bool  domain175Triggered = false;
    bool  domain25Triggered  = false;
    bool  mahoragaSummoned   = false;
    int   damagePool         = 0;
    Difficulty gameDifficulty = MID;

    float hitFlashTimer    = 0.0f;
    float deathTimer       = 0.0f;
    static constexpr float DEATH_DURATION = 1.2f;

    float dodgeCooldown    = 0.0f;
    float aiOvershotTarget = 0.0f;
    float attackRate       = 0.8f;
    float dodgeChance      = 0.0f;

    bool IsAlive()     { return health > 0 || deathTimer > 0.0f; }
    bool IsTrulyDead() { return health <= 0 && deathTimer <= 0.0f; }

    void Reset(Difficulty diff) {
        health    = maxHealth;
        state     = IDLE;
        actionTimer = 0.0f;
        attackCooldown = 0.0f;
        facingLeft = true;
        domainCooldown = 0.0f;
        domain175Triggered = false;
        domain25Triggered  = false;
        mahoragaSummoned   = false;
        damagePool  = 0;
        punchDamage = 5;
        kickDamage  = 8;
        hitFlashTimer = 0.0f;
        deathTimer    = 0.0f;
        dodgeCooldown = 0.0f;
        aiOvershotTarget = 0.0f;
        animTimer = 0.0f;
        gameDifficulty = diff;

        switch (diff) {
            case EASY:  speed = 90.0f;  attackRate = 1.4f; dodgeChance = 0.03f; break;
            case MID:   speed = 100.0f; attackRate = 0.8f; dodgeChance = 0.08f; break;
            case HARD:  speed = 110.0f; attackRate = 0.45f; dodgeChance = 0.18f; break;
        }
    }

    void TakeHit(int amount, float targetX, int screenW) {
        health -= amount;
        hitFlashTimer = 0.25f;
        damagePool += amount;
        int threshold = (gameDifficulty == HARD) ? 20 : 25;
        if (damagePool >= threshold) {
            if (x < targetX) x = targetX + 160.0f;
            else              x = targetX - 160.0f;
            x = clamp(x, 50.0f, (float)(screenW - 50));
            damagePool -= threshold;
        }
        if (health <= 0) {
            health     = 0;
            state      = DYING;
            deathTimer = DEATH_DURATION;
        }
    }

    void UpdatePlayer2(int screenWidth, float targetX, bool targetAlive, float dt) {
        if (state == DYING) { deathTimer -= dt; if (deathTimer < 0.0f) deathTimer = 0.0f; return; }
        if (health <= 0) return;
        if (hitFlashTimer > 0.0f) hitFlashTimer -= dt;
        if (domainCooldown > 0.0f) { domainCooldown -= dt; if (domainCooldown < 0.0f) domainCooldown = 0.0f; }
        if (attackCooldown > 0.0f) attackCooldown -= dt;

        if (targetAlive && IsKeyPressed(KEY_RIGHT_SHIFT)) {
            if (x < targetX) x = targetX + 150.0f;
            else              x = targetX - 150.0f;
            x = clamp(x, 50.0f, (float)(screenWidth - 50));
        }
        if (state == PUNCH || state == KICK) {
            actionTimer += dt;
            if (actionTimer >= 0.5f) { state = IDLE; actionTimer = 0.0f; }
        } else {
            state = IDLE;
            float nextX = x;
            if (IsKeyDown(KEY_RIGHT)) { nextX += (speed + 30.0f) * dt; state = WALK_RIGHT; }
            if (IsKeyDown(KEY_LEFT))  { nextX -= (speed + 30.0f) * dt; state = WALK_LEFT; }
            if (targetAlive && fabsf(nextX - targetX) < 60.0f) {
                if (fabsf(nextX - targetX) < fabsf(x - targetX)) nextX = x;
            }
            x = clamp(nextX, 50.0f, (float)(screenWidth - 50));
        }

        if (state == WALK_LEFT || state == WALK_RIGHT)
            animTimer += dt;
        else
            animTimer = 0.0f;
    }

    void UpdateAI(float targetX, bool targetAlive, int screenW, float dt) {
        if (state == DYING) { deathTimer -= dt; if (deathTimer < 0.0f) deathTimer = 0.0f; return; }
        if (health <= 0) return;
        if (hitFlashTimer > 0.0f) hitFlashTimer -= dt;
        if (attackCooldown > 0.0f) attackCooldown -= dt;
        if (dodgeCooldown  > 0.0f) dodgeCooldown  -= dt;

        if (state == PUNCH || state == KICK) {
            actionTimer += dt;
            if (actionTimer >= 0.5f) { state = IDLE; actionTimer = 0.0f; attackCooldown = attackRate; }
            return;
        }

        float distance = targetX - x;
        float absDist  = fabsf(distance);

        if (absDist < 90.0f && dodgeCooldown <= 0.0f && targetAlive) {
            float roll = (float)GetRandomValue(0, 1000) / 1000.0f;
            if (roll < dodgeChance) {
                if (x < targetX) x = targetX + (float)GetRandomValue(120, 200);
                else              x = targetX - (float)GetRandomValue(120, 200);
                x = clamp(x, 50.0f, (float)(screenW - 50));
                dodgeCooldown = 1.5f;
                state = IDLE;
                return;
            }
        }

        if (absDist < 80.0f && attackCooldown <= 0.0f) {
            state       = (GetRandomValue(1, 100) % 2 == 0) ? PUNCH : KICK;
            actionTimer = 0.0f;
            return;
        }

        float predictedTarget = targetX + (distance > 0 ? 20.0f : -20.0f);
        if (aiOvershotTarget == 0.0f) aiOvershotTarget = predictedTarget;
        aiOvershotTarget += (predictedTarget - aiOvershotTarget) * dt * 3.0f;

        float moveDir = aiOvershotTarget - x;
        if (fabsf(moveDir) > 10.0f) {
            float nextX = x + (moveDir > 0 ? speed : -speed) * dt;
            state = moveDir > 0 ? WALK_RIGHT : WALK_LEFT;
            if (targetAlive && fabsf(nextX - targetX) < 60.0f) {
                if (fabsf(nextX - targetX) < fabsf(x - targetX)) nextX = x;
            }
            x = clamp(nextX, 50.0f, (float)(screenW - 50));
        } else {
            state = IDLE;
        }

        if (state == WALK_LEFT || state == WALK_RIGHT)
            animTimer += dt;
        else
            animTimer = 0.0f;
    }
};

// Screen shake effects
struct ScreenEffects {
    float shakeTimer     = 0.0f;
    float shakeIntensity = 0.0f;
    float offsetX        = 0.0f;
    float offsetY        = 0.0f;

    void TriggerShake(float time, float intensity) {
        shakeTimer     = time;
        shakeIntensity = intensity;
    }
    void Update(float dt) {
        if (shakeTimer > 0.0f) {
            shakeTimer -= dt;
            offsetX = (float)GetRandomValue(-(int)shakeIntensity, (int)shakeIntensity);
            offsetY = (float)GetRandomValue(-(int)shakeIntensity, (int)shakeIntensity);
            if (shakeTimer <= 0.0f) { offsetX = 0.0f; offsetY = 0.0f; }
        }
    }
};

// Reset game state
void ResetGame(Gojo& player, Sukuna& boss, float& mahoragaFreezeTimer, int screenWidth, int screenHeight, Difficulty diff) {
    player.Reset();
    boss.Reset(diff);
    mahoragaFreezeTimer = 0.0f;
    player.x = screenWidth  * 0.20f;
    player.y = (float)(screenHeight - 30);
    boss.x   = screenWidth  * 0.80f;
    boss.y   = (float)(screenHeight - 30);
}

// Helper to stop domain videos
void StopDomainVideos(VideoPlayer& sukuna, VideoPlayer& gojo, VideoPlayer& heal) {
    if (sukuna.IsPlaying()) { while (sukuna.IsPlaying()) sukuna.Update(1.0f); }
    if (gojo.IsPlaying())   { while (gojo.IsPlaying())   gojo.Update(1.0f); }
    if (heal.IsPlaying())   { while (heal.IsPlaying())   heal.Update(1.0f); }
}

// Music handling
const char* songFiles[] = {
    "assets/audio/music/MONTAGEM_TENTANA.mp3",
    "assets/audio/music/IWillSurvive.mp3",
    "assets/audio/music/matadora.mp3",
    "assets/audio/music/judas.mp3"
};
const char* songDisplayNames[] = {
    "Montagem Tentana",
    "I Will Survive",
    "Matadora",
    "Judas"
};
const int songCount = 4;
Music songs[songCount];
int   currentSong  = 0;
bool  repeatSong   = false;
bool  musicMuted   = false;
float musicVolume  = 0.2f;
bool  musicInitialized = false;
GameState previousState = MAIN_MENU;

void LoadAllMusic() {
    for (int i = 0; i < songCount; i++) {
        songs[i] = LoadMusicStream(songFiles[i]);
        SetMusicVolume(songs[i], musicMuted ? 0.0f : musicVolume);
    }
    musicInitialized = true;
}
void UnloadAllMusic() {
    for (int i = 0; i < songCount; i++) UnloadMusicStream(songs[i]);
}
void PlayCurrentSong() {
    StopMusicStream(songs[currentSong]);
    PlayMusicStream(songs[currentSong]);
}
void UpdateMusic(GameState st) {
    if (!musicInitialized) return;
    if (st == GAMEPLAY) {
        UpdateMusicStream(songs[currentSong]);
        float played = GetMusicTimePlayed(songs[currentSong]);
        float length = GetMusicTimeLength(songs[currentSong]);
        if (length > 0.0f && played >= (length - 0.1f)) {
            if (repeatSong) PlayCurrentSong();
            else { currentSong = (currentSong + 1) % songCount; PlayCurrentSong(); }
        }
    } else {
        if (IsMusicStreamPlaying(songs[currentSong])) PauseMusicStream(songs[currentSong]);
    }
}
void SetMusicMute(bool mute) {
    musicMuted = mute;
    for (int i = 0; i < songCount; i++)
        SetMusicVolume(songs[i], mute ? 0.0f : musicVolume);
}
void SetMusicVolumeLevel(float volume) {
    musicVolume = volume;
    if (!musicMuted)
        for (int i = 0; i < songCount; i++)
            SetMusicVolume(songs[i], musicVolume);
}

// Announcer voice lines
Sound voFight, voCombo, voLost, voWinner;

void LoadAnnouncerSounds() {
    voFight  = LoadSound("assets/audio/voice/vo_fight.wav");
    voCombo  = LoadSound("assets/audio/voice/vo_combo.wav");
    voLost   = LoadSound("assets/audio/voice/vo_lost.wav");
    voWinner = LoadSound("assets/audio/voice/vo_winner.wav");
}
void UnloadAnnouncerSounds() {
    UnloadSound(voFight);
    UnloadSound(voCombo);
    UnloadSound(voLost);
    UnloadSound(voWinner);
}

// ---------------------------------------------------------------------------
// Global game state (moved from main() for Emscripten compatibility)
// ---------------------------------------------------------------------------
const int screenWidth  = 1280;
const int screenHeight = 720;

GameState currentEngineState = MAIN_MENU;
bool isTwoPlayerMode = false;
Difficulty currentDifficulty = MID;
bool shouldExit = false;

// Background textures
const int bgCount = 4;
Texture2D bgs[bgCount];
int selectedBgIndex = 0;

// Character and menu textures
Texture2D menuBg;
Texture2D gojoStill, gojoWalkRight, gojoWalkLeft, gojoPunch, gojoKick;
Texture2D sukunaStill, sukunaWalkRight, sukunaWalkLeft, sukunaPunch, sukunaKick, mahoragaTex;

// Video players (heap-allocated because they require constructor arguments)
VideoPlayer* pSukunaDomain = nullptr;
VideoPlayer* pGojoDomain   = nullptr;
VideoPlayer* pGojoHeal     = nullptr;

// Sound effects
Sound sukunaDomainSound;
Sound gojoDomainSound;
Sound gojoHealSound;
Sound mahoragaSound;
Sound combatHitSound;
Sound teleportSound;

// Game objects
Gojo   player;
Sukuna boss;
ScreenEffects vfx;
Camera2D camera = {0};
float mahoragaFreezeTimer = 0.0f;

// UI state
int  menuOption     = 0;
int  pauseOption    = 0;
int  gameOverOption = 0;
int  selectedSongIndex = 0;
bool adjustingVolume = false;
Rectangle volumeSlider = {1280/2.0f - 100, 480, 200, 20};
float volumeSliderValue = 20.0f;

int  modalOption         = 0;
int  modalDifficultyIndex= 1;
int  modalMusicIndex     = 0;
int  modalBgIndex        = 0;

int  charSelectHighlight = 0;
bool charSelectConfirmed = false;

bool fightVoPlayed = false;
bool comboVoPlayed = false;
const int MENU_OPT_COUNT = 3;

// ---------------------------------------------------------------------------
// UpdateDrawFrame — one frame of update + draw (called by emscripten or while loop)
// ---------------------------------------------------------------------------
void UpdateDrawFrame(void) {
    if (shouldExit) {
#if defined(PLATFORM_WEB)
        emscripten_cancel_main_loop();
#endif
        return;
    }

#if defined(PLATFORM_WEB)
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || GetKeyPressed() != 0) {
        EM_ASM({
            if (typeof window.resumeAllAudio === 'function') {
                window.resumeAllAudio();
            }
        });
    }
#endif

    // Local reference aliases so all existing code using '.' syntax works unchanged
    VideoPlayer& sukunaDomain = *pSukunaDomain;
    VideoPlayer& gojoDomain   = *pGojoDomain;
    VideoPlayer& gojoHeal     = *pGojoHeal;

    float dt = GetFrameTime();

    if (IsKeyPressed(KEY_M)) SetMusicMute(!musicMuted);
    UpdateMusic(currentEngineState);
    vfx.Update(dt);

    // Main menu logic
    if (currentEngineState == MAIN_MENU) {
        UpdateMenuParticles(dt, screenWidth, screenHeight);
        if (IsKeyPressed(KEY_UP))   menuOption = (menuOption - 1 + MENU_OPT_COUNT) % MENU_OPT_COUNT;
        if (IsKeyPressed(KEY_DOWN)) menuOption = (menuOption + 1) % MENU_OPT_COUNT;
        if (IsKeyPressed(KEY_ENTER)) {
            if (menuOption == 0) {  // Single Player
                isTwoPlayerMode = false;
                previousState   = MAIN_MENU;
                currentEngineState = CHAR_SELECT;
                charSelectHighlight= 0;
                charSelectConfirmed= false;
            } else if (menuOption == 1) {  // Two Player
                isTwoPlayerMode = true;
                previousState   = MAIN_MENU;
                currentEngineState = DIFFICULTY_MODAL;
                modalOption          = 0;
                modalDifficultyIndex = (int)currentDifficulty;
                modalMusicIndex      = currentSong;
                modalBgIndex         = selectedBgIndex;
            } else if (menuOption == 2) {  // Settings
                previousState = MAIN_MENU;
                currentEngineState = SETTINGS;
                selectedSongIndex  = currentSong;
                volumeSliderValue  = musicVolume * 100;
            }
        }
    }

    // Character selection logic
    else if (currentEngineState == CHAR_SELECT) {
        if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_RIGHT))
            charSelectHighlight = 1 - charSelectHighlight;
        if (IsKeyPressed(KEY_ENTER) && !charSelectConfirmed) {
            charSelectConfirmed = true;
            currentEngineState   = DIFFICULTY_MODAL;
            modalOption          = 0;
            modalDifficultyIndex = (int)currentDifficulty;
            modalMusicIndex      = currentSong;
            modalBgIndex         = selectedBgIndex;
        }
        if (IsKeyPressed(KEY_B)) currentEngineState = MAIN_MENU;
    }

    // Difficulty modal logic
    else if (currentEngineState == DIFFICULTY_MODAL) {
        int modalOptCount = isTwoPlayerMode ? 3 : 4;
        if (IsKeyPressed(KEY_UP))   modalOption = (modalOption - 1 + modalOptCount) % modalOptCount;
        if (IsKeyPressed(KEY_DOWN)) modalOption = (modalOption + 1) % modalOptCount;
        if (IsKeyPressed(KEY_LEFT)) {
            if (!isTwoPlayerMode && modalOption == 0) {
                modalDifficultyIndex = (modalDifficultyIndex - 1 + 3) % 3;
            } else if (modalOption == (isTwoPlayerMode ? 0 : 1)) {
                modalMusicIndex = (modalMusicIndex - 1 + songCount) % songCount;
            } else if (modalOption == (isTwoPlayerMode ? 1 : 2)) {
                modalBgIndex = (modalBgIndex - 1 + bgCount) % bgCount;
            }
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            if (!isTwoPlayerMode && modalOption == 0) {
                modalDifficultyIndex = (modalDifficultyIndex + 1) % 3;
            } else if (modalOption == (isTwoPlayerMode ? 0 : 1)) {
                modalMusicIndex = (modalMusicIndex + 1) % songCount;
            } else if (modalOption == (isTwoPlayerMode ? 1 : 2)) {
                modalBgIndex = (modalBgIndex + 1) % bgCount;
            }
        }
        if (IsKeyPressed(KEY_ENTER) && modalOption == modalOptCount - 1) {
            if (!isTwoPlayerMode) {
                currentDifficulty = (Difficulty)modalDifficultyIndex;
            }
            currentSong       = modalMusicIndex;
            selectedBgIndex   = modalBgIndex;
            PlayCurrentSong();
            ResumeMusicStream(songs[currentSong]);
            ResetGame(player, boss, mahoragaFreezeTimer, screenWidth, screenHeight, currentDifficulty);
            currentEngineState = GAMEPLAY;
            fightVoPlayed = false;
            comboVoPlayed = false;
        }
        if (IsKeyPressed(KEY_B)) {
            if (isTwoPlayerMode)
                currentEngineState = MAIN_MENU;
            else
                currentEngineState = CHAR_SELECT;
            charSelectConfirmed = false;
        }
    }

    // Gameplay logic
    else if (currentEngineState == GAMEPLAY) {
        if (!fightVoPlayed) {
            PlaySound(voFight);
            fightVoPlayed = true;
        }

        if (IsKeyPressed(KEY_ENTER) && !sukunaDomain.IsPlaying() && !gojoDomain.IsPlaying() && !gojoHeal.IsPlaying()) {
            currentEngineState = PAUSE;
            pauseOption = 0;
            PauseMusicStream(songs[currentSong]);
        }

        camera.offset = {vfx.offsetX, vfx.offsetY};

        if (player.health > 0 && boss.health > 0) {
            player.facingLeft = (player.x > boss.x);
            boss.facingLeft   = (boss.x   > player.x);
        }

        // RCT (single player only)
        if (player.health <= 0 && !player.rctUsed && !isTwoPlayerMode) {
            player.rctUsed = true;
            player.health  = 1;
            player.state   = IDLE;
            player.deathTimer = 0.0f;
            player.hitFlashTimer = 0.0f;
            gojoHeal.Play();
            PlaySound(gojoHealSound);
        }

        bool playerTrulyDead = player.IsTrulyDead();
        bool bossTrulyDead   = boss.IsTrulyDead();

        if (playerTrulyDead || bossTrulyDead) {
            StopDomainVideos(sukunaDomain, gojoDomain, gojoHeal);
            StopSound(sukunaDomainSound);
            StopSound(gojoDomainSound);
            if (playerTrulyDead) PlaySound(voLost);
            if (bossTrulyDead)   PlaySound(voWinner);
            currentEngineState = GAME_OVER;
            gameOverOption = 0;
            PauseMusicStream(songs[currentSong]);
            // Was 'continue;' in the original while loop — now just return from this frame
            return;
        }

        if (player.state == DYING) player.Update(screenWidth, boss.x, boss.health > 0, isTwoPlayerMode, currentDifficulty, dt);
        if (boss.state   == DYING) {
            if (isTwoPlayerMode) boss.UpdatePlayer2(screenWidth, player.x, player.health > 0, dt);
            else boss.UpdateAI(player.x, player.health > 0, screenWidth, dt);
        }

        if (!sukunaDomain.IsPlaying() && !gojoDomain.IsPlaying() && !gojoHeal.IsPlaying()) {
            // Mahoraga summon
            if (!isTwoPlayerMode && boss.health <= 100 && !boss.mahoragaSummoned && boss.health > 0) {
                boss.mahoragaSummoned = true;
                mahoragaFreezeTimer   = 6.0f;
                PlaySound(mahoragaSound);
                vfx.TriggerShake(0.3f, 25.0f);
                switch (currentDifficulty) {
                    case EASY: boss.punchDamage=15; boss.kickDamage=15; boss.speed=100.0f; break;
                    case MID:  boss.punchDamage=21; boss.kickDamage=21; boss.speed=120.0f; break;
                    case HARD: boss.punchDamage=30; boss.kickDamage=35; boss.speed=1450.0f; break;
                }
            }
            if (mahoragaFreezeTimer > 0.0f) {
                mahoragaFreezeTimer -= dt;
                if ((int)(mahoragaFreezeTimer * 10.0f) % 15 == 0) vfx.TriggerShake(0.2f, 20.0f);
            } else {
                // Gojo Domain
                if (player.domainCooldown == 0.0f && player.health > 0 && IsKeyPressed(KEY_Q)) {
                    gojoDomain.Play();
                    PlaySound(gojoDomainSound);
                    boss.TakeHit(25, player.x, screenWidth);
                    player.domainCooldown = 15.0f;
                }

                // Sukuna AI Domain
                if (!isTwoPlayerMode && !gojoDomain.IsPlaying() && boss.health > 0) {
                    if (boss.health <= 175 && !boss.domain175Triggered) {
                        sukunaDomain.Play();
                        PlaySound(sukunaDomainSound);
                        player.TakeHit(25);
                        boss.domain175Triggered = true;
                    } else if (boss.health <= 25 && !boss.domain25Triggered) {
                        sukunaDomain.Play();
                        PlaySound(sukunaDomainSound);
                        player.TakeHit(25);
                        boss.domain25Triggered = true;
                    }
                }

                // Sukuna P2 Domain
                if (isTwoPlayerMode && !gojoDomain.IsPlaying() && boss.health > 0) {
                    if ((IsKeyPressed(KEY_O) || IsKeyPressed(KEY_I)) && boss.domainCooldown == 0.0f) {
                        sukunaDomain.Play();
                        PlaySound(sukunaDomainSound);
                        player.TakeHit(25);
                        boss.domainCooldown = 15.0f;
                    }
                }

                if (!sukunaDomain.IsPlaying() && !gojoDomain.IsPlaying()) {
                    if (player.health > 0 && boss.health > 0 && IsKeyPressed(KEY_SPACE))
                        PlaySound(teleportSound);

                    player.Update(screenWidth, boss.x, boss.health > 0, isTwoPlayerMode, currentDifficulty, dt);

                    if (isTwoPlayerMode) {
                        if (boss.health > 0 && IsKeyPressed(KEY_RIGHT_SHIFT)) PlaySound(teleportSound);
                        boss.UpdatePlayer2(screenWidth, player.x, player.health > 0, dt);

                        bool p2Punched = IsKeyPressed(KEY_P);
                        bool p2Kicked  = IsKeyPressed(KEY_K);
                        if ((p2Punched || p2Kicked) && boss.attackCooldown <= 0.0f) {
                            boss.state      = p2Punched ? PUNCH : KICK;
                            boss.actionTimer= 0.0f;
                            boss.attackCooldown = 0.8f;
                            if (player.health > 0 && fabsf(player.x - boss.x) < 120.0f && !player.infinityActive) {
                                PlaySound(combatHitSound);
                                int dmg = p2Punched ? boss.punchDamage : boss.kickDamage;
                                player.TakeHit(dmg);
                                vfx.TriggerShake(0.2f, 4.0f);
                                float kbDir = (player.x < boss.x) ? -1.0f : 1.0f;
                                player.x = std::clamp(player.x + kbDir * 30.0f, 50.0f, (float)(screenWidth-50));
                            }
                        }
                    } else {
                        AnimState oldState = boss.state;
                        boss.UpdateAI(player.x, player.health > 0, screenWidth, dt);
                        if (boss.state != oldState && (boss.state == PUNCH || boss.state == KICK)) {
                            if (!player.infinityActive && player.health > 0 && fabsf(player.x - boss.x) < 120.0f) {
                                PlaySound(combatHitSound);
                                int dmg = (boss.state == PUNCH) ? boss.punchDamage : boss.kickDamage;
                                player.TakeHit(dmg);
                                vfx.TriggerShake(boss.mahoragaSummoned ? 0.4f : 0.2f,boss.mahoragaSummoned ? 15.0f : 4.0f);
                                float kbDir = (player.x < boss.x) ? -1.0f : 1.0f;
                                player.x = std::clamp(player.x + kbDir * 35.0f, 50.0f, (float)(screenWidth-50));
                            }
                        }
                    }

                    // Gojo attacks
                    if (player.health > 0 && player.state != PUNCH && player.state != KICK) {
                        bool punched = IsKeyPressed(KEY_Y);
                        bool kicked  = IsKeyPressed(KEY_G);
                        if (punched || kicked) {
                            player.state      = punched ? PUNCH : KICK;
                            player.actionTimer= 0.0f;
                            if (boss.health > 0 && fabsf(player.x - boss.x) < 120.0f) {
                                PlaySound(combatHitSound);
                                int dmg = punched ? player.punchDamage : player.kickDamage;
                                boss.TakeHit(dmg, player.x, screenWidth);
                                float kbDir = (boss.x < player.x) ? -1.0f : 1.0f;
                                boss.x = std::clamp(boss.x + kbDir * 30.0f, 50.0f, (float)(screenWidth-50));

                                player.comboCount++;
                                if (player.comboCount == 3 && !comboVoPlayed) {
                                    PlaySound(voCombo);
                                    comboVoPlayed = true;
                                }
                                if (player.comboCount < 3) comboVoPlayed = false;

                                if (player.comboCount >= 3) {
                                    vfx.TriggerShake(0.4f, 15.0f);
                                    player.comboCount = 0;
                                    boss.TakeHit(5, player.x, screenWidth);
                                }
                            }
                        }
                    }
                }
            }
        } else {
            // Cutscene handling
            if (IsKeyPressed(KEY_F)) {
                bool healWasPlaying = gojoHeal.IsPlaying();
                while (sukunaDomain.IsPlaying()) sukunaDomain.Update(1.0f);
                while (gojoDomain.IsPlaying())   gojoDomain.Update(1.0f);
                while (gojoHeal.IsPlaying())      gojoHeal.Update(1.0f);
                StopSound(sukunaDomainSound);
                StopSound(gojoDomainSound);
                if (healWasPlaying) player.health = 100;
            }
            if (sukunaDomain.IsPlaying()) {
                sukunaDomain.Update(dt);
                if (!sukunaDomain.IsPlaying() && IsSoundPlaying(sukunaDomainSound))
                    StopSound(sukunaDomainSound);
            }
            if (gojoDomain.IsPlaying()) {
                gojoDomain.Update(dt);
                if (!gojoDomain.IsPlaying() && IsSoundPlaying(gojoDomainSound))
                    StopSound(gojoDomainSound);
            }
            if (gojoHeal.IsPlaying()) {
                gojoHeal.Update(dt);
                if (!gojoHeal.IsPlaying()) player.health = 50;
            }
        }
    }

    // Pause menu logic
    else if (currentEngineState == PAUSE) {
        if (IsKeyPressed(KEY_UP))   pauseOption = (pauseOption - 1 + 4) % 4;
        if (IsKeyPressed(KEY_DOWN)) pauseOption = (pauseOption + 1) % 4;
        if (IsKeyPressed(KEY_ENTER)) {
            if (pauseOption == 0) {
                player.state = IDLE;
                boss.state = IDLE;
                currentEngineState = GAMEPLAY;
                ResumeMusicStream(songs[currentSong]);
            } else if (pauseOption == 1) {
                previousState = PAUSE;
                currentEngineState = SETTINGS;
                selectedSongIndex = currentSong;
                volumeSliderValue = musicVolume * 100;
            } else if (pauseOption == 2) {
                currentEngineState = MAIN_MENU;
                ResetGame(player, boss, mahoragaFreezeTimer, screenWidth, screenHeight, currentDifficulty);
                menuOption = 0;
            } else if (pauseOption == 3) {
                // Was 'break;' in the original while loop
                shouldExit = true;
                return;
            }
        }
    }

    // Game over logic
    else if (currentEngineState == GAME_OVER) {
        if (IsKeyPressed(KEY_UP))   gameOverOption = (gameOverOption - 1 + 3) % 3;
        if (IsKeyPressed(KEY_DOWN)) gameOverOption = (gameOverOption + 1) % 3;
        if (IsKeyPressed(KEY_ENTER)) {
            if (gameOverOption == 0) {
                ResetGame(player, boss, mahoragaFreezeTimer, screenWidth, screenHeight, currentDifficulty);
                currentEngineState = GAMEPLAY;
                fightVoPlayed = false;
                comboVoPlayed = false;
                ResumeMusicStream(songs[currentSong]);
            } else if (gameOverOption == 1) {
                currentEngineState = MAIN_MENU;
                ResetGame(player, boss, mahoragaFreezeTimer, screenWidth, screenHeight, currentDifficulty);
                menuOption = 0;
            } else {
                // Was 'break;' in the original while loop
                shouldExit = true;
                return;
            }
        }
    }

    // Settings menu logic
    else if (currentEngineState == SETTINGS) {
        if (IsKeyPressed(KEY_UP))   selectedSongIndex = (selectedSongIndex - 1 + songCount) % songCount;
        if (IsKeyPressed(KEY_DOWN)) selectedSongIndex = (selectedSongIndex + 1) % songCount;
        if (IsKeyPressed(KEY_R))    repeatSong = !repeatSong;
        if (IsKeyPressed(KEY_ENTER)) {
            currentSong = selectedSongIndex;
            PlayCurrentSong();
            if (previousState != GAMEPLAY) PauseMusicStream(songs[currentSong]);
        }
        Vector2 mouse = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, volumeSlider))
            adjustingVolume = true;
        if (adjustingVolume) {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                float nx = std::clamp(mouse.x, volumeSlider.x, volumeSlider.x + volumeSlider.width);
                volumeSliderValue = ((nx - volumeSlider.x) / volumeSlider.width) * 100.0f;
                SetMusicVolumeLevel(volumeSliderValue / 100.0f);
            } else adjustingVolume = false;
        }
        if (IsKeyPressed(KEY_B)) {
            currentEngineState = previousState;
            if (currentEngineState == GAMEPLAY) ResumeMusicStream(songs[currentSong]);
        }
    }

    // ------------------------------------------------------------------
    // DRAWING
    // ------------------------------------------------------------------
    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode2D(camera);

    bool isMenuState = (currentEngineState == MAIN_MENU   ||
                        currentEngineState == CHAR_SELECT  ||
                        currentEngineState == SETTINGS     ||
                        currentEngineState == DIFFICULTY_MODAL ||
                        currentEngineState == PAUSE        ||
                        currentEngineState == GAME_OVER);

    // Background drawing
    {
        Texture2D currentBg = isMenuState ? menuBg : bgs[selectedBgIndex];
        float scaleX = (float)screenWidth  / currentBg.width;
        float scaleY = (float)screenHeight / currentBg.height;
        float scale  = fmaxf(scaleX, scaleY) * 1.05f;
        float panX = 0.0f;
        if (currentEngineState == MAIN_MENU) {
            float maxPan = currentBg.width * scale * 0.05f;
            panX = maxPan * 0.5f * (1.0f + sinf(GetTime() * 0.8f));
        }
        Rectangle bgSource = {0, 0, (float)currentBg.width, (float)currentBg.height};
        Rectangle bgDest   = {
            (float)screenWidth / 2.0f - panX,
            (float)screenHeight / 2.0f,
            currentBg.width  * scale,
            currentBg.height * scale
        };
        DrawTexturePro(currentBg, bgSource, bgDest,{bgDest.width / 2.0f, bgDest.height / 2.0f}, 0.0f, WHITE);
    }

    auto getTint = [](float flashTimer) -> Color {
        return (flashTimer > 0.0f) ? (Color){255, 100, 100, 255} : WHITE;
    };

    if (currentEngineState != MAIN_MENU    &&
        currentEngineState != SETTINGS     &&
        currentEngineState != DIFFICULTY_MODAL &&
        currentEngineState != CHAR_SELECT) {

        if (sukunaDomain.IsPlaying()) sukunaDomain.Draw(screenWidth, screenHeight);
        else if (gojoDomain.IsPlaying()) gojoDomain.Draw(screenWidth, screenHeight);
        else if (gojoHeal.IsPlaying())   gojoHeal.Draw(screenWidth, screenHeight);
        else {
            const float TARGET_HEIGHT = 280.0f;

            // Draw Sukuna
            if (boss.IsAlive()) {
                Texture2D tex = sukunaStill;
                if (boss.state != DYING) {
                    switch(boss.state) {
                        case WALK_LEFT:
                            tex = ((int)(boss.animTimer * 8.0f) % 2 == 0) ? sukunaWalkLeft : sukunaStill;
                            break;
                        case WALK_RIGHT:
                            tex = ((int)(boss.animTimer * 8.0f) % 2 == 0) ? sukunaWalkRight : sukunaStill;
                            break;
                        case PUNCH: tex = sukunaPunch; break;
                        case KICK:  tex = sukunaKick;  break;
                        default: break;
                    }
                }
                float sc  = TARGET_HEIGHT / tex.height;
                if (tex.id == sukunaWalkRight.id) sc *= 1.15f;
                if (tex.id == sukunaPunch.id)     sc *= 1.20f;
                if (tex.id == sukunaKick.id)      sc *= 1.00f;

                float dir = boss.facingLeft ? -1.0f : 1.0f;
                float dx  = boss.x;
                if (tex.id == sukunaWalkRight.id) dx += (boss.facingLeft ? 12.0f : -12.0f);

                float alpha = 1.0f, fallOff = 0.0f;
                if (boss.state == DYING) {
                    float t = 1.0f - (boss.deathTimer / Sukuna::DEATH_DURATION);
                    alpha = 1.0f - t;
                    fallOff = t * 80.0f;
                }

                Color tint = getTint(boss.hitFlashTimer);
                tint.a = (unsigned char)(alpha * 255);

                Rectangle src = {0, 0, (float)tex.width * dir, (float)tex.height};
                Rectangle dst = {dx, boss.y + fallOff, (float)tex.width * sc, (float)tex.height * sc};
                Vector2   org = {dst.width / 2, dst.height};
                DrawTexturePro(tex, src, dst, org, 0.0f, tint);
            }

            // Draw Gojo
            if (player.IsAlive()) {
                Texture2D tex = gojoStill;
                if (player.state != DYING && !player.infinityActive) {
                    switch(player.state) {
                        case WALK_LEFT:
                            tex = ((int)(player.animTimer * 8.0f) % 2 == 0) ? gojoWalkLeft : gojoStill;
                            break;
                        case WALK_RIGHT:
                            tex = ((int)(player.animTimer * 8.0f) % 2 == 0) ? gojoWalkRight : gojoStill;
                            break;
                        case PUNCH: tex = gojoPunch; break;
                        case KICK:  tex = gojoKick;  break;
                        default: break;
                    }
                }
                float sc  = TARGET_HEIGHT / tex.height;
                float dir = player.facingLeft ? -1.0f : 1.0f;

                float alpha = 1.0f, fallOff = 0.0f;
                if (player.state == DYING) {
                    float t = 1.0f - (player.deathTimer / Gojo::DEATH_DURATION);
                    alpha = 1.0f - t;
                    fallOff = t * 80.0f;
                }

                Color tint = getTint(player.hitFlashTimer);
                tint.a = (unsigned char)(alpha * 255);

                Rectangle src = {0, 0, (float)tex.width * dir, (float)tex.height};
                float yOffset = -10.0f;
                Rectangle dst = {player.x, player.y + fallOff + yOffset, (float)tex.width * sc, (float)tex.height * sc};
                Vector2   org = {dst.width / 2, dst.height};
                DrawTexturePro(tex, src, dst, org, 0.0f, tint);
            }

            // Mahoraga overlay
            if (mahoragaFreezeTimer > 0.0f) {
                DrawRectangle(-50,-50,screenWidth+100,screenHeight+100,
                    Fade(BLACK,((6.0f-mahoragaFreezeTimer)/6.0f)*0.85f));
                DrawText("MAHORAGA SUMMONED",
                    screenWidth/2-MeasureText("MAHORAGA SUMMONED",30)/2, 40, 30, PURPLE);
                float mScale = (float)screenHeight / mahoragaTex.height;
                float mX     = screenWidth/2 - (mahoragaTex.width*mScale)/2;
                DrawTextureEx(mahoragaTex, {mX, 0}, 0, mScale, WHITE);
            }
        }
    }
    EndMode2D();

    // HUD drawing
    bool showHud = (currentEngineState == GAMEPLAY || currentEngineState == PAUSE) &&
                   !sukunaDomain.IsPlaying() && !gojoDomain.IsPlaying() && !gojoHeal.IsPlaying();

    if (showHud) {
        float gw = ((float)player.health / player.maxHealth) * 400.0f;
        float sw = ((float)boss.health   / boss.maxHealth)   * 400.0f;
        if (gw < 0) gw = 0; if (sw < 0) sw = 0;

        DrawRectangle(36,36,408,38,BLACK);
        DrawRectangle(40,40,400,30,DARKGRAY);
        DrawRectangle(40,40,(int)gw,30,SKYBLUE);
        DrawText("GOJO",45,45,20,WHITE);

        DrawRectangle(screenWidth-444,36,408,38,BLACK);
        DrawRectangle(screenWidth-440,40,400,30,DARKGRAY);
        DrawRectangle(screenWidth-440,40,(int)sw,30,MAROON);
        DrawText("SUKUNA",screenWidth-140,45,20,WHITE);

        if (player.health > 0 && boss.health > 0) {
            if (player.domainCooldown > 0)
                DrawText(TextFormat("Gojo CD: %.1fs", player.domainCooldown), 40, 80, 20, RED);
            else
                DrawText("Gojo Domain Ready [Q]", 40, 80, 20, GREEN);

            if (!isTwoPlayerMode) {
                const char* suLabel = "Sukuna Domain: AUTO";
                Color suColor = {120, 120, 120, 200};
                DrawText(suLabel, screenWidth-440, 85, 20, suColor);
                if ((boss.health <= 175 && !boss.domain175Triggered) ||
                    (boss.health <= 25  && !boss.domain25Triggered)) {
                    DrawText("! INCOMING !", screenWidth-440, 108, 18, ORANGE);
                }
            } else {
                if (boss.domainCooldown > 0)
                    DrawText(TextFormat("Sukuna CD: %.1fs", boss.domainCooldown), screenWidth-440, 85, 20, RED);
                else
                    DrawText("Sukuna Ready [O/I]", screenWidth-440, 85, 20, GREEN);
            }

            if (player.comboCount > 0)
                DrawText(TextFormat("COMBO %d", player.comboCount), screenWidth/2-50, 50, 30, ORANGE);
            if (currentDifficulty == HARD && player.infinityActive)
                DrawText("INFINITY ACTIVE", screenWidth/2-80, 100, 30, SKYBLUE);
            if (boss.mahoragaSummoned && mahoragaFreezeTimer <= 0 && (int)(GetTime()*5)%2==0)
                DrawText("SUKUNA GOT A MASSIVE BUFF!",
                    screenWidth/2-MeasureText("SUKUNA GOT A MASSIVE BUFF!",40)/2, 100, 40, RED);
        }
    }

    // Main menu UI
    if (currentEngineState == MAIN_MENU) {
        DrawMenuParticles();
        DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.45f));
        DrawText("JUJUTSU ARENA", screenWidth/2-MeasureText("JUJUTSU ARENA",60)/2, 100, 60, RED);
        DrawText("Choose your battle", screenWidth/2-MeasureText("Choose your battle",30)/2, 180, 30, LIGHTGRAY);
        const char* opts[] = {"1. Single Player (vs AI)", "2. Two Player (PvP)", "3. Settings"};
        int y = 290;
        for (int i = 0; i < MENU_OPT_COUNT; i++) {
            bool sel = (i == menuOption);
            if (sel) DrawRectangle(screenWidth/2-200, y-4, 400, 36, Fade(RED, 0.25f));
            Color c = sel ? YELLOW : WHITE;
            DrawText(opts[i], screenWidth/2-MeasureText(opts[i],28)/2, y, 28, c);
            y += 52;
        }
        DrawText("Controls:", 50, screenHeight-140, 20, SKYBLUE);
        DrawText("P1: A/D | Y/G | Q | E (Hard Infinity) | SPACE", 50, screenHeight-112, 18, WHITE);
        DrawText("P2: Arrows | P/K | O/I | RIGHT_SHIFT", 50, screenHeight-84, 18, WHITE);
        DrawText("M: Mute Music", screenWidth-220, screenHeight-50, 18, LIGHTGRAY);
    }

    // Character select UI
    if (currentEngineState == CHAR_SELECT) {
        DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.80f));
        DrawText("SELECT FIGHTER", screenWidth/2-MeasureText("SELECT FIGHTER",50)/2, 50, 50, YELLOW);
        // Gojo card
        {
            bool sel = (charSelectHighlight == 0);
            Rectangle card = {100, 150, 480, 420};
            DrawRectangleRec(card, Fade(sel ? SKYBLUE : DARKGRAY, 0.35f));
            DrawRectangleLinesEx(card, 3, sel ? SKYBLUE : GRAY);
            float h = 260.0f;
            float sc = h / gojoStill.height;
            DrawTextureEx(gojoStill, {card.x + card.width/2 - gojoStill.width*sc/2, card.y + 20}, 0, sc, WHITE);
            DrawText("GOJO SATORU", (int)(card.x+30), (int)(card.y+300), 28, sel?WHITE:LIGHTGRAY);
            DrawText("HP: 200  SPD: High", (int)(card.x+30), (int)(card.y+335), 20, LIGHTGRAY);
            DrawText("Special: Infinity | Domain", (int)(card.x+30), (int)(card.y+360), 18, SKYBLUE);
            DrawText("Punch: 8  Kick: 5", (int)(card.x+30), (int)(card.y+385), 18, WHITE);
            if (sel) DrawText("PRESS ENTER", (int)(card.x+140), (int)(card.y+410), 20, GREEN);
        }
        // Sukuna card
        {
            bool sel = (charSelectHighlight == 1);
            Rectangle card = {700, 150, 480, 420};
            DrawRectangleRec(card, Fade(sel ? MAROON : DARKGRAY, 0.35f));
            DrawRectangleLinesEx(card, 3, sel ? RED : GRAY);
            float h = 260.0f;
            float sc = h / sukunaStill.height;
            DrawTextureEx(sukunaStill, {card.x + card.width/2 - sukunaStill.width*sc/2, card.y + 20}, 0, sc, WHITE);
            DrawText("RYOMEN SUKUNA", (int)(card.x+30), (int)(card.y+300), 28, sel?WHITE:LIGHTGRAY);
            DrawText("HP: 200  SPD: Med", (int)(card.x+30), (int)(card.y+335), 20, LIGHTGRAY);
            DrawText("Special: Domain Expansion", (int)(card.x+30), (int)(card.y+360), 18, RED);
            DrawText("Punch: 5  Kick: 8", (int)(card.x+30), (int)(card.y+385), 18, WHITE);
            DrawText("[AI Controlled]", (int)(card.x+30), (int)(card.y+410), 18, GRAY);
        }
        DrawText("LEFT/RIGHT to browse  |  ENTER to confirm  |  B to go back",
            screenWidth/2-MeasureText("LEFT/RIGHT to browse  |  ENTER to confirm  |  B to go back",20)/2,
            screenHeight-50, 20, LIGHTGRAY);
    }

    // Difficulty modal UI
    if (currentEngineState == DIFFICULTY_MODAL) {
        DrawRectangle(0,0,screenWidth,screenHeight,Fade(BLACK,0.85f));
        DrawText(isTwoPlayerMode ? "TWO PLAYER SETTINGS" : "LOBBY SETTINGS",
            screenWidth/2-MeasureText(isTwoPlayerMode ? "TWO PLAYER SETTINGS" : "LOBBY SETTINGS",35)/2, 60, 35, YELLOW);

        int yOffset = 130;
        int optIdx = 0;

        if (!isTwoPlayerMode) {
            const char* diffText[] = {"EASY","MID","HARD"};
            DrawText("Difficulty: ", screenWidth/2-200, yOffset, 30, (modalOption==optIdx)?YELLOW:WHITE);
            DrawText(diffText[modalDifficultyIndex], screenWidth/2+80, yOffset, 30, (modalOption==optIdx)?SKYBLUE:WHITE);
            yOffset += 60;
            optIdx++;
        }

        DrawText("Music: ", screenWidth/2-200, yOffset, 30, (modalOption==optIdx)?YELLOW:WHITE);
        DrawText(songDisplayNames[modalMusicIndex], screenWidth/2+80, yOffset, 30, (modalOption==optIdx)?SKYBLUE:WHITE);
        yOffset += 60;
        optIdx++;

        DrawText("Map: ", screenWidth/2-200, yOffset, 30, (modalOption==optIdx)?YELLOW:WHITE);
        string mapName = "Background " + to_string(modalBgIndex+1);
        DrawText(mapName.c_str(), screenWidth/2+80, yOffset, 30, (modalOption==optIdx)?SKYBLUE:WHITE);
        yOffset += 60;
        optIdx++;

        Rectangle previewDest = {screenWidth/2.0f-160.0f, (float)yOffset, 320.0f, 180.0f};
        Texture2D previewTex  = bgs[modalBgIndex];
        DrawTexturePro(previewTex, {0,0,(float)previewTex.width,(float)previewTex.height}, previewDest, {0,0}, 0.0f, WHITE);
        DrawRectangleLinesEx(previewDest, 3.0f, (modalOption==optIdx-1)?SKYBLUE:DARKGRAY);
        yOffset += 200;

        DrawText("START GAME", screenWidth/2-MeasureText("START GAME",30)/2, yOffset, 30, (modalOption==optIdx)?GREEN:WHITE);
        DrawText("Use LEFT/RIGHT to change, ENTER to start, B to go back",
            screenWidth/2-270, 620, 20, LIGHTGRAY);
    }

    // Pause menu UI
    if (currentEngineState == PAUSE) {
        DrawRectangle(0,0,screenWidth,screenHeight,Fade(BLACK,0.7f));
        DrawText("GAME PAUSED", screenWidth/2-MeasureText("GAME PAUSED",50)/2, 150, 50, WHITE);
        const char* opts[] = {"Resume","Settings","Main Menu","Exit"};
        for (int i=0;i<4;i++) {
            Color c = (i==pauseOption)?YELLOW:WHITE;
            DrawText(opts[i], screenWidth/2-MeasureText(opts[i],32)/2, 300+i*55, 32, c);
        }
        DrawText("Press ENTER to select", screenWidth/2-MeasureText("Press ENTER to select",20)/2, 550, 20, LIGHTGRAY);
    }

    // Game over UI
    if (currentEngineState == GAME_OVER) {
        DrawRectangle(0,0,screenWidth,screenHeight,Fade(BLACK,0.8f));
        const char* winner = (boss.health <= 0) ? "GOJO WINS!" : "SUKUNA WINS!";
        Color winColor     = (boss.health <= 0) ? GREEN : RED;
        DrawText(winner, screenWidth/2-MeasureText(winner,60)/2, 150, 60, winColor);
        const char* opts[] = {"Retry","Main Menu","Exit"};
        for (int i=0;i<3;i++) {
            Color c = (i==gameOverOption)?YELLOW:WHITE;
            DrawText(opts[i], screenWidth/2-MeasureText(opts[i],32)/2, 320+i*55, 32, c);
        }
    }

    // Settings UI
    if (currentEngineState == SETTINGS) {
        DrawRectangle(0,0,screenWidth,screenHeight,Fade(BLACK,0.85f));
        DrawText("SETTINGS",screenWidth/2-MeasureText("SETTINGS",50)/2,60,50,YELLOW);
        DrawText("MUSIC",screenWidth/2-MeasureText("MUSIC",30)/2,130,30,SKYBLUE);
        DrawText("Select Song (UP/DOWN, ENTER to play)", screenWidth/2-MeasureText("Select Song (UP/DOWN, ENTER to play)",24)/2, 180, 24, WHITE);
        for (int i=0;i<songCount;i++) {
            Color col = (i==selectedSongIndex)?YELLOW:LIGHTGRAY;
            string prefix = (i==currentSong)?"> ":"  ";
            string fullText = prefix + songDisplayNames[i];
            DrawText(fullText.c_str(),screenWidth/2-MeasureText(fullText.c_str(),25)/2,230+i*35,25,col);
        }
        DrawText("Volume:",screenWidth/2-100,420,25,WHITE);
        DrawText(TextFormat("%.0f%%",volumeSliderValue),screenWidth/2+120,420,25,WHITE);
        DrawRectangleRec(volumeSlider,LIGHTGRAY);
        DrawRectangle(volumeSlider.x,volumeSlider.y, volumeSlider.width*(volumeSliderValue/100.0f),volumeSlider.height,BLUE);
        DrawText("Click & drag slider", screenWidth/2-MeasureText("Click & drag slider",18)/2,510,18,LIGHTGRAY);
        DrawText("Repeat: ",screenWidth/2-50,550,25,WHITE);
        DrawText(repeatSong?"ON":"OFF",screenWidth/2+50,550,25,repeatSong?GREEN:RED);
        DrawText("R: Toggle Repeat",screenWidth/2-MeasureText("R: Toggle Repeat",20)/2,600,20,LIGHTGRAY);
        DrawText("M: Mute/Unmute",screenWidth/2-MeasureText("M: Mute/Unmute",20)/2,630,20,LIGHTGRAY);
        DrawText("B: Back",screenWidth/2-MeasureText("B: Back",20)/2,660,20,LIGHTGRAY);
    }

    if (musicMuted) DrawText("[MUTED]", screenWidth-100, 10, 20, RED);
    EndDrawing();
}

// ---------------------------------------------------------------------------
// Main entry point
// ---------------------------------------------------------------------------
int main() {
    InitWindow(screenWidth, screenHeight, "Jujutsu Arena");
    InitAudioDevice();

    LoadAllMusic();
    LoadAnnouncerSounds();

    currentSong = 0; repeatSong = false; musicMuted = false; musicVolume = 0.2f;
    SetMusicVolumeLevel(musicVolume);
    PlayCurrentSong();
    PauseMusicStream(songs[currentSong]);

    currentEngineState = MAIN_MENU;
    isTwoPlayerMode = false;
    currentDifficulty = MID;

    // Background textures
    bgs[0] = LoadTexture("assets/images/backgrounds/bg1.jpeg");
    bgs[1] = LoadTexture("assets/images/backgrounds/bg2.jpeg");
    bgs[2] = LoadTexture("assets/images/backgrounds/bg3.jpeg");
    bgs[3] = LoadTexture("assets/images/backgrounds/bg4.jpeg");
    selectedBgIndex = 0;

    // Menu and character textures
    menuBg       = LoadTexture("assets/images/backgrounds/menuBg.jpeg");
    gojoStill    = LoadTexture("assets/images/gojo/gojoStill.png");
    gojoWalkRight= LoadTexture("assets/images/gojo/gojoWalkRight.png");
    gojoWalkLeft = LoadTexture("assets/images/gojo/gojoWalkLeft.png");
    gojoPunch    = LoadTexture("assets/images/gojo/gojoPunch.png");
    gojoKick     = LoadTexture("assets/images/gojo/gojoKick.png");

    sukunaStill    = LoadTexture("assets/images/sukuna/sukunaStill.png");
    sukunaWalkRight= LoadTexture("assets/images/sukuna/sukunaWalkRight.png");
    sukunaWalkLeft = LoadTexture("assets/images/sukuna/sukunaWalkLeft.png");
    sukunaPunch    = LoadTexture("assets/images/sukuna/sukunaPunch.png");
    sukunaKick     = LoadTexture("assets/images/sukuna/sukunaKick.png");
    mahoragaTex    = LoadTexture("assets/images/mahoraga.png");

    // Video players (heap-allocated — require constructor arguments)
    pSukunaDomain = new VideoPlayer("assets/video/sukuna_domain","ffout",".gif",193,10.0f);
    pGojoDomain   = new VideoPlayer("assets/video/gojo_domain","ffout",".gif",233,10.0f);
    pGojoHeal     = new VideoPlayer("assets/video/gojo_heal","ffout",".gif",50,10.0f);

    // Sound effects
    sukunaDomainSound = LoadSound("assets/audio/sfx/sukunaDomain.wav");
    gojoDomainSound   = LoadSound("assets/audio/sfx/gojoDomain.wav");
    gojoHealSound     = LoadSound("assets/audio/sfx/gojoHeal.wav");
    mahoragaSound     = LoadSound("assets/audio/sfx/mahoraga.wav");
    combatHitSound    = LoadSound("assets/audio/sfx/punch.wav");
    teleportSound     = LoadSound("assets/audio/sfx/teleport.wav");
    SetSoundVolume(mahoragaSound, 2.0f);

    // Game objects
    camera.zoom = 1.0f;
    mahoragaFreezeTimer = 0.0f;
    SetTargetFPS(60);

    // UI defaults
    menuOption = 0;
    pauseOption = 0;
    gameOverOption = 0;
    selectedSongIndex = 0;
    adjustingVolume = false;
    volumeSlider = {screenWidth/2.0f - 100, 480, 200, 20};
    volumeSliderValue = musicVolume * 100;

    modalOption = 0;
    modalDifficultyIndex = 1;
    modalMusicIndex = 0;
    modalBgIndex = 0;

    charSelectHighlight = 0;
    charSelectConfirmed = false;

    fightVoPlayed = false;
    comboVoPlayed = false;

    InitMenuParticles(screenWidth, screenHeight);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    while (!WindowShouldClose() && !shouldExit) {
        UpdateDrawFrame();
    }
#endif

    // Cleanup resources
    delete pSukunaDomain;
    delete pGojoDomain;
    delete pGojoHeal;

    UnloadAllMusic();
    UnloadAnnouncerSounds();
    for (int i=0;i<bgCount;i++) UnloadTexture(bgs[i]);
    UnloadTexture(gojoStill); UnloadTexture(gojoWalkRight); UnloadTexture(gojoWalkLeft);
    UnloadTexture(gojoPunch); UnloadTexture(gojoKick); UnloadTexture(menuBg);
    UnloadTexture(sukunaStill); UnloadTexture(sukunaWalkRight); UnloadTexture(sukunaWalkLeft);
    UnloadTexture(sukunaPunch); UnloadTexture(sukunaKick); UnloadTexture(mahoragaTex);
    UnloadSound(sukunaDomainSound); UnloadSound(gojoDomainSound);
    UnloadSound(mahoragaSound); UnloadSound(gojoHealSound);
    UnloadSound(combatHitSound); UnloadSound(teleportSound);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
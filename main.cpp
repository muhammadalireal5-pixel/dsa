// --- INCLUDES & NAMESPACES ---
#include <iostream>
#include <cstdlib>
#include <string>
#include "raylib.h"
#include "videoPlayer.h"

using namespace std;

// --- ENUMS & STRUCTS ---
enum GameState { MAIN_MENU, GAMEPLAY, PAUSE, GAME_OVER, SETTINGS, DIFFICULTY_MODAL };
enum AnimState { IDLE, WALK_LEFT, WALK_RIGHT, PUNCH, KICK };
enum Difficulty { EASY, MID, HARD };

// --- GOJO LOGIC ---
struct Gojo {
    float x, y;
    int health = 200;
    int maxHealth = 200;
    int speed = 4;
    int punchDamage = 8;
    int kickDamage = 5;

    AnimState state = IDLE;
    float actionTimer = 0.0f;
    int comboCount = 0;
    float domainCooldown = 0.0f;
    bool facingLeft = false;
    bool rctUsed = false;

    // Hard mode specific
    bool infinityActive = false;
    float infinityTimer = 0.0f;
    int hardModeHitStreak = 0;

    bool IsAlive() { return health > 0; }

    void Reset() {
        health = maxHealth;
        state = IDLE;
        actionTimer = 0.0f;
        comboCount = 0;
        domainCooldown = 0.0f;
        facingLeft = false;
        rctUsed = false;
        infinityActive = false;
        infinityTimer = 0.0f;
        hardModeHitStreak = 0;
    }

    void Update(int screenWidth, float bossX, bool bossAlive, bool isTwoPlayer, Difficulty difficulty) {
        if (!IsAlive()) return;

        // Domain Cooldown
        if (domainCooldown > 0.0f) {
            domainCooldown -= GetFrameTime();
            if (domainCooldown < 0.0f) domainCooldown = 0.0f;
        }

        // Infinity activation (Hard mode only, single player)
        if (difficulty == HARD && !isTwoPlayer && IsKeyPressed(KEY_E) && !infinityActive && bossAlive) {
            infinityActive = true;
            infinityTimer = 3.0f;
        }
        if (infinityActive) {
            infinityTimer -= GetFrameTime();
            if (infinityTimer <= 0.0f) {
                infinityActive = false;
            }
        }

        // Teleport (Space)
        if (bossAlive && IsKeyPressed(KEY_SPACE)) {
            if (x < bossX) x = bossX + 150.0f;
            else x = bossX - 150.0f;
            if (x < 50) x = 50;
            if (x > screenWidth - 50) x = screenWidth - 50;
        }

        // Attack animation lock
        if (state == PUNCH || state == KICK) {
            actionTimer += GetFrameTime();
            if (actionTimer >= 0.5f) {
                state = IDLE;
                actionTimer = 0.0f;
            }
        }
        else {
            state = IDLE;
            float nextX = x;
            if (IsKeyDown(KEY_D)) { nextX += speed; state = WALK_RIGHT; }
            else if (IsKeyDown(KEY_A)) { nextX -= speed; state = WALK_LEFT; }
            if (bossAlive && abs(nextX - bossX) < 60.0f) {
                if (abs(nextX - bossX) < abs(x - bossX)) nextX = x;
            }
            x = nextX;
            if (x < 50) x = 50;
            if (x > screenWidth - 50) x = screenWidth - 50;
        }
    }
};

// --- SUKUNA LOGIC ---
struct Sukuna {
    float x, y;
    int health = 200;
    int maxHealth = 200;
    int speed = 2;
    int punchDamage = 5;
    int kickDamage = 8;

    AnimState state = IDLE;
    float actionTimer = 0.0f;
    float attackCooldown = 0.0f;
    bool facingLeft = true;

    float domainCooldown = 0.0f;
    bool domain175Triggered = false;
    bool domain25Triggered = false;
    bool mahoragaSummoned = false;
    int damagePool = 0;
    Difficulty gameDifficulty = MID;

    bool IsAlive() { return health > 0; }

    void Reset(Difficulty diff) {
        health = maxHealth;
        state = IDLE;
        actionTimer = 0.0f;
        attackCooldown = 0.0f;
        facingLeft = true;
        domainCooldown = 0.0f;
        domain175Triggered = false;
        domain25Triggered = false;
        mahoragaSummoned = false;
        damagePool = 0;
        punchDamage = 5;
        kickDamage = 8;
        speed = 2;
        gameDifficulty = diff;
    }

    void TakeDamage(int amount, float targetX, int screenW) {
        health -= amount;
        damagePool += amount;
        int threshold = (gameDifficulty == HARD) ? 20 : 25;
        if (damagePool >= threshold) {
            // FIX: was "if (x < 50) x = screenW - 50" which teleported boss to wrong side
            if (x < targetX) x = targetX + 150.0f;
            else x = targetX - 150.0f;
            if (x < 50) x = 50;
            if (x > screenW - 50) x = screenW - 50;
            damagePool -= threshold;
        }
    }

    void UpdatePlayer2(int screenWidth, float targetX, bool targetAlive) {
        if (!IsAlive()) return;
        if (domainCooldown > 0.0f) {
            domainCooldown -= GetFrameTime();
            if (domainCooldown < 0.0f) domainCooldown = 0.0f;
        }
        if (attackCooldown > 0.0f) attackCooldown -= GetFrameTime();
        if (targetAlive && IsKeyPressed(KEY_RIGHT_SHIFT)) {
            if (x < targetX) x = targetX + 150.0f;
            else x = targetX - 150.0f;
            if (x < 50) x = 50;
            if (x > screenWidth - 50) x = screenWidth - 50;
        }
        if (state == PUNCH || state == KICK) {
            actionTimer += GetFrameTime();
            if (actionTimer >= 0.5f) {
                state = IDLE;
                actionTimer = 0.0f;
            }
        }
        else {
            state = IDLE;
            float nextX = x;
            if (IsKeyDown(KEY_RIGHT)) { nextX += (speed + 1); state = WALK_RIGHT; }
            if (IsKeyDown(KEY_LEFT)) { nextX -= (speed + 1); state = WALK_LEFT; }
            if (targetAlive && abs(nextX - targetX) < 60.0f) {
                if (abs(nextX - targetX) < abs(x - targetX)) nextX = x;
            }
            x = nextX;
            if (x < 50) x = 50;
            if (x > screenWidth - 50) x = screenWidth - 50;
        }
    }

    void UpdateAI(float targetX, bool targetAlive) {
        if (!IsAlive()) return;
        if (attackCooldown > 0.0f) attackCooldown -= GetFrameTime();
        if (state == PUNCH || state == KICK) {
            actionTimer += GetFrameTime();
            if (actionTimer >= 0.5f) {
                state = IDLE;
                actionTimer = 0.0f;
                attackCooldown = 0.8f;
            }
        }
        else {
            float distance = targetX - x;
            if (abs(distance) < 80.0f && attackCooldown <= 0.0f) {
                state = (GetRandomValue(1, 100) % 2 == 0) ? PUNCH : KICK;
                actionTimer = 0.0f;
            }
            else {
                float nextX = x;
                if (distance > 10.0f) { nextX += speed; state = WALK_RIGHT; }
                else if (distance < -10.0f) { nextX -= speed; state = WALK_LEFT; }
                else { state = IDLE; }
                if (targetAlive && abs(nextX - targetX) < 60.0f) {
                    if (abs(nextX - targetX) < abs(x - targetX)) nextX = x;
                }
                x = nextX;
            }
        }
    }
};

// --- VISUAL EFFECTS ---
struct ScreenEffects {
    float shakeTimer = 0.0f;
    float shakeIntensity = 0.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    void TriggerShake(float time, float intensity) {
        shakeTimer = time;
        shakeIntensity = intensity;
    }
    void Update() {
        if (shakeTimer > 0.0f) {
            shakeTimer -= GetFrameTime();
            offsetX = (float)GetRandomValue(-(int)shakeIntensity, (int)shakeIntensity);
            offsetY = (float)GetRandomValue(-(int)shakeIntensity, (int)shakeIntensity);
            if (shakeTimer <= 0.0f) { offsetX = 0.0f; offsetY = 0.0f; }
        }
    }
};

// --- GLOBAL RESET FUNCTION ---
void ResetGame(Gojo& player, Sukuna& boss, float& mahoragaFreezeTimer,
               int screenWidth, int screenHeight, Difficulty diff) {
    player.Reset();
    boss.Reset(diff);
    mahoragaFreezeTimer = 0.0f;
    player.x = screenWidth * 0.20f;
    player.y = screenHeight - 30.0f;
    boss.x = screenWidth * 0.80f;
    boss.y = screenHeight - 30.0f;
}

// --- HELPERS TO STOP VIDEOS ---
void StopDomainVideos(VideoPlayer& sukuna, VideoPlayer& gojo, VideoPlayer& heal) {
    if (sukuna.IsPlaying()) { while (sukuna.IsPlaying()) sukuna.Update(1.0f); }
    if (gojo.IsPlaying()) { while (gojo.IsPlaying()) gojo.Update(1.0f); }
    if (heal.IsPlaying()) { while (heal.IsPlaying()) heal.Update(1.0f); }
}

// --- MUSIC MANAGEMENT ---
const char* songFiles[] = {
    "MONTAGEM_TENTANA.wav",
    "IWillSurvive.wav",
    "matadora.wav",
    "judas.wav"
};
const int songCount = 4;
Music songs[songCount];
int currentSong = 0;
bool repeatSong = false;
bool musicMuted = false;
float musicVolume = 0.2f;
bool musicInitialized = false;
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

void UpdateMusic(GameState currentState) {
    if (!musicInitialized) return;
    if (currentState == GAMEPLAY) {
        UpdateMusicStream(songs[currentSong]);
        if (!IsMusicStreamPlaying(songs[currentSong])) {
            if (repeatSong) PlayCurrentSong();
            else {
                currentSong = (currentSong + 1) % songCount;
                PlayCurrentSong();
            }
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

// --- MAIN ENGINE ---
int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Jujutsu Arena");
    InitAudioDevice();

    LoadAllMusic();
    currentSong = 0; repeatSong = false; musicMuted = false; musicVolume = 0.2f;
    SetMusicVolumeLevel(musicVolume);
    PlayCurrentSong();
    PauseMusicStream(songs[currentSong]);

    GameState currentEngineState = MAIN_MENU;
    bool isTwoPlayerMode = false;
    Difficulty currentDifficulty = MID;

    // Load Backgrounds
    const int bgCount = 4;
    Texture2D bgs[bgCount];
    bgs[0] = LoadTexture("bg1.jpeg");
    bgs[1] = LoadTexture("bg2.jpeg");
    bgs[2] = LoadTexture("bg3.jpeg");
    bgs[3] = LoadTexture("bg4.jpeg");

    int selectedBgIndex = 0;

    // Entity Textures
    Texture2D menuBg = LoadTexture("menuBg.jpeg");
    Texture2D gojoStill = LoadTexture("gojoStill.png");
    Texture2D gojoWalkRight = LoadTexture("gojoWalkRight.png");
    Texture2D gojoWalkLeft = LoadTexture("gojoWalkLeft.png");
    Texture2D gojoPunch = LoadTexture("gojoPunch.png");
    Texture2D gojoKick = LoadTexture("gojoKick.png");

    Texture2D sukunaStill = LoadTexture("sukunaStill.png");
    Texture2D sukunaWalkRight = LoadTexture("sukunaWalkRight.png");
    Texture2D sukunaWalkLeft = LoadTexture("sukunaWalkLeft.png");
    Texture2D sukunaPunch = LoadTexture("sukunaPunch.png");
    Texture2D sukunaKick = LoadTexture("sukunaKick.png");
    Texture2D mahoragaTex = LoadTexture("mahoraga.png");

    VideoPlayer sukunaDomain("sukuna","ffout",".gif",193,10.0f);
    Sound sukunaDomainSound = LoadSound("sukunaDomain.wav");
    VideoPlayer gojoDomain("gojoDomain","ffout",".gif",233,10.0f);
    Sound gojoDomainSound = LoadSound("gojoDomain.wav");
    VideoPlayer gojoHeal("gojoHeal","ffout",".gif",50,10.0f);
    Sound gojoHealSound = LoadSound("gojoHeal.wav");
    Sound mahoragaSound = LoadSound("mahoraga.wav");
    Sound combatHitSound = LoadSound("punch.wav");
    Sound teleportSound = LoadSound("teleport.wav");
    SetSoundVolume(mahoragaSound, 2.0f);

    Gojo player;
    Sukuna boss;
    ScreenEffects vfx;
    Camera2D camera = {0};
    camera.zoom = 1.0f;
    float mahoragaFreezeTimer = 0.0f;
    SetTargetFPS(60);

    // Menu vars
    int menuOption = 0;
    int pauseOption = 0;
    int gameOverOption = 0;
    int selectedSongIndex = 0;
    bool adjustingVolume = false;
    Rectangle volumeSlider = {screenWidth/2 - 100, 480, 200, 20};
    float volumeSliderValue = musicVolume * 100;

    // Modal vars
    int modalOption = 0;
    int modalDifficultyIndex = 1; // 0=EASY,1=MID,2=HARD
    int modalMusicIndex = 0;
    int modalBgIndex = 0;

    // --- MAIN LOOP ---
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_M)) SetMusicMute(!musicMuted);
        UpdateMusic(currentEngineState);

        // ========== STATE UPDATES ==========
        if (currentEngineState == MAIN_MENU) {
            if (IsKeyPressed(KEY_UP)) menuOption = (menuOption - 1 + 3) % 3;
            if (IsKeyPressed(KEY_DOWN)) menuOption = (menuOption + 1) % 3;
            if (IsKeyPressed(KEY_ENTER)) {
                if (menuOption == 0) { // Single Player
                    isTwoPlayerMode = false;
                    previousState = MAIN_MENU;
                    currentEngineState = DIFFICULTY_MODAL;
                    modalOption = 0;
                    modalDifficultyIndex = (int)currentDifficulty;
                    modalMusicIndex = currentSong;
                    modalBgIndex = selectedBgIndex;
                } else if (menuOption == 1) { // Two Player
                    isTwoPlayerMode = true;
                    currentDifficulty = MID;
                    ResetGame(player, boss, mahoragaFreezeTimer, screenWidth, screenHeight, currentDifficulty);
                    currentEngineState = GAMEPLAY;
                    ResumeMusicStream(songs[currentSong]);
                } else if (menuOption == 2) { // Settings
                    previousState = MAIN_MENU;
                    currentEngineState = SETTINGS;
                    selectedSongIndex = currentSong;
                    volumeSliderValue = musicVolume * 100;
                }
            }
        }
        else if (currentEngineState == DIFFICULTY_MODAL) {
            if (IsKeyPressed(KEY_UP)) modalOption = (modalOption - 1 + 4) % 4;
            if (IsKeyPressed(KEY_DOWN)) modalOption = (modalOption + 1) % 4;

            if (IsKeyPressed(KEY_LEFT)) {
                if (modalOption == 0) modalDifficultyIndex = (modalDifficultyIndex - 1 + 3) % 3;
                else if (modalOption == 1) modalMusicIndex = (modalMusicIndex - 1 + songCount) % songCount;
                else if (modalOption == 2) modalBgIndex = (modalBgIndex - 1 + bgCount) % bgCount;
            }
            if (IsKeyPressed(KEY_RIGHT)) {
                if (modalOption == 0) modalDifficultyIndex = (modalDifficultyIndex + 1) % 3;
                else if (modalOption == 1) modalMusicIndex = (modalMusicIndex + 1) % songCount;
                else if (modalOption == 2) modalBgIndex = (modalBgIndex + 1) % bgCount;
            }
            if (IsKeyPressed(KEY_ENTER) && modalOption == 3) {
                currentDifficulty = (Difficulty)modalDifficultyIndex;
                currentSong = modalMusicIndex;
                selectedBgIndex = modalBgIndex;

                PlayCurrentSong();
                ResumeMusicStream(songs[currentSong]);
                ResetGame(player, boss, mahoragaFreezeTimer, screenWidth, screenHeight, currentDifficulty);
                currentEngineState = GAMEPLAY;
            }
            if (IsKeyPressed(KEY_B)) {
                currentEngineState = MAIN_MENU;
            }
        }
        else if (currentEngineState == GAMEPLAY) {
            if (IsKeyPressed(KEY_ENTER) && !sukunaDomain.IsPlaying() && !gojoDomain.IsPlaying() && !gojoHeal.IsPlaying()) {
                currentEngineState = PAUSE;
                pauseOption = 0;
                PauseMusicStream(songs[currentSong]);
            }
            vfx.Update();
            camera.offset = {vfx.offsetX, vfx.offsetY};
            if (player.IsAlive() && boss.IsAlive()) {
                player.facingLeft = (player.x > boss.x);
                boss.facingLeft = (boss.x > player.x);
            }

            // FIX: RCT heal only triggers in single player (was already gated, but the
            // heal video completion block was incorrectly setting HP even outside 1P)
            if (player.health <= 0 && !player.rctUsed && !isTwoPlayerMode) {
                player.rctUsed = true;
                player.health = 1;
                gojoHeal.Play();
                PlaySound(gojoHealSound);
            }

            if (!player.IsAlive() || !boss.IsAlive()) {
                StopDomainVideos(sukunaDomain, gojoDomain, gojoHeal);
                StopSound(sukunaDomainSound);
                StopSound(gojoDomainSound);
                currentEngineState = GAME_OVER;
                gameOverOption = 0;
                PauseMusicStream(songs[currentSong]);
                continue;
            }

            // Core gameplay
            if (!sukunaDomain.IsPlaying() && !gojoDomain.IsPlaying() && !gojoHeal.IsPlaying()) {
                // FIX: Mahoraga summon only in single-player AI mode
                if (!isTwoPlayerMode && boss.health <= 100 && !boss.mahoragaSummoned) {
                    boss.mahoragaSummoned = true;
                    mahoragaFreezeTimer = 6.0f;
                    PlaySound(mahoragaSound);
                    vfx.TriggerShake(0.3f, 25.0f);
                    switch (currentDifficulty) {
                        case EASY:
                            boss.punchDamage = 15; boss.kickDamage = 15; boss.speed = 2; break;
                        case MID:
                            boss.punchDamage = 21; boss.kickDamage = 21; boss.speed = 3; break;
                        case HARD:
                            boss.punchDamage = 30; boss.kickDamage = 35; boss.speed = 4; break;
                    }
                }
                if (mahoragaFreezeTimer > 0.0f) {
                    mahoragaFreezeTimer -= GetFrameTime();
                    if ((int)(mahoragaFreezeTimer * 10.0f) % 15 == 0) vfx.TriggerShake(0.2f, 20.0f);
                }
                else {
                    // --- Gojo Domain (Q) ---
                    if (player.domainCooldown == 0.0f && player.IsAlive() && IsKeyPressed(KEY_Q)) {
                        gojoDomain.Play();
                        PlaySound(gojoDomainSound);
                        boss.TakeDamage(25, player.x, screenWidth);
                        player.domainCooldown = 15.0f;
                        // FIX: removed incorrect setting of boss.domain175Triggered / domain25Triggered
                        // here — those flags belong to Sukuna's AI domain logic only
                    }

                    // --- Sukuna AI Domain (single player only) ---
                    // FIX: wrapped entirely in !isTwoPlayerMode so it never fires in 2P
                    if (!isTwoPlayerMode && !gojoDomain.IsPlaying() && boss.IsAlive()) {
                        if (boss.health <= 175 && !boss.domain175Triggered) {
                            sukunaDomain.Play();
                            PlaySound(sukunaDomainSound);
                            player.health -= 25;
                            boss.domain175Triggered = true;
                        } else if (boss.health <= 25 && !boss.domain25Triggered) {
                            sukunaDomain.Play();
                            PlaySound(sukunaDomainSound);
                            player.health -= 25;
                            boss.domain25Triggered = true;
                        }
                    }

                    // --- Sukuna P2 Domain (two player only) ---
                    if (isTwoPlayerMode && !gojoDomain.IsPlaying() && boss.IsAlive()) {
                        if ((IsKeyPressed(KEY_O) || IsKeyPressed(KEY_I)) && boss.domainCooldown == 0.0f) {
                            sukunaDomain.Play();
                            PlaySound(sukunaDomainSound);
                            player.health -= 25;
                            boss.domainCooldown = 15.0f;
                        }
                    }

                    if (!sukunaDomain.IsPlaying() && !gojoDomain.IsPlaying()) {
                        // Teleport sounds
                        if (player.IsAlive() && boss.IsAlive() && IsKeyPressed(KEY_SPACE)) PlaySound(teleportSound);
                        player.Update(screenWidth, boss.x, boss.IsAlive(), isTwoPlayerMode, currentDifficulty);

                        if (isTwoPlayerMode) {
                            if (boss.IsAlive() && IsKeyPressed(KEY_RIGHT_SHIFT)) PlaySound(teleportSound);
                            boss.UpdatePlayer2(screenWidth, player.x, player.IsAlive());
                            bool p2Punched = IsKeyPressed(KEY_P), p2Kicked = IsKeyPressed(KEY_K);
                            if ((p2Punched || p2Kicked) && boss.attackCooldown <= 0.0f) {
                                // FIX: check infinityActive before dealing damage (was already here but
                                // the attack state/sound was still triggering even with infinity on)
                                boss.state = p2Punched ? PUNCH : KICK;
                                boss.actionTimer = 0.0f;
                                boss.attackCooldown = 0.8f;
                                if (player.IsAlive() && abs(player.x - boss.x) < 120.0f && !player.infinityActive) {
                                    PlaySound(combatHitSound);
                                    player.health -= p2Punched ? boss.punchDamage : boss.kickDamage;
                                    vfx.TriggerShake(0.2f, 4.0f);
                                    player.hardModeHitStreak = 0;
                                }
                            }
                        } else {
                            AnimState oldState = boss.state;
                            boss.UpdateAI(player.x, player.IsAlive());
                            if (boss.state != oldState && (boss.state == PUNCH || boss.state == KICK)) {
                                if (!player.infinityActive) {
                                    PlaySound(combatHitSound);
                                    if (player.IsAlive() && abs(player.x - boss.x) < 120.0f) {
                                        player.health -= (boss.state == PUNCH) ? boss.punchDamage : boss.kickDamage;
                                        vfx.TriggerShake(boss.mahoragaSummoned ? 0.4f : 0.2f, boss.mahoragaSummoned ? 15.0f : 4.0f);
                                        player.hardModeHitStreak = 0;
                                    }
                                }
                            }
                        }

                        // --- Gojo attacks ---
                        if (player.IsAlive() && (player.state != PUNCH && player.state != KICK)) {
                            bool punched = IsKeyPressed(KEY_Y), kicked = IsKeyPressed(KEY_G);
                            if (punched || kicked) {
                                player.state = punched ? PUNCH : KICK;
                                player.actionTimer = 0.0f;
                                if (boss.IsAlive() && abs(player.x - boss.x) < 120.0f) {
                                    // FIX: infinity check moved here — Gojo can still swing but
                                    // the hit only lands if infinity is NOT active (self-buff,
                                    // not self-block; infinity blocks incoming not outgoing)
                                    PlaySound(combatHitSound);
                                    int dmg = punched ? player.punchDamage : player.kickDamage;
                                    boss.TakeDamage(dmg, player.x, screenWidth);
                                    player.comboCount++;
                                    if (currentDifficulty == HARD && !isTwoPlayerMode) {
                                        player.hardModeHitStreak++;
                                        if (player.hardModeHitStreak >= 5) {
                                            boss.health = 0;
                                        }
                                    } else {
                                        player.hardModeHitStreak = 0;
                                    }
                                    if (player.comboCount >= 3) {
                                        vfx.TriggerShake(0.4f, 15.0f);
                                        player.comboCount = 0;
                                        boss.TakeDamage(5, player.x, screenWidth);
                                    }
                                } else {
                                    player.hardModeHitStreak = 0;
                                }
                            }
                        }
                    }
                }
            } else {
                // --- Cutscene handling ---
                // FIX: on F-skip, heal HP assignment was inverted (!gojoHeal.IsPlaying()).
                // Now we fast-forward all videos and then apply heal HP if the heal was playing.
                if (IsKeyPressed(KEY_F)) {
                    bool healWasPlaying = gojoHeal.IsPlaying();
                    while (sukunaDomain.IsPlaying()) sukunaDomain.Update(1.0f);
                    while (gojoDomain.IsPlaying()) gojoDomain.Update(1.0f);
                    while (gojoHeal.IsPlaying()) gojoHeal.Update(1.0f);
                    StopSound(sukunaDomainSound);
                    StopSound(gojoDomainSound);
                    if (healWasPlaying) player.health = 100;
                }
                if (sukunaDomain.IsPlaying()) {
                    sukunaDomain.Update(GetFrameTime());
                    if (!sukunaDomain.IsPlaying() && IsSoundPlaying(sukunaDomainSound))
                        StopSound(sukunaDomainSound);
                }
                if (gojoDomain.IsPlaying()) {
                    gojoDomain.Update(GetFrameTime());
                    if (!gojoDomain.IsPlaying() && IsSoundPlaying(gojoDomainSound))
                        StopSound(gojoDomainSound);
                }
                // FIX: heal completion HP restore was running every frame when NO video was
                // playing (the else of an if-chain), restoring HP to 50 constantly.
                // Now it only fires on the actual frame the heal video finishes.
                if (gojoHeal.IsPlaying()) {
                    gojoHeal.Update(GetFrameTime());
                    if (!gojoHeal.IsPlaying()) {
                        player.health = 50;
                    }
                }
            }
        }
        else if (currentEngineState == PAUSE) {
            if (IsKeyPressed(KEY_UP)) pauseOption = (pauseOption - 1 + 4) % 4;
            if (IsKeyPressed(KEY_DOWN)) pauseOption = (pauseOption + 1) % 4;
            if (IsKeyPressed(KEY_ENTER)) {
                if (pauseOption == 0) { currentEngineState = GAMEPLAY; ResumeMusicStream(songs[currentSong]); }
                else if (pauseOption == 1) { previousState = PAUSE; currentEngineState = SETTINGS; selectedSongIndex = currentSong; volumeSliderValue = musicVolume * 100; }
                else if (pauseOption == 2) { currentEngineState = MAIN_MENU; ResetGame(player,boss,mahoragaFreezeTimer,screenWidth,screenHeight,currentDifficulty); menuOption=0; }
                else if (pauseOption == 3) break;
            }
        }
        else if (currentEngineState == GAME_OVER) {
            if (IsKeyPressed(KEY_UP)) gameOverOption = (gameOverOption - 1 + 3) % 3;
            if (IsKeyPressed(KEY_DOWN)) gameOverOption = (gameOverOption + 1) % 3;
            if (IsKeyPressed(KEY_ENTER)) {
                if (gameOverOption == 0) {
                    ResetGame(player,boss,mahoragaFreezeTimer,screenWidth,screenHeight,currentDifficulty);
                    currentEngineState = GAMEPLAY;
                    ResumeMusicStream(songs[currentSong]);
                } else if (gameOverOption == 1) {
                    currentEngineState = MAIN_MENU;
                    ResetGame(player,boss,mahoragaFreezeTimer,screenWidth,screenHeight,currentDifficulty);
                    menuOption = 0;
                } else if (gameOverOption == 2) break;
            }
        }
        else if (currentEngineState == SETTINGS) {
            if (IsKeyPressed(KEY_UP)) selectedSongIndex = (selectedSongIndex - 1 + songCount) % songCount;
            if (IsKeyPressed(KEY_DOWN)) selectedSongIndex = (selectedSongIndex + 1) % songCount;
            if (IsKeyPressed(KEY_R)) repeatSong = !repeatSong;
            if (IsKeyPressed(KEY_ENTER)) {
                currentSong = selectedSongIndex;
                PlayCurrentSong();
                // FIX: only pause preview if we're NOT in active gameplay
                if (previousState != GAMEPLAY) PauseMusicStream(songs[currentSong]);
            }
            Vector2 mouse = GetMousePosition();
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, volumeSlider)) adjustingVolume = true;
            if (adjustingVolume) {
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    float nx = mouse.x;
                    if (nx < volumeSlider.x) nx = volumeSlider.x;
                    if (nx > volumeSlider.x + volumeSlider.width) nx = volumeSlider.x + volumeSlider.width;
                    volumeSliderValue = ((nx - volumeSlider.x) / volumeSlider.width) * 100;
                    SetMusicVolumeLevel(volumeSliderValue / 100.0f);
                } else adjustingVolume = false;
            }
            if (IsKeyPressed(KEY_B)) {
                currentEngineState = previousState;
                if (currentEngineState == GAMEPLAY) ResumeMusicStream(songs[currentSong]);
            }
        }

        // ========== DRAWING ==========
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera);

        bool isMenuState = (currentEngineState == MAIN_MENU ||
                    currentEngineState == SETTINGS ||
                    currentEngineState == DIFFICULTY_MODAL ||
                    currentEngineState == PAUSE ||
                    currentEngineState == GAME_OVER);

        Texture2D currentBg = isMenuState ? menuBg : bgs[selectedBgIndex];

        float scaleX = (float)screenWidth / currentBg.width;
        float scaleY = (float)screenHeight / currentBg.height;
        float scale = fmax(scaleX, scaleY);

        Rectangle bgSource = {0, 0, (float)currentBg.width, (float)currentBg.height};
        Rectangle bgDest = {
            (float)screenWidth / 2.0f,
            (float)screenHeight / 2.0f,
            currentBg.width * scale,
            currentBg.height * scale
        };
        DrawTexturePro(currentBg, bgSource, bgDest, {bgDest.width / 2.0f, bgDest.height / 2.0f}, 0.0f, WHITE);

        if (currentEngineState != MAIN_MENU && currentEngineState != SETTINGS && currentEngineState != DIFFICULTY_MODAL) {
            if (sukunaDomain.IsPlaying()) sukunaDomain.Draw(screenWidth, screenHeight);
            else if (gojoDomain.IsPlaying()) gojoDomain.Draw(screenWidth, screenHeight);
            else if (gojoHeal.IsPlaying()) gojoHeal.Draw(screenWidth, screenHeight);
            else {
                const float TARGET_HEIGHT = 280.0f;
                // Sukuna
                if (boss.IsAlive()) {
                    Texture2D tex = sukunaStill;
                    switch(boss.state) {
                        case WALK_LEFT: tex = ((int)(GetTime()*4)%2==0)?sukunaWalkLeft:sukunaStill; break;
                        case WALK_RIGHT: tex = ((int)(GetTime()*4)%2==0)?sukunaWalkRight:sukunaStill; break;
                        case PUNCH: tex = sukunaPunch; break;
                        case KICK: tex = sukunaKick; break;
                        default: break;
                    }
                    float sc = TARGET_HEIGHT/tex.height;
                    if (tex.id == sukunaWalkRight.id) sc *= 1.15f;
                    if (tex.id == sukunaPunch.id) sc *= 1.20f;
                    if (tex.id == sukunaKick.id) sc *= 1.25f;
                    float dir = boss.facingLeft ? -1.0f : 1.0f;
                    float dx = boss.x;
                    if (tex.id == sukunaWalkRight.id) dx += (boss.facingLeft ? 12.0f : -12.0f);
                    Rectangle src = {0,0,(float)tex.width*dir,(float)tex.height};
                    Rectangle dst = {dx,boss.y,(float)tex.width*sc,(float)tex.height*sc};
                    Vector2 org = {dst.width/2,dst.height};
                    DrawTexturePro(tex, src, dst, org, 0.0f, WHITE);
                }
                // Gojo
                if (player.IsAlive()) {
                    Texture2D tex = gojoStill;
                    if (!player.infinityActive) {
                        switch(player.state) {
                            case WALK_LEFT: tex = ((int)(GetTime()*4)%2==0)?gojoWalkLeft:gojoStill; break;
                            case WALK_RIGHT: tex = ((int)(GetTime()*4)%2==0)?gojoWalkRight:gojoStill; break;
                            case PUNCH: tex = gojoPunch; break;
                            case KICK: tex = gojoKick; break;
                            default: break;
                        }
                    }
                    float sc = TARGET_HEIGHT/tex.height;
                    float dir = player.facingLeft ? -1.0f : 1.0f;
                    Rectangle src = {0,0,(float)tex.width*dir,(float)tex.height};
                    Rectangle dst = {player.x,player.y,(float)tex.width*sc,(float)tex.height*sc};
                    Vector2 org = {dst.width/2,dst.height};
                    DrawTexturePro(tex, src, dst, org, 0.0f, WHITE);
                }
                if (mahoragaFreezeTimer > 0.0f) {
                    DrawRectangle(-50,-50,screenWidth+100,screenHeight+100,Fade(BLACK,((6.0f-mahoragaFreezeTimer)/6.0f)*0.85f));
                    DrawText("MAHORAGA SUMMONED", screenWidth/2-MeasureText("MAHORAGA SUMMONED",30)/2,40,30,PURPLE);
                    float mScale = (float)screenHeight/mahoragaTex.height;
                    float mX = screenWidth/2 - (mahoragaTex.width*mScale)/2;
                    DrawTextureEx(mahoragaTex,{mX,0},0,mScale,WHITE);
                }
            }
        }
        EndMode2D();

        // UI / HUD
        if (currentEngineState != MAIN_MENU && currentEngineState != SETTINGS && currentEngineState != DIFFICULTY_MODAL && !sukunaDomain.IsPlaying() && !gojoDomain.IsPlaying() && !gojoHeal.IsPlaying()) {
            float gw = ((float)player.health/player.maxHealth)*400, sw = ((float)boss.health/boss.maxHealth)*400;
            if(gw<0)gw=0; if(sw<0)sw=0;
            DrawRectangle(36,36,408,38,BLACK); DrawRectangle(40,40,400,30,DARKGRAY); DrawRectangle(40,40,(int)gw,30,SKYBLUE); DrawText("GOJO",45,45,20,WHITE);
            DrawRectangle(screenWidth-444,36,408,38,BLACK); DrawRectangle(screenWidth-440,40,400,30,DARKGRAY); DrawRectangle(screenWidth-440,40,(int)sw,30,MAROON); DrawText("SUKUNA",screenWidth-140,45,20,WHITE);
            if (player.IsAlive() && boss.IsAlive()) {
                if (player.domainCooldown>0) DrawText(TextFormat("Gojo CD: %.1fs",player.domainCooldown),40,80,20,RED);
                else DrawText("Gojo Domain Ready [Q]",40,80,20,GREEN);
                if (isTwoPlayerMode) {
                    int x = screenWidth-440, y=85;
                    if (boss.domainCooldown>0) DrawText(TextFormat("Sukuna CD: %.1fs",boss.domainCooldown),x,y,20,RED);
                    else DrawText("Sukuna Ready [O/I]",x,y,20,GREEN);
                }
                if (player.comboCount>0) DrawText(TextFormat("COMBO %d",player.comboCount),screenWidth/2-50,50,30,ORANGE);
                if (currentDifficulty == HARD && player.infinityActive)
                    DrawText("INFINITY ACTIVE", screenWidth/2-80, 100, 30, SKYBLUE);
                if (boss.mahoragaSummoned && mahoragaFreezeTimer<=0 && (int)(GetTime()*5)%2==0)
                    DrawText("SUKUNA GOT A MASSIVE BUFF!", screenWidth/2-MeasureText("SUKUNA GOT A MASSIVE BUFF!",40)/2,100,40,RED);
            }
        }

        // Menus drawing
        if (currentEngineState == MAIN_MENU) {
            DrawText("JUJUTSU ARENA", screenWidth/2-MeasureText("JUJUTSU ARENA",60)/2, 120, 60, RED);
            DrawText("Choose your battle", screenWidth/2-MeasureText("Choose your battle",30)/2, 200, 30, LIGHTGRAY);
            const char* opts[] = {"1. Single Player (vs AI)", "2. Two Player (PvP)", "3. Settings"};
            int y = 300;
            for (int i=0;i<3;i++) {
                Color c = (i==menuOption)?YELLOW:WHITE;
                DrawText(opts[i], screenWidth/2-MeasureText(opts[i],28)/2, y, 28, c);
                y += 55;
            }
            DrawText("Controls:", 50, screenHeight-140, 20, SKYBLUE);
            DrawText("P1: A/D | Y/G | Q", 50, screenHeight-110, 18, WHITE);
            DrawText("P2: Arrows | P/K | O/I", 50, screenHeight-80, 18, WHITE);
            DrawText("M: Mute Music", screenWidth-200, screenHeight-50, 18, LIGHTGRAY);
        }

        if (currentEngineState == DIFFICULTY_MODAL) {
            DrawRectangle(0,0,screenWidth,screenHeight,Fade(BLACK,0.85f));
            DrawText("LOBBY SETTINGS", screenWidth/2-MeasureText("LOBBY SETTINGS",35)/2, 60, 35, YELLOW);

            const char* diffText[] = {"EASY", "MID", "HARD"};
            Color diffColor = (modalOption == 0) ? YELLOW : WHITE;
            DrawText("Difficulty: ", screenWidth/2 - 200, 130, 30, diffColor);
            DrawText(diffText[modalDifficultyIndex], screenWidth/2 + 80, 130, 30, (modalOption == 0) ? SKYBLUE : WHITE);
            DrawText("<  >", screenWidth/2 + 200, 130, 30, (modalOption == 0) ? LIGHTGRAY : DARKGRAY);

            Color musicColor = (modalOption == 1) ? YELLOW : WHITE;
            DrawText("Music: ", screenWidth/2 - 200, 190, 30, musicColor);
            DrawText(songFiles[modalMusicIndex], screenWidth/2 + 80, 190, 30, (modalOption == 1) ? SKYBLUE : WHITE);
            DrawText("<  >", screenWidth/2 + 200, 190, 30, (modalOption == 1) ? LIGHTGRAY : DARKGRAY);

            Color bgColor = (modalOption == 2) ? YELLOW : WHITE;
            DrawText("Map: ", screenWidth/2 - 200, 250, 30, bgColor);
            string mapName = "Background " + to_string(modalBgIndex + 1);
            DrawText(mapName.c_str(), screenWidth/2 + 80, 250, 30, (modalOption == 2) ? SKYBLUE : WHITE);
            DrawText("<  >", screenWidth/2 + 260, 250, 30, (modalOption == 2) ? LIGHTGRAY : DARKGRAY);

            Rectangle previewDest = { screenWidth / 2.0f - 160.0f, 310.0f, 320.0f, 180.0f };
            Texture2D previewTex = bgs[modalBgIndex];
            Rectangle previewSrc = {0, 0, (float)previewTex.width, (float)previewTex.height};
            DrawTexturePro(previewTex, previewSrc, previewDest, {0,0}, 0.0f, WHITE);
            DrawRectangleLinesEx(previewDest, 3.0f, (modalOption == 2) ? SKYBLUE : DARKGRAY);

            DrawText("START GAME", screenWidth/2 - MeasureText("START GAME", 30)/2, 530, 30, (modalOption == 3) ? GREEN : WHITE);
            DrawText("Use LEFT/RIGHT to change, ENTER to start, B to go back", screenWidth/2 - 270, 620, 20, LIGHTGRAY);
        }

        if (currentEngineState == PAUSE) {
            DrawRectangle(0,0,screenWidth,screenHeight,Fade(BLACK,0.7f));
            DrawText("GAME PAUSED", screenWidth/2-MeasureText("GAME PAUSED",50)/2, 150, 50, WHITE);
            const char* opts[] = {"Resume", "Settings", "Main Menu", "Exit"};
            for (int i=0;i<4;i++) {
                Color c = (i==pauseOption)?YELLOW:WHITE;
                DrawText(opts[i], screenWidth/2-MeasureText(opts[i],32)/2, 300+i*55, 32, c);
            }
            DrawText("Press ENTER to select", screenWidth/2-MeasureText("Press ENTER to select",20)/2, 550, 20, LIGHTGRAY);
        }

        if (currentEngineState == GAME_OVER) {
            DrawRectangle(0,0,screenWidth,screenHeight,Fade(BLACK,0.8f));
            const char* winner = player.IsAlive() ? "GOJO WINS!" : "SUKUNA WINS!";
            Color winColor = player.IsAlive() ? GREEN : RED;
            DrawText(winner, screenWidth/2-MeasureText(winner,60)/2, 150, 60, winColor);
            const char* opts[] = {"Retry", "Main Menu", "Exit"};
            for (int i=0;i<3;i++) {
                Color c = (i==gameOverOption)?YELLOW:WHITE;
                DrawText(opts[i], screenWidth/2-MeasureText(opts[i],32)/2, 320+i*55, 32, c);
            }
        }

        if (currentEngineState == SETTINGS) {
            DrawRectangle(0,0,screenWidth,screenHeight,Fade(BLACK,0.85f));
            DrawText("SETTINGS", screenWidth/2-MeasureText("SETTINGS",50)/2, 60, 50, YELLOW);
            DrawText("MUSIC", screenWidth/2-MeasureText("MUSIC",30)/2, 130, 30, SKYBLUE);
            DrawText("Select Song (UP/DOWN, ENTER to play)", screenWidth/2-MeasureText("Select Song (UP/DOWN, ENTER to play)",24)/2, 180, 24, WHITE);
            for (int i=0;i<songCount;i++) {
                Color col = (i==selectedSongIndex)?YELLOW:LIGHTGRAY;
                string prefix = (i==currentSong)?"> ":"  ";
                string fullText = prefix + songFiles[i];
                DrawText(fullText.c_str(), screenWidth/2-MeasureText(fullText.c_str(), 25)/2, 230+i*35, 25, col);
            }
            DrawText("Volume:", screenWidth/2-100, 420, 25, WHITE);
            DrawText(TextFormat("%.0f%%",volumeSliderValue), screenWidth/2+120, 420, 25, WHITE);
            DrawRectangleRec(volumeSlider, LIGHTGRAY);
            DrawRectangle(volumeSlider.x, volumeSlider.y, volumeSlider.width*(volumeSliderValue/100.0f), volumeSlider.height, BLUE);
            DrawText("Click & drag slider", screenWidth/2-MeasureText("Click & drag slider",18)/2, 510, 18, LIGHTGRAY);
            DrawText("Repeat: ", screenWidth/2-50, 550, 25, WHITE);
            DrawText(repeatSong?"ON":"OFF", screenWidth/2+50, 550, 25, repeatSong?GREEN:RED);
            DrawText("R: Toggle Repeat", screenWidth/2-MeasureText("R: Toggle Repeat",20)/2, 600, 20, LIGHTGRAY);
            DrawText("M: Mute/Unmute", screenWidth/2-MeasureText("M: Mute/Unmute",20)/2, 630, 20, LIGHTGRAY);
            DrawText("B: Back", screenWidth/2-MeasureText("B: Back",20)/2, 660, 20, LIGHTGRAY);
        }
        if (musicMuted) DrawText("[MUTED]", screenWidth-100, 10, 20, RED);
        EndDrawing();
    }

    // Cleanup
    UnloadAllMusic();
    for (int i = 0; i < bgCount; i++) UnloadTexture(bgs[i]);
    UnloadTexture(gojoStill); UnloadTexture(gojoWalkRight); UnloadTexture(gojoWalkLeft); UnloadTexture(menuBg);
    UnloadTexture(gojoPunch); UnloadTexture(gojoKick); UnloadTexture(sukunaStill); UnloadTexture(sukunaWalkRight);
    UnloadTexture(sukunaWalkLeft); UnloadTexture(sukunaPunch); UnloadTexture(sukunaKick); UnloadTexture(mahoragaTex);
    UnloadSound(sukunaDomainSound); UnloadSound(gojoDomainSound); UnloadSound(mahoragaSound);
    UnloadSound(gojoHealSound); UnloadSound(combatHitSound); UnloadSound(teleportSound);
    CloseAudioDevice(); CloseWindow();
    return 0;
}
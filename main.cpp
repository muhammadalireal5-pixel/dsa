// --- INCLUDES & NAMESPACES ---
#include <iostream>
#include "raylib.h"
#include "videoPlayer.h"

using namespace std;

// --- ENUMS & STRUCTS ---
enum AnimState { IDLE, WALK_LEFT, WALK_RIGHT, PUNCH, KICK };

// --- GOJO LOGIC ---
struct Gojo {
    float x, y;
    int health = 200;
    int maxHealth = 200;
    int speed = 4;
    int punchDamage = 10;
    int kickDamage = 15;

    AnimState state = IDLE;
    float actionTimer = 0.0f;
    int comboCount = 0;
    float domainCooldown = 0.0f;
    bool facingLeft = false;
    bool rctUsed = false;

    bool IsAlive() { return health > 0; }

    void Update(int screenWidth, float bossX, bool bossAlive) {
        if (!IsAlive()) return;

        // Domain Cooldown Timer
        if (domainCooldown > 0.0f) {
            domainCooldown -= GetFrameTime();
            if (domainCooldown < 0.0f) domainCooldown = 0.0f;
        }

        // Limitless Teleport
        if (bossAlive && IsKeyPressed(KEY_SPACE)) {
            if (x < bossX) x = bossX + 150.0f;
            else x = bossX - 150.0f;

            if (x < 50) x = 50;
            if (x > screenWidth - 50) x = screenWidth - 50;
        }

        // Attack Animation Lock
        if (state == PUNCH || state == KICK) {
            actionTimer += GetFrameTime();
            if (actionTimer >= 0.5f) {
                state = IDLE;
                actionTimer = 0.0f;
            }
        }
        else {
            // Movement Controls
            state = IDLE;
            float nextX = x;

            if (IsKeyDown(KEY_D)) { nextX += speed; state = WALK_RIGHT; }
            else if (IsKeyDown(KEY_A)) { nextX -= speed; state = WALK_LEFT; }

            // Collision Prevention (Don't walk through Sukuna)
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

    bool domain175Triggered = false;
    bool domain25Triggered = false;
    bool mahoragaSummoned = false;
    int damagePool = 0;

    bool IsAlive() { return health > 0; }

    void TakeDamage(int amount, float targetX, int screenW) {
        health -= amount;
        damagePool += amount;

        // Knockback / Teleport after heavy damage
        if (damagePool >= 25) {
            if (x < targetX) x = targetX + 150.0f;
            else x = targetX - 150.0f;

            if (x < 50) x = screenW - 50;
            if (x > screenW - 50) x = screenW - 50;
            damagePool -= 25;
        }
    }

    void UpdateAI(float targetX, bool targetAlive) {
        if (!IsAlive()) return;

        if (attackCooldown > 0.0f) attackCooldown -= GetFrameTime();

        // Attack Animation Lock
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

            // Attack Decision
            if (abs(distance) < 80.0f && attackCooldown <= 0.0f) {
                state = (GetRandomValue(1, 100) % 2 == 0) ? PUNCH : KICK;
                actionTimer = 0.0f;
            }
            else {
                // Movement Decision
                float nextX = x;
                if (distance > 10.0f) { nextX += speed; state = WALK_RIGHT; }
                else if (distance < -10.0f) { nextX -= speed; state = WALK_LEFT; }
                else { state = IDLE; }

                // Collision Prevention
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

// --- MAIN ENGINE ---
int main() {
    // Window Setup
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Jujutsu Arena");
    InitAudioDevice();

    // Asset Loading: Textures
    Texture2D bg = LoadTexture("bg.jpeg");
    Rectangle bgSource = { 0.0f, 0.0f, (float)bg.width, (float)bg.height };
    Rectangle bgDest = { (float)screenWidth / 2.0f, (float)screenHeight / 2.0f, (float)screenHeight, (float)screenWidth };
    Vector2 bgOrigin = { bgDest.width / 2.0f, bgDest.height / 2.0f };

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

    // Asset Loading: Videos & Audio
    VideoPlayer sukunaDomain("sukuna", "ffout", ".gif", 193, 10.0f);
    Sound sukunaDomainSound = LoadSound("sukunaDomain.wav");

    VideoPlayer gojoDomain("gojoDomain", "ffout", ".gif", 233, 10.0f);
    Sound gojoDomainSound = LoadSound("gojoDomain.wav");

    VideoPlayer gojoHeal("gojoHeal", "ffout", ".gif", 50, 10.0f);
    Sound gojoHealSound = LoadSound("gojoHeal.wav");

    Sound mahoragaSound = LoadSound("mahoraga.wav");

    // Combat Audio
    Sound combatHitSound = LoadSound("punch.wav");

    // Entity Initialization
    const float TARGET_HEIGHT = 280.0f;
    const float GROUND_LEVEL = (float)screenHeight - 30.0f;

    Gojo player;
    player.x = screenWidth * 0.20f;
    player.y = GROUND_LEVEL;

    Sukuna boss;
    boss.x = screenWidth * 0.80f;
    boss.y = GROUND_LEVEL;

    // Sub-Systems
    ScreenEffects vfx;
    Camera2D camera = { 0 };
    camera.target = { 0.0f, 0.0f };
    camera.offset = { 0.0f, 0.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    float mahoragaFreezeTimer = 0.0f;
    SetTargetFPS(60);

    // --- GAMEPLAY LOOP ---
    while (!WindowShouldClose()) {

        // System Updates
        vfx.Update();
        camera.offset = { vfx.offsetX, vfx.offsetY };

        // Face Tracking
        if (player.IsAlive() && boss.IsAlive()) {
            player.facingLeft = (player.x > boss.x);
            boss.facingLeft = (boss.x > player.x);
        }

        // Revival Event (RCT)
        if (player.health <= 0 && !player.rctUsed) {
            player.rctUsed = true;
            player.health = 1;
            gojoHeal.Play();
            PlaySound(gojoHealSound);
        }

        // Core Combat Interactions
        if (!sukunaDomain.IsPlaying() && !gojoDomain.IsPlaying() && !gojoHeal.IsPlaying() && player.IsAlive() && boss.IsAlive()) {

            // Mahoraga Summon Event
            if (boss.health <= 100 && !boss.mahoragaSummoned) {
                boss.mahoragaSummoned = true;
                mahoragaFreezeTimer = 6.0f;
                PlaySound(mahoragaSound);
                vfx.TriggerShake(0.3f, 25.0f);
                boss.punchDamage = 40;
                boss.kickDamage = 40;
                boss.speed = 3;
            }

            // Cinematic Freeze Processing
            if (mahoragaFreezeTimer > 0.0f) {
                mahoragaFreezeTimer -= GetFrameTime();
                if ((int)(mahoragaFreezeTimer * 10.0f) % 15 == 0) {
                    vfx.TriggerShake(0.2f, 20.0f);
                }
            }
            else {
                // Domain Casts
                if (player.domainCooldown == 0.0f && player.IsAlive()) {
                    if (IsKeyPressed(KEY_Q)) {
                        gojoDomain.Play();
                        PlaySound(gojoDomainSound);
                        boss.TakeDamage(25, player.x, screenWidth);
                        player.domainCooldown = 15.0f;

                        if (boss.health <= 175) boss.domain175Triggered = true;
                        if (boss.health <= 25) boss.domain25Triggered = true;
                    }
                }

                if (!gojoDomain.IsPlaying() && boss.IsAlive()) {
                    if (boss.health <= 175 && !boss.domain175Triggered) {
                        sukunaDomain.Play();
                        PlaySound(sukunaDomainSound);
                        player.health -= 25;
                        boss.domain175Triggered = true;
                    }
                    else if (boss.health <= 25 && !boss.domain25Triggered) {
                        sukunaDomain.Play();
                        PlaySound(sukunaDomainSound);
                        player.health -= 25;
                        boss.domain25Triggered = true;
                    }
                }

                // Standard Physics and Attacks
                if (!sukunaDomain.IsPlaying() && !gojoDomain.IsPlaying()) {

                    player.Update(screenWidth, boss.x, boss.IsAlive());

                    AnimState oldBossState = boss.state;
                    boss.UpdateAI(player.x, player.IsAlive());

                    // Sukuna attacks Gojo
                    if (boss.state != oldBossState && (boss.state == PUNCH || boss.state == KICK)) {
                        PlaySound(combatHitSound);
                        if (player.IsAlive() && abs(player.x - boss.x) < 120.0f) {
                            player.health -= (boss.state == PUNCH) ? boss.punchDamage : boss.kickDamage;
                            vfx.TriggerShake(boss.mahoragaSummoned ? 0.4f : 0.2f, boss.mahoragaSummoned ? 15.0f : 4.0f);
                        }
                    }

                    // Gojo attacks Sukuna
                    if (player.IsAlive() && (player.state != PUNCH && player.state != KICK)) {
                        bool punched = IsKeyPressed(KEY_P);
                        bool kicked = IsKeyPressed(KEY_K);

                        if (punched || kicked) {
                            player.state = punched ? PUNCH : KICK;
                            player.actionTimer = 0.0f;

                            PlaySound(combatHitSound);

                            if (boss.IsAlive() && abs(player.x - boss.x) < 120.0f) {
                                int dmg = punched ? player.punchDamage : player.kickDamage;
                                boss.TakeDamage(dmg, player.x, screenWidth);
                                player.comboCount++;

                                if (player.comboCount >= 3) {
                                    vfx.TriggerShake(0.4f, 15.0f);
                                    player.comboCount = 0;
                                    boss.TakeDamage(5, player.x, screenWidth);
                                }
                            }
                        }
                    }
                }
            }
        }
        else {
            // Cutscene Logic & Skipping
            if (IsKeyPressed(KEY_F)) {
                if (sukunaDomain.IsPlaying()) {
                    while (sukunaDomain.IsPlaying()) sukunaDomain.Update(1.0f);
                    StopSound(sukunaDomainSound);
                }
                if (gojoDomain.IsPlaying()) {
                    while (gojoDomain.IsPlaying()) gojoDomain.Update(1.0f);
                    StopSound(gojoDomainSound);
                }
                if (gojoHeal.IsPlaying()) {
                    while (gojoHeal.IsPlaying()) gojoHeal.Update(1.0f);
                    StopSound(gojoHealSound);
                    player.health = 100;
                }
            }

            if (sukunaDomain.IsPlaying()) {
                sukunaDomain.Update(GetFrameTime());
                if (!sukunaDomain.IsPlaying() && IsSoundPlaying(sukunaDomainSound)) StopSound(sukunaDomainSound);
            }
            if (gojoDomain.IsPlaying()) {
                gojoDomain.Update(GetFrameTime());
            }
            if (gojoHeal.IsPlaying()) {
                gojoHeal.Update(GetFrameTime());
                if (!gojoHeal.IsPlaying()) {
                    player.health = 50;
                }
            }
        }

        // --- RENDERING / DRAWING ---
        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode2D(camera);

        // Background
        DrawTexturePro(bg, bgSource, bgDest, bgOrigin, 270.0f, WHITE);

        // Videos overlap everything if active
        if (sukunaDomain.IsPlaying()) sukunaDomain.Draw(screenWidth, screenHeight);
        else if (gojoDomain.IsPlaying()) gojoDomain.Draw(screenWidth, screenHeight);
        else if (gojoHeal.IsPlaying()) gojoHeal.Draw(screenWidth, screenHeight);
        else {

            // Draw Sukuna
            if (boss.IsAlive()) {
                Texture2D currentBossTex = sukunaStill;
                switch(boss.state) {
                    // Alternates between the Walk and Still image 4 times a second
                    case WALK_LEFT:  currentBossTex = ((int)(GetTime() * 4.0f) % 2 == 0) ? sukunaWalkLeft : sukunaWalkRight; break;
                    case WALK_RIGHT: currentBossTex = ((int)(GetTime() * 4.0f) % 2 == 0) ? sukunaWalkRight : sukunaWalkLeft; break;
                    case PUNCH:      currentBossTex = sukunaPunch; break;
                    case KICK:       currentBossTex = sukunaKick; break;
                    case IDLE:       currentBossTex = sukunaStill; break;
                }

                // 1. Calculate the base scale locked to his idle pose
                float currentBossScale = TARGET_HEIGHT / (float)sukunaStill.height;

                // 2. Only scale up the specifically smaller images
                if (currentBossTex.id == sukunaWalkRight.id) {
                    currentBossScale *= 1.10f; // Scales up ~10% to match WalkLeft's native height
                }
                if (currentBossTex.id == sukunaPunch.id) {
                    currentBossScale *= 1.20f; // Keep your custom punch scale here
                }

                float direction = boss.facingLeft ? -1.0f : 1.0f;
                float drawX = boss.x;

                // 3. Nudge the X-axis to fix the off-center drawing
                if (currentBossTex.id == sukunaWalkRight.id || currentBossTex.id == sukunaWalkLeft.id) {
                    // Tweak the 12.0f up or down until his planted foot stops sliding
                    drawX += (boss.facingLeft ? 12.0f : -12.0f);
                }

                Rectangle sourceRec = { 0.0f, 0.0f, (float)currentBossTex.width * direction, (float)currentBossTex.height };
                Rectangle destRec = { drawX, boss.y, (float)currentBossTex.width * currentBossScale, (float)currentBossTex.height * currentBossScale };
                Vector2 origin = { ((float)currentBossTex.width * currentBossScale) / 2.0f, (float)currentBossTex.height * currentBossScale };

                DrawTexturePro(currentBossTex, sourceRec, destRec, origin, 0.0f, WHITE);
            }

            // Draw Gojo
            if (player.IsAlive()) {
                Texture2D currentPlayerTex = gojoStill;
                switch(player.state) {
                    // Alternates between the Walk and Still image 4 times a second
                    case WALK_LEFT:  currentPlayerTex = ((int)(GetTime() * 4.0f) % 2 == 0) ? gojoWalkLeft : gojoStill; break;
                    case WALK_RIGHT: currentPlayerTex = ((int)(GetTime() * 4.0f) % 2 == 0) ? gojoWalkRight : gojoStill; break;
                    case PUNCH:      currentPlayerTex = gojoPunch; break;
                    case KICK:       currentPlayerTex = gojoKick; break;
                    case IDLE:       currentPlayerTex = gojoStill; break;
                }

                float currentPlayerScale = TARGET_HEIGHT / (float)currentPlayerTex.height;
                float direction = player.facingLeft ? -1.0f : 1.0f;
                Rectangle sourceRec = { 0.0f, 0.0f, (float)currentPlayerTex.width * direction, (float)currentPlayerTex.height };
                Rectangle destRec = { player.x, player.y, (float)currentPlayerTex.width * currentPlayerScale, (float)currentPlayerTex.height * currentPlayerScale };
                Vector2 origin = { ((float)currentPlayerTex.width * currentPlayerScale) / 2.0f, (float)currentPlayerTex.height * currentPlayerScale };

                DrawTexturePro(currentPlayerTex, sourceRec, destRec, origin, 0.0f, WHITE);
            }

            // Draw Mahoraga Freeze Overlay
            if (mahoragaFreezeTimer > 0.0f) {
                DrawRectangle(-50, -50, screenWidth + 100, screenHeight + 100, Fade(BLACK, ((6.0f - mahoragaFreezeTimer) / 6.0f) * 0.85f));
                DrawText("MAHORAGA SUMMONED", screenWidth / 2 - MeasureText("MAHORAGA SUMMONED", 30) / 2, 40, 30, PURPLE);

                float mScale = (float)screenHeight / (float)mahoragaTex.height;
                float mDrawX = ((float)screenWidth / 2.0f) - (((float)mahoragaTex.width * mScale) / 2.0f);
                DrawTextureEx(mahoragaTex, { mDrawX, 0.0f }, 0.0f, mScale, WHITE);
            }
        }

        EndMode2D();

        // --- UI / HUD ---
        if (!sukunaDomain.IsPlaying() && !gojoDomain.IsPlaying() && !gojoHeal.IsPlaying()) {

            float gojoHealthWidth = ((float)player.health / (float)player.maxHealth) * 400.0f;
            float sukunaHealthWidth = ((float)boss.health / (float)boss.maxHealth) * 400.0f;

            if (gojoHealthWidth < 0) gojoHealthWidth = 0;
            if (sukunaHealthWidth < 0) sukunaHealthWidth = 0;

            // Gojo health bar
            DrawRectangle(36, 36, 408, 38, BLACK);
            DrawRectangle(40, 40, 400, 30, DARKGRAY);
            DrawRectangle(40, 40, (int)gojoHealthWidth, 30, SKYBLUE);
            DrawText("GOJO", 45, 45, 20, WHITE);

            // Sukuna health bar
            DrawRectangle(screenWidth - 444, 36, 408, 38, BLACK);
            DrawRectangle(screenWidth - 440, 40, 400, 30, DARKGRAY);
            DrawRectangle(screenWidth - 440, 40, (int)sukunaHealthWidth, 30, MAROON);
            DrawText("SUKUNA", screenWidth - 140, 45, 20, WHITE);

            // Cooldowns & Combo Trackers
            if (player.IsAlive() && boss.IsAlive()) {
                if (player.domainCooldown > 0.0f) DrawText(TextFormat("Domain Cooldown: %.1fs", player.domainCooldown), 40, 80, 20, RED);
                else DrawText("Domain Ready! [Q]", 40, 80, 20, GREEN);

                if (player.comboCount > 0) DrawText(TextFormat("COMBO: %d", player.comboCount), screenWidth / 2 - 50, 50, 30, ORANGE);

                if (boss.mahoragaSummoned && mahoragaFreezeTimer <= 0.0f) {
                    if ((int)(GetTime() * 5.0f) % 2 == 0) {
                        const char* warningMsg = "SUKUNA GOT A MASSIVE BUFF!";
                        DrawText(warningMsg, screenWidth / 2 - MeasureText(warningMsg, 40) / 2, 100, 40, RED);
                    }
                }
            }

            // Win / Loss Screens
            if (!player.IsAlive()) {
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.6f));
                DrawText("LOST", screenWidth / 2 - MeasureText("LOST", 80) / 2, screenHeight / 2 - 40, 80, RED);
            }
            else if (!boss.IsAlive()) {
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.6f));
                DrawText("You Win", screenWidth / 2 - MeasureText("You Win", 80) / 2, screenHeight / 2 - 40, 80, GREEN);
            }
        }

        EndDrawing();
    }

    // --- UNLOAD / CLEANUP ---
    UnloadTexture(bg);
    UnloadTexture(gojoStill);
    UnloadTexture(gojoWalkRight);
    UnloadTexture(gojoWalkLeft);
    UnloadTexture(gojoPunch);
    UnloadTexture(gojoKick);
    UnloadTexture(sukunaStill);
    UnloadTexture(sukunaWalkRight);
    UnloadTexture(sukunaWalkLeft);
    UnloadTexture(sukunaPunch);
    UnloadTexture(sukunaKick);
    UnloadTexture(mahoragaTex);
    UnloadSound(sukunaDomainSound);
    UnloadSound(gojoDomainSound);
    UnloadSound(mahoragaSound);
    UnloadSound(gojoHealSound);
    UnloadSound(combatHitSound);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
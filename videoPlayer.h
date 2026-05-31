#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <vector>
#include <string>
#include <sstream> // Required for formatting numbers
#include <iomanip> // Required for setting widths and zero-padding
#include "raylib.h"

class VideoPlayer {
private:
    std::vector<Texture2D> frames;
    int totalFrames;
    int currentFrame;
    float frameTimer;
    float frameDuration;
    bool isActive;

public:
    VideoPlayer(std::string folderPath, std::string prefix, std::string extension, int frameCount, float fps) {
        totalFrames = frameCount;
        currentFrame = 0;
        frameTimer = 0.0f;
        frameDuration = 1.0f / fps;
        isActive = false;

        // Load all sequential padded files into memory
        for (int i = 1; i <= totalFrames; i++) { // Note: Starting at 1 to match ffout001
            std::stringstream ss;
            // This line forces 'i' to be 3 digits wide, filling empty space with '0'
            ss << folderPath << "/" << prefix << std::setw(3) << std::setfill('0') << i << extension;
            std::string path = ss.str();

            frames.push_back(LoadTexture(path.c_str()));
        }
    }

    void Play() {
        isActive = true;
        currentFrame = 0;
        frameTimer = 0.0f;
    }

    void Update(float deltaTime) {
        if (!isActive) return;

        frameTimer += deltaTime;
        if (frameTimer >= frameDuration) {
            frameTimer = 0.0f;
            currentFrame++;

            if (currentFrame >= totalFrames) {
                isActive = false;
                currentFrame = 0;
            }
        }
    }

    void Draw(int screenWidth, int screenHeight) {
        if (!isActive) return;

        Rectangle source = { 0.0f, 0.0f, (float)frames[currentFrame].width, (float)frames[currentFrame].height };
        Rectangle dest = { 0.0f, 0.0f, (float)screenWidth, (float)screenHeight };
        DrawTexturePro(frames[currentFrame], source, dest, {0,0}, 0.0f, WHITE);
    }

    bool IsPlaying() const { return isActive; }

    ~VideoPlayer() {
        for (auto& texture : frames) {
            UnloadTexture(texture);
        }
    }
};

#endif // VIDEOPLAYER_H
#pragma once
#include <cstdio>   // FILE
#include <cstdint>  // uint family
#include <SDL.h>    // SDL_Texture*

typedef struct {
  uint32_t index;
  SDL_Texture* texture;
  uint8_t* pixelData;
} Frame;

typedef struct {
  uint32_t frameCount;
  uint32_t width;
  uint32_t height;
  uint32_t fps;
} VideoHeader;

class Video {
public:
  Video() = default;

  ~Video();

  VideoHeader& header() {
    return m_header;
  }

  FILE* file() {
    return m_fp;
  }

  int getCurrentFrameIDByElapsed(uint64_t elapsed) const;

  bool readVideoFromFile(const char* path);

  bool readFrame(uint32_t frameId, uint8_t* out);

  //static void loadFrameThread(void* arg);

  uint64_t startTime() {
    return m_startTime;
  }

  void startTime(uint64_t time) {
    if (m_startTime == 0) {
      m_startTime = time;
    }
  }

private:
  VideoHeader m_header = { 0 };

  uint64_t* m_framePixelOffsets = { nullptr };

  FILE* m_fp = { nullptr };

  float m_indexUnit = { 0.0f };

  uint64_t m_startTime = { 0 };
};
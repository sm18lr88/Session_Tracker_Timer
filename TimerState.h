// TimerState.h - Timer configuration and state management

#ifndef TIMERSTATE_H
#define TIMERSTATE_H

#include <windows.h>

#include <cstdio>

// Configuration from setup dialog
struct TimerConfig {
  int timePerBlock;  // Time per block in minutes
  int numBlocks;     // Total number of blocks
  int numQuestions;  // Questions per block
  int transparency;  // Window transparency 0-100 (100 = opaque)

  // Computed values
  int timePerBlockSeconds;
  int timePerQuestion;
  int totalTime;

  void ComputeDerivedValues() {
    timePerBlockSeconds = timePerBlock * 60;
    totalTime = timePerBlockSeconds * numBlocks;
    timePerQuestion = (numQuestions > 0) ? timePerBlockSeconds / numQuestions
                                         : timePerBlockSeconds;
  }
};

// Status codes returned from Tick()
enum class TickStatus {
  Continue,          // Normal tick, timer continues
  QuestionAdvanced,  // Moved to next question
  BlockAdvanced,     // Moved to next block
  Completed          // All blocks finished
};

// Current timer state
struct TimerState {
  int currentTime;          // Total remaining time in seconds
  int currentQuestion;      // Current question number (1-based)
  int currentBlock;         // Current block number (1-based)
  int blockTimeElapsed;     // Seconds elapsed in current block
  int questionTimeElapsed;  // Seconds elapsed in current question
  bool paused;              // Timer is paused
  bool stopped;             // Timer is stopped (not running)

  TimerConfig config;

  void Initialize(const TimerConfig& cfg) {
    config = cfg;
    config.ComputeDerivedValues();
    Reset();
  }

  void Reset() {
    currentTime = config.totalTime;
    currentQuestion = 1;
    currentBlock = 1;
    blockTimeElapsed = 0;
    questionTimeElapsed = 0;
    paused = false;
    stopped = false;  // Auto-start since user clicked "Start" in setup
  }

  TickStatus Tick() {
    if (paused || stopped) {
      return TickStatus::Continue;
    }

    currentTime--;
    blockTimeElapsed++;
    questionTimeElapsed++;

    TickStatus status = TickStatus::Continue;

    // Check if question time is up
    if (questionTimeElapsed >= config.timePerQuestion) {
      questionTimeElapsed = 0;
      if (currentQuestion < config.numQuestions) {
        currentQuestion++;
        status = TickStatus::QuestionAdvanced;
      }
    }

    // Check if block time is up
    if (blockTimeElapsed >= config.timePerBlockSeconds) {
      blockTimeElapsed = 0;
      questionTimeElapsed = 0;
      currentQuestion = 1;

      if (currentBlock < config.numBlocks) {
        currentBlock++;
        status = TickStatus::BlockAdvanced;
      } else {
        status = TickStatus::Completed;
      }
    }

    return status;
  }

  void TogglePause() { paused = !paused; }

  void Start() {
    stopped = false;
    paused = false;
  }

  void Stop() { stopped = true; }

  bool IsRunning() const { return !stopped && !paused; }

  // Get formatted time string MM:SS
  static void FormatTime(int seconds, wchar_t* buffer, size_t bufferSize) {
    int mins = seconds / 60;
    int secs = seconds % 60;
    swprintf_s(buffer, bufferSize, L"%02d:%02d", mins, secs);
  }

  // Get question progress (0-100)
  int GetQuestionProgress() const {
    if (config.timePerQuestion == 0) return 0;
    return (questionTimeElapsed * 100) / config.timePerQuestion;
  }

  // Get block progress (0-100)
  int GetBlockProgress() const {
    if (config.timePerBlockSeconds == 0) return 0;
    return (blockTimeElapsed * 100) / config.timePerBlockSeconds;
  }
};

#endif  // TIMERSTATE_H

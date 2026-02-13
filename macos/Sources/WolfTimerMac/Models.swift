import Foundation

struct TimerConfig: Equatable {
    var timePerBlockMinutes: Int
    var numberOfBlocks: Int
    var questionsPerBlock: Int
    var transparencyPercent: Int

    static let `default` = TimerConfig(
        timePerBlockMinutes: 60,
        numberOfBlocks: 2,
        questionsPerBlock: 40,
        transparencyPercent: 75
    )

    var timePerBlockSeconds: Int {
        max(1, timePerBlockMinutes) * 60
    }

    var timePerQuestionSeconds: Int {
        max(1, timePerBlockSeconds / max(1, questionsPerBlock))
    }
}

enum SetupAction {
    case cancel
    case start
    case coverOnly
}

enum TickStatus {
    case `continue`
    case questionAdvanced
    case blockAdvanced
    case completed
}

final class TimerState {
    var currentQuestion = 1
    var currentBlock = 1
    var blockTimeElapsed = 0
    var questionTimeElapsed = 0
    var paused = false
    var stopped = false
    var config: TimerConfig

    init(config: TimerConfig) {
        self.config = config
        reset(autoStart: true)
    }

    func reset(autoStart: Bool) {
        currentQuestion = 1
        currentBlock = 1
        blockTimeElapsed = 0
        questionTimeElapsed = 0
        paused = false
        stopped = !autoStart
    }

    var isRunning: Bool {
        !stopped && !paused
    }

    func start() {
        stopped = false
        paused = false
    }

    func stop() {
        stopped = true
        paused = false
    }

    func togglePause() {
        guard !stopped else { return }
        paused.toggle()
    }

    func tick() -> TickStatus {
        if paused || stopped {
            return .continue
        }

        blockTimeElapsed += 1
        questionTimeElapsed += 1
        var status: TickStatus = .continue

        if questionTimeElapsed >= config.timePerQuestionSeconds {
            questionTimeElapsed = 0
            if currentQuestion < config.questionsPerBlock {
                currentQuestion += 1
                status = .questionAdvanced
            }
        }

        if blockTimeElapsed >= config.timePerBlockSeconds {
            blockTimeElapsed = 0
            questionTimeElapsed = 0
            currentQuestion = 1

            if currentBlock < config.numberOfBlocks {
                currentBlock += 1
                status = .blockAdvanced
            } else {
                status = .completed
            }
        }

        return status
    }

    var questionProgressPercent: Double {
        let denom = Double(max(1, config.timePerQuestionSeconds))
        return min(100.0, max(0.0, Double(questionTimeElapsed) * 100.0 / denom))
    }

    var blockProgressPercent: Double {
        let denom = Double(max(1, config.timePerBlockSeconds))
        return min(100.0, max(0.0, Double(blockTimeElapsed) * 100.0 / denom))
    }

    var blockRemainingSeconds: Int {
        max(0, config.timePerBlockSeconds - blockTimeElapsed)
    }
}

func formatClock(_ seconds: Int) -> String {
    let clamped = max(0, seconds)
    let mins = clamped / 60
    let secs = clamped % 60
    return String(format: "%02d:%02d", mins, secs)
}

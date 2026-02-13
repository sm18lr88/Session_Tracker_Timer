import AppKit

private final class TimerBarRootView: NSView {
    private var dragStartMouse = CGPoint.zero
    private var dragStartFrame = CGRect.zero

    override func mouseDown(with event: NSEvent) {
        guard let window = self.window else { return }
        dragStartMouse = NSEvent.mouseLocation
        dragStartFrame = window.frame
    }

    override func mouseDragged(with event: NSEvent) {
        guard let window = self.window else { return }
        let mouse = NSEvent.mouseLocation
        let dx = mouse.x - dragStartMouse.x
        let dy = mouse.y - dragStartMouse.y
        var frame = dragStartFrame
        frame.origin.x += dx
        frame.origin.y += dy
        frame = clampFrame(frame, to: virtualVisibleBounds(margin: 0))
        window.setFrame(frame, display: true)
    }
}

final class TimerWindowController: NSWindowController {
    var onStartStop: (() -> Void)?
    var onPause: (() -> Void)?
    var onSettings: (() -> Void)?
    var onClose: (() -> Void)?

    private let questionLabel = NSTextField(labelWithString: "Q: 1/40")
    private let questionTimeLabel = NSTextField(labelWithString: "00:00")
    private let questionProgress = NSProgressIndicator()

    private let blockLabel = NSTextField(labelWithString: "Block 1/2")
    private let blockTimeLabel = NSTextField(labelWithString: "00:00")
    private let blockProgress = NSProgressIndicator()

    private let startStopButton = NSButton(title: "Start", target: nil, action: nil)
    private let pauseButton = NSButton(title: "Pause", target: nil, action: nil)
    private let settingsButton = NSButton(title: "⚙︎", target: nil, action: nil)
    private let closeButton = NSButton(title: "X", target: nil, action: nil)

    init() {
        let frame = CGRect(x: 140, y: 110, width: 760, height: 72)
        let window = NSWindow(contentRect: frame, styleMask: [.borderless], backing: .buffered, defer: false)
        window.isOpaque = false
        window.hasShadow = true
        window.backgroundColor = NSColor(calibratedWhite: 0.18, alpha: 0.96)
        window.level = .floating
        window.collectionBehavior = [.canJoinAllSpaces, .fullScreenAuxiliary]
        window.title = "Wolf-Timer"

        super.init(window: window)
        buildUI()
    }

    required init?(coder: NSCoder) {
        return nil
    }

    func apply(state: TimerState, config: TimerConfig) {
        questionLabel.stringValue = "Q: \(state.currentQuestion)/\(config.questionsPerBlock)"
        questionTimeLabel.stringValue = formatClock(state.questionTimeElapsed)
        questionProgress.doubleValue = state.questionProgressPercent

        blockLabel.stringValue = "Block \(state.currentBlock)/\(config.numberOfBlocks)"
        blockTimeLabel.stringValue = formatClock(state.blockRemainingSeconds)
        blockProgress.doubleValue = state.blockProgressPercent

        startStopButton.title = state.stopped ? "Start" : "Stop"
        pauseButton.title = state.paused ? "Resume" : "Pause"
        pauseButton.isEnabled = !state.stopped

        window?.alphaValue = CGFloat(max(20, min(100, config.transparencyPercent))) / 100.0
        if let window = window {
            let clamped = clampFrame(window.frame, to: virtualVisibleBounds(margin: 0))
            window.setFrame(clamped, display: true)
        }
    }

    func setVisible(_ visible: Bool) {
        guard let window = window else { return }
        if visible {
            window.makeKeyAndOrderFront(nil)
            NSApp.activate(ignoringOtherApps: true)
        } else {
            window.orderOut(nil)
        }
    }

    func clampToVisibleBounds() {
        guard let window = window else { return }
        let clamped = clampFrame(window.frame, to: virtualVisibleBounds(margin: 0))
        window.setFrame(clamped, display: true)
    }

    private func buildUI() {
        guard let window = window else { return }

        let root = TimerBarRootView(frame: window.contentView?.bounds ?? .zero)
        root.autoresizingMask = [.width, .height]
        root.wantsLayer = true
        root.layer?.cornerRadius = 8
        root.layer?.backgroundColor = NSColor(calibratedWhite: 0.18, alpha: 0.96).cgColor
        window.contentView = root

        let container = NSStackView()
        container.orientation = .vertical
        container.spacing = 5
        container.translatesAutoresizingMaskIntoConstraints = false
        root.addSubview(container)

        let topRow = NSStackView()
        topRow.orientation = .horizontal
        topRow.alignment = .centerY
        topRow.spacing = 8

        let bottomRow = NSStackView()
        bottomRow.orientation = .horizontal
        bottomRow.alignment = .centerY
        bottomRow.spacing = 8

        for label in [questionLabel, questionTimeLabel, blockLabel, blockTimeLabel] {
            label.font = NSFont.monospacedDigitSystemFont(ofSize: 16, weight: .medium)
            label.textColor = NSColor(calibratedWhite: 0.93, alpha: 1.0)
        }

        questionLabel.alignment = .left
        blockLabel.alignment = .left
        questionTimeLabel.alignment = .center
        blockTimeLabel.alignment = .center

        configureProgress(questionProgress, color: NSColor.systemGreen)
        configureProgress(blockProgress, color: NSColor.systemBlue)

        configureButton(startStopButton, action: #selector(onStartStopTapped))
        configureButton(pauseButton, action: #selector(onPauseTapped))
        configureButton(settingsButton, action: #selector(onSettingsTapped))
        configureButton(closeButton, action: #selector(onCloseTapped))

        topRow.addArrangedSubview(questionLabel)
        topRow.addArrangedSubview(questionTimeLabel)
        topRow.addArrangedSubview(questionProgress)
        topRow.addArrangedSubview(closeButton)
        topRow.addArrangedSubview(settingsButton)

        bottomRow.addArrangedSubview(blockLabel)
        bottomRow.addArrangedSubview(blockTimeLabel)
        bottomRow.addArrangedSubview(blockProgress)
        bottomRow.addArrangedSubview(startStopButton)
        bottomRow.addArrangedSubview(pauseButton)

        container.addArrangedSubview(topRow)
        container.addArrangedSubview(bottomRow)

        let progressMinWidth: CGFloat = 180
        questionProgress.setContentHuggingPriority(.defaultLow, for: .horizontal)
        blockProgress.setContentHuggingPriority(.defaultLow, for: .horizontal)
        questionProgress.widthAnchor.constraint(greaterThanOrEqualToConstant: progressMinWidth).isActive = true
        blockProgress.widthAnchor.constraint(greaterThanOrEqualToConstant: progressMinWidth).isActive = true
        questionProgress.heightAnchor.constraint(equalToConstant: 12).isActive = true
        blockProgress.heightAnchor.constraint(equalToConstant: 12).isActive = true

        questionLabel.widthAnchor.constraint(equalToConstant: 110).isActive = true
        blockLabel.widthAnchor.constraint(equalToConstant: 110).isActive = true
        questionTimeLabel.widthAnchor.constraint(equalToConstant: 64).isActive = true
        blockTimeLabel.widthAnchor.constraint(equalToConstant: 64).isActive = true
        settingsButton.widthAnchor.constraint(equalToConstant: 34).isActive = true
        closeButton.widthAnchor.constraint(equalToConstant: 34).isActive = true
        startStopButton.widthAnchor.constraint(equalToConstant: 74).isActive = true
        pauseButton.widthAnchor.constraint(equalToConstant: 74).isActive = true

        NSLayoutConstraint.activate([
            container.leadingAnchor.constraint(equalTo: root.leadingAnchor, constant: 8),
            container.trailingAnchor.constraint(equalTo: root.trailingAnchor, constant: -8),
            container.topAnchor.constraint(equalTo: root.topAnchor, constant: 8),
            container.bottomAnchor.constraint(equalTo: root.bottomAnchor, constant: -8),
        ])
    }

    private func configureProgress(_ progress: NSProgressIndicator, color: NSColor) {
        progress.isIndeterminate = false
        progress.minValue = 0
        progress.maxValue = 100
        progress.doubleValue = 0
        progress.controlSize = .small
        progress.style = .bar
        progress.wantsLayer = true
        progress.contentTintColor = color
    }

    private func configureButton(_ button: NSButton, action: Selector) {
        button.target = self
        button.action = action
        button.bezelStyle = .texturedRounded
        button.font = NSFont.systemFont(ofSize: 13, weight: .semibold)
    }

    @objc
    private func onStartStopTapped() {
        onStartStop?()
    }

    @objc
    private func onPauseTapped() {
        onPause?()
    }

    @objc
    private func onSettingsTapped() {
        onSettings?()
    }

    @objc
    private func onCloseTapped() {
        onClose?()
    }
}

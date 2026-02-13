import AppKit

final class SetupWindowController: NSWindowController, NSWindowDelegate {
    private enum Layout {
        static let width: CGFloat = 460
        static let height: CGFloat = 340
    }

    private let modeIsSettings: Bool
    private var action: SetupAction = .cancel
    private var outputConfig: TimerConfig?

    private let questionsField = SetupWindowController.makeNumberField()
    private let timePerBlockField = SetupWindowController.makeNumberField()
    private let blockCountField = SetupWindowController.makeNumberField()
    private let transparencySlider = NSSlider(value: 75, minValue: 20, maxValue: 100, target: nil, action: nil)
    private let transparencyValueLabel = NSTextField(labelWithString: "75%")

    init(config: TimerConfig, modeIsSettings: Bool) {
        self.modeIsSettings = modeIsSettings

        let rect = NSRect(x: 0, y: 0, width: Layout.width, height: Layout.height)
        let style: NSWindow.StyleMask = [.titled, .closable]
        let window = NSWindow(contentRect: rect, styleMask: style, backing: .buffered, defer: false)
        window.title = modeIsSettings ? "Wolf-Timer Settings" : "Wolf-Timer Setup"
        window.isReleasedWhenClosed = false
        window.appearance = NSAppearance(named: .darkAqua)
        window.center()

        super.init(window: window)
        window.delegate = self

        buildUI(config: config)
    }

    required init?(coder: NSCoder) {
        return nil
    }

    static func runModal(initialConfig: TimerConfig, modeIsSettings: Bool) -> (SetupAction, TimerConfig?) {
        let controller = SetupWindowController(config: initialConfig, modeIsSettings: modeIsSettings)
        guard let window = controller.window else {
            return (.cancel, nil)
        }

        NSApp.activate(ignoringOtherApps: true)
        window.makeKeyAndOrderFront(nil)
        _ = NSApp.runModal(for: window)
        window.orderOut(nil)
        return (controller.action, controller.outputConfig)
    }

    func windowWillClose(_ notification: Notification) {
        NSApp.stopModal()
    }

    private func buildUI(config: TimerConfig) {
        guard let window = window else { return }

        let root = NSVisualEffectView(frame: window.contentView?.bounds ?? .zero)
        root.autoresizingMask = [.width, .height]
        root.material = .sidebar
        root.state = .active
        root.blendingMode = .behindWindow
        window.contentView = root

        let container = NSStackView()
        container.orientation = .vertical
        container.spacing = 12
        container.translatesAutoresizingMaskIntoConstraints = false
        root.addSubview(container)

        container.addArrangedSubview(makeRow(label: "Questions per block:", field: questionsField))
        container.addArrangedSubview(makeRow(label: "Time per block (min):", field: timePerBlockField))
        container.addArrangedSubview(makeRow(label: "Number of blocks:", field: blockCountField))
        container.addArrangedSubview(makeTransparencyRow())

        let buttonRow = NSStackView()
        buttonRow.orientation = .horizontal
        buttonRow.spacing = 10
        buttonRow.distribution = .fillEqually

        let startButton = NSButton(title: modeIsSettings ? "Apply" : "Start", target: self, action: #selector(onStartOrApply))
        startButton.bezelStyle = .rounded

        let coverOnlyButton = NSButton(title: "Cover only", target: self, action: #selector(onCoverOnly))
        coverOnlyButton.bezelStyle = .rounded

        let cancelButton = NSButton(title: "Cancel", target: self, action: #selector(onCancel))
        cancelButton.bezelStyle = .rounded

        buttonRow.addArrangedSubview(startButton)
        buttonRow.addArrangedSubview(coverOnlyButton)
        buttonRow.addArrangedSubview(cancelButton)
        container.addArrangedSubview(buttonRow)

        NSLayoutConstraint.activate([
            container.leadingAnchor.constraint(equalTo: root.leadingAnchor, constant: 20),
            container.trailingAnchor.constraint(equalTo: root.trailingAnchor, constant: -20),
            container.topAnchor.constraint(equalTo: root.topAnchor, constant: 22),
            container.bottomAnchor.constraint(lessThanOrEqualTo: root.bottomAnchor, constant: -20),

            questionsField.widthAnchor.constraint(equalToConstant: 110),
            timePerBlockField.widthAnchor.constraint(equalToConstant: 110),
            blockCountField.widthAnchor.constraint(equalToConstant: 110),
        ])

        questionsField.integerValue = config.questionsPerBlock
        timePerBlockField.integerValue = config.timePerBlockMinutes
        blockCountField.integerValue = config.numberOfBlocks
        transparencySlider.integerValue = config.transparencyPercent
        transparencyValueLabel.stringValue = "\(config.transparencyPercent)%"
    }

    private func makeRow(label: String, field: NSTextField) -> NSView {
        let row = NSStackView()
        row.orientation = .horizontal
        row.spacing = 14
        row.alignment = .centerY

        let labelView = NSTextField(labelWithString: label)
        labelView.font = NSFont.systemFont(ofSize: 14, weight: .medium)
        labelView.textColor = NSColor(calibratedWhite: 0.92, alpha: 1.0)

        row.addArrangedSubview(labelView)
        row.addArrangedSubview(NSView())
        row.addArrangedSubview(field)
        return row
    }

    private func makeTransparencyRow() -> NSView {
        transparencySlider.target = self
        transparencySlider.action = #selector(onTransparencyChanged)

        let row = NSStackView()
        row.orientation = .horizontal
        row.spacing = 14
        row.alignment = .centerY

        let labelView = NSTextField(labelWithString: "Transparency:")
        labelView.font = NSFont.systemFont(ofSize: 14, weight: .medium)
        labelView.textColor = NSColor(calibratedWhite: 0.92, alpha: 1.0)

        transparencyValueLabel.font = NSFont.monospacedDigitSystemFont(ofSize: 14, weight: .semibold)
        transparencyValueLabel.textColor = NSColor(calibratedWhite: 0.94, alpha: 1.0)
        transparencyValueLabel.alignment = .right

        row.addArrangedSubview(labelView)
        row.addArrangedSubview(transparencySlider)
        row.addArrangedSubview(transparencyValueLabel)
        return row
    }

    @objc
    private func onTransparencyChanged() {
        transparencyValueLabel.stringValue = "\(transparencySlider.integerValue)%"
    }

    @objc
    private func onStartOrApply() {
        finish(with: .start)
    }

    @objc
    private func onCoverOnly() {
        finish(with: .coverOnly)
    }

    @objc
    private func onCancel() {
        action = .cancel
        outputConfig = nil
        closeAndStopModal()
    }

    private func finish(with nextAction: SetupAction) {
        let parsedConfig = readConfig()
        guard parsedConfig != nil else { return }
        action = nextAction
        outputConfig = parsedConfig
        closeAndStopModal()
    }

    private func closeAndStopModal() {
        NSApp.stopModal()
        window?.close()
    }

    private func readConfig() -> TimerConfig? {
        let questions = questionsField.integerValue
        let minutes = timePerBlockField.integerValue
        let blocks = blockCountField.integerValue
        let transparency = transparencySlider.integerValue

        if questions <= 0 {
            showValidationError("Please enter a valid number of questions (> 0).")
            return nil
        }
        if minutes <= 0 {
            showValidationError("Please enter a valid time per block (> 0).")
            return nil
        }
        if blocks <= 0 {
            showValidationError("Please enter a valid number of blocks (> 0).")
            return nil
        }

        return TimerConfig(
            timePerBlockMinutes: minutes,
            numberOfBlocks: blocks,
            questionsPerBlock: questions,
            transparencyPercent: max(20, min(100, transparency))
        )
    }

    private func showValidationError(_ message: String) {
        let alert = NSAlert()
        alert.alertStyle = .warning
        alert.messageText = "Invalid Input"
        alert.informativeText = message
        alert.runModal()
    }

    private static func makeNumberField() -> NSTextField {
        let field = NSTextField(string: "")
        field.font = NSFont.monospacedDigitSystemFont(ofSize: 14, weight: .regular)
        field.alignment = .left
        field.placeholderString = "0"
        return field
    }
}

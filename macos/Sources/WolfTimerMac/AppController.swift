import AppKit

final class AppController: NSObject, NSApplicationDelegate {
    private var config = TimerConfig.default
    private var state = TimerState(config: .default)

    private var timerWindowController: TimerWindowController!
    private var coverWindowController: CoverWindowController!

    private var tickTimer: Timer?
    private var localKeyMonitor: Any?
    private var globalKeyMonitor: Any?
    private var screenChangeObserver: NSObjectProtocol?

    func applicationDidFinishLaunching(_ notification: Notification) {
        configureMainMenu()

        timerWindowController = TimerWindowController()
        coverWindowController = CoverWindowController()
        wireCallbacks()
        installCoverToggleHotkey()
        installScreenChangeObserver()
        startTickLoop()

        let (action, setupConfig) = SetupWindowController.runModal(
            initialConfig: config,
            modeIsSettings: false
        )

        guard action != .cancel, let setupConfig else {
            NSApp.terminate(nil)
            return
        }

        config = setupConfig
        state = TimerState(config: config)

        switch action {
        case .start:
            state.start()
            enterTimerMode()
        case .coverOnly:
            state.stop()
            enterCoverOnlyMode()
        case .cancel:
            NSApp.terminate(nil)
            return
        }

        updateTimerWindow()
    }

    func applicationWillTerminate(_ notification: Notification) {
        if let monitor = localKeyMonitor {
            NSEvent.removeMonitor(monitor)
        }
        if let monitor = globalKeyMonitor {
            NSEvent.removeMonitor(monitor)
        }
        if let observer = screenChangeObserver {
            NotificationCenter.default.removeObserver(observer)
        }
        tickTimer?.invalidate()
    }

    private func configureMainMenu() {
        let mainMenu = NSMenu()
        let appMenuItem = NSMenuItem()
        mainMenu.addItem(appMenuItem)
        NSApp.mainMenu = mainMenu

        let appMenu = NSMenu()
        let quitTitle = "Quit Wolf-Timer"
        appMenu.addItem(withTitle: quitTitle,
                        action: #selector(NSApplication.terminate(_:)),
                        keyEquivalent: "q")
        appMenuItem.submenu = appMenu
    }

    private func wireCallbacks() {
        timerWindowController.onStartStop = { [weak self] in
            guard let self else { return }
            if self.state.stopped {
                self.state.start()
            } else {
                self.state.stop()
            }
            self.updateTimerWindow()
        }

        timerWindowController.onPause = { [weak self] in
            self?.state.togglePause()
            self?.updateTimerWindow()
        }

        timerWindowController.onSettings = { [weak self] in
            self?.openSettings(fromCoverMenu: false)
        }

        timerWindowController.onClose = { [weak self] in
            self?.terminateApp()
        }

        coverWindowController.onOpenSettings = { [weak self] in
            self?.openSettings(fromCoverMenu: true)
        }

        coverWindowController.onCloseApp = { [weak self] in
            self?.terminateApp()
        }
    }

    private func installCoverToggleHotkey() {
        localKeyMonitor = NSEvent.addLocalMonitorForEvents(matching: .keyDown) { [weak self] event in
            guard let self else { return event }
            if self.isCoverToggle(event) {
                self.coverWindowController.toggleVisible()
                return nil
            }
            return event
        }

        globalKeyMonitor = NSEvent.addGlobalMonitorForEvents(matching: .keyDown) { [weak self] event in
            guard let self else { return }
            if self.isCoverToggle(event) {
                self.coverWindowController.toggleVisible()
            }
        }
    }

    private func isCoverToggle(_ event: NSEvent) -> Bool {
        if event.keyCode != 49 { // Space
            return false
        }
        let flags = event.modifierFlags.intersection(.deviceIndependentFlagsMask)
        return flags == .shift
    }

    private func startTickLoop() {
        tickTimer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
            self?.handleTick()
        }
        if let tickTimer {
            RunLoop.main.add(tickTimer, forMode: .common)
        }
    }

    private func installScreenChangeObserver() {
        screenChangeObserver = NotificationCenter.default.addObserver(
            forName: NSApplication.didChangeScreenParametersNotification,
            object: nil,
            queue: .main
        ) { [weak self] _ in
            self?.clampAllWindowsToVisibleBounds()
        }
    }

    private func handleTick() {
        let status = state.tick()
        updateTimerWindow()

        if status == .completed {
            let alert = NSAlert()
            alert.alertStyle = .informational
            alert.messageText = "All blocks completed!"
            alert.runModal()
            terminateApp()
        }
    }

    private func updateTimerWindow() {
        timerWindowController.apply(state: state, config: config)
    }

    private func enterTimerMode() {
        timerWindowController.setVisible(true)
        coverWindowController.show()
        clampAllWindowsToVisibleBounds()
    }

    private func enterCoverOnlyMode() {
        state.stop()
        timerWindowController.setVisible(false)
        coverWindowController.show()
        clampAllWindowsToVisibleBounds()
    }

    private func openSettings(fromCoverMenu: Bool) {
        let wasStopped = state.stopped
        let wasPaused = state.paused
        let wasRunning = state.isRunning

        if wasRunning {
            state.togglePause()
            updateTimerWindow()
        }

        let (action, nextConfig) = SetupWindowController.runModal(
            initialConfig: config,
            modeIsSettings: true
        )

        guard action != .cancel, let nextConfig else {
            if wasRunning {
                state.paused = false
                updateTimerWindow()
            }
            if fromCoverMenu && timerWindowController.window?.isVisible == false {
                coverWindowController.show()
            }
            return
        }

        let timingChanged = (
            config.numberOfBlocks != nextConfig.numberOfBlocks ||
            config.timePerBlockMinutes != nextConfig.timePerBlockMinutes ||
            config.questionsPerBlock != nextConfig.questionsPerBlock
        )

        config = nextConfig

        if timingChanged {
            state = TimerState(config: nextConfig)
            if wasStopped {
                state.stop()
            } else if wasPaused {
                state.paused = true
            } else {
                state.start()
            }
        } else {
            state.config = nextConfig
            if wasRunning {
                state.paused = false
            }
        }

        if action == .coverOnly {
            enterCoverOnlyMode()
        } else {
            enterTimerMode()
        }
        updateTimerWindow()
        clampAllWindowsToVisibleBounds()
    }

    private func clampAllWindowsToVisibleBounds() {
        timerWindowController.clampToVisibleBounds()
        coverWindowController.clampToVisibleBounds()
    }

    private func terminateApp() {
        NSApp.terminate(nil)
    }
}

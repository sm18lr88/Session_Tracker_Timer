import AppKit

private enum CoverDragMode {
    case none
    case move
    case left
    case right
    case top
    case bottom
    case topLeft
    case topRight
    case bottomLeft
    case bottomRight
}

private final class CoverInteractiveView: NSView {
    var onOpenSettings: (() -> Void)?
    var onCloseApp: (() -> Void)?
    var onFrameChanged: (() -> Void)?

    private let resizeGrip: CGFloat = 12
    private let minSize: CGFloat = 120
    private var dragMode: CoverDragMode = .none
    private var dragStartMouse = CGPoint.zero
    private var dragStartFrame = CGRect.zero

    override var isOpaque: Bool { true }

    override func draw(_ dirtyRect: NSRect) {
        NSColor.black.setFill()
        dirtyRect.fill()
    }

    override func rightMouseDown(with event: NSEvent) {
        let menu = NSMenu(title: "Cover")
        menu.addItem(NSMenuItem(title: "Settings...", action: #selector(onSettingsMenu), keyEquivalent: ""))
        menu.addItem(.separator())
        menu.addItem(NSMenuItem(title: "Close", action: #selector(onCloseMenu), keyEquivalent: ""))
        menu.items.forEach { $0.target = self }
        NSMenu.popUpContextMenu(menu, with: event, for: self)
    }

    @objc
    private func onSettingsMenu() {
        onOpenSettings?()
    }

    @objc
    private func onCloseMenu() {
        onCloseApp?()
    }

    override func resetCursorRects() {
        super.resetCursorRects()
        let bounds = self.bounds
        let inner = bounds.insetBy(dx: resizeGrip, dy: resizeGrip)

        addCursorRect(NSRect(x: bounds.minX, y: bounds.minY, width: resizeGrip, height: bounds.height), cursor: .resizeLeftRight)
        addCursorRect(NSRect(x: bounds.maxX - resizeGrip, y: bounds.minY, width: resizeGrip, height: bounds.height), cursor: .resizeLeftRight)
        addCursorRect(NSRect(x: bounds.minX, y: bounds.minY, width: bounds.width, height: resizeGrip), cursor: .resizeUpDown)
        addCursorRect(NSRect(x: bounds.minX, y: bounds.maxY - resizeGrip, width: bounds.width, height: resizeGrip), cursor: .resizeUpDown)
        addCursorRect(NSRect(x: bounds.minX, y: bounds.maxY - resizeGrip, width: resizeGrip, height: resizeGrip), cursor: .crosshair)
        addCursorRect(NSRect(x: bounds.maxX - resizeGrip, y: bounds.maxY - resizeGrip, width: resizeGrip, height: resizeGrip), cursor: .crosshair)
        addCursorRect(NSRect(x: bounds.minX, y: bounds.minY, width: resizeGrip, height: resizeGrip), cursor: .crosshair)
        addCursorRect(NSRect(x: bounds.maxX - resizeGrip, y: bounds.minY, width: resizeGrip, height: resizeGrip), cursor: .crosshair)
        addCursorRect(inner, cursor: .openHand)
    }

    override func mouseDown(with event: NSEvent) {
        guard let window = window else { return }
        dragStartMouse = NSEvent.mouseLocation
        dragStartFrame = window.frame
        dragMode = hitMode(at: convert(event.locationInWindow, from: nil))
    }

    override func mouseDragged(with event: NSEvent) {
        guard let window = window else { return }
        guard dragMode != .none else { return }

        let mouse = NSEvent.mouseLocation
        let dx = mouse.x - dragStartMouse.x
        let dy = mouse.y - dragStartMouse.y
        var frame = dragStartFrame
        let limits = windowLimits()

        switch dragMode {
        case .none:
            return
        case .move:
            frame.origin.x += dx
            frame.origin.y += dy
            frame = enforceRectConstraints(frame, limits: limits)
        case .left:
            frame.origin.x += dx
            frame.size.width -= dx
            frame = enforceResizeConstraints(frame, mode: .left, limits: limits)
        case .right:
            frame.size.width += dx
            frame = enforceResizeConstraints(frame, mode: .right, limits: limits)
        case .top:
            frame.size.height += dy
            frame = enforceResizeConstraints(frame, mode: .top, limits: limits)
        case .bottom:
            frame.origin.y += dy
            frame.size.height -= dy
            frame = enforceResizeConstraints(frame, mode: .bottom, limits: limits)
        case .topLeft:
            frame.origin.x += dx
            frame.size.width -= dx
            frame.size.height += dy
            frame = enforceResizeConstraints(frame, mode: .topLeft, limits: limits)
        case .topRight:
            frame.size.width += dx
            frame.size.height += dy
            frame = enforceResizeConstraints(frame, mode: .topRight, limits: limits)
        case .bottomLeft:
            frame.origin.x += dx
            frame.size.width -= dx
            frame.origin.y += dy
            frame.size.height -= dy
            frame = enforceResizeConstraints(frame, mode: .bottomLeft, limits: limits)
        case .bottomRight:
            frame.size.width += dx
            frame.origin.y += dy
            frame.size.height -= dy
            frame = enforceResizeConstraints(frame, mode: .bottomRight, limits: limits)
        }

        window.setFrame(frame, display: true)
        onFrameChanged?()
    }

    override func mouseUp(with event: NSEvent) {
        dragMode = .none
    }

    private func hitMode(at point: CGPoint) -> CoverDragMode {
        let left = point.x <= resizeGrip
        let right = point.x >= bounds.width - resizeGrip
        let top = point.y >= bounds.height - resizeGrip
        let bottom = point.y <= resizeGrip

        if top && left { return .topLeft }
        if top && right { return .topRight }
        if bottom && left { return .bottomLeft }
        if bottom && right { return .bottomRight }
        if left { return .left }
        if right { return .right }
        if top { return .top }
        if bottom { return .bottom }
        return .move
    }

    private func windowLimits() -> (bounds: CGRect, minWidth: CGFloat, minHeight: CGFloat) {
        let bounds = virtualVisibleBounds(margin: 8)
        return (
            bounds: bounds,
            minWidth: min(minSize, bounds.width),
            minHeight: min(minSize, bounds.height)
        )
    }

    private func resizingLeft(_ mode: CoverDragMode) -> Bool {
        mode == .left || mode == .topLeft || mode == .bottomLeft
    }

    private func resizingRight(_ mode: CoverDragMode) -> Bool {
        mode == .right || mode == .topRight || mode == .bottomRight
    }

    private func resizingTop(_ mode: CoverDragMode) -> Bool {
        mode == .top || mode == .topLeft || mode == .topRight
    }

    private func resizingBottom(_ mode: CoverDragMode) -> Bool {
        mode == .bottom || mode == .bottomLeft || mode == .bottomRight
    }

    private func enforceRectConstraints(
        _ frame: CGRect,
        limits: (bounds: CGRect, minWidth: CGFloat, minHeight: CGFloat)
    ) -> CGRect {
        var constrained = frame
        constrained.size.width = max(limits.minWidth, min(constrained.width, limits.bounds.width))
        constrained.size.height = max(limits.minHeight, min(constrained.height, limits.bounds.height))
        return clampFrame(constrained, to: limits.bounds)
    }

    private func enforceResizeConstraints(
        _ frame: CGRect,
        mode: CoverDragMode,
        limits: (bounds: CGRect, minWidth: CGFloat, minHeight: CGFloat)
    ) -> CGRect {
        var left = frame.minX
        var right = frame.maxX
        var bottom = frame.minY
        var top = frame.maxY

        if resizingLeft(mode) {
            if left < limits.bounds.minX { left = limits.bounds.minX }
            if right - left < limits.minWidth { left = right - limits.minWidth }
        } else if resizingRight(mode) {
            if right > limits.bounds.maxX { right = limits.bounds.maxX }
            if right - left < limits.minWidth { right = left + limits.minWidth }
        }

        if resizingTop(mode) {
            if top > limits.bounds.maxY { top = limits.bounds.maxY }
            if top - bottom < limits.minHeight { top = bottom + limits.minHeight }
        } else if resizingBottom(mode) {
            if bottom < limits.bounds.minY { bottom = limits.bounds.minY }
            if top - bottom < limits.minHeight { bottom = top - limits.minHeight }
        }

        let resized = CGRect(x: left, y: bottom, width: right - left, height: top - bottom)
        return enforceRectConstraints(resized, limits: limits)
    }
}

final class CoverWindowController: NSObject, NSWindowDelegate {
    var onOpenSettings: (() -> Void)?
    var onCloseApp: (() -> Void)?

    private let frameXKey = "cover_frame_x"
    private let frameYKey = "cover_frame_y"
    private let frameWKey = "cover_frame_w"
    private let frameHKey = "cover_frame_h"

    let window: NSWindow

    override init() {
        let bounds = virtualVisibleBounds(margin: 8)
        let defaultSize = min(CGFloat(260), min(bounds.width, bounds.height))
        let frame = CGRect(
            x: bounds.midX - (defaultSize / 2),
            y: bounds.midY - (defaultSize / 2),
            width: defaultSize,
            height: defaultSize
        )

        window = NSWindow(contentRect: frame, styleMask: [.borderless], backing: .buffered, defer: false)
        window.isOpaque = true
        window.backgroundColor = .black
        window.level = .floating
        window.collectionBehavior = [.canJoinAllSpaces, .fullScreenAuxiliary]
        window.hasShadow = true
        window.title = "Wolf-Timer Cover"

        super.init()

        let content = CoverInteractiveView(frame: window.contentView?.bounds ?? .zero)
        content.autoresizingMask = [.width, .height]
        content.wantsLayer = true
        content.layer?.backgroundColor = NSColor.black.cgColor
        content.onOpenSettings = { [weak self] in self?.onOpenSettings?() }
        content.onCloseApp = { [weak self] in self?.onCloseApp?() }
        content.onFrameChanged = { [weak self] in self?.saveFrame() }
        window.contentView = content
        window.delegate = self

        restoreFrame()
    }

    func windowDidMove(_ notification: Notification) {
        saveFrame()
    }

    func windowDidResize(_ notification: Notification) {
        saveFrame()
    }

    func show() {
        window.makeKeyAndOrderFront(nil)
        window.level = .floating
        clampToVisibleBounds()
        NSApp.activate(ignoringOtherApps: true)
    }

    func hide() {
        window.orderOut(nil)
    }

    func isVisible() -> Bool {
        window.isVisible
    }

    func toggleVisible() {
        if isVisible() {
            hide()
        } else {
            show()
        }
    }

    func clampToVisibleBounds() {
        let clamped = clampFrame(window.frame, to: virtualVisibleBounds(margin: 8))
        window.setFrame(clamped, display: true)
        saveFrame()
    }

    private func saveFrame() {
        let frame = window.frame
        let defaults = UserDefaults.standard
        defaults.set(frame.origin.x, forKey: frameXKey)
        defaults.set(frame.origin.y, forKey: frameYKey)
        defaults.set(frame.width, forKey: frameWKey)
        defaults.set(frame.height, forKey: frameHKey)
    }

    private func restoreFrame() {
        let defaults = UserDefaults.standard
        let hasFrame = defaults.object(forKey: frameWKey) != nil && defaults.object(forKey: frameHKey) != nil
        guard hasFrame else {
            let centered = centerFrame(width: 260, height: 260)
            window.setFrame(centered, display: false)
            return
        }

        var frame = CGRect(
            x: defaults.double(forKey: frameXKey),
            y: defaults.double(forKey: frameYKey),
            width: max(120, defaults.double(forKey: frameWKey)),
            height: max(120, defaults.double(forKey: frameHKey))
        )
        frame = clampFrame(frame, to: virtualVisibleBounds(margin: 8))
        window.setFrame(frame, display: false)
    }

    private func centerFrame(width: CGFloat, height: CGFloat) -> CGRect {
        let bounds = virtualVisibleBounds(margin: 8)
        var frame = CGRect(x: bounds.midX - width / 2, y: bounds.midY - height / 2, width: width, height: height)
        frame = clampFrame(frame, to: bounds)
        return frame
    }
}

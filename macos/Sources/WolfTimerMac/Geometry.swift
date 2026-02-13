import AppKit

func virtualVisibleBounds(margin: CGFloat) -> CGRect {
    var unionRect = CGRect.null
    for screen in NSScreen.screens {
        unionRect = unionRect.union(screen.visibleFrame)
    }

    if unionRect.isNull {
        unionRect = CGRect(x: 0, y: 0, width: 1024, height: 768)
    }

    return unionRect.insetBy(dx: margin, dy: margin)
}

func clampFrame(_ frame: CGRect, to bounds: CGRect) -> CGRect {
    var adjusted = frame
    let width = min(adjusted.width, bounds.width)
    let height = min(adjusted.height, bounds.height)
    adjusted.size = CGSize(width: width, height: height)

    if adjusted.minX < bounds.minX {
        adjusted.origin.x = bounds.minX
    }
    if adjusted.maxX > bounds.maxX {
        adjusted.origin.x = bounds.maxX - adjusted.width
    }
    if adjusted.minY < bounds.minY {
        adjusted.origin.y = bounds.minY
    }
    if adjusted.maxY > bounds.maxY {
        adjusted.origin.y = bounds.maxY - adjusted.height
    }

    return adjusted
}

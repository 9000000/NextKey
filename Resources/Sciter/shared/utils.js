// OpenKey Shared JavaScript Utilities
// Reusable functions for all Sciter dialogs

// ============================================
// SCROLLBAR RESIZE - Thin when idle, normal when scrolling
// Sciter approach: Find scrollbar element and style directly
// ============================================
var scrollResizeTimeout = null;

function initializeScrollbarResize(containerSelector) {
    containerSelector = containerSelector || ".tab-body, .macro-list, .app-list";
    var containers = document.querySelectorAll(containerSelector);

    containers.forEach(function (container) {
        // Find the scrollbar element inside container
        var scrollbar = container.querySelector("scrollbar");

        if (scrollbar) {
            // Initially thin
            setScrollbarThin(scrollbar);

            // Expand on scroll
            container.addEventListener("scroll", function () {
                setScrollbarNormal(scrollbar);
                resetShrinkTimer(scrollbar);
            });

            // Expand on mouse wheel
            container.addEventListener("wheel", function () {
                setScrollbarNormal(scrollbar);
                resetShrinkTimer(scrollbar);
            });

            // Expand on hover
            container.addEventListener("mouseenter", function () {
                setScrollbarNormal(scrollbar);
            });

            // Shrink on leave
            container.addEventListener("mouseleave", function () {
                shrinkScrollbarDelayed(scrollbar, 500);
            });
        }
    });
}

function setScrollbarThin(scrollbar) {
    scrollbar.style.width = "2px";
    scrollbar.style.opacity = "0.3";
}

function setScrollbarNormal(scrollbar) {
    scrollbar.style.width = "6px";
    scrollbar.style.opacity = "1";
}

function resetShrinkTimer(scrollbar) {
    if (scrollResizeTimeout) {
        clearTimeout(scrollResizeTimeout);
        scrollResizeTimeout = null;
    }
}

function shrinkScrollbarDelayed(scrollbar, delay) {
    if (scrollResizeTimeout) {
        clearTimeout(scrollResizeTimeout);
    }
    scrollResizeTimeout = setTimeout(function () {
        setScrollbarThin(scrollbar);
    }, delay);
}

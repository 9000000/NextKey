// OpenKey Settings JavaScript
// Handles div-based toggles with hidden inputs for VALUE_CHANGED events

document.on("ready", function () {

    initializeToggles();
    initializeAdvancedPanel();
    initializeOpacitySlider();
    initializeScrollbarResize(".tab-body");
});

function initializeToggles() {
    // Get all toggle elements and attach handlers
    const allToggles = document.querySelectorAll(".toggle-switch, .toggle-switch-small");

    allToggles.forEach(function (toggle) {
        toggle.onclick = function (evt) {
            const isChecked = this.classList.contains("checked");

            // Toggle the visual state
            if (isChecked) {
                this.classList.remove("checked");
            } else {
                this.classList.add("checked");
            }

            const id = this.id;
            const newState = !isChecked;

            // For show-advanced toggle, do force reflow on all toggles
            // to prevent visual glitch when expanding/collapsing
            if (id === "show-advanced") {
                const container = document.getElementById("main-container");
                if (container) {
                    const toggles = document.querySelectorAll(".toggle-switch, .toggle-switch-small");
                    toggles.forEach(function (t) {
                        t.style.display = "none";
                    });
                    container.offsetHeight; // Force synchronous reflow
                    toggles.forEach(function (t) {
                        t.style.display = "";
                    });
                }
            }

            // Update the hidden input to fire VALUE_CHANGED
            const hiddenInput = document.getElementById("val-" + id);
            if (hiddenInput) {
                hiddenInput.value = newState ? "1" : "0";
                // Trigger change event for C++ to catch
                hiddenInput.dispatchEvent(new Event("change", { bubbles: true }));
            }

            return true;
        };
    });
}

// Initialize advanced panel toggle and tabs
function initializeAdvancedPanel() {


    // Tab switching
    const tabItems = document.querySelectorAll(".tab-item");
    tabItems.forEach(function (tab) {
        tab.onclick = function () {
            switchTab(this.getAttribute("data-tab"));
            return true;
        };
    });

}

// Note: Advanced settings toggle is now handled by the toggle-switch-small #show-advanced
// The toggle click handler in initializeToggles() will dispatch val-show-advanced change
// C++ will handle the expand/collapse logic

// Toggle advanced settings panel expansion
function toggleAdvancedSettings() {

    const container = document.getElementById("main-container");
    if (container) {
        const isExpanded = container.classList.contains("expanded");

        if (isExpanded) {
            container.classList.remove("expanded");

        } else {
            container.classList.add("expanded");

        }

        // IMMEDIATE synchronous force reflow on toggles
        // Do it as fast as possible to minimize visible flash
        const toggles = document.querySelectorAll(".toggle-switch, .toggle-switch-small");
        toggles.forEach(function (toggle) {
            toggle.style.display = "none";
        });
        // Force synchronous reflow by reading offsetHeight
        container.offsetHeight;
        toggles.forEach(function (toggle) {
            toggle.style.display = "";
        });

        // Notify C++ to resize window
        const hiddenInput = document.getElementById("val-expand-state");
        if (hiddenInput) {
            hiddenInput.value = !isExpanded ? "1" : "0";
            hiddenInput.dispatchEvent(new Event("change", { bubbles: true }));
        }
    }
}

// Switch between tabs
function switchTab(tabIndex) {


    // Update active tab item
    const tabItems = document.querySelectorAll(".tab-item");
    tabItems.forEach(function (tab) {
        if (tab.getAttribute("data-tab") === tabIndex) {
            tab.classList.add("active");
        } else {
            tab.classList.remove("active");
        }
    });

    // Update active tab panel
    const tabPanels = document.querySelectorAll(".tab-panel");
    tabPanels.forEach(function (panel) {
        const panelIndex = panel.id.replace("tab-panel-", "");
        if (panelIndex === tabIndex) {
            panel.classList.add("active");
        } else {
            panel.classList.remove("active");
        }
    });
}

// Handle dropdown changes (already works via C++ VALUE_CHANGED handler)
document.on("change", "select", function (evt, select) {
    const id = select.id || select.getAttribute("id");
    const value = parseInt(select.value);

});

// Handle text input changes
document.on("change", "#switch-key-char", function (evt, input) {
    const char = input.value.toUpperCase();
    if (char.length > 0) {
        input.value = char.charAt(0);

    }
});

// Handle button clicks
document.on("click", "button", function (evt, button) {
    const id = button.id || button.getAttribute("id");

});

// ============================================
// CUSTOM OPACITY SLIDER - Background Transparency
// ============================================
var sliderDragging = false;

function initializeOpacitySlider() {
    var slider = document.getElementById("bg-opacity-slider");
    var thumb = document.getElementById("bg-opacity-thumb");
    var fill = document.getElementById("bg-opacity-fill");
    var valueLabel = document.getElementById("bg-opacity-value");
    var hiddenInput = document.getElementById("val-bg-opacity");

    if (!slider || !thumb) return;

    // Click on track to set value
    slider.onmousedown = function (evt) {
        sliderDragging = true;
        updateSliderFromMouse(evt, slider, thumb, fill, valueLabel, hiddenInput);
    };

    // Drag thumb
    document.onmousemove = function (evt) {
        if (sliderDragging) {
            updateSliderFromMouse(evt, slider, thumb, fill, valueLabel, hiddenInput);
        }
    };

    document.onmouseup = function (evt) {
        if (sliderDragging) {
            sliderDragging = false;
            // Save to C++ on release
            hiddenInput.dispatchEvent(new Event("change", { bubbles: true }));
        }
    };
}

function updateSliderFromMouse(evt, slider, thumb, fill, valueLabel, hiddenInput) {
    var rect = slider.getBoundingClientRect();
    var x = evt.clientX - rect.left;
    var width = rect.width;

    // Clamp to 0-100%
    var percent = Math.max(0, Math.min(100, (x / width) * 100));
    var value = Math.round(percent);

    // Update UI
    thumb.style.left = percent + "%";
    fill.style.width = percent + "%";
    valueLabel.textContent = value + "%";
    hiddenInput.value = value.toString();

    // Apply background immediately
    var opacity = value / 100;
    var container = document.getElementById("main-container");
    if (container) {
        container.style.backgroundColor = "rgba(255, 255, 255, " + opacity + ")";
    }
}

// Called from C++ to set initial opacity value
function setBackgroundOpacity(value) {
    var thumb = document.getElementById("bg-opacity-thumb");
    var fill = document.getElementById("bg-opacity-fill");
    var valueLabel = document.getElementById("bg-opacity-value");
    var hiddenInput = document.getElementById("val-bg-opacity");

    if (thumb && fill) {
        thumb.style.left = value + "%";
        fill.style.width = value + "%";
        valueLabel.textContent = value + "%";
        hiddenInput.value = value.toString();

        // Apply background directly on container
        var opacity = value / 100;
        var container = document.getElementById("main-container");
        if (container) {
            container.style.backgroundColor = "rgba(255, 255, 255, " + opacity + ")";
        }
    }
}


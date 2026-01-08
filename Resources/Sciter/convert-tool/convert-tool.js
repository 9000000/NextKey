// Convert Tool Dialog JavaScript
// Uses Sciter patterns from settings.js

document.on("ready", function () {
    initConvertToolDialog();
});

function initConvertToolDialog() {
    initializeToggles();
    initSwapButton();
    initHotkeyInput();
    initModeSwitching();
    initBrowseButtons();
}

// Mode switching: Clipboard vs File
function initModeSwitching() {
    var container = document.getElementById("main-container");
    var modeInput = document.getElementById("val-ui-mode");

    function updateMode(value) {
        if (value === "file") {
            container.classList.add("mode-file");
        } else {
            container.classList.remove("mode-file");
        }

        // Sync with hidden input for C++
        if (modeInput) {
            modeInput.value = value;
            modeInput.dispatchEvent(new Event("change", { bubbles: true }));
        }
    }

    // Event listener for radio buttons
    document.on("change", "input[name='source-type']", function (evt, input) {
        updateMode(input.value);
    });

    // Initial sync
    var checkedRadio = document.querySelector("input[name='source-type']:checked");
    if (checkedRadio) {
        updateMode(checkedRadio.value);
    }
}

// Browse buttons - trigger C++ actions
function initBrowseButtons() {
    var btnBrowseSource = document.getElementById("btn-browse-source");
    var btnBrowseDest = document.getElementById("btn-browse-dest");

    if (btnBrowseSource) {
        btnBrowseSource.onclick = function () {
            triggerAction("select-source-file");
        };
    }

    if (btnBrowseDest) {
        btnBrowseDest.onclick = function () {
            triggerAction("select-dest-file");
        };
    }
}

// Initialize toggle switches using Sciter pattern from settings.js
function initializeToggles() {
    var allToggles = document.querySelectorAll(".toggle-switch-small");

    allToggles.forEach(function (toggle) {
        toggle.onclick = function (evt) {
            var isChecked = this.classList.contains("checked");

            // Toggle the visual state
            if (isChecked) {
                this.classList.remove("checked");
            } else {
                this.classList.add("checked");
            }

            var id = this.id;
            var newState = !isChecked;

            // Update the hidden input to fire VALUE_CHANGED
            var hiddenInput = document.getElementById("val-" + id);
            if (hiddenInput) {
                hiddenInput.value = newState ? "1" : "0";
                hiddenInput.dispatchEvent(new Event("change", { bubbles: true }));
            }

            return true;
        };
    });
}

// Swap button - swap dropdown values directly in JS
function initSwapButton() {
    var btnSwap = document.getElementById("btn-swap");
    if (btnSwap) {
        btnSwap.onclick = function () {
            var sourceEnc = document.getElementById("source-encoding");
            var destEnc = document.getElementById("dest-encoding");

            if (sourceEnc && destEnc) {
                // Read current values
                var srcVal = sourceEnc.value;
                var dstVal = destEnc.value;

                // Swap: source = old dest, dest = old source
                sourceEnc.value = dstVal;
                destEnc.value = srcVal;

                // Trigger change events so C++ saves to registry
                sourceEnc.dispatchEvent(new Event("change", { bubbles: true }));
                destEnc.dispatchEvent(new Event("change", { bubbles: true }));
            }
            return true;
        };
    }
}

// Hotkey input - using Sciter patterns from settings.js switch-key-char
function initHotkeyInput() {
    // Nothing needed here - handled by document.on handlers below
}

// Handle text input changes - display "Space" for space character
document.on("change", "#hotkey-char", function (evt, input) {
    var char = input.value;

    if (char === " ") {
        input.value = "Space";
    } else if (char === "Space") {
        // Keep "Space" displayed
    } else if (char.length === 0) {
        // Empty - user deleted everything
    } else if (char.length === 1) {
        // Single character - uppercase
        input.value = char.toUpperCase();
    } else {
        // Partial text - clear it
        input.value = "";
    }
});

// Real-time conversion for space
document.on("input", "#hotkey-char", function (evt, input) {
    if (input.value === " ") {
        input.value = "Space";
    }
});

// Trigger action via hidden input (for C++ to detect)
function triggerAction(action) {
    var actionInput = document.getElementById("val-action");
    if (actionInput) {
        actionInput.value = action;
        var event = new Event("change", { bubbles: true });
        actionInput.dispatchEvent(event);
    }
}

// Called from C++ to set toggle state
function setToggleState(id, checked) {
    var toggle = document.getElementById(id);
    if (toggle) {
        if (checked) {
            toggle.classList.add("checked");
        } else {
            toggle.classList.remove("checked");
        }
    }
}

// Called from C++ to set dropdown value
function setDropdownValue(id, value) {
    var dropdown = document.getElementById(id);
    if (dropdown) {
        dropdown.value = value;
    }
}

// Called from C++ to set hotkey display
function setHotkeyDisplay(value) {
    var hotkeyInput = document.getElementById("hotkey-char");
    if (hotkeyInput) {
        hotkeyInput.value = value;
    }
}

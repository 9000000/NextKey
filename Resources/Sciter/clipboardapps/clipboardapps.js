// Clipboard Apps Dialog JavaScript

// Global dropdown controller instance
var dropdownController = null;

document.ready = function () {
    initClipboardAppsDialog();
    initializeScrollbarResize(".app-list");
};

function initClipboardAppsDialog() {
    // Initialize dropdown using shared component
    dropdownController = createRunningAppsDropdown({
        inputId: "app-name",
        dropdownId: "running-apps-dropdown",
        triggerAction: triggerAction
    });
    dropdownController.init();

    var btnAdd = document.getElementById("btn-add");
    var btnPickWindow = document.getElementById("btn-pick-window");
    var btnClose = document.getElementById("btn-close");
    var btnRefresh = document.getElementById("btn-refresh");

    if (btnAdd) {
        btnAdd.addEventListener("click", function () {
            onAddApp();
        });
    }

    if (btnPickWindow) {
        btnPickWindow.addEventListener("click", function () {
            triggerAction("pick-window");
        });
    }

    if (btnClose) {
        btnClose.addEventListener("click", function () {
            triggerAction("close");
        });
    }

    if (btnRefresh) {
        btnRefresh.addEventListener("click", function () {
            dropdownController.resetCache();
            triggerAction("get-running-apps");
        });
    }

    // Event delegation for dynamically created elements
    // Handle method change in app list
    document.on("change", ".app-item select", function (evt, select) {
        var item = select.closest(".app-item");
        if (item) {
            var appName = item.getAttribute("data-name");
            if (appName) {
                onChangeMethod(appName, select.value);
            }
        }
    });

    // Handle delete button click in app list
    document.on("click", ".app-item-delete", function (evt, btn) {
        var item = btn.closest(".app-item");
        if (item) {
            var appName = item.getAttribute("data-name");
            if (appName) {
                onDeleteApp(appName);
            }
        }
        evt.stopPropagation();
    });

    // Handle delay input change in app list
    document.on("change", ".app-item-delay input", function (evt, input) {
        var item = input.closest(".app-item");
        if (item) {
            var appName = item.getAttribute("data-name");
            if (appName) {
                var delayVal = parseInt(input.value, 10) || 0;
                if (delayVal < 0) delayVal = 0;
                if (delayVal > 500) delayVal = 500;
                input.value = delayVal;  // Clamp value in UI
                onChangeDelay(appName, delayVal);
            }
        }
    });
}

// Called by C++ to set the list of running apps
function setRunningApps(apps) {
    if (dropdownController) {
        dropdownController.setApps(apps);
    }
}

function onAddApp() {
    var nameField = document.getElementById("app-name");
    var methodField = document.getElementById("paste-method");
    var delayField = document.getElementById("delay-ms");

    if (!nameField || !methodField) return;

    var name = nameField.value.trim();
    if (name === "") return;

    var methodInt = parseInt(methodField.value, 10) || 0;
    var delayMs = parseInt(delayField.value, 10) || 0;
    if (delayMs < 0) delayMs = 0;
    if (delayMs > 500) delayMs = 500;

    document.getElementById("val-app-name").value = name;
    document.getElementById("val-paste-method").value = methodInt;
    document.getElementById("val-delay-ms").value = delayMs;
    triggerAction("add-app");

    nameField.value = "";
    delayField.value = "0";
    nameField.focus();

    // Show dropdown again so user can continue adding apps
    // Use setTimeout to avoid race condition with document click handler
    setTimeout(function () {
        if (dropdownController) {
            dropdownController.filterAndShow("");
        }
    }, 100);
}

function onDeleteApp(name) {
    document.getElementById("val-app-name").value = name;
    triggerAction("delete-app");
}

function onChangeMethod(name, newMethod) {
    document.getElementById("val-app-name").value = name;
    document.getElementById("val-paste-method").value = newMethod;
    triggerAction("change-method");
}

function onChangeDelay(name, newDelay) {
    document.getElementById("val-app-name").value = name;
    document.getElementById("val-delay-ms").value = newDelay;
    triggerAction("change-delay");
}

function clearInput() {
    var nameField = document.getElementById("app-name");
    var delayField = document.getElementById("delay-ms");
    if (nameField) {
        nameField.value = "";
    }
    if (delayField) {
        delayField.value = "0";
    }
}

function triggerAction(action) {
    var actionInput = document.getElementById("val-action");
    if (actionInput) {
        actionInput.value = action;
        var event = new Event("change", { bubbles: true });
        actionInput.dispatchEvent(event);
    }
}

// Called by C++ to add items to the list
// methodInt: 0 = Shift+Insert, 1 = Ctrl+V
// delayMs: delay after paste (0-500)
function addAppToList(name, methodInt, delayMs) {
    var list = document.getElementById("app-list");
    if (!list) return;

    var item = document.createElement("div");
    item.className = "app-item";
    item.setAttribute("data-name", name);
    item.setAttribute("data-method", methodInt);
    item.setAttribute("data-delay", delayMs);

    // Name span with tooltip for long names
    var nameSpan = document.createElement("span");
    nameSpan.className = "app-item-name";
    nameSpan.textContent = name;
    nameSpan.setAttribute("title", name);  // Tooltip shows full name on hover
    item.appendChild(nameSpan);

    // Method dropdown container
    var methodDiv = document.createElement("div");
    methodDiv.className = "app-item-method";
    var methodSelect = document.createElement("select");

    var opt0 = document.createElement("option");
    opt0.value = "0";
    opt0.textContent = "Shift+Ins";
    if (methodInt == 0) opt0.setAttribute("selected", "selected");
    methodSelect.appendChild(opt0);

    var opt1 = document.createElement("option");
    opt1.value = "1";
    opt1.textContent = "Ctrl+V";
    if (methodInt == 1) opt1.setAttribute("selected", "selected");
    methodSelect.appendChild(opt1);

    var opt2 = document.createElement("option");
    opt2.value = "2";
    opt2.textContent = "SendInput";
    if (methodInt == 2) opt2.setAttribute("selected", "selected");
    methodSelect.appendChild(opt2);

    methodDiv.appendChild(methodSelect);
    item.appendChild(methodDiv);

    // Delay input (editable)
    var delayDiv = document.createElement("div");
    delayDiv.className = "app-item-delay";
    // Delay input (cloned from the working top input to preserve behaviors)
    var templateInput = document.getElementById("delay-ms");
    var delayInput;

    if (templateInput) {
        delayInput = templateInput.cloneNode(true);
        delayInput.id = ""; // Remove ID to prevent duplicates
        delayInput.value = delayMs;
        delayInput.className = "setting-input delay-input"; // Ensure classes are set
    } else {
        // Fallback if template not found
        delayInput = document.createElement("input");
        delayInput.type = "number";
        delayInput.value = delayMs;
        delayInput.min = "0";
        delayInput.max = "500";
        delayInput.className = "setting-input delay-input";
        delayInput.step = "5";
    }

    delayDiv.appendChild(delayInput);
    item.appendChild(delayDiv);

    // Delete button
    var deleteBtn = document.createElement("button");
    deleteBtn.className = "app-item-delete";
    deleteBtn.textContent = "×";
    item.appendChild(deleteBtn);

    list.appendChild(item);
}

function removeAppFromList(name) {
    var list = document.getElementById("app-list");
    if (!list) return;

    var items = list.querySelectorAll('.app-item');
    for (var i = 0; i < items.length; i++) {
        if (items[i].getAttribute('data-name') === name) {
            items[i].remove();
            break;
        }
    }
}

function clearAppList() {
    var list = document.getElementById("app-list");
    if (list) {
        list.innerHTML = "";
    }
}

function forceRefresh(scrollToBottom) {
    var list = document.getElementById("app-list");
    if (list) {
        var oldScroll = list.scrollTop;
        var origDisplay = list.style.display || "";
        list.style.display = "none";
        void list.offsetHeight;
        list.style.display = origDisplay || "block";

        if (scrollToBottom === true) {
            list.scrollTop = list.scrollHeight;
        } else {
            list.scrollTop = oldScroll;
        }
    }
}

function setBackgroundOpacity(value) {
    var opacity = value / 100;
    document.documentElement.style.setProperty(
        "--bg-glass",
        "rgba(255, 255, 255, " + opacity + ")"
    );
}

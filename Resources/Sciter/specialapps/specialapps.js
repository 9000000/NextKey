// Special Apps Dialog JavaScript

// Global dropdown controller instance
var dropdownController = null;

document.ready = function () {
    initSpecialAppsDialog();
    initializeScrollbarResize(".app-list");
};

function initSpecialAppsDialog() {
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

    // Event delegation for dynamically created elements (Sciter pattern)
    // Handle type change in app list
    document.on("change", ".app-item select", function (evt, select) {
        var item = select.closest(".app-item");
        if (item) {
            var appName = item.getAttribute("data-name");
            if (appName) {
                onChangeType(appName, select.value);
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
}

// Called by C++ to set the list of running apps
function setRunningApps(apps) {
    if (dropdownController) {
        dropdownController.setApps(apps);
    }
}

function onAddApp() {
    var nameField = document.getElementById("app-name");
    var typeField = document.getElementById("app-type");

    if (!nameField || !typeField) return;

    var name = nameField.value.trim();
    if (name === "") return;

    var typeInt = parseInt(typeField.value, 10) || 0;

    document.getElementById("val-app-name").value = name;
    document.getElementById("val-app-type").value = typeInt;
    triggerAction("add-app");

    nameField.value = "";
    nameField.focus();
}

function onDeleteApp(name) {
    document.getElementById("val-app-name").value = name;
    triggerAction("delete-app");
}

function onChangeType(name, newType) {
    document.getElementById("val-app-name").value = name;
    document.getElementById("val-app-type").value = newType;
    triggerAction("change-type");
}

function clearInput() {
    var nameField = document.getElementById("app-name");
    if (nameField) {
        nameField.value = "";
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
// typeInt: 0 = Qt/Electron, 1 = Skip IME
// isDefault: true = default app (type locked), false = user added (fully editable)
function addAppToList(name, typeInt, isDefault) {
    var list = document.getElementById("app-list");
    if (!list) return;

    var item = document.createElement("div");
    item.className = "app-item";
    item.setAttribute("data-name", name);
    item.setAttribute("data-type", typeInt);

    // Name span
    var nameSpan = document.createElement("span");
    nameSpan.className = "app-item-name";
    nameSpan.textContent = name;
    if (isDefault) {
        var defaultTag = document.createElement("span");
        defaultTag.className = "app-item-default";
        defaultTag.textContent = "(mặc định)";
        nameSpan.appendChild(defaultTag);
    }
    item.appendChild(nameSpan);

    // Type dropdown container
    var typeDiv = document.createElement("div");
    typeDiv.className = "app-item-type";
    var typeSelect = document.createElement("select");
    if (isDefault) {
        typeSelect.disabled = true;
    }
    // Use truthy check for typeInt
    var isSkipIme = (typeInt && typeInt != 0 && typeInt !== "0");
    var opt0 = document.createElement("option");
    opt0.value = "0";
    opt0.textContent = "Qt/Electron";
    if (!isSkipIme) opt0.setAttribute("selected", "selected");
    typeSelect.appendChild(opt0);
    var opt1 = document.createElement("option");
    opt1.value = "1";
    opt1.textContent = "Skip IME Check";
    if (isSkipIme) opt1.setAttribute("selected", "selected");
    typeSelect.appendChild(opt1);
    // Event handling done via document.on delegation in initSpecialAppsDialog
    typeDiv.appendChild(typeSelect);
    item.appendChild(typeDiv);

    // Delete button - ALWAYS enabled
    var deleteBtn = document.createElement("button");
    deleteBtn.className = "app-item-delete";
    deleteBtn.textContent = "×";
    // Event handling done via document.on delegation in initSpecialAppsDialog
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

function escapeHtml(text) {
    var div = document.createElement("div");
    div.textContent = text;
    return div.innerHTML;
}

function setBackgroundOpacity(value) {
    var opacity = value / 100;
    document.documentElement.style.setProperty(
        "--bg-glass",
        "rgba(255, 255, 255, " + opacity + ")"
    );
}

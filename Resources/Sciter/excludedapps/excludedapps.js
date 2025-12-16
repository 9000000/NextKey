// Excluded Apps Dialog JavaScript

document.ready = function () {
    initExcludedAppsDialog();
};

function initExcludedAppsDialog() {
    // Bind button clicks
    var btnAddManual = document.getElementById("btn-add-manual");
    var btnAddCurrent = document.getElementById("btn-add-current");
    var btnDelete = document.getElementById("btn-delete");
    var btnClose = document.getElementById("btn-close");
    var appNameInput = document.getElementById("app-name");

    if (btnAddManual) {
        btnAddManual.addEventListener("click", function () {
            onAddManual();
        });
    }

    if (btnAddCurrent) {
        btnAddCurrent.addEventListener("click", function () {
            triggerAction("add-current");
        });
    }

    if (btnDelete) {
        btnDelete.addEventListener("click", function () {
            onDeleteApp();
        });
    }

    if (btnClose) {
        btnClose.addEventListener("click", function () {
            triggerAction("close");
        });
    }

    // Refresh button: Force reload running apps
    var btnRefresh = document.getElementById("btn-refresh");
    if (btnRefresh) {
        btnRefresh.addEventListener("click", function () {
            runningAppsLoaded = false;  // Reset cache
            runningApps = [];
            triggerAction("get-running-apps");
        });
    }

    // ===== Running Apps Dropdown =====
    if (appNameInput) {
        // Focus: Request running apps from C++ (only first time)
        appNameInput.addEventListener("focus", function () {
            if (!runningAppsLoaded) {
                triggerAction("get-running-apps");
            } else {
                // Already loaded, just show dropdown
                filterAndShowDropdown(appNameInput.value);
            }
        });

        // Input: Filter dropdown as user types
        appNameInput.addEventListener("input", function () {
            filterAndShowDropdown(this.value);
        });

        // Keydown: Navigate dropdown with arrow keys
        appNameInput.addEventListener("keydown", function (e) {
            if (e.code === "ArrowDown" || e.code === "ArrowUp" || e.code === "Enter") {
                handleDropdownKeyboard(e);
            } else if (e.code === "Escape") {
                hideDropdown();
            }
        });
    }

    // Document click: Close dropdown if clicking outside (NOT blur - avoids race condition!)
    document.addEventListener("click", function (e) {
        var dropdown = document.getElementById("running-apps-dropdown");
        var input = document.getElementById("app-name");
        var inputContainer = input ? input.parentElement : null;

        // Check if click is inside input container (includes dropdown)
        if (inputContainer && !inputContainer.contains(e.target)) {
            hideDropdown();
        }
    });
}

// ===== Running Apps State =====
var runningApps = [];         // Full list from C++
var runningAppsLoaded = false;
var highlightedIndex = -1;    // Current highlighted item in dropdown

// Called by C++ to set the list of running apps
function setRunningApps(apps) {
    runningApps = [];

    // apps is a Sciter array-like value
    if (apps && apps.length) {
        for (var i = 0; i < apps.length; i++) {
            runningApps.push(apps[i]);
        }
    }

    runningAppsLoaded = true;

    // Show dropdown immediately after loading
    var input = document.getElementById("app-name");
    if (input) {
        filterAndShowDropdown(input.value);
    }
}

// Filter apps and show dropdown
function filterAndShowDropdown(query) {
    var dropdown = document.getElementById("running-apps-dropdown");
    if (!dropdown) return;

    // Clear previous items
    dropdown.innerHTML = "";
    highlightedIndex = -1;

    // Filter apps
    var filtered = [];
    var lowerQuery = query.toLowerCase();

    for (var i = 0; i < runningApps.length; i++) {
        if (lowerQuery === "" || runningApps[i].toLowerCase().indexOf(lowerQuery) !== -1) {
            filtered.push(runningApps[i]);
        }
    }

    // Show message if no apps
    if (filtered.length === 0) {
        if (runningAppsLoaded && runningApps.length === 0) {
            dropdown.innerHTML = '<div class="dropdown-empty">Không tìm thấy ứng dụng nào</div>';
        } else if (query !== "") {
            dropdown.innerHTML = '<div class="dropdown-empty">Không có kết quả phù hợp</div>';
        } else {
            hideDropdown();
            return;
        }
        showDropdown();
        return;
    }

    // Build dropdown items
    for (var j = 0; j < filtered.length; j++) {
        var item = document.createElement("div");
        item.className = "dropdown-item";
        item.textContent = filtered[j];
        item.setAttribute("data-app", filtered[j]);

        // Use closure to capture correct app name
        (function (appName) {
            item.addEventListener("click", function (e) {
                e.stopPropagation();  // Prevent document click from closing
                selectDropdownItem(appName);
            });
        })(filtered[j]);

        dropdown.appendChild(item);
    }

    showDropdown();
}

function showDropdown() {
    var dropdown = document.getElementById("running-apps-dropdown");
    if (dropdown) {
        dropdown.classList.add("visible");
    }
}

function hideDropdown() {
    var dropdown = document.getElementById("running-apps-dropdown");
    if (dropdown) {
        dropdown.classList.remove("visible");
    }
    highlightedIndex = -1;
}

function selectDropdownItem(appName) {
    var input = document.getElementById("app-name");
    if (input) {
        input.value = appName;
    }
    hideDropdown();

    // Focus back to input for potential add
    if (input) {
        input.focus();
    }
}

function handleDropdownKeyboard(e) {
    var dropdown = document.getElementById("running-apps-dropdown");
    if (!dropdown || !dropdown.classList.contains("visible")) return;

    var items = dropdown.querySelectorAll(".dropdown-item");
    if (items.length === 0) return;

    if (e.code === "ArrowDown") {
        e.preventDefault();
        highlightedIndex = (highlightedIndex + 1) % items.length;
        updateHighlight(items);
    } else if (e.code === "ArrowUp") {
        e.preventDefault();
        highlightedIndex = highlightedIndex <= 0 ? items.length - 1 : highlightedIndex - 1;
        updateHighlight(items);
    } else if (e.code === "Enter") {
        e.preventDefault();
        if (highlightedIndex >= 0 && highlightedIndex < items.length) {
            var appName = items[highlightedIndex].getAttribute("data-app");
            selectDropdownItem(appName);
        }
    }
}

function updateHighlight(items) {
    for (var i = 0; i < items.length; i++) {
        if (i === highlightedIndex) {
            items[i].classList.add("highlighted");
            // Scroll into view
            items[i].scrollIntoView({ block: "nearest" });
        } else {
            items[i].classList.remove("highlighted");
        }
    }
}

function onAddManual() {
    var nameField = document.getElementById("app-name");

    if (!nameField) return;

    var name = nameField.value.trim();

    if (name === "") {
        return;
    }

    // Set hidden inputs for C++ to read
    document.getElementById("val-app-name").value = name;
    triggerAction("add-manual");

    // Clear input after add
    nameField.value = "";
    nameField.focus();
}

function onDeleteApp() {
    var selectedItem = document.querySelector(".app-item.selected");
    if (!selectedItem) {
        return;
    }

    var name = selectedItem.getAttribute("data-name");
    if (!name) {
        return;
    }

    document.getElementById("val-app-name").value = name;
    triggerAction("delete");

    // Clear input after delete
    clearInput();
}

// Clear input field and selection - called by C++ after window picker add
function clearInput() {
    var nameField = document.getElementById("app-name");
    if (nameField) {
        nameField.value = "";
    }

    // Also clear selection
    var items = document.querySelectorAll(".app-item.selected");
    for (var i = 0; i < items.length; i++) {
        items[i].classList.remove("selected");
    }
}

function selectAppItem(element, name) {
    // Remove selected class from all items
    var items = document.querySelectorAll(".app-item");
    for (var i = 0; i < items.length; i++) {
        items[i].classList.remove("selected");
    }

    // Add selected class to clicked item
    element.classList.add("selected");

    // Fill input field
    document.getElementById("app-name").value = name;
}

function triggerAction(action) {
    var actionInput = document.getElementById("val-action");
    if (actionInput) {
        actionInput.value = action;
        // Dispatch change event for C++ to detect
        var event = new Event("change", { bubbles: true });
        actionInput.dispatchEvent(event);
    }
}

// Called by C++ to add items to the list
function addAppToList(name) {
    var list = document.getElementById("app-list");
    if (!list) return;

    var item = document.createElement("div");
    item.className = "app-item";
    item.setAttribute("data-name", name);
    item.innerHTML = '<span class="app-item-name">' + escapeHtml(name) + '</span>';

    item.addEventListener("click", function () {
        selectAppItem(this, name);
    });

    list.appendChild(item);
}

// Called by C++ to remove a single item without full reload
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

// Called by C++ to clear the list before refreshing
function clearAppList() {
    var list = document.getElementById("app-list");
    if (list) {
        list.innerHTML = "";
    }
}

// Called by C++ after updating list to force Sciter to refresh visuals
function forceRefresh() {
    var list = document.getElementById("app-list");
    if (list) {
        // Save original display, hide, force reflow, restore
        var origDisplay = list.style.display || "";
        list.style.display = "none";
        void list.offsetHeight;  // Force reflow - void to ensure execution
        list.style.display = origDisplay || "block";

        // Also scroll to bottom to show new items
        list.scrollTop = list.scrollHeight;
    }
}

function escapeHtml(text) {
    var div = document.createElement("div");
    div.textContent = text;
    return div.innerHTML;
}

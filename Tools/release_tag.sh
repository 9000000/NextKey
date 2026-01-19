#!/bin/bash
set -e

# Function to get current git branch
get_git_branch() {
    git rev-parse --abbrev-ref HEAD
}

# Function to update resource file
update_resource_file() {
    local file_path="$1"
    local version="$2"

    # Convert "1.0.8" to "1,0,8,0"
    local comma_version="${version//./,}"
    # Count dots to check if we need to append ",0"
    local dots="${version//[^.]}"
    if [ "${#dots}" -eq 2 ]; then
        comma_version="${comma_version},0"
    fi

    echo -e "\033[36mUpdating $file_path to version $version ($comma_version)...\033[0m"

    # Use sed to replace in place. 
    # NOTE: We assume the file is UTF-8 or compatible ASCII.
    
    # Update FILEVERSION 1,0,8,0
    sed -i -E "s/FILEVERSION [0-9]+,[0-9]+,[0-9]+,[0-9]+/FILEVERSION $comma_version/" "$file_path"
    
    # Update PRODUCTVERSION 1,0,8,0
    sed -i -E "s/PRODUCTVERSION [0-9]+,[0-9]+,[0-9]+,[0-9]+/PRODUCTVERSION $comma_version/" "$file_path"

    # Update "FileVersion", "1.0.8"
    sed -i -E "s/\"FileVersion\", \"[0-9]+\.[0-9]+\.[0-9]+(\.[0-9]+)?\"/\"FileVersion\", \"$version\"/" "$file_path"

    # Update "ProductVersion", "1.0.8"
    sed -i -E "s/\"ProductVersion\", \"[0-9]+\.[0-9]+\.[0-9]+(\.[0-9]+)?\"/\"ProductVersion\", \"$version\"/" "$file_path"

    echo -e "\033[32mFile updated successfully.\033[0m"
}

# --- Main Script ---

# 1. Ask for version
read -p "Enter new version (e.g. 1.0.8): " version
if [ -z "$version" ]; then
    echo "Version cannot be empty."
    exit 1
fi

if ! [[ $version =~ ^[0-9]+\.[0-9]+\.[0-9]+(\.[0-9]+)?$ ]]; then
    echo "Invalid version format. Expected X.Y.Z or X.Y.Z.W"
    exit 1
fi

# 2. Confirm
branch=$(get_git_branch)
echo -e "\033[33mYou are on branch: $branch\033[0m"
read -p "Bump version to $version and create tag v$version? (y/n) " confirm
if [[ ! "$confirm" =~ ^[yY]$ ]]; then
    echo "Aborted."
    exit 0
fi

# 3. Update .rc file
# Relative path from Tools/ directory to OpenKey.rc
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
RC_PATH="$SCRIPT_DIR/../Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.rc"

if [ ! -f "$RC_PATH" ]; then
    echo "Error: Could not find OpenKey.rc at $RC_PATH"
    exit 1
fi

update_resource_file "$RC_PATH" "$version"

# 4. Git operations
echo -e "\033[36mPerforming Git operations...\033[0m"

# Stage ALL changes (as requested)
git add -A

# Commit
git commit -m "Bump version to v$version"

# Tag
git tag -a "v$version" -m "Release v$version"

# Push
echo -e "\033[36mPushing to origin ($branch)...\033[0m"
git push origin "$branch"
git push origin "v$version"

echo -e "\033[32mDone! v$version released on branch $branch.\033[0m"

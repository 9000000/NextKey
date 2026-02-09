#!/bin/bash
set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${CYAN}=== Delete Latest Release ===${NC}"

# Check if gh CLI is installed
if ! command -v gh &> /dev/null; then
    echo -e "${RED}Error: GitHub CLI (gh) is not installed!${NC}"
    echo "Install it with: sudo apt install gh"
    echo "Then authenticate: gh auth login"
    exit 1
fi

# Check if authenticated
if ! gh auth status &> /dev/null; then
    echo -e "${RED}Error: Not authenticated with GitHub CLI!${NC}"
    echo "Run: gh auth login"
    exit 1
fi

# Get the latest release tag
echo -e "${CYAN}Fetching latest release...${NC}"
LATEST_TAG=$(gh release list --limit 1 --json tagName -q '.[0].tagName' 2>/dev/null)

if [ -z "$LATEST_TAG" ]; then
    echo -e "${RED}No releases found!${NC}"
    exit 1
fi

echo -e "${YELLOW}Latest release: ${LATEST_TAG}${NC}"

# Confirm deletion
read -p "Are you sure you want to delete release ${LATEST_TAG}? (y/n) " confirm
if [[ ! "$confirm" =~ ^[yY]$ ]]; then
    echo "Aborted."
    exit 0
fi

# Delete the release on GitHub
echo -e "${CYAN}Deleting GitHub release...${NC}"
gh release delete "$LATEST_TAG" --yes

# Delete the remote tag
echo -e "${CYAN}Deleting remote tag...${NC}"
gh api -X DELETE "/repos/{owner}/{repo}/git/refs/tags/${LATEST_TAG}" 2>/dev/null || true

# Delete local tag if exists
echo -e "${CYAN}Deleting local tag...${NC}"
git tag -d "$LATEST_TAG" 2>/dev/null || echo "Local tag not found (already deleted)"

echo -e "${GREEN}✓ Release ${LATEST_TAG} deleted successfully!${NC}"
echo ""
echo -e "${YELLOW}To re-release, run: ./Tools/release_tag.sh${NC}"

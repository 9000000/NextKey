_claude_with_profile() {
  export CLAUDE_CONFIG_DIR="$1"
  command claude "${@:2}"
}

# Personal profile (default)
claude() {
  _claude_with_profile "$HOME/.claude" "$@"
}

# Work profile
wclaude() {
  _claude_with_profile "$HOME/.claude-work" "$@"
}

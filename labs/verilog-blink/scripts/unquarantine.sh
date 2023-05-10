find $1 -type f -exec sh -c '
  xattr -dr com.apple.quarantine "$0"
' {} \;

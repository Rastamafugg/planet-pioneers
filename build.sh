#!/bin/bash
set -euo pipefail

IMAGE_NAME="ppsrc.dsk"

SOURCE="${BASH_SOURCE[0]}"
while [ -L "$SOURCE" ]; do
  DIR="$(cd -P "$(dirname "$SOURCE")" >/dev/null 2>&1 && pwd)"
  SOURCE="$(readlink "$SOURCE")"
  [[ "$SOURCE" != /* ]] && SOURCE="$DIR/$SOURCE"
done
SCRIPT_DIR="$(cd -P "$(dirname "$SOURCE")" >/dev/null 2>&1 && pwd)"
CSOURCE_DIR="$SCRIPT_DIR/src/c"
SCRIPT_SOURCE_DIR="$SCRIPT_DIR/src/script"
DISK_DIR="$SCRIPT_DIR/disks"
IMAGE_PATH="$DISK_DIR/$IMAGE_NAME"

if ! command -v os9 >/dev/null 2>&1; then
  echo "ERROR: ToolShed os9 utility not found on PATH."
  exit 1
fi

if [ ! -d "$CSOURCE_DIR" ]; then
  echo "ERROR: C source directory not found: $CSOURCE_DIR"
  exit 1
fi

if [ ! -d "$SCRIPT_SOURCE_DIR" ]; then
  echo "ERROR: Script directory not found: $SCRIPT_SOURCE_DIR"
  exit 1
fi

mkdir -p "$DISK_DIR"

if [ ! -f "$IMAGE_PATH" ]; then
  echo "Creating image: $IMAGE_PATH"
  os9 format "$IMAGE_PATH" -ds -t80 -e -9
  echo "Setting image attributes: $IMAGE_PATH"
  os9 attr "$IMAGE_PATH" -ews
fi

if ! os9 dir "$IMAGE_PATH" >/dev/null 2>&1; then
  echo "ERROR: '$IMAGE_PATH' is not a valid OS-9 image."
  exit 1
fi

echo "Copying DCC C sources from $CSOURCE_DIR to $IMAGE_PATH root"
find "$CSOURCE_DIR" -type f -name "*.c" | sort | while read -r file; do
  file_name="$(basename "$file")"
  echo "  Copying $file_name"
  os9 copy -l -r "$file" "$IMAGE_PATH,$file_name"
done

echo "Copying NitrOS-9 procedure files from $SCRIPT_SOURCE_DIR to $IMAGE_PATH"
find "$SCRIPT_SOURCE_DIR" -type f | sort | while read -r file; do
  relative_path="${file#$SCRIPT_SOURCE_DIR/}"
  relative_dir="$(dirname "$relative_path")"
  if [ "$relative_dir" != "." ]; then
    os9 makdir "$IMAGE_PATH,$relative_dir" >/dev/null 2>&1 || true
  fi
  echo "  Copying $relative_path"
  os9 copy -l -r "$file" "$IMAGE_PATH,$relative_path"
  os9 attr "$IMAGE_PATH,$relative_path" -e -pe
done

echo "Build complete: $IMAGE_PATH"

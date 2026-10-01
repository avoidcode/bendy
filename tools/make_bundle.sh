#!/bin/bash
#
# Assemble a ready-to-copy BENDY plugin folder for FL Studio on macOS.
#
# This script only *produces* a folder in the repository tree. It never copies
# anything into FL Studio's plugin directory; that part is left to you.
#
# Usage:
#   tools/make_bundle.sh [output_dir] [--build-dir DIR]
#
# Defaults:
#   output_dir = <repo>/dist        -> creates <repo>/dist/BENDY/
#   build_dir  = <repo>/build-macos
#
# The resulting folder contains:
#   BENDY/BENDY_x64.dylib
#   BENDY/bmp*.png

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

OUTPUT_DIR="${REPO_ROOT}/dist"
BUILD_DIR="${REPO_ROOT}/build-macos"

while [[ $# -gt 0 ]]; do
	case "$1" in
		--build-dir)
			BUILD_DIR="$2"
			shift 2
			;;
		-h|--help)
			sed -n '2,20p' "${BASH_SOURCE[0]}"
			exit 0
			;;
		*)
			OUTPUT_DIR="$1"
			shift
			;;
	esac
done

if [[ "$(uname -s)" != "Darwin" ]]; then
	echo "error: this script builds a macOS plugin and must run on macOS." >&2
	exit 1
fi

find_dylib() {
	local candidates=(
		"${BUILD_DIR}/BENDY_x64.dylib"
		"${BUILD_DIR}/Release/BENDY_x64.dylib"
		"${BUILD_DIR}/Debug/BENDY_x64.dylib"
	)
	local f
	for f in "${candidates[@]}"; do
		if [[ -f "$f" ]]; then
			echo "$f"
			return 0
		fi
	done
	return 1
}

DYLIB="$(find_dylib || true)"
if [[ -z "${DYLIB}" ]]; then
	echo "No BENDY_x64.dylib found in ${BUILD_DIR}; configuring and building..."
	cmake -S "${REPO_ROOT}" -B "${BUILD_DIR}"
	cmake --build "${BUILD_DIR}" --config Release
	DYLIB="$(find_dylib || true)"
fi

if [[ -z "${DYLIB}" ]]; then
	echo "error: build finished but BENDY_x64.dylib could not be located in ${BUILD_DIR}." >&2
	exit 1
fi

BUNDLE_DIR="${OUTPUT_DIR}/BENDY"
RESOURCES_TMP="$(mktemp -d)"
trap 'rm -rf "${RESOURCES_TMP}"' EXIT

echo "Generating bitmaps..."
python3 "${REPO_ROOT}/tools/build_resources.py" \
	-in "${REPO_ROOT}/assets" \
	-out "${RESOURCES_TMP}" \
	-scale 3

echo "Assembling ${BUNDLE_DIR}..."
rm -rf "${BUNDLE_DIR}"
mkdir -p "${BUNDLE_DIR}"

cp "${DYLIB}" "${BUNDLE_DIR}/BENDY_x64.dylib"
cp "${RESOURCES_TMP}"/bmp*.png "${BUNDLE_DIR}/"

echo
echo "Done. Ready-to-copy plugin folder:"
echo "  ${BUNDLE_DIR}"
echo
echo "Copy it into FL Studio's Fruity plugin folder, e.g.:"
echo "  /Applications/FL Studio.app/Contents/Resources/FL/Plugins/Fruity/Effects/BENDY"

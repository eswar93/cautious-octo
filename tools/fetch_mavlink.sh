#!/usr/bin/env sh
# Fetch the generated MAVLink 2 C headers (common dialect) into third_party/.
set -e
cd "$(dirname "$0")/.."
mkdir -p third_party/mavlink
[ -d third_party/mavlink/c_library_v2 ] || git clone --depth 1 https://github.com/mavlink/c_library_v2.git third_party/mavlink/c_library_v2
echo "MAVLink headers ready."

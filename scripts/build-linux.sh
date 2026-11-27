#!/usr/bin/env bash
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cmake -S "$root/chatserver" -B "$root/chatserver/build" \
    -DCMAKE_BUILD_TYPE=Debug -DMUDUO_ROOT="$root/.deps/muduo-install"
cmake --build "$root/chatserver/build" --parallel 2
printf '\nExecutables:\n%s\n%s\n' "$root/chatserver/bin/ChatServer" "$root/chatserver/bin/ChatClient"

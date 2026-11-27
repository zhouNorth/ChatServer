#!/usr/bin/env bash
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ $(id -u) -eq 0 ]]; then
    sudo_cmd=()
else
    sudo_cmd=(sudo)
fi
"${sudo_cmd[@]}" apt-get update
"${sudo_cmd[@]}" apt-get install -y build-essential cmake git libboost-dev libmysqlclient-dev libhiredis-dev redis-server nginx libnginx-mod-stream gdb python3
deps="$root/.deps"
mkdir -p "$deps"
if [[ ! -d "$deps/muduo/.git" ]]; then
    git clone --depth 1 --branch v2.0.2 https://github.com/chenshuo/muduo.git "$deps/muduo"
fi
cmake -S "$deps/muduo" -B "$deps/muduo-build" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DCMAKE_BUILD_TYPE=Release -DMUDUO_BUILD_EXAMPLES=OFF \
    -DCMAKE_INSTALL_PREFIX="$deps/muduo-install"
cmake --build "$deps/muduo-build" --target muduo_base muduo_net --parallel 2
mkdir -p "$deps/muduo-install/lib" "$deps/muduo-install/include"
cp "$deps/muduo-build/lib/libmuduo_base.a" "$deps/muduo-build/lib/libmuduo_net.a" "$deps/muduo-install/lib/"
(
    cd "$deps/muduo"
    find muduo/base muduo/net -name '*.h' -exec cp --parents {} "$deps/muduo-install/include/" \;
)
bash "$root/scripts/build-linux.sh"

#!/usr/bin/env bash
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"
mkdir -p .run
# Redis and the shared MySQL database must already be available.
# Inherit CHAT_DB_* and CHAT_REDIS_* from the caller, never hardcode credentials here.
children=()
nginx_started=0
cleanup() {
    if [[ $nginx_started == 1 ]]; then
        nginx -p "$root/.run/" -c "$root/deploy/nginx.conf" -s quit || true
    fi
    for pid in "${children[@]}"; do kill -TERM "$pid" 2>/dev/null || true; done
    for pid in "${children[@]}"; do wait "$pid" 2>/dev/null || true; done
}
trap cleanup EXIT
trap 'exit 0' INT TERM
nginx -t -p "$root/.run/" -c "$root/deploy/nginx.conf"
"$root/chatserver/bin/ChatServer" 127.0.0.1 6001 >.run/server-6001.log 2>&1 &
children+=("$!")
"$root/chatserver/bin/ChatServer" 127.0.0.1 6002 >.run/server-6002.log 2>&1 &
children+=("$!")
sleep 1
for pid in "${children[@]}"; do kill -0 "$pid"; done
nginx -p "$root/.run/" -c "$root/deploy/nginx.conf"
nginx_started=1
echo 'Cluster running: client -> 127.0.0.1:8000 -> ChatServer 6001/6002'
echo 'Ctrl+C stops the child servers and this Nginx instance.'
# Exit and clean up the other server if either server terminates.
wait -n "${children[@]}"

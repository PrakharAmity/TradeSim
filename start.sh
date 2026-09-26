#!/usr/bin/env bash
cd "$(dirname "$0")"

mkdir -p build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target tradesim -j2

PORT=8080 ./build/tradesim &
SERVER_PID=$!

cleanup() {
    kill -TERM "$SERVER_PID" 2>/dev/null || true
    exit 0
}
trap cleanup SIGTERM SIGINT EXIT

echo "[TradeSim] Server started (PID: $SERVER_PID) with live-reload watching src/ and include/..."

get_snapshot() {
    find src include -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) -exec stat -c "%Y %n" {} + 2>/dev/null | sort
}

LAST_SNAPSHOT=$(get_snapshot)

while true; do
    sleep 1.5
    CURRENT_SNAPSHOT=$(get_snapshot)
    if [ "$CURRENT_SNAPSHOT" != "$LAST_SNAPSHOT" ]; then
        echo "[TradeSim] Detected code changes. Rebuilding tradesim..."
        if cmake --build build --target tradesim -j2; then
            echo "[TradeSim] Build succeeded. Restarting server..."
            kill -TERM "$SERVER_PID" 2>/dev/null || true
            wait "$SERVER_PID" 2>/dev/null || true
            PORT=8080 ./build/tradesim &
            SERVER_PID=$!
            echo "[TradeSim] Server restarted (PID: $SERVER_PID)."
        else
            echo "[TradeSim] Build failed. Keeping previous server running until syntax errors are fixed."
        fi
        LAST_SNAPSHOT=$(get_snapshot)
    fi
done
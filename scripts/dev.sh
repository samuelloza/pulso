#!/usr/bin/env bash
# Compila pulso y levanta el stack de observabilidad (Pushgateway + Prometheus
# + Grafana), luego arranca el agente en primer plano.
#
#   scripts/dev.sh          compila + stack + agente (Ctrl+C detiene el agente)
#   scripts/dev.sh build    solo compila
#   scripts/dev.sh stack    solo levanta el stack docker
#   scripts/dev.sh down     detiene el stack (y borra sus volúmenes)
set -euo pipefail

cd "$(dirname "$0")/.."
ROOT="$PWD"
COMPOSE="docker compose -f deploy/docker-compose.yml"

need() { command -v "$1" >/dev/null 2>&1 || { echo "falta '$1' en el PATH"; exit 1; }; }

build() {
    need cmake
    cmake -S "$ROOT" -B "$ROOT/build"
    cmake --build "$ROOT/build" -j"$(nproc 2>/dev/null || echo 4)"
    echo "binario: $ROOT/build/bin/pulso"
}

stack_up() {
    need docker
    $COMPOSE up -d
    printf 'esperando al Pushgateway'
    for _ in $(seq 1 30); do
        curl -sf http://localhost:9091/-/healthy >/dev/null 2>&1 && break
        printf '.'; sleep 1
    done
    echo
    echo "  Pushgateway: http://localhost:9091"
    echo "  Prometheus:  http://localhost:9090"
    echo "  Grafana:     http://localhost:9393  (admin / admin)"
}

case "${1:-all}" in
    build) build ;;
    stack) stack_up ;;
    down)  need docker; $COMPOSE down -v ;;
    all)
        build
        stack_up
        echo "--- arrancando agente (Ctrl+C para detener; el stack sigue) ---"
        exec "$ROOT/build/bin/pulso" --config "$ROOT/pulso.toml"
        ;;
    *) echo "uso: $0 [build|stack|down]"; exit 1 ;;
esac

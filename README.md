# Pulso

[![Build](https://github.com/sis-inf/pulso/actions/workflows/build.yml/badge.svg)](https://github.com/sis-inf/pulso/actions/workflows/build.yml)
[![CI](https://github.com/sis-inf/pulso/actions/workflows/ci.yml/badge.svg)](https://github.com/sis-inf/pulso/actions/workflows/ci.yml)
[![Licencia MIT](https://img.shields.io/badge/licencia-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)

Agente de monitoreo del sistema en C++17 para **Linux**. Muestrea métricas de
`/proc` y `/sys` cada N segundos y las envía por **push** a un Prometheus
Pushgateway. No persiste nada localmente.

```
[ pulso @ máquina 1 ] ──push──┐
[ pulso @ máquina 2 ] ──push──┤──▶ [ Pushgateway ] ◀──scrape── [ Prometheus ] ◀── [ Grafana ]
[ pulso @ máquina N ] ──push──┘
```

## Inicio rápido

```bash
sudo apt install git cmake g++
git clone https://github.com/sis-inf/pulso.git && cd pulso
cmake -S . -B build && cmake --build build

# Lectura única a stdout (no necesita Pushgateway)
./build/bin/pulso --once --format json

# Modo agente: muestrea y hace push según pulso.toml
./build/bin/pulso --config pulso.toml
```

Las dependencias (cpp-httplib, nlohmann/json, toml++) se descargan solas con
CMake FetchContent. OpenSSL es opcional y habilita `https://` en el push.

## Descargas

Binario para Linux x86_64 ya compilado (enlace directo, sin login):

- **Última build de `main`** — se regenera en cada push (pre-release rodante
  con tag `latest`):
  [página](https://github.com/sis-inf/pulso/releases/tag/latest) ·
  [`pulso-latest-linux-x86_64.tar.gz`](https://github.com/sis-inf/pulso/releases/download/latest/pulso-latest-linux-x86_64.tar.gz)
- **Versiones estables** — cada tag `vX.Y.Z` publica su
  `pulso-vX.Y.Z-linux-x86_64.tar.gz` (con `.sha256`) en
  [Releases](https://github.com/sis-inf/pulso/releases).
- **Artefacto de una corrida concreta** (requiere login): pestaña
  [Actions → Build](https://github.com/sis-inf/pulso/actions/workflows/build.yml),
  retención 90 días.

```bash
curl -fsSLO https://github.com/sis-inf/pulso/releases/download/latest/pulso-latest-linux-x86_64.tar.gz
tar -xzf pulso-latest-linux-x86_64.tar.gz
./pulso-latest-linux-x86_64/pulso --once
```

## Uso

```
pulso [opciones]

  --config <path>   Archivo de configuración (default: pulso.toml)
  --interval <ms>   Intervalo de lectura
  --metrics <list>  Métricas a recolectar: cpu,ram,disk
  --once            Ejecutar una sola lectura y salir
  --format <fmt>    Formato de salida con --once: json | csv | prometheus
  -h, --help        Mostrar ayuda
  --version         Mostrar versión
```

Sin `--once`, `pulso` corre como agente: cada `intervalo_segundos` recolecta un
snapshot, lo empuja al Pushgateway y expone un servidor HTTP local:

| Endpoint | Descripción |
|---|---|
| `GET /health` | Estado y uptime del agente (JSON) |
| `GET /version` | Versión del agente (JSON) |
| `GET /metrics/prometheus` | Última muestra en RAM, formato Prometheus (pull / depuración) |

Si `pushgateway.url` está vacío, el agente solo expone `/metrics/prometheus`
(modo pull puro).

## Configuración

`pulso.toml` — todas las claves tienen valor por defecto:

```toml
[servidor]              # servidor HTTP local del agente
host = "0.0.0.0"
puerto = 8080

[sampler]
intervalo_segundos = 10  # recomendado 10-60

[procesos]
activo = true            # métricas por programa, agrupadas por nombre
uid_minimo = 1000        # ignora daemons de sistema / root

[pushgateway]
url = "http://localhost:9091"  # "" desactiva el push
job = "pulso"
instance = ""                  # vacío = hostname de la máquina
token = ""                     # Bearer opcional (lo valida un reverse proxy)
tls_skip_verify = false        # aceptar certificados self-signed

nivel_log = "info"             # debug | info | warn | error
output_format = "json"         # json | csv | prometheus
```

Variables de entorno equivalentes (tienen prioridad sobre el archivo):
`PULSO_SERVIDOR_HOST`, `PULSO_SERVIDOR_PUERTO`, `PULSO_SAMPLER_INTERVALO_SEGUNDOS`,
`PULSO_PUSHGATEWAY_URL`, `PULSO_PUSHGATEWAY_JOB`, `PULSO_PUSHGATEWAY_INSTANCE`,
`PULSO_PUSHGATEWAY_TOKEN`, `PULSO_NIVEL_LOG`, `PULSO_OUTPUT_FORMAT`.

## Métricas

CPU (total y por núcleo), memoria + swap, disco por punto de montaje + I/O por
dispositivo, red por interfaz (con errores/descartes), carga, procesos,
uptime e info del host, temperaturas, batería, y auto-observabilidad del propio
agente. En Prometheus el nombre lleva prefijo `pulso_` y los `.` pasan a `_`
(`cpu.usage` → `pulso_cpu_usage`).

Detalle completo: [metricas-disponibles.md](metricas-disponibles.md).

## Stack de observabilidad

`deploy/` trae un `docker-compose.yml` con Pushgateway (`:9091`),
Prometheus (`:9090`) y Grafana (`:9393`, `admin`/`admin`, dashboard
*Pulso / Vista general*):

```bash
cd deploy && docker compose up -d
```

Guía: [deploy/README.md](deploy/README.md). Atajo de desarrollo
(compila + levanta el stack + arranca el agente): `scripts/dev.sh`.

## Compilación

```bash
cmake -S . -B build [-DCMAKE_BUILD_TYPE=Release]
cmake --build build -j"$(nproc)"
```

| Opción CMake | Efecto |
|---|---|
| `-DBUILD_TESTS=ON` | Compila la suite de tests (GoogleTest vía FetchContent) |
| `-DBUILD_WITH_ASAN=ON` | AddressSanitizer en los tests (requiere `BUILD_TESTS=ON`) |
| `-DPULSO_STATIC_LINK=ON` | Enlaza `libgcc`/`libstdc++` estáticos (binario portable) |

También hay un `Makefile` para entornos sin CMake (`make`, `make CXX=clang++`).

## Tests

```bash
cmake -S . -B build -DBUILD_TESTS=ON && cmake --build build
ctest --test-dir build --output-on-failure
```

Un test concreto: `ctest --test-dir build -R Prometheus`, o el binario directo
en `build/bin/` (`test_types`, `test_config`, `test_ram_usage`,
`test_disk_usage`, `test_formatter_prometheus`). Ver [tests/README.md](tests/README.md).

## Documentación

- [Índice general](docs/indice-general.md)
- [Guía de instalación](docs/instalacion.md)
- [Contribuir](CONTRIBUTING.md)

## Licencia

MIT — ver [LICENSE](LICENSE).

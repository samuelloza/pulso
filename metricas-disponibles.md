# Métricas disponibles en Pulso

Cada muestreo produce un `Snapshot` con estas métricas. En Prometheus el nombre
lleva el prefijo `pulso_` y los `.` se vuelven `_` (ej: `cpu.usage` →
`pulso_cpu_usage`). Todas se exponen como `gauge`; las marcadas *(contador)* son
acumuladas desde el arranque del sistema — usar `rate()` en Prometheus.

## CPU — `src/platform/linux/collector_cpu.cpp` (`/proc/stat`)

| Métrica | Etiquetas | Unidad | Descripción |
|---|---|---|---|
| `cpu.usage` | — | % | Uso agregado en el intervalo de muestreo |
| `cpu.usage` | `core` | % | Uso por núcleo |
| `cpu.cores` | — | cantidad | Núcleos detectados |
| `cpu.time_seconds` *(contador)* | `mode` (user, nice, system, idle, iowait, irq, softirq, steal) | segundos | Tiempo de CPU acumulado por modo |

## Memoria — `src/collectors/memory/ram_usage.cpp` (`/proc/meminfo`)

| Métrica | Unidad | Descripción |
|---|---|---|
| `memory.total_bytes` | bytes | RAM total |
| `memory.available_bytes` | bytes | RAM disponible (MemAvailable) |
| `memory.free_bytes` | bytes | RAM libre (MemFree) |
| `memory.used_bytes` | bytes | `total - available` |
| `memory.buffers_bytes` | bytes | Buffers |
| `memory.cached_bytes` | bytes | Page cache |
| `swap.total_bytes` / `swap.free_bytes` / `swap.used_bytes` | bytes | Swap |

## Disco — `src/platform/linux/collector_disk.cpp`

Espacio por punto de montaje real (`/proc/self/mountinfo` + `statvfs`; se
excluyen fs virtuales y montajes bajo `/sys`, `/proc`, `/dev`, `/run`, `/snap`):

| Métrica | Etiquetas | Unidad |
|---|---|---|
| `disk.total_bytes` / `disk.used_bytes` / `disk.free_bytes` | `mount`, `fstype` | bytes |
| `disk.used_ratio` | `mount`, `fstype` | ratio (0–1) |

I/O por dispositivo (`/proc/diskstats`; se excluyen `loop*`, `ram*`, `dm-*`):

| Métrica | Etiquetas | Unidad |
|---|---|---|
| `disk.reads_completed` / `disk.writes_completed` *(contador)* | `device` | cantidad |
| `disk.read_bytes` / `disk.written_bytes` *(contador)* | `device` | bytes |

## Red — `src/platform/linux/collector_network.cpp` (`/proc/net/dev`)

Por interfaz (se excluyen `lo` y `veth*`). Todas *(contador)*:

| Métrica | Etiquetas | Unidad |
|---|---|---|
| `network.rx_bytes` / `network.tx_bytes` | `interface` | bytes |
| `network.rx_packets` / `network.tx_packets` | `interface` | cantidad |
| `network.rx_errors` / `network.tx_errors` | `interface` | cantidad |
| `network.rx_dropped` / `network.tx_dropped` | `interface` | cantidad |

## Carga y procesos — `src/platform/linux/collector_carga.cpp`

| Métrica | Unidad | Fuente |
|---|---|---|
| `load.avg1` / `load.avg5` / `load.avg15` | carga | `/proc/loadavg` |
| `procesos.total` | cantidad | `/proc/loadavg` |
| `procesos.running` / `procesos.blocked` | cantidad | `/proc/stat` |
| `procesos.zombie` | cantidad | escaneo de `/proc/<pid>/stat` |

## Sistema y agente — `src/platform/linux/collector_sistema.cpp`

| Métrica | Etiquetas | Descripción |
|---|---|---|
| `system.uptime_seconds` | — | Uptime del sistema |
| `system.boot_time_seconds` | — | Timestamp Unix del arranque |
| `system.info` (=1) | `hostname`, `kernel`, `os`, `arch` | Info del host |
| `pulso.build_info` (=1) | `version` | Versión del agente |

## Temperatura — `src/platform/linux/collector_temperatura.cpp`

| Métrica | Etiquetas | Unidad | Fuente |
|---|---|---|---|
| `temperature.celsius` | `chip`, `sensor` | °C | `/sys/class/hwmon`, fallback `/sys/class/thermal` |

Sin sensores accesibles no emite nada (no falla).

## Batería — `src/collectors/bateria/bateria_collector.cpp`

| Métrica | Unidad | Notas |
|---|---|---|
| `bateria.porcentaje` | % | Solo si hay batería (`/sys/class/power_supply/BAT0`) |
| `bateria.cargando` | booleano | 1 = "Charging" |

## Auto-observabilidad (añadidas por el sampler)

| Métrica | Unidad | Descripción |
|---|---|---|
| `pulso.sample_duration_seconds` | segundos | Cuánto tardó el ciclo de muestreo |
| `pulso.collector_errors` | cantidad | Colectores que fallaron en el ciclo |
| `pulso.up` (=1) | booleano | El agente ejecutó un ciclo |

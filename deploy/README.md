# Stack de observabilidad de Pulso

Arquitectura **push**: cada agente `pulso` no persiste nada localmente; en cada
muestreo hace `PUT` de sus métricas al **Pushgateway**. Prometheus scrapea el
Pushgateway y almacena las series; Grafana grafica.

```
[ pulso @ maquina 1 ] ──push──┐
[ pulso @ maquina 2 ] ──push──┤──▶ [ Pushgateway ] ◀──scrape── [ Prometheus ] ◀── [ Grafana ]
[ pulso @ maquina N ] ──push──┘
```

## Uso

1. Levantar el stack central:

   ```bash
   cd deploy
   docker compose up -d
   ```

   - Pushgateway: <http://localhost:9091>
   - Prometheus:  <http://localhost:9090>  (Status → Targets: `pushgateway` UP)
   - Grafana:     <http://localhost:9393>  (`admin` / `admin`) → dashboard *Pulso / Vista general*

   > La UI de Grafana está mapeada al puerto **9393** del host
   > (`ports: "9393:3000"`). Si el stack corre en otra máquina, accedés por
   > `http://<IP-DEL-STACK>:9393/` y debés ajustar `GF_SERVER_ROOT_URL` en
   > `docker-compose.yml` (o exportá esa env var antes de `compose up`).

2. En cada máquina a monitorear, compilar y correr el agente:

   ```bash
   cmake -S . -B build && cmake --build build
   ./build/bin/pulso --config pulso.toml
   ```

   Configurar el destino en `pulso.toml`:

   ```toml
   [pushgateway]
   url = "http://<IP-DEL-STACK>:9091"   # localhost si es la misma máquina
   job = "pulso"
   instance = ""                        # vacío = usa el hostname
   ```

   o por variables de entorno: `PULSO_PUSHGATEWAY_URL`, `PULSO_PUSHGATEWAY_JOB`,
   `PULSO_PUSHGATEWAY_INSTANCE`.

## Notas

- `honor_labels: true` en `prometheus.yml` hace que Prometheus respete las
  etiquetas `job`/`instance` que puso cada agente (si no, las pisaría con las del
  Pushgateway).
- El agente además expone `http://<agente>:8080/metrics/prometheus` con la última
  muestra en RAM — útil para `curl` de depuración, o para modo **pull** si dejás
  `pushgateway.url = ""`.
- Métricas expuestas: ver `../metricas-disponibles.md` (CPU total y por núcleo,
  memoria + swap, disco por montaje + I/O por dispositivo, red por interfaz con
  errores/descartes, carga, procesos, uptime, temperaturas, info del host,
  auto-observabilidad del agente).
- Si un agente se apaga, su serie queda "stale" en el Pushgateway. Para limpiarla:
  `curl -X DELETE http://localhost:9091/metrics/job/pulso/instance/<host>`.

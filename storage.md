# Almacenamiento: no hay (por diseño)

El agente `pulso` **no persiste métricas localmente**. En cada muestreo:

1. Ejecuta los colectores y arma un `Snapshot`.
2. Lo serializa en formato de exposición Prometheus.
3. Hace `PUT /metrics/job/<job>/instance/<instance>` al **Pushgateway**
   configurado en `[pushgateway]` de `pulso.toml`.
4. Guarda ese snapshot como "última muestra" **solo en memoria**
   (`src/core/muestra_actual.hpp`), para poder servir `/metrics/prometheus`
   local con fines de depuración / modo pull.

El almacenamiento de series temporales vive en el **colector central**
(Prometheus, que scrapea el Pushgateway). Ver `deploy/`.

## Consecuencias

- Si el agente se reinicia, no pierde histórico: el histórico está en Prometheus.
- Si el Pushgateway está caído, `push()` registra un warning y el agente sigue
  muestreando; el siguiente ciclo reintenta. No hay buffer local.
- No hay archivos `.db` ni dependencia de SQLite.

## Modo pull alternativo

Si `pushgateway.url` está vacío, el agente no hace push y queda como *exporter*
clásico: Prometheus (u otro scraper) consulta `http://<agente>:8080/metrics/prometheus`.

#pragma once

#include <stdexcept>
#include <string>

namespace pulso::config {

// ---------------------------------------------------------------------------
// Excepción
// ---------------------------------------------------------------------------

/// Lanzada cuando el archivo de configuración no existe o contiene errores.
class ErrorConfig : public std::runtime_error {
public:
    explicit ErrorConfig(const std::string& mensaje)
        : std::runtime_error(mensaje) {}
};

// ---------------------------------------------------------------------------
// Estructuras de configuración
// ---------------------------------------------------------------------------
/// Configuración del monitor del sistema.
struct MonitorConfig
{
    /// Intervalo de actualización de métricas en milisegundos.
    int interval_ms = 1000;

    /// Habilita la lectura de métricas de CPU.
    bool cpu = true;

    /// Habilita la lectura de métricas de RAM.
    bool ram = true;

    /// Habilita la lectura de métricas de disco.
    bool disk = true;

    /// Constructor con valores por defecto.
    MonitorConfig() = default;
};
struct ConfigServidor {
    std::string host  = "0.0.0.0";
    int         puerto = 8080;
};

struct ConfigSampler {
    int intervalo_segundos = 10;
};

struct ConfigProcesos {
    /// Recolectar métricas de procesos agrupadas por nombre. Ojo cardinalidad:
    /// ~1 serie por nombre de programa distinto por máquina.
    bool activo = true;
    /// Solo procesos cuyo dueño tenga UID >= este valor (1000 = usuarios reales,
    /// deja fuera daemons de sistema y root).
    int64_t uid_minimo = 1000;
};

struct ConfigPushgateway {
    /// URL base del Pushgateway o del reverse proxy (http:// o https://).
    /// Vacío = no hacer push.
    std::string url = "http://localhost:9091";
    /// Etiqueta de grouping `job`.
    std::string job = "pulso";
    /// Etiqueta de grouping `instance`. Vacío = usar el hostname.
    std::string instance = "";
    /// Bearer token que envía el agente (header `Authorization: Bearer <token>`).
    /// Vacío = sin header. Lo valida el reverse proxy delante del Pushgateway.
    std::string token = "";
    /// Aceptar certificados TLS self-signed (útil si el proxy usa un cert propio).
    bool tls_skip_verify = false;
};

struct Config {
    ConfigServidor     servidor;
    ConfigSampler      sampler;
    ConfigProcesos     procesos;
    ConfigPushgateway  pushgateway;
    std::string        nivel_log = "info";
    /// Formato de salida de métricas.
    /// Valores válidos: json, csv, prometheus
    std::string    output_format = "json";
};

// ---------------------------------------------------------------------------
// API pública
// ---------------------------------------------------------------------------

/// Carga el archivo TOML ubicado en `ruta`.
/// Lanza ErrorConfig si la ruta no existe o el archivo es inválido.
Config cargar(const std::string& ruta);

/// Devuelve una Config construida con todos los valores por defecto.
Config porDefecto();

} // namespace pulso::config
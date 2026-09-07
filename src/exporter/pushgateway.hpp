#pragma once

#include <string>

#include "core/types.hpp"

namespace pulso::exporter {

/**
 * @brief Cliente de Prometheus Pushgateway (directo o vía reverse proxy).
 *
 * En cada muestreo hace `PUT /metrics/job/<job>/instance/<instance>` con el
 * snapshot serializado en formato de exposición Prometheus. El PUT reemplaza
 * todo el grupo {job, instance}, así que cada envío es el estado completo.
 *
 * Soporta `http://` y `https://`. Si `token` no está vacío, agrega el header
 * `Authorization: Bearer <token>` (lo valida el reverse proxy, no el Pushgateway,
 * que no tiene auth propia).
 *
 * No persiste nada localmente. Si el destino no responde, `push()` registra un
 * warning y devuelve false; el agente sigue muestreando.
 */
class Pushgateway {
public:
    struct Opciones {
        std::string url;             // "http(s)://host:puerto" o el del proxy
        std::string job;
        std::string instance;
        std::string token;           // vacío = sin header Authorization
        bool tls_skip_verify = false;
    };

    explicit Pushgateway(Opciones opts);

    /// @return true si el destino respondió 2xx.
    bool push(const pulso::core::Snapshot& snapshot) const;

    const std::string& destino() const { return path_; }

private:
    Opciones opts_;
    std::string path_;
};

} // namespace pulso::exporter

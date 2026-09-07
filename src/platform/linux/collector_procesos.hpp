#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../../collectors/icollector.hpp"
#include "../../core/types.hpp"

namespace pulso::collectors {

/**
 * @brief Procesos del usuario agrupados por nombre de programa (`comm`).
 *
 * Nunca emite una serie por PID (explotaría la cardinalidad). Por cada nombre
 * distinto de programa cuyo dueño tenga UID >= `uid_minimo`:
 *
 *  - proceso.instancias{comm}           cuántos procesos con ese nombre
 *  - proceso.start_time_seconds{comm}   arranque de la instancia más antigua
 *                                       (en Grafana: time() - esto = tiempo abierto)
 *  - proceso.rss_bytes{comm}            memoria residente sumada
 *  - proceso.cpu_seconds{comm}          CPU (user+sys) sumada, acumulada
 */
class CollectorProcesos : public ICollector {
public:
    explicit CollectorProcesos(std::int64_t uid_minimo) : uid_minimo_(uid_minimo) {}

    std::string nombre() const override;
    std::vector<pulso::core::Metrica> recolectar() override;

private:
    std::int64_t uid_minimo_;
};

} // namespace pulso::collectors

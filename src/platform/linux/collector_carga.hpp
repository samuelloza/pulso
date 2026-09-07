#pragma once

#include <string>
#include <vector>

#include "../../collectors/icollector.hpp"
#include "../../core/types.hpp"

namespace pulso::collectors {

/**
 * @brief Carga del sistema y conteo de procesos.
 *
 *  - load.avg1 / load.avg5 / load.avg15        (/proc/loadavg)
 *  - procesos.total                            (/proc/loadavg)
 *  - procesos.running / procesos.blocked       (/proc/stat)
 *  - procesos.zombie                           (escaneo de /proc/<pid>/stat)
 */
class CollectorCarga : public ICollector {
public:
    std::string nombre() const override;
    std::vector<pulso::core::Metrica> recolectar() override;
};

} // namespace pulso::collectors

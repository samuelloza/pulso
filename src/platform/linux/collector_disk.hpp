#pragma once

#include <string>
#include <vector>

#include "../../collectors/icollector.hpp"
#include "../../core/types.hpp"

namespace pulso::collectors {

/**
 * @brief Métricas de disco.
 *
 * Espacio por punto de montaje real (statvfs sobre /proc/self/mountinfo):
 *  - disk.total_bytes / disk.used_bytes / disk.free_bytes / disk.used_ratio
 *    con etiquetas {mount, fstype}
 *
 * I/O por dispositivo (/proc/diskstats), contadores acumulados:
 *  - disk.reads_completed / disk.writes_completed / disk.read_bytes /
 *    disk.written_bytes  con etiqueta {device}
 */
class CollectorDisco : public ICollector {
public:
    std::string nombre() const override;
    std::vector<pulso::core::Metrica> recolectar() override;
};

} // namespace pulso::collectors

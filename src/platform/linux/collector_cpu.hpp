#pragma once

#include <string>
#include <vector>

#include "../../collectors/icollector.hpp"
#include "../../core/types.hpp"

namespace pulso::collectors {

/**
 * @brief Métricas de CPU desde /proc/stat.
 *
 * Emite:
 *  - cpu.usage                       uso agregado (%)
 *  - cpu.usage{core="N"}             uso por núcleo (%)
 *  - cpu.cores                       número de núcleos
 *  - cpu.time_seconds{mode="..."}    contadores acumulados por modo
 */
class CollectorCPU : public ICollector {
public:
    std::string nombre() const override;
    std::vector<pulso::core::Metrica> recolectar() override;
};

} // namespace pulso::collectors

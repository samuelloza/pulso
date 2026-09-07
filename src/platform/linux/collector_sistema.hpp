#pragma once

#include <string>
#include <vector>

#include "../../collectors/icollector.hpp"
#include "../../core/types.hpp"

namespace pulso::collectors {

/**
 * @brief Datos del sistema y del propio agente.
 *
 *  - system.uptime_seconds / system.boot_time_seconds
 *  - system.info{hostname,kernel,os,arch} = 1   (info metric)
 *  - pulso.build_info{version} = 1
 */
class CollectorSistema : public ICollector {
public:
    std::string nombre() const override;
    std::vector<pulso::core::Metrica> recolectar() override;
};

} // namespace pulso::collectors

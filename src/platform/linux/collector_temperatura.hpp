#pragma once

#include <string>
#include <vector>

#include "../../collectors/icollector.hpp"
#include "../../core/types.hpp"

namespace pulso::collectors {

/**
 * @brief Temperaturas de /sys/class/hwmon (fallback: /sys/class/thermal).
 *
 *  - temperature.celsius{chip,sensor}
 *
 * Sin sensores accesibles devuelve vector vacío (no lanza).
 */
class CollectorTemperatura : public ICollector {
public:
    std::string nombre() const override;
    std::vector<pulso::core::Metrica> recolectar() override;
};

} // namespace pulso::collectors

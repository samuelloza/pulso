#pragma once

#include <mutex>
#include <optional>

#include "core/types.hpp"

namespace pulso::core {

/**
 * @brief Guarda el último Snapshot recolectado, en memoria, sin persistir.
 *
 * El sampler escribe con set(); el handler HTTP /metrics/prometheus lee con
 * get(). No hay base de datos: el almacenamiento vive en el colector central
 * (Prometheus vía Pushgateway).
 */
class MuestraActual {
public:
    void set(Snapshot s) {
        std::lock_guard<std::mutex> lk(mtx_);
        ultima_ = std::move(s);
    }

    std::optional<Snapshot> get() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return ultima_;
    }

private:
    mutable std::mutex mtx_;
    std::optional<Snapshot> ultima_;
};

} // namespace pulso::core

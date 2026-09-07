#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "../collectors/icollector.hpp"
#include "../core/types.hpp"

namespace pulso::sampler {

/// Función que recibe cada Snapshot recolectado (ej: enviarlo al Pushgateway
/// y/o guardarlo como "última muestra" en memoria).
using Sink = std::function<void(const pulso::core::Snapshot&)>;

class Sampler {
public:
    Sampler(
        std::vector<std::shared_ptr<pulso::collectors::ICollector>> collectors,
        Sink sink,
        int intervaloSegundos);

    ~Sampler();

    /// Inicia el thread de muestreo. No bloquea.
    void iniciar();

    /// Solicita la detención y espera a que el thread termine.
    void detener();

    /// Devuelve true si el sampler está corriendo.
    bool corriendo() const;

private:
    void cicloUnico();

    std::vector<std::shared_ptr<pulso::collectors::ICollector>> collectors_;
    Sink sink_;
    int intervaloSegundos_;
    std::thread thread_;
    std::atomic<bool> corriendo_{false};
    std::condition_variable cv_;
    std::mutex mutex_;
};

} // namespace pulso::sampler

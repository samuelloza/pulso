#include "sampler.hpp"

#include <chrono>
#include <ctime>

#include "../utils/logging/logger.hpp"

namespace pulso::sampler {

Sampler::Sampler(
    std::vector<std::shared_ptr<pulso::collectors::ICollector>> collectors,
    Sink sink,
    int intervaloSegundos)
    : collectors_(std::move(collectors)),
      sink_(std::move(sink)),
      intervaloSegundos_(intervaloSegundos) {}

Sampler::~Sampler() {
    detener();
}

void Sampler::cicloUnico() {
    auto& log = pulso::utils::logging::Logger::instancia();
    const auto t0 = std::chrono::steady_clock::now();

    pulso::core::Snapshot snapshot;
    snapshot.timestamp = std::time(nullptr);

    int errores = 0;
    for (const auto& collector : collectors_) {
        try {
            auto metricas = collector->recolectar();
            snapshot.metricas.insert(snapshot.metricas.end(),
                                     metricas.begin(), metricas.end());
        } catch (const std::exception& e) {
            ++errores;
            log.warn("Collector '" + collector->nombre() +
                     "' falló: " + e.what());
        }
    }

    // Auto-observabilidad del agente.
    const double dur = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - t0).count();
    snapshot.metricas.push_back(
        {"pulso.sample_duration_seconds", dur, "segundos", snapshot.timestamp});
    snapshot.metricas.push_back(
        {"pulso.collector_errors", static_cast<double>(errores),
         "cantidad", snapshot.timestamp});
    snapshot.metricas.push_back(
        {"pulso.up", 1.0, "booleano", snapshot.timestamp});

    try {
        sink_(snapshot);
    } catch (const std::exception& e) {
        log.error("Sink del sampler falló: " + std::string(e.what()));
    }
}

void Sampler::iniciar() {
    if (corriendo_) {
        return;
    }
    corriendo_ = true;

    thread_ = std::thread([this]() {
        while (corriendo_) {
            cicloUnico();

            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock,
                         std::chrono::seconds(intervaloSegundos_),
                         [this]() { return !corriendo_.load(); });
        }
    });
}

void Sampler::detener() {
    if (!corriendo_) {
        return;
    }
    corriendo_ = false;
    cv_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
}

bool Sampler::corriendo() const {
    return corriendo_;
}

} // namespace pulso::sampler

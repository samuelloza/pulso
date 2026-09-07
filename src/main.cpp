#include <unistd.h>

#include <atomic>
#include <chrono>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <httplib.h>

#include "cli/arg_parser.h"
#include "config/config.hpp"
#include "core/muestra_actual.hpp"
#include "core/types.hpp"
#include "core/version.hpp"
#include "exporter/pushgateway.hpp"
#include "sampler/sampler.hpp"
#include "utils/logging/logger.hpp"

#include "formatters/formatter_csv.hpp"
#include "formatters/formatter_json.hpp"
#include "formatters/formatter_prometheus.hpp"

#include "http/handler_health.hpp"
#include "http/handler_version.hpp"

// Colectores (todos Linux, namespace pulso::collectors).
#include "collectors/bateria/bateria_collector.hpp"
#include "collectors/memory/ram_usage.hpp"
#include "platform/linux/collector_carga.hpp"
#include "platform/linux/collector_cpu.hpp"
#include "platform/linux/collector_disk.hpp"
#include "platform/linux/collector_network.hpp"
#include "platform/linux/collector_procesos.hpp"
#include "platform/linux/collector_sistema.hpp"
#include "platform/linux/collector_temperatura.hpp"

extern std::atomic<bool> isRunning;
void setupSignalHandler();

using pulso::collectors::ICollector;
using pulso::utils::logging::Logger;

namespace {

std::vector<std::shared_ptr<ICollector>> construirCollectors(
    const pulso::cli::CliOptions& cli, const pulso::config::Config& cfg)
{
    std::vector<std::shared_ptr<ICollector>> c;
    if (cli.monitor.cpu)
        c.push_back(std::make_shared<pulso::collectors::CollectorCPU>());
    if (cli.monitor.ram)
        c.push_back(std::make_shared<pulso::collectors::memory::CollectorMemory>());
    if (cli.monitor.disk)
        c.push_back(std::make_shared<pulso::collectors::CollectorDisco>());
    // Siempre activos: no tienen flag dedicado y son baratos.
    c.push_back(std::make_shared<pulso::collectors::CollectorRed>());
    c.push_back(std::make_shared<pulso::collectors::CollectorCarga>());
    c.push_back(std::make_shared<pulso::collectors::CollectorSistema>());
    c.push_back(std::make_shared<pulso::collectors::CollectorTemperatura>());
    c.push_back(std::make_shared<pulso::collectors::bateria::CollectorBateria>());
    if (cfg.procesos.activo)
        c.push_back(std::make_shared<pulso::collectors::CollectorProcesos>(
            cfg.procesos.uid_minimo));
    return c;
}

pulso::core::Snapshot recolectarUno(
    const std::vector<std::shared_ptr<ICollector>>& collectors)
{
    pulso::core::Snapshot snap;
    snap.timestamp = std::time(nullptr);
    for (const auto& col : collectors) {
        try {
            auto ms = col->recolectar();
            snap.metricas.insert(snap.metricas.end(), ms.begin(), ms.end());
        } catch (const std::exception& e) {
            Logger::instancia().warn(
                "Collector '" + col->nombre() + "' falló: " + e.what());
        }
    }
    return snap;
}

int modoOnce(const std::vector<std::shared_ptr<ICollector>>& collectors,
             const std::string& formato)
{
    const auto snap = recolectarUno(collectors);

    std::unique_ptr<pulso::formatters::IFormatter> fmt;
    if (formato == "json")            fmt = std::make_unique<pulso::formatters::FormatterJSON>();
    else if (formato == "csv")        fmt = std::make_unique<pulso::formatters::FormatterCSV>();
    else if (formato == "prometheus") fmt = std::make_unique<pulso::formatters::FormatterPrometheus>();
    else {
        Logger::instancia().error("Formato desconocido: '" + formato + "'.");
        return 1;
    }

    std::string out = fmt->formatear(snap);
    std::cout << out;
    if (!out.empty() && out.back() != '\n') std::cout << '\n';
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    pulso::cli::CliOptions cli_opts;
    if (!pulso::cli::parse_arguments(argc, argv, cli_opts)) {
        return 1;
    }

    std::string config_path = "pulso.toml";
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--config" && i + 1 < argc) {
            config_path = argv[++i];
        }
    }

    pulso::config::Config cfg;
    try {
        cfg = pulso::config::cargar(config_path);
    } catch (const pulso::config::ErrorConfig& e) {
        std::cerr << "[pulso] Error al cargar configuracion: " << e.what() << "\n";
        return 1;
    }

    using pulso::utils::logging::LogLevel;
    auto& log = Logger::instancia();
    if      (cfg.nivel_log == "debug") log.setMinLevel(LogLevel::DEBUG);
    else if (cfg.nivel_log == "warn")  log.setMinLevel(LogLevel::WARN);
    else if (cfg.nivel_log == "error") log.setMinLevel(LogLevel::ERROR);
    else                               log.setMinLevel(LogLevel::INFO);

    log.info("pulso v" + pulso::APP_VERSION + " iniciando");

    const auto collectors = construirCollectors(cli_opts, cfg);

    if (cli_opts.once) {
        return modoOnce(collectors, cli_opts.format);
    }

    // ---- Modo daemon: muestrea y hace push al Pushgateway ----
    std::string instancia = cfg.pushgateway.instance;
    if (instancia.empty()) {
        char host[256] = {0};
        instancia = (gethostname(host, sizeof(host) - 1) == 0) ? host : "unknown";
    }

    pulso::core::MuestraActual muestra;
    pulso::exporter::Pushgateway pusher({
        cfg.pushgateway.url, cfg.pushgateway.job, instancia,
        cfg.pushgateway.token, cfg.pushgateway.tls_skip_verify});
    const bool push_activo = !cfg.pushgateway.url.empty();

    log.info(push_activo
        ? "Push a " + cfg.pushgateway.url + pusher.destino()
        : "Push desactivado (pushgateway.url vacío); solo /metrics/prometheus local");

    pulso::sampler::Sampler sampler(
        collectors,
        [&](const pulso::core::Snapshot& s) {
            muestra.set(s);
            if (push_activo) pusher.push(s);
        },
        cfg.sampler.intervalo_segundos);
    sampler.iniciar();

    setupSignalHandler();

    httplib::Server server;
    const auto start_time = std::chrono::steady_clock::now();

    server.Get("/health", [&](const httplib::Request&, httplib::Response& res) {
        res.set_content(pulso::http::handleHealth(start_time), "application/json");
    });
    server.Get("/version", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(pulso::http::handleVersion(), "application/json");
    });
    // Endpoint de pull opcional / depuración: sirve la última muestra en RAM.
    server.Get("/metrics/prometheus", [&](const httplib::Request&, httplib::Response& res) {
        auto s = muestra.get();
        pulso::formatters::FormatterPrometheus fmt;
        res.set_content(s ? fmt.formatear(*s) : std::string(),
                        "text/plain; version=0.0.4");
    });

    std::thread http_thread([&]() {
        log.info("Servidor HTTP escuchando en " + cfg.servidor.host + ":" +
                 std::to_string(cfg.servidor.puerto));
        server.listen(cfg.servidor.host.c_str(), cfg.servidor.puerto);
    });

    while (isRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    log.info("Senal recibida — iniciando shutdown...");
    server.stop();
    if (http_thread.joinable()) http_thread.join();
    sampler.detener();
    log.info("pulso detenido correctamente.");
    return 0;
}

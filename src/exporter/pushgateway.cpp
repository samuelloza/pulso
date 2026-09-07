#include "exporter/pushgateway.hpp"

#include <httplib.h>

#include "formatters/formatter_prometheus.hpp"
#include "utils/logging/logger.hpp"

namespace pulso::exporter {

namespace {

// Escapa un segmento de path del Pushgateway: '/' rompería la ruta de grouping.
std::string segmento(const std::string& s) {
    std::string out;
    for (char c : s) {
        out += (c == '/') ? '_' : c;
    }
    return out.empty() ? "_" : out;
}

} // namespace

Pushgateway::Pushgateway(Opciones opts) : opts_(std::move(opts)) {
    if (!opts_.url.empty() && opts_.url.back() == '/') {
        opts_.url.pop_back();
    }
    path_ = "/metrics/job/" + segmento(opts_.job) +
            "/instance/" + segmento(opts_.instance);
}

bool Pushgateway::push(const pulso::core::Snapshot& snapshot) const {
    auto& log = pulso::utils::logging::Logger::instancia();

    pulso::formatters::FormatterPrometheus fmt;
    const std::string body = fmt.formatear(snapshot);

    httplib::Client cli(opts_.url.c_str());
    cli.set_connection_timeout(5);
    cli.set_write_timeout(5);
    cli.set_read_timeout(5);
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    if (opts_.tls_skip_verify) {
        cli.enable_server_certificate_verification(false);
    }
#else
    if (opts_.url.rfind("https://", 0) == 0) {
        log.warn("pushgateway.url es https:// pero pulso se compiló sin OpenSSL");
        return false;
    }
#endif

    httplib::Headers headers;
    if (!opts_.token.empty()) {
        headers.emplace("Authorization", "Bearer " + opts_.token);
    }

    auto res = cli.Put(path_.c_str(), headers, body, "text/plain; version=0.0.4");

    if (!res) {
        log.warn("Pushgateway inalcanzable (" + opts_.url + path_ + "): " +
                 httplib::to_string(res.error()));
        return false;
    }
    if (res->status == 401 || res->status == 403) {
        log.warn("Pushgateway rechazó el token (HTTP " +
                 std::to_string(res->status) + ")");
        return false;
    }
    if (res->status / 100 != 2) {
        log.warn("Pushgateway respondió " + std::to_string(res->status) +
                 ": " + res->body);
        return false;
    }

    log.debug("Snapshot enviado (" +
              std::to_string(snapshot.metricas.size()) + " métricas)");
    return true;
}

} // namespace pulso::exporter

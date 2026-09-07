#include "formatter_prometheus.hpp"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>

namespace pulso::formatters {

namespace {

// "cpu.usage" -> "pulso_cpu_usage"; "pulso.up" -> "pulso_up".
std::string nombreProm(const std::string& nombre) {
    std::string base = nombre;
    if (base.rfind("pulso.", 0) == 0 || base.rfind("pulso_", 0) == 0) {
        base = base.substr(6);
    }
    std::string salida = "pulso_";
    for (char c : base) {
        salida += (std::isalnum(static_cast<unsigned char>(c)) ? c : '_');
    }
    return salida;
}

std::string escaparValor(const std::string& v) {
    std::string out;
    for (char c : v) {
        if (c == '\\' || c == '"') { out += '\\'; out += c; }
        else if (c == '\n') { out += "\\n"; }
        else { out += c; }
    }
    return out;
}

// {clave="valor",clave="valor"} ordenado; cadena vacía si no hay etiquetas.
std::string etiquetasProm(pulso::core::Etiquetas etiquetas) {
    if (etiquetas.empty()) return "";
    std::sort(etiquetas.begin(), etiquetas.end());
    std::string out = "{";
    for (std::size_t i = 0; i < etiquetas.size(); ++i) {
        if (i) out += ',';
        out += etiquetas[i].first + "=\"" + escaparValor(etiquetas[i].second) + "\"";
    }
    out += "}";
    return out;
}

} // namespace

std::string FormatterPrometheus::formato() const { return "prometheus"; }

std::string FormatterPrometheus::contentType() const {
    return "text/plain; version=0.0.4";
}

std::string FormatterPrometheus::formatear(
    const pulso::core::Snapshot& snapshot) const {

    std::ostringstream output;
    std::set<std::string> vistos;  // HELP/TYPE una sola vez por métrica

    for (const auto& m : snapshot.metricas) {
        const std::string nombre = nombreProm(m.nombre);
        if (vistos.insert(nombre).second) {
            output << "# HELP " << nombre << " " << m.nombre
                   << " (" << m.unidad << ")\n";
            output << "# TYPE " << nombre << " gauge\n";
        }
        output << nombre << etiquetasProm(m.etiquetas) << " " << m.valor << "\n";
    }

    return output.str();
}

std::string FormatterPrometheus::formatearHistorial(
    const std::vector<pulso::core::Snapshot>& snapshots) const {
    if (snapshots.empty()) return "";
    return formatear(snapshots.back());
}

} // namespace pulso::formatters

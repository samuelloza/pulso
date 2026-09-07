#include "formatter_csv.hpp"

#include <sstream>

#include "core/types.hpp"

namespace pulso::formatters {

std::string FormatterCSV::formato() const { return "csv"; }

std::string FormatterCSV::contentType() const { return "text/csv"; }

std::string FormatterCSV::cabecera() {
    return "timestamp,nombre,etiquetas,valor,unidad\n";
}

std::string FormatterCSV::filasDeSnapshot(const pulso::core::Snapshot& snapshot) {
    std::ostringstream oss;
    for (const auto& m : snapshot.metricas) {
        oss << snapshot.timestamp << ','
            << m.nombre << ','
            << '"' << pulso::core::serializarEtiquetas(m.etiquetas) << '"' << ','
            << m.valor << ','
            << m.unidad << '\n';
    }
    return oss.str();
}

std::string FormatterCSV::formatear(const pulso::core::Snapshot& snapshot) const {
    std::string resultado;
    if (!cabeceraEmitida_) {
        resultado += cabecera();
        cabeceraEmitida_ = true;
    }
    resultado += filasDeSnapshot(snapshot);
    return resultado;
}

std::string FormatterCSV::formatearHistorial(
    const std::vector<pulso::core::Snapshot>& snapshots) const {
    std::string resultado = cabecera();
    for (const auto& s : snapshots) {
        resultado += filasDeSnapshot(s);
    }
    return resultado;
}

} // namespace pulso::formatters

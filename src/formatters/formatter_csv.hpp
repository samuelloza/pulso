#pragma once

#include "iformatter.hpp"
#include <string>
#include <vector>

namespace pulso::formatters {

/**
 * @brief Formateador CSV en formato largo: una fila por métrica.
 *
 * Columnas: timestamp,nombre,etiquetas,valor,unidad
 * La primera llamada a formatear() emite la cabecera.
 */
class FormatterCSV : public IFormatter {
public:
    std::string formato() const override;
    std::string contentType() const override;
    std::string formatear(const pulso::core::Snapshot& snapshot) const override;
    std::string formatearHistorial(
        const std::vector<pulso::core::Snapshot>& snapshots) const override;

private:
    mutable bool cabeceraEmitida_ = false;

    static std::string cabecera();
    static std::string filasDeSnapshot(const pulso::core::Snapshot& snapshot);
};

} // namespace pulso::formatters

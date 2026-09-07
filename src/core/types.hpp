#pragma once

#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include <cstdint>

namespace pulso::core {

/// Par etiqueta -> valor (ej: {"mount", "/"} o {"interface", "eth0"}).
using Etiqueta = std::pair<std::string, std::string>;
using Etiquetas = std::vector<Etiqueta>;

/**
 * @brief Representa una metrica individual del sistema con sus metadatos.
 *
 * Una metrica encapsula un valor de medicion puntual, junto con el nombre
 * que lo identifica, la unidad en que se expresa, el momento en que fue tomado
 * y opcionalmente etiquetas para distinguir series (ej: por interfaz de red o
 * por punto de montaje).
 */
struct Metrica {
    /** @brief Nombre identificador de la metrica. Ej: "cpu.usage", "ram.used" */
    std::string nombre;

    /** @brief Valor numerico de la medicion */
    double valor = 0.0;

    /** @brief Unidad en que se expresa el valor. Ej: "porcentaje", "bytes" */
    std::string unidad;

    /** @brief Momento en que se tomo la medicion, Unix timestamp en segundos. */
    std::int64_t timestamp = 0;

    /** @brief Etiquetas opcionales para distinguir series con el mismo nombre. */
    Etiquetas etiquetas;
};

/**
 * @brief Conjunto de métricas capturadas en un mismo instante de muestreo.
 */
struct Snapshot {
    /** @brief Momento del muestreo, Unix timestamp en segundos. */
    std::int64_t timestamp = 0;

    /** @brief Métricas individuales capturadas en este snapshot. */
    std::vector<Metrica> metricas;
};

/**
 * @brief Serializa etiquetas a una forma canónica y estable: "k1=v1,k2=v2"
 *        con las claves ordenadas alfabéticamente. Cadena vacía si no hay.
 *
 * ponytail: formato plano sin escapado; los valores reales (rutas de montaje,
 * nombres de interfaz, índices de core) no contienen '=' ni ','. Si algún día
 * hiciera falta, migrar a JSON en la columna.
 */
inline std::string serializarEtiquetas(Etiquetas etiquetas) {
    std::sort(etiquetas.begin(), etiquetas.end());
    std::string out;
    for (const auto& [k, v] : etiquetas) {
        if (!out.empty()) out += ',';
        out += k;
        out += '=';
        out += v;
    }
    return out;
}

/// @brief Inverso de serializarEtiquetas().
inline Etiquetas parsearEtiquetas(const std::string& s) {
    Etiquetas out;
    std::size_t i = 0;
    while (i < s.size()) {
        std::size_t coma = s.find(',', i);
        if (coma == std::string::npos) coma = s.size();
        const std::string par = s.substr(i, coma - i);
        const std::size_t eq = par.find('=');
        if (eq != std::string::npos) {
            out.emplace_back(par.substr(0, eq), par.substr(eq + 1));
        }
        i = coma + 1;
    }
    return out;
}

} // namespace pulso::core

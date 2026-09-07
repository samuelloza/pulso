#include "ram_usage.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <chrono>

namespace pulso::collectors::memory {

RamInfo getRamUsage(const std::string& path) {
    std::ifstream file(path);

    if (!file.is_open()) {
        throw std::runtime_error(
            "No se pudo abrir el archivo " + path + ": "
            "verifique permisos y disponibilidad del sistema"
        );
    }

    std::string line;
    long long memTotal     = 0;
    long long memAvailable = 0;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        long long value;

        iss >> key >> value;

        if (key == "MemTotal:") {
            memTotal = value;
        }

        if (key == "MemAvailable:") {
            memAvailable = value;
        }
    }

    if (memTotal == 0 || memAvailable == 0) {
        throw std::runtime_error(
            "No se encontraron las claves MemTotal o MemAvailable en " + path
        );
    }

    // Convertir de kB a bytes
    memTotal     *= 1024;
    memAvailable *= 1024;

    // Calcular memoria usada: MemTotal - MemAvailable
    long long memUsed = memTotal - memAvailable;

    return {
        static_cast<uint64_t>(memTotal),
        static_cast<uint64_t>(memUsed),
        static_cast<uint64_t>(memAvailable)
    };
}

std::string CollectorMemory::nombre() const {
    return "memory";
}

std::vector<pulso::core::Metrica> CollectorMemory::recolectar() {
    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir /proc/meminfo");
    }

    // Todos los valores de /proc/meminfo están en kB.
    std::string clave;
    long long valor_kb = 0;
    long long total = 0, disponible = 0, libre = 0, buffers = 0, cached = 0;
    long long swap_total = 0, swap_libre = 0;

    std::string linea;
    while (std::getline(file, linea)) {
        std::istringstream iss(linea);
        iss >> clave >> valor_kb;
        const long long b = valor_kb * 1024;
        if      (clave == "MemTotal:")     total = b;
        else if (clave == "MemAvailable:") disponible = b;
        else if (clave == "MemFree:")      libre = b;
        else if (clave == "Buffers:")      buffers = b;
        else if (clave == "Cached:")       cached = b;
        else if (clave == "SwapTotal:")    swap_total = b;
        else if (clave == "SwapFree:")     swap_libre = b;
    }

    const auto ts = static_cast<std::int64_t>(
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    const double usada = static_cast<double>(total - disponible);
    const double swap_usada = static_cast<double>(swap_total - swap_libre);

    return {
        {"memory.total_bytes",     static_cast<double>(total),      "bytes", ts},
        {"memory.available_bytes", static_cast<double>(disponible), "bytes", ts},
        {"memory.free_bytes",      static_cast<double>(libre),      "bytes", ts},
        {"memory.used_bytes",      usada,                           "bytes", ts},
        {"memory.buffers_bytes",   static_cast<double>(buffers),    "bytes", ts},
        {"memory.cached_bytes",    static_cast<double>(cached),     "bytes", ts},
        {"swap.total_bytes",       static_cast<double>(swap_total), "bytes", ts},
        {"swap.free_bytes",        static_cast<double>(swap_libre), "bytes", ts},
        {"swap.used_bytes",        swap_usada,                      "bytes", ts},
    };
}

} // namespace pulso::collectors::memory
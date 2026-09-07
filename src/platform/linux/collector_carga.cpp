#include "collector_carga.hpp"

#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "../../collectors/error_recoleccion.hpp"

namespace pulso::collectors {

namespace {

// ponytail: escaneo O(nº procesos) de /proc en cada muestreo. Suficiente a
// intervalos de 10s; si molesta, muestrear zombies con menor frecuencia.
double contarZombies() {
    namespace fs = std::filesystem;
    double zombies = 0;
    std::error_code ec;
    for (const auto& entrada : fs::directory_iterator("/proc", ec)) {
        if (ec) break;
        const std::string nombre = entrada.path().filename().string();
        if (nombre.empty() || !std::isdigit(static_cast<unsigned char>(nombre[0])))
            continue;
        std::ifstream st(entrada.path() / "stat");
        std::string contenido;
        if (!std::getline(st, contenido)) continue;
        // "<pid> (comm) <estado> ..."  -> el estado va tras el ')'.
        const auto cierre = contenido.rfind(')');
        if (cierre == std::string::npos || cierre + 2 >= contenido.size()) continue;
        if (contenido[cierre + 2] == 'Z') zombies += 1;
    }
    return zombies;
}

} // namespace

std::string CollectorCarga::nombre() const { return "carga"; }

std::vector<pulso::core::Metrica> CollectorCarga::recolectar() {
    const std::int64_t ts = std::time(nullptr);
    std::vector<pulso::core::Metrica> m;

    // /proc/loadavg: "l1 l5 l15 running/total lastpid"
    {
        std::ifstream f("/proc/loadavg");
        if (!f.is_open()) throw ErrorRecoleccion("No se pudo abrir /proc/loadavg");
        double l1 = 0, l5 = 0, l15 = 0;
        std::string ratio;
        f >> l1 >> l5 >> l15 >> ratio;
        m.push_back({"load.avg1",  l1,  "carga", ts});
        m.push_back({"load.avg5",  l5,  "carga", ts});
        m.push_back({"load.avg15", l15, "carga", ts});
        const auto barra = ratio.find('/');
        if (barra != std::string::npos) {
            m.push_back({"procesos.total",
                         std::stod(ratio.substr(barra + 1)), "cantidad", ts});
        }
    }

    // /proc/stat: procs_running / procs_blocked
    {
        std::ifstream f("/proc/stat");
        std::string linea;
        while (std::getline(f, linea)) {
            std::istringstream ss(linea);
            std::string clave;
            double valor = 0;
            ss >> clave >> valor;
            if (clave == "procs_running")
                m.push_back({"procesos.running", valor, "cantidad", ts});
            else if (clave == "procs_blocked")
                m.push_back({"procesos.blocked", valor, "cantidad", ts});
        }
    }

    m.push_back({"procesos.zombie", contarZombies(), "cantidad", ts});
    return m;
}

} // namespace pulso::collectors

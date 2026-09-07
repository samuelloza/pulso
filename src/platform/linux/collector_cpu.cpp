#include "collector_cpu.hpp"

#include <unistd.h>

#include <array>
#include <chrono>
#include <ctime>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <thread>

#include "../../collectors/error_recoleccion.hpp"

namespace pulso::collectors {

namespace {

// jiffies acumulados de una línea "cpu"/"cpuN" de /proc/stat.
struct Tiempos {
    unsigned long long user = 0, nice = 0, system = 0, idle = 0,
                       iowait = 0, irq = 0, softirq = 0, steal = 0;
    unsigned long long total() const {
        return user + nice + system + idle + iowait + irq + softirq + steal;
    }
    unsigned long long ocupado() const { return total() - idle - iowait; }
};

// Lee todas las líneas "cpu" y "cpuN" de /proc/stat. Clave "" = agregado.
std::map<std::string, Tiempos> leerStat() {
    std::ifstream f("/proc/stat");
    if (!f.is_open()) {
        throw ErrorRecoleccion("No se pudo abrir /proc/stat");
    }
    std::map<std::string, Tiempos> out;
    std::string linea;
    while (std::getline(f, linea)) {
        if (linea.rfind("cpu", 0) != 0) break;
        std::istringstream ss(linea);
        std::string etiqueta;
        ss >> etiqueta;
        Tiempos t;
        ss >> t.user >> t.nice >> t.system >> t.idle
           >> t.iowait >> t.irq >> t.softirq >> t.steal;
        out[etiqueta == "cpu" ? "" : etiqueta.substr(3)] = t;
    }
    return out;
}

double usoPct(const Tiempos& a, const Tiempos& b) {
    const long long dTotal = static_cast<long long>(b.total()) - a.total();
    const long long dOcup  = static_cast<long long>(b.ocupado()) - a.ocupado();
    if (dTotal <= 0) return 0.0;
    double pct = 100.0 * static_cast<double>(dOcup) / static_cast<double>(dTotal);
    return pct < 0 ? 0.0 : (pct > 100 ? 100.0 : pct);
}

} // namespace

std::string CollectorCPU::nombre() const { return "cpu"; }

std::vector<pulso::core::Metrica> CollectorCPU::recolectar() {
    const auto a = leerStat();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    const auto b = leerStat();

    const std::int64_t ts = std::time(nullptr);
    const double hz = static_cast<double>(sysconf(_SC_CLK_TCK));
    std::vector<pulso::core::Metrica> m;

    // Uso agregado + por núcleo.
    int cores = 0;
    for (const auto& [clave, tb] : b) {
        auto it = a.find(clave);
        if (it == a.end()) continue;
        const double pct = usoPct(it->second, tb);
        if (clave.empty()) {
            m.push_back({"cpu.usage", pct, "porcentaje", ts});
        } else {
            ++cores;
            m.push_back({"cpu.usage", pct, "porcentaje", ts, {{"core", clave}}});
        }
    }
    m.push_back({"cpu.cores", static_cast<double>(cores), "cantidad", ts});

    // Contadores acumulados por modo (segundos), sobre el agregado.
    if (auto it = b.find(""); it != b.end() && hz > 0) {
        const Tiempos& t = it->second;
        const std::array<std::pair<const char*, unsigned long long>, 8> modos{{
            {"user", t.user}, {"nice", t.nice}, {"system", t.system},
            {"idle", t.idle}, {"iowait", t.iowait}, {"irq", t.irq},
            {"softirq", t.softirq}, {"steal", t.steal},
        }};
        for (const auto& [modo, jiffies] : modos) {
            m.push_back({"cpu.time_seconds", static_cast<double>(jiffies) / hz,
                         "segundos", ts, {{"mode", modo}}});
        }
    }

    return m;
}

} // namespace pulso::collectors

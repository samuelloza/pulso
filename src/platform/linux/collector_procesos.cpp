#include "collector_procesos.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace pulso::collectors {

namespace {
namespace fs = std::filesystem;

struct Agregado {
    long instancias = 0;
    double start_time_min = 0;   // arranque de la más antigua (unix seconds)
    double rss_bytes = 0;
    double cpu_seconds = 0;
};

std::string leerArchivo(const fs::path& p) {
    std::ifstream f(p);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

} // namespace

std::string CollectorProcesos::nombre() const { return "procesos"; }

std::vector<pulso::core::Metrica> CollectorProcesos::recolectar() {
    const std::int64_t ts = std::time(nullptr);
    const double hz = static_cast<double>(sysconf(_SC_CLK_TCK));
    const double page = static_cast<double>(sysconf(_SC_PAGESIZE));

    // boot_time = ahora - uptime, para pasar starttime (ticks desde boot) a unix.
    double uptime = 0;
    { std::ifstream f("/proc/uptime"); f >> uptime; }
    const double boot_time = static_cast<double>(ts) - uptime;

    std::map<std::string, Agregado> porNombre;
    std::error_code ec;

    for (const auto& e : fs::directory_iterator("/proc", ec)) {
        if (ec) break;
        const std::string pid = e.path().filename().string();
        if (pid.empty() || !std::isdigit(static_cast<unsigned char>(pid[0]))) continue;

        // Dueño del proceso.
        struct stat st {};
        if (stat(e.path().c_str(), &st) != 0) continue;
        if (static_cast<std::int64_t>(st.st_uid) < uid_minimo_) continue;

        // Kernel threads: cmdline vacío.
        if (leerArchivo(e.path() / "cmdline").empty()) continue;

        const std::string stat_raw = leerArchivo(e.path() / "stat");
        const auto abre = stat_raw.find('(');
        const auto cierra = stat_raw.rfind(')');
        if (abre == std::string::npos || cierra == std::string::npos || cierra < abre)
            continue;

        const std::string comm = stat_raw.substr(abre + 1, cierra - abre - 1);
        std::istringstream resto(stat_raw.substr(cierra + 2));
        std::vector<std::string> t{std::istream_iterator<std::string>(resto),
                                   std::istream_iterator<std::string>()};
        // t[0] = campo 3 (state); campo F -> t[F-3].
        if (t.size() < 20) continue;
        const double utime = std::stod(t[11]);
        const double stime = std::stod(t[12]);
        const double starttime = std::stod(t[19]);

        double rss_pages = 0;
        { std::ifstream f(e.path() / "statm"); double total; f >> total >> rss_pages; }

        const double start_unix = boot_time + (hz > 0 ? starttime / hz : 0);

        auto& a = porNombre[comm];
        a.instancias += 1;
        a.rss_bytes += rss_pages * page;
        a.cpu_seconds += (hz > 0 ? (utime + stime) / hz : 0);
        if (a.start_time_min == 0 || start_unix < a.start_time_min)
            a.start_time_min = start_unix;
    }

    std::vector<pulso::core::Metrica> m;
    for (const auto& [comm, a] : porNombre) {
        const pulso::core::Etiquetas et{{"comm", comm}};
        m.push_back({"proceso.instancias", static_cast<double>(a.instancias),
                     "cantidad", ts, et});
        m.push_back({"proceso.start_time_seconds", a.start_time_min, "segundos", ts, et});
        m.push_back({"proceso.rss_bytes", a.rss_bytes, "bytes", ts, et});
        m.push_back({"proceso.cpu_seconds", a.cpu_seconds, "segundos", ts, et});
    }
    return m;
}

} // namespace pulso::collectors

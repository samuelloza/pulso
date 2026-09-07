#include "collector_sistema.hpp"

#include <sys/utsname.h>
#include <unistd.h>

#include <ctime>
#include <fstream>
#include <string>

#include "../../core/version.hpp"

namespace pulso::collectors {

namespace {

std::string hostname() {
    char buf[256] = {0};
    if (gethostname(buf, sizeof(buf) - 1) != 0) return "unknown";
    return buf;
}

std::string osPrettyName() {
    std::ifstream f("/etc/os-release");
    std::string linea;
    while (std::getline(f, linea)) {
        if (linea.rfind("PRETTY_NAME=", 0) == 0) {
            std::string v = linea.substr(12);
            if (v.size() >= 2 && v.front() == '"' && v.back() == '"')
                v = v.substr(1, v.size() - 2);
            return v;
        }
    }
    return "Linux";
}

} // namespace

std::string CollectorSistema::nombre() const { return "sistema"; }

std::vector<pulso::core::Metrica> CollectorSistema::recolectar() {
    const std::int64_t ts = std::time(nullptr);
    std::vector<pulso::core::Metrica> m;

    double uptime = 0;
    if (std::ifstream f("/proc/uptime"); f >> uptime) {
        m.push_back({"system.uptime_seconds", uptime, "segundos", ts});
        m.push_back({"system.boot_time_seconds",
                     static_cast<double>(ts) - uptime, "segundos", ts});
    }

    struct utsname u {};
    std::string kernel = "unknown", arch = "unknown";
    if (uname(&u) == 0) {
        kernel = u.release;
        arch = u.machine;
    }

    m.push_back({"system.info", 1.0, "info", ts, {
        {"hostname", hostname()},
        {"kernel", kernel},
        {"os", osPrettyName()},
        {"arch", arch},
    }});
    m.push_back({"pulso.build_info", 1.0, "info", ts,
                 {{"version", pulso::APP_VERSION}}});

    return m;
}

} // namespace pulso::collectors

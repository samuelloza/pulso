#include "collector_temperatura.hpp"

#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>

namespace pulso::collectors {

namespace {
namespace fs = std::filesystem;

std::string leerLinea(const fs::path& p) {
    std::ifstream f(p);
    std::string s;
    std::getline(f, s);
    return s;
}

} // namespace

std::string CollectorTemperatura::nombre() const { return "temperatura"; }

std::vector<pulso::core::Metrica> CollectorTemperatura::recolectar() {
    const std::int64_t ts = std::time(nullptr);
    std::vector<pulso::core::Metrica> m;
    std::error_code ec;

    // --- /sys/class/hwmon/hwmon*/temp*_input ---
    for (const auto& hw : fs::directory_iterator("/sys/class/hwmon", ec)) {
        if (ec) break;
        const std::string chip =
            leerLinea(hw.path() / "name").empty() ? hw.path().filename().string()
                                                  : leerLinea(hw.path() / "name");
        for (const auto& e : fs::directory_iterator(hw.path(), ec)) {
            const std::string fn = e.path().filename().string();
            if (fn.rfind("temp", 0) != 0 || fn.find("_input") == std::string::npos)
                continue;
            const std::string raw = leerLinea(e.path());
            if (raw.empty()) continue;
            const std::string prefijo = fn.substr(0, fn.find("_input"));
            std::string sensor = leerLinea(hw.path() / (prefijo + "_label"));
            if (sensor.empty()) sensor = prefijo;
            m.push_back({"temperature.celsius", std::stod(raw) / 1000.0,
                         "celsius", ts, {{"chip", chip}, {"sensor", sensor}}});
        }
    }
    if (!m.empty()) return m;

    // --- fallback: /sys/class/thermal/thermal_zone*/temp ---
    ec.clear();
    for (const auto& z : fs::directory_iterator("/sys/class/thermal", ec)) {
        if (ec) break;
        if (z.path().filename().string().rfind("thermal_zone", 0) != 0) continue;
        const std::string raw = leerLinea(z.path() / "temp");
        if (raw.empty()) continue;
        std::string tipo = leerLinea(z.path() / "type");
        if (tipo.empty()) tipo = z.path().filename().string();
        m.push_back({"temperature.celsius", std::stod(raw) / 1000.0, "celsius", ts,
                     {{"chip", z.path().filename().string()}, {"sensor", tipo}}});
    }
    return m;
}

} // namespace pulso::collectors

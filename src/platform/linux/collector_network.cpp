#include "collector_network.hpp"

#include <ctime>
#include <fstream>
#include <sstream>
#include <string>

#include "../../collectors/error_recoleccion.hpp"

namespace pulso::collectors {

std::string CollectorRed::nombre() const { return "network"; }

std::vector<pulso::core::Metrica> CollectorRed::recolectar() {
    std::ifstream f("/proc/net/dev");
    if (!f.is_open()) {
        throw ErrorRecoleccion("No se pudo abrir /proc/net/dev");
    }

    std::string linea;
    std::getline(f, linea);  // cabecera 1
    std::getline(f, linea);  // cabecera 2

    const std::int64_t ts = std::time(nullptr);
    std::vector<pulso::core::Metrica> m;

    while (std::getline(f, linea)) {
        const auto colon = linea.find(':');
        if (colon == std::string::npos) continue;

        std::string iface = linea.substr(0, colon);
        const auto ini = iface.find_first_not_of(" \t");
        if (ini == std::string::npos) continue;
        iface = iface.substr(ini);
        iface.erase(iface.find_last_not_of(" \t") + 1);
        // Excluir loopback y los pares veth efímeros de contenedores (alta
        // cardinalidad, aparecen/desaparecen constantemente).
        if (iface == "lo" || iface.rfind("veth", 0) == 0) continue;

        std::istringstream ss(linea.substr(colon + 1));
        // rx: bytes packets errs drop fifo frame compressed multicast
        // tx: bytes packets errs drop fifo colls carrier compressed
        unsigned long long rx_b, rx_p, rx_e, rx_d, x1, x2, x3, x4;
        unsigned long long tx_b, tx_p, tx_e, tx_d;
        if (!(ss >> rx_b >> rx_p >> rx_e >> rx_d >> x1 >> x2 >> x3 >> x4
                 >> tx_b >> tx_p >> tx_e >> tx_d)) {
            continue;
        }

        const pulso::core::Etiquetas et{{"interface", iface}};
        m.push_back({"network.rx_bytes",   static_cast<double>(rx_b), "bytes",    ts, et});
        m.push_back({"network.rx_packets", static_cast<double>(rx_p), "cantidad", ts, et});
        m.push_back({"network.rx_errors",  static_cast<double>(rx_e), "cantidad", ts, et});
        m.push_back({"network.rx_dropped", static_cast<double>(rx_d), "cantidad", ts, et});
        m.push_back({"network.tx_bytes",   static_cast<double>(tx_b), "bytes",    ts, et});
        m.push_back({"network.tx_packets", static_cast<double>(tx_p), "cantidad", ts, et});
        m.push_back({"network.tx_errors",  static_cast<double>(tx_e), "cantidad", ts, et});
        m.push_back({"network.tx_dropped", static_cast<double>(tx_d), "cantidad", ts, et});
    }

    return m;
}

} // namespace pulso::collectors

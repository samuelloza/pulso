#include "collector_disk.hpp"

#include <sys/statvfs.h>

#include <ctime>
#include <fstream>
#include <set>
#include <sstream>
#include <string>

namespace pulso::collectors {

namespace {

// Sistemas de archivos virtuales/pseudo: no representan almacenamiento real.
const std::set<std::string> kFsIgnorados = {
    "proc", "sysfs", "devtmpfs", "devpts", "tmpfs", "cgroup", "cgroup2",
    "overlay", "squashfs", "autofs", "mqueue", "debugfs", "tracefs",
    "securityfs", "pstore", "bpf", "configfs", "ramfs", "hugetlbfs",
    "fusectl", "binfmt_misc", "nsfs", "rpc_pipefs", "fuse.gvfsd-fuse",
    "fuse.portal", "efivarfs", "none",
};

// Puntos de montaje del sistema que no representan almacenamiento del usuario.
bool montajeIgnorado(const std::string& mp) {
    for (const char* p : {"/sys", "/proc", "/dev", "/run", "/snap"}) {
        if (mp == p || mp.rfind(std::string(p) + "/", 0) == 0) return true;
    }
    return false;
}

bool dispositivoIgnorado(const std::string& dev) {
    return dev.rfind("loop", 0) == 0 || dev.rfind("ram", 0) == 0 ||
           dev.rfind("fd", 0) == 0 || dev.rfind("dm-", 0) == 0;
}

} // namespace

std::string CollectorDisco::nombre() const { return "disk"; }

std::vector<pulso::core::Metrica> CollectorDisco::recolectar() {
    const std::int64_t ts = std::time(nullptr);
    std::vector<pulso::core::Metrica> m;

    // ---- Espacio por punto de montaje ----
    std::ifstream mi("/proc/self/mountinfo");
    std::set<std::string> vistos;
    std::string linea;
    while (std::getline(mi, linea)) {
        // formato: id pid maj:min root MOUNTPOINT opts... - FSTYPE source superopts
        std::istringstream ss(linea);
        std::string campo, mountpoint, fstype;
        for (int i = 0; i < 5 && ss >> campo; ++i) {
            if (i == 4) mountpoint = campo;
        }
        while (ss >> campo && campo != "-") { /* saltar opts opcionales */ }
        ss >> fstype;
        if (mountpoint.empty() || fstype.empty()) continue;
        if (kFsIgnorados.count(fstype) || montajeIgnorado(mountpoint)) continue;
        if (!vistos.insert(mountpoint).second) continue;

        struct statvfs vfs {};
        if (statvfs(mountpoint.c_str(), &vfs) != 0) continue;
        const double bs = static_cast<double>(vfs.f_frsize);
        const double total = static_cast<double>(vfs.f_blocks) * bs;
        const double libre = static_cast<double>(vfs.f_bavail) * bs;
        const double usado = total - libre;
        if (total <= 0) continue;

        const pulso::core::Etiquetas et{{"mount", mountpoint}, {"fstype", fstype}};
        m.push_back({"disk.total_bytes", total, "bytes", ts, et});
        m.push_back({"disk.used_bytes",  usado, "bytes", ts, et});
        m.push_back({"disk.free_bytes",  libre, "bytes", ts, et});
        m.push_back({"disk.used_ratio",  usado / total, "ratio", ts, et});
    }

    // ---- I/O por dispositivo ----
    std::ifstream ds("/proc/diskstats");
    while (std::getline(ds, linea)) {
        std::istringstream ss(linea);
        long long maj, min_;
        std::string dev;
        unsigned long long rd_ok, rd_merg, rd_sect, rd_ms,
                           wr_ok, wr_merg, wr_sect, wr_ms;
        if (!(ss >> maj >> min_ >> dev
                 >> rd_ok >> rd_merg >> rd_sect >> rd_ms
                 >> wr_ok >> wr_merg >> wr_sect >> wr_ms)) {
            continue;
        }
        if (dispositivoIgnorado(dev)) continue;

        const pulso::core::Etiquetas et{{"device", dev}};
        m.push_back({"disk.reads_completed",  static_cast<double>(rd_ok), "cantidad", ts, et});
        m.push_back({"disk.writes_completed", static_cast<double>(wr_ok), "cantidad", ts, et});
        m.push_back({"disk.read_bytes",    static_cast<double>(rd_sect) * 512.0, "bytes", ts, et});
        m.push_back({"disk.written_bytes", static_cast<double>(wr_sect) * 512.0, "bytes", ts, et});
    }

    return m;
}

} // namespace pulso::collectors

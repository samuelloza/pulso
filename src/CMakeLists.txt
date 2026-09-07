add_executable(pulso main.cpp)

# El agente no persiste nada: muestrea y hace push al Pushgateway. Sin SQLite.
target_sources(pulso PRIVATE
    cli/arg_parser.cpp
    config/config.cpp
    exporter/pushgateway.cpp
    sampler/sampler.cpp
    utils/logging/logger.cpp
    utils/signal_handler.cpp

    formatters/formatter_json.cpp
    formatters/formatter_csv.cpp
    formatters/formatter_prometheus.cpp

    http/handler_health.cpp
    http/handler_version.cpp

    collectors/bateria/bateria_collector.cpp
    collectors/memory/ram_usage.cpp
    platform/linux/collector_cpu.cpp
    platform/linux/collector_disk.cpp
    platform/linux/collector_network.cpp
    platform/linux/collector_procesos.cpp
    platform/linux/collector_carga.cpp
    platform/linux/collector_sistema.cpp
    platform/linux/collector_temperatura.cpp
)

target_compile_features(pulso PRIVATE cxx_std_17)

if(PULSO_STATIC_LINK)
    target_link_options(pulso PRIVATE -static-libgcc -static-libstdc++)
endif()

target_include_directories(pulso PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pulso PRIVATE
    httplib
    nlohmann_json
    tomlplusplus::tomlplusplus
)

# OpenSSL opcional: habilita `https://` en el cliente de Pushgateway.
# Sin OpenSSL el agente compila igual pero solo acepta `http://`.
find_package(OpenSSL)
if(OpenSSL_FOUND)
    target_compile_definitions(pulso PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    target_link_libraries(pulso PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    message(STATUS "pulso: soporte HTTPS habilitado (OpenSSL ${OPENSSL_VERSION})")
else()
    message(STATUS "pulso: sin OpenSSL, el push solo funcionará por http://")
endif()

# -Wno-missing-field-initializers: Metrica se inicializa con {nombre,valor,unidad,ts}
# y `etiquetas` queda por defecto a propósito (patrón usado en todos los colectores).
target_compile_options(pulso PRIVATE
    -Wall -Wextra -Wpedantic -Wno-missing-field-initializers -O2)

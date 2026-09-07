#include <gtest/gtest.h>

#include "core/types.hpp"

using namespace pulso::core;

TEST(TestConstruccionMetrica, ValoresExplicitos) {
    Metrica metrica{
        "cpu.usage",
        75.5,
        "porcentaje",
        1710000000
    };

    EXPECT_EQ(metrica.nombre, "cpu.usage");
    EXPECT_DOUBLE_EQ(metrica.valor, 75.5);
    EXPECT_EQ(metrica.unidad, "porcentaje");
    EXPECT_EQ(metrica.timestamp, 1710000000);
}

TEST(TestConstruccionSnapshot, SnapshotConMetricas) {
    Metrica m1{
        "ram.used",
        2048,
        "MB",
        1710000000
    };

    Snapshot snapshot{
        1710000001,
        {m1}
    };

    EXPECT_EQ(snapshot.timestamp, 1710000001);
    EXPECT_EQ(snapshot.metricas.size(), 1);

    EXPECT_EQ(snapshot.metricas[0].nombre, "ram.used");
}

TEST(TestSnapshotVacio, SinMetricas) {
    Snapshot snapshot{
        1710000001,
        {}
    };

    EXPECT_EQ(snapshot.metricas.size(), 0);
}

TEST(TestAgregarMetricasASnapshot, IncrementoCorrecto) {
    Snapshot snapshot{
        1710000001,
        {}
    };

    snapshot.metricas.push_back({
        "cpu.usage",
        50.0,
        "%",
        1710000001
    });

    snapshot.metricas.push_back({
        "disk.used",
        80.0,
        "%",
        1710000001
    });

    EXPECT_EQ(snapshot.metricas.size(), 2);
}

TEST(TestCopiaDeSnapshot, CopiaIndependiente) {
    Snapshot original{
        1710000001,
        {
            {
                "cpu.usage",
                60.0,
                "%",
                1710000001
            }
        }
    };

    Snapshot copia = original;

    copia.metricas[0].valor = 99.0;

    EXPECT_DOUBLE_EQ(original.metricas[0].valor, 60.0);
    EXPECT_DOUBLE_EQ(copia.metricas[0].valor, 99.0);
}

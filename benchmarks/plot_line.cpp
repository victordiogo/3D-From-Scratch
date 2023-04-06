#include "graphics.hpp"

#include <benchmark/benchmark.h>
#include <glm/vec2.hpp>

void BM_plot_line(benchmark::State& state)
{
  Graphics graphics{ 800, 600 };
  for (auto _ : state) {
    graphics.plot_line(glm::ivec2{ 0, 0 }, glm::ivec2{ 500, 100 });
  }
}

BENCHMARK(BM_plot_line);
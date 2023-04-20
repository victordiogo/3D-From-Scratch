#ifndef _3D_FROM_SCRATCH_TIMER_HPP
#define _3D_FROM_SCRATCH_TIMER_HPP

#include <chrono>

class Timer {
private:
  using Clock = std::chrono::steady_clock;
  using Duration = std::chrono::duration<double, std::milli>;
  std::chrono::time_point<Clock> m_begin{ Clock::now() };

public:
  void reset() { m_begin = Clock::now(); }

  double elapsed() const
  {
    return std::chrono::duration_cast<Duration>(Clock::now() - m_begin).count();
  }
};

#endif
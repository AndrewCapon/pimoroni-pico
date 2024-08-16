// class for geting execution times

#pragma once


class ElapsedUs {
  public:
    ElapsedUs()
    {
      last_time = time_us_64();
    }

    float elapsed(void)
    {
      uint64_t time_now = time_us_64();
      uint64_t elapsed = time_now - last_time;
      last_time = time_now;
      return (float)elapsed/1000.0f;
    }

    void reset()
    {
      uint64_t time_now = time_us_64();
      last_time = time_now;
    }

  private:
    uint64_t last_time;
};


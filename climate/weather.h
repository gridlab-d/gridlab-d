/** $Id: weather.h 4738 2014-07-03 00:55:39Z dchassin $
        Copyright (C) 2009 Battelle Memorial Institute
        @file weather.h
        @addtogroup climate
        @ingroup modules
 @{
 **/

#ifndef CLIMATE_WEATHER_
#define CLIMATE_WEATHER_

#include <memory>
#include <utility>

#include "gridlabd.h"

class weather : public gld_object {
public:
  // Static class pointer
  static CLASS *oclass;

  // Constructors and Destructor
  weather() = default;
  explicit weather(MODULE *module);
  ~weather() override = default;

  void reset();
  void release() override;
  bool is_valid_weather();

  // Use the custom deleter type
  using unique_ptr_type = std::unique_ptr<weather, gld_object_deleter>;

  unique_ptr_type create_weather();

  // Prevent copying, allow moving
  weather(const weather &) = delete;
  weather &operator=(const weather &) = delete;

  // Move constructor
  weather(weather &&other) noexcept
      : temperature(std::exchange(other.temperature, 0.0)),
        humidity(std::exchange(other.humidity, 0.0)),
        solar_dir(std::exchange(other.solar_dir, 0.0)),
        solar_diff(std::exchange(other.solar_diff, 0.0)),
        solar_global(std::exchange(other.solar_global, 0.0)),
        global_horizontal_extra(
            std::exchange(other.global_horizontal_extra, 0.0)),
        wind_speed(std::exchange(other.wind_speed, 0.0)),
        wind_dir(std::exchange(other.wind_dir, 0.0)),
        opq_sky_cov(std::exchange(other.opq_sky_cov, 0.0)),
        tot_sky_cov(std::exchange(other.tot_sky_cov, 0.0)),
        rainfall(std::exchange(other.rainfall, 0.0)),
        snowdepth(std::exchange(other.snowdepth, 0.0)),
        pressure(std::exchange(other.pressure, 0.0)),
        month(std::exchange(other.month, 0)), day(std::exchange(other.day, 0)),
        hour(std::exchange(other.hour, 0)),
        minute(std::exchange(other.minute, 0)),
        second(std::exchange(other.second, 0)), next(std::move(other.next)) {}

  // Move assignment operator
  weather &operator=(weather &&other) noexcept {
    if (this != &other) {
      temperature = std::exchange(other.temperature, 0.0);
      humidity = std::exchange(other.humidity, 0.0);
      solar_dir = std::exchange(other.solar_dir, 0.0);
      solar_diff = std::exchange(other.solar_diff, 0.0);
      solar_global = std::exchange(other.solar_global, 0.0);
      global_horizontal_extra =
          std::exchange(other.global_horizontal_extra, 0.0);
      wind_speed = std::exchange(other.wind_speed, 0.0);
      wind_dir = std::exchange(other.wind_dir, 0.0);
      opq_sky_cov = std::exchange(other.opq_sky_cov, 0.0);
      tot_sky_cov = std::exchange(other.tot_sky_cov, 0.0);
      rainfall = std::exchange(other.rainfall, 0.0);
      snowdepth = std::exchange(other.snowdepth, 0.0);
      pressure = std::exchange(other.pressure, 0.0);

      month = std::exchange(other.month, 0);
      day = std::exchange(other.day, 0);
      hour = std::exchange(other.hour, 0);
      minute = std::exchange(other.minute, 0);
      second = std::exchange(other.second, 0);

      next = std::move(other.next);
    }
    return *this;
  }

  // Existing methods
  int create();
  TIMESTAMP sync(TIMESTAMP t0);

  // Data members
  double temperature = 0.0; // F
  double humidity = 0.0;
  double solar_dir = 0.0;
  double solar_diff = 0.0;
  double solar_global = 0.0;
  double global_horizontal_extra = 0.0;
  double wind_speed = 0.0;
  double wind_dir = 0.0;
  double opq_sky_cov = 0.0;
  double tot_sky_cov = 0.0;
  double rainfall = 0.0;
  double snowdepth = 0.0;
  double pressure = 0.0;
  int16 month = 0, day = 0, hour = 0, minute = 0, second = 0;

  // Use the custom deleter unique_ptr
  std::unique_ptr<weather, gld_object_deleter> next;
};

/*
class weather: public gld_object {
public:
        static CLASS *oclass;

        weather() = default;
        ~weather() override = default;

        weather(MODULE *module);
        int create();
        TIMESTAMP sync(TIMESTAMP t0);

        double temperature; // F
        double humidity;
        double solar_dir;
        double solar_diff;
        double solar_global;
        double global_horizontal_extra;
        double wind_speed;
        double wind_dir;
        double opq_sky_cov;
        double tot_sky_cov;
        double rainfall;
        double snowdepth;
        double pressure;
        int16 month, day, hour, minute, second;
        std::unique_ptr<weather> next;
};
*/

#endif

// EOF
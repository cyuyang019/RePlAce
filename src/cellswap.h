#ifndef __CELLSWAP__
#define __CELLSWAP__


#include <iostream>
#include <ot/timer/timer.hpp>
#include "cadb23IO.h"
#include "json.hpp"

namespace cellswap {

  static float z_btm, z_top;

  void setupIOConstraint(ot::Timer &timer, std::string sdc_file);

  void parse_config(std::string config_file);
  void swap_gates(ot::Timer &timer);

  void printDieStatistics(ot::Timer &timer);
  void printTimingStatistics(ot::Timer &timer);

}

namespace cadb23 {
  void UpdateGateLocation(const std::unordered_map<std::string, ot::Coord3d> &gate_locations, const std::unordered_map<std::string, ot::Coord2d> &HBT_locations);
}

#endif
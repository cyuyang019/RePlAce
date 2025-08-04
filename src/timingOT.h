#ifndef __TIMINGOT__
#define __TIMINGOT__


#include <iostream>
#include <ot/timer/timer.hpp>
#include <flute.h>
#include "replace_private.h"
#include "cadb23IO.h"

namespace ot {

void BuildSteiner(Timer &timer);

void DecomposeNet(NET *curNet, bool is_top, std::vector<std::tuple<std::string, std::string, float>> &caps,
  std::vector<std::tuple<std::string, std::string, float>> &ress);

}

void UpdateTimingGrad(std::unordered_map<std::string, std::vector<ot::WireGradData>> &wire_gradients);
void UpdateSteinerPoint(prec timing_phi_cof, prec alpha);

void timing_grad(int cell_idx, FPOS *grad);

#endif
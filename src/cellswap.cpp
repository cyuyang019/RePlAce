#include "cellswap.h"

namespace cellswap {

  void setupIOConstraint(ot::Timer &timer, std::string sdc_file) {
    timer.update_timing();

    // setup io constraints
    float maxClkPeriod = 0.0;
    for ( auto clk : timer.clocks() ) {
      if ( clk.second.period() > maxClkPeriod ) {
        maxClkPeriod = clk.second.period();
      }
    }

    for ( auto pi : timer.primary_inputs() ) {
      timer.set_at(pi.first, ot::MIN, ot::RISE, maxClkPeriod);
      timer.set_at(pi.first, ot::MAX, ot::RISE, -1e10);
      timer.set_at(pi.first, ot::MIN, ot::FALL, maxClkPeriod);
      timer.set_at(pi.first, ot::MAX, ot::FALL, -1e10);
      timer.set_slew(pi.first, ot::MIN, ot::RISE, 0.01f);
      timer.set_slew(pi.first, ot::MAX, ot::RISE, 0.01f);
      timer.set_slew(pi.first, ot::MIN, ot::FALL, 0.01f);
      timer.set_slew(pi.first, ot::MAX, ot::FALL, 0.01f);
    }

    for ( auto clk : timer.clocks() ) {
      timer.set_at(clk.first, ot::MIN, ot::RISE, 0);
      timer.set_at(clk.first, ot::MAX, ot::RISE, 0);
      timer.set_at(clk.first, ot::MIN, ot::FALL, 0);
      timer.set_at(clk.first, ot::MAX, ot::FALL, 0);
      timer.set_slew(clk.first, ot::MIN, ot::RISE, 0.01f);
      timer.set_slew(clk.first, ot::MAX, ot::RISE, 0.01f);
      timer.set_slew(clk.first, ot::MIN, ot::FALL, 0.01f);
      timer.set_slew(clk.first, ot::MAX, ot::FALL, 0.01f);
    }

    for ( auto po : timer.primary_outputs() ) {
      timer.set_rat(po.first, ot::MIN, ot::RISE, 0);
      timer.set_rat(po.first, ot::MAX, ot::RISE, 1e10);
      timer.set_rat(po.first, ot::MIN, ot::FALL, 0);
      timer.set_rat(po.first, ot::MAX, ot::FALL, 1e10);
      timer.set_load(po.first, ot::MIN, ot::RISE, 0.05f);
      timer.set_load(po.first, ot::MAX, ot::RISE, 0.05f);
      timer.set_load(po.first, ot::MIN, ot::FALL, 0.05f);
      timer.set_load(po.first, ot::MAX, ot::FALL, 0.05f);
    }

    if ( sdc_file != "" ) {
      timer.read_sdc(sdc_file);
    }
  }

  void printDieStatistics(ot::Timer &timer) {
    float top_area = 0, btn_area = 0;
    int top_count = 0, btn_count = 0;

    for ( auto &[gname, gate] : timer.gates() ) {
      if ( *gate.z_pos() > z_top / 2.f ) {
        if ( *gate.cell_view()[ot::Split::MIN]->is_top ) {
          top_area += *gate.cell_view()[ot::Split::MIN]->area;
        }
        else {
          auto conjugate_cell = gate.cell_view()[ot::Split::MIN]->conjugate_cell;
          top_area += ( *conjugate_cell )->area.value();
        }
        ++top_count;
      }
      else {
        if ( !( *gate.cell_view()[ot::Split::MIN]->is_top ) ) {
          btn_area += *gate.cell_view()[ot::Split::MIN]->area;
        }
        else {
          auto conjugate_cell = gate.cell_view()[ot::Split::MIN]->conjugate_cell;
          btn_area += ( *conjugate_cell )->area.value();
        }
        ++btn_count;
      }
    }

    std::cout << std::fixed;
    std::cout << "Top area = " << top_area << ", Btn area = " << btn_area << std::endl;
    std::cout << "Top count = " << top_count << ", Btn count = " << btn_count << std::endl;
  }

  void printTimingStatistics(ot::Timer &timer) {
    std::cout << std::fixed;
    std::cout << "TNS: " << timer.report_tns_elw(ot::Split::MAX).value() << std::endl;
    std::cout << "WNS: " << timer.report_wns(ot::Split::MAX).value() << std::endl;
  }

  void swap_gates(ot::Timer &timer) {

    float gamma = 1.f;
    float step_length = 10.f;
    float target_slack = 2.f;
    float wns_coeff = 10.0f;
    float tns_coeff = 1.0f;

    int iter_count = 50;

    timer.set_gamma(gamma)
      .set_step_length(step_length)
      .set_target_slack(target_slack)
      .set_wns_coeff(wns_coeff)
      .set_tns_coeff(tns_coeff);

    for ( int it = 0; it < iter_count; ++it ) {
      timer.update_gradient(true);
      timer.update_states();
      // printf("[it %2d]: wns = %7.2f, tns = %7.2f  \n", it + 1, *timer.report_wns(ot::Split::MAX), *timer.report_tns_elw(ot::Split::MAX));
      OT_LOGI("[it ", it+1, "]: wns = ", *timer.report_wns(ot::Split::MAX), ", tns = ", *timer.report_tns_elw(ot::Split::MAX));
    }
  }

}

namespace cadb23 {

void UpdateGateLocation(const std::unordered_map<std::string, ot::Coord3d> &gate_locations, const std::unordered_map<std::string, ot::Coord2d> &HBT_locations) {

  for ( const auto &[name, location] : gate_locations ) {
    if ( cadb23::gate_map.find(name) == cadb23::gate_map.end() ) {
      OT_LOGF("can't find gate ", name, " from cadb23 file");
    }

    cadb23::gate_map[name].pos.first = std::get<0>(location);
    cadb23::gate_map[name].pos.second = std::get<1>(location);
    cadb23::gate_map[name].reference_point = cadb23::Gate::CENTER;

    if ( std::abs(( float ) std::get<2>(location) - *ot::Gate::z_top) < 1e-5f ) {
      cadb23::gate_map[name].is_top = true;
    }
    else if ( std::abs(( float ) std::get<2>(location) - *ot::Gate::z_btm) < 1e-5f ) {
      cadb23::gate_map[name].is_top = false;
    }
    else {
      printf("%.5f\n", std::get<2>(location) - *ot::Gate::z_top);
      OT_LOGF("gate ", name, " is not discretized (z = ", std::get<2>(location), ")");
    }
  }

  cadb23::HBT_Count = 0;

  for ( const auto &[name, location] : HBT_locations ) {

    if ( cadb23::net_map.find(name) == cadb23::net_map.end() ) {
      OT_LOGW("can't find net ", name, " from cadb23 file, skipped");
      continue;
    }

    cadb23::net_map[name].HBT_pos.first = std::get<0>(location);
    cadb23::net_map[name].HBT_pos.second = std::get<1>(location);

    cadb23::net_map[name].is_3D = false;

    auto &pin_list = cadb23::net_map[name].pin_list;
    bool z_location;

    for ( int i = 0; i < pin_list.size(); ++i ) {
      std::string instName, pinName;
      size_t slash_pos = pin_list[i].find('/');
      if ( slash_pos != std::string::npos ) {
        instName = pin_list[i].substr(0, slash_pos);
        pinName = pin_list[i].substr(slash_pos + 1);
      }
      else {
        PrintError("Invalid pin format: " + pin_list[i]);
      }

      if ( i == 0 ) {
        z_location = cadb23::gate_map[instName].is_top;
      }
      else if ( cadb23::gate_map[instName].is_top != z_location ) {
        cadb23::net_map[name].is_3D = true;
        ++cadb23::HBT_Count;
        break;
      }
    }

  }

}

}
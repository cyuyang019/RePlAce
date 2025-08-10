#include "timingOT.h"

struct PairHash {
  std::size_t operator()(const std::pair<long long, long long> &p) const noexcept {
    std::size_t h1 = std::hash<long long>{}( p.first );
    std::size_t h2 = std::hash<long long>{}( p.second );
    return h1 ^ ( h2 + 0x9e3779b97f4a7c15 + ( h1 << 6 ) + ( h1 >> 2 ) );  // boost::hash_combine
  }
};

const std::string AUX_NAME = "AUX_POINT";

namespace ot {

  void BuildSteiner(Timer &timer) {

    Flute::readLUT("/mnt/RePlAce/module/flute/POWV9.dat", "/mnt/RePlAce/module/flute/PORT9.dat");

    for ( int netID = 0; netID < netCNT; ++netID ) {
      NET *curNet = &netInstance[netID];

      std::string netName = curNet->Name();
      std::vector<std::tuple<std::string, std::string, float>> caps;
      std::vector<std::tuple<std::string, std::string, float>> ress;

      if ( netName.substr(netName.size() >= 4 ? netName.size() - 4 : 0) == "_top" ) {
        continue;
      }

      // is 3D net
      if ( netName.substr(netName.size() >= 4 ? netName.size() - 4 : 0) == "_btm" ) {
        netName = netName.substr(0, netName.size() > 4 ? netName.size() - 4 : 0);

        NET *topNet = &netInstance[netID - 1];
        NET *btmNet = &netInstance[netID];

        std::string top_name = topNet->Name();
        std::string btm_name = btmNet->Name();

        std::vector<std::tuple<std::string, std::string, float>> caps_top, caps_btm;
        std::vector<std::tuple<std::string, std::string, float>> ress_top, ress_btm;

        assert(top_name.substr(top_name.size() >= 4 ? top_name.size() - 4 : 0) == "_top");
        assert(btm_name.substr(0, btm_name.size() > 4 ? btm_name.size() - 4 : 0) == top_name.substr(0, top_name.size() > 4 ? top_name.size() - 4 : 0));

        DecomposeNet(topNet, true, caps_top, ress_top);
        DecomposeNet(btmNet, false, caps_btm, ress_btm);

        caps = std::move(caps_top);
        caps.insert(caps.end(),
          std::make_move_iterator(caps_btm.begin()),
          std::make_move_iterator(caps_btm.end()));

        ress = std::move(ress_top);
        ress.insert(ress.end(),
          std::make_move_iterator(ress_btm.begin()),
          std::make_move_iterator(ress_btm.end()));

        // * Add HBT parasitics
        std::string topHBTPinName = netName + "_HBT:TOP";
        std::string btmHBTPinName = netName + "_HBT:BTM";

        ress.push_back(std::make_tuple(topHBTPinName, btmHBTPinName, HBT_res));

        bool foundTop = false, foundBtm = false;
        for ( int i = 0; i < caps.size(); ++i ) {
          std::string pinName = std::get<0>(caps[i]);
          float cap;
          if ( pinName == topHBTPinName ) {
            foundTop = true;
            cap = std::get<2>(caps[i]) + HBT_cap / 2.f;
            caps[i] = std::make_tuple(topHBTPinName, "", cap);
          }
          else if ( pinName == btmHBTPinName ) {
            foundBtm = true;
            cap = std::get<2>(caps[i]) + HBT_cap / 2.f;
            caps[i] = std::make_tuple(btmHBTPinName, "", cap);
          }
        }
        assert(foundTop && foundBtm);
      }
      else {
        // not 3D net
        bool is_top;
        if ( moduleInstance[curNet->pin[0]->moduleID].tier == 0 ) {
          is_top = true;
        }
        else if ( moduleInstance[curNet->pin[0]->moduleID].tier == 2 ) {
          is_top = false;
        }
        else {
          printf("[ERROR] module %s in net %s is at tier %d (neither in top die nor in bottom die).\n"
            , moduleInstance[curNet->pin[0]->moduleID].Name(), netName.c_str(), moduleInstance[curNet->pin[0]->moduleID].tier);
          exit(1);
        }

        DecomposeNet(curNet, is_top, caps, ress);

      }

      timer.set_net_rct(netName, caps, ress);

    }

  }

  void DecomposeNet(NET *curNet, bool is_top, std::vector<std::tuple<std::string, std::string, float>> &caps,
    std::vector<std::tuple<std::string, std::string, float>> &ress) {

    // printing

    // std::string netName = curNet->Name();
    // std::cout << netName << std::endl;

    // for ( auto pin : cadb23::net_map[netName].pin_list ) {
    //   std::cout << pin << " ";
    // }
    // std::cout << std::endl;

    // for ( int j = 0; j < curNet->pinCNTinObject; ++j ) {
    //   PIN *pin = curNet->pin[j];
    //   MODULE *mod = &moduleInstance[pin->moduleID];
    //   std::string pin_name = mPinName[mod->idx][pin->pinIDinModule];
    //   std::cout << mod->Name() << "(" << pin_name << "): " << pin->fp.x << " " << pin->fp.y << ", " << std::endl;
    // }
    // std::cout << std::endl;

    curNet->pin_map.clear();
    for ( int j = 0; j < curNet->pinCNTinObject; j++ ) {
      PIN *curPin = curNet->pin[j];
      std::string curPinName = std::string(moduleInstance[curPin->moduleID].Name()) + ":" + std::string(mPinName[curPin->moduleID][curPin->pinIDinModule]);
      curNet->pin_map[curPinName] = curPin;
    }

    std::string firPin_name, secPin_name;
    caps.clear();
    ress.clear();

    if ( curNet->pinCNTinObject == 1 ) {
      return;
    }
    else if ( curNet->pinCNTinObject == 2 ) {
      PIN *firPin = curNet->pin[0];
      PIN *secPin = curNet->pin[1];

      firPin_name = std::string(moduleInstance[firPin->moduleID].Name()) + ":" + std::string(mPinName[firPin->moduleID][firPin->pinIDinModule]);
      secPin_name = std::string(moduleInstance[secPin->moduleID].Name()) + ":" + std::string(mPinName[secPin->moduleID][secPin->pinIDinModule]);

      // std::cout << firPin_name << " " << secPin_name << " : " << wl << std::endl;

      float wl = fabs(firPin->fp.x - secPin->fp.x) + fabs(firPin->fp.y - secPin->fp.y);

      float cap = ( is_top ) ? wl * top_unit_cap / 2.f : wl * bot_unit_cap / 2.f;
      float res = ( is_top ) ? wl * top_unit_res : wl * bot_unit_res;

      caps.push_back(std::make_tuple(firPin_name, "", cap));
      caps.push_back(std::make_tuple(secPin_name, "", cap));
      ress.push_back(std::make_tuple(firPin_name, secPin_name, res));
    }
    else {
      std::string steinerPinName = ( is_top ) ? AUX_NAME + ":TSP" : AUX_NAME + ":BSP";

      // * Initialization for FLUTE
      std::unordered_map<std::string, float> pin2cap;
      long long *x = new long long[curNet->pinCNTinObject];
      long long *y = new long long[curNet->pinCNTinObject];

      int *mapping = new int[curNet->pinCNTinObject];

      // x, y coordi --> pin's name
      std::unordered_map< std::pair< long long, long long >, std::string, PairHash> pinMap;

      for ( int j = 0; j < curNet->pinCNTinObject; j++ ) {
        PIN *curPin = curNet->pin[j];
        std::string curPinName = std::string(moduleInstance[curPin->moduleID].Name()) + ":" + std::string(mPinName[curPin->moduleID][curPin->pinIDinModule]);

        x[j] = ( long long ) ( curPin->fp.x + 0.5f );
        y[j] = ( long long ) ( curPin->fp.y + 0.5f );

        bool toggle = true;
        while ( pinMap.find(std::make_pair(x[j], y[j])) != pinMap.end() ) {
          std::string overlapPinName = pinMap[std::make_pair(x[j], y[j])];
          // printf("[INFO] %s and %s are overlapped\n", overlapPinName.c_str(), curPinName.c_str());
          ( toggle ) ? ++x[j] : ++y[j];
          toggle = !toggle;
        }

        pinMap[std::make_pair(x[j], y[j])] = curPinName;
      }

      // * FLUTE
      Flute::Tree fluteTree = Flute::flute(curNet->pinCNTinObject, x, y, FLUTE_ACCURACY, mapping);
      delete[] x;
      delete[] y;
      delete[] mapping;

      // printtree(fluteTree);

      // * Decompose flute tree into wire segments
      for ( int i = 0; i < 2 * fluteTree.deg - 2; i++ ) {
        if ( i == fluteTree.branch[i].n ) {
          continue;
        }

        int j = fluteTree.branch[i].n;
        DBU x_max = std::max(fluteTree.branch[i].x, fluteTree.branch[j].x);
        DBU x_min = std::min(fluteTree.branch[i].x, fluteTree.branch[j].x);
        DBU y_max = std::max(fluteTree.branch[i].y, fluteTree.branch[j].y);
        DBU y_min = std::min(fluteTree.branch[i].y, fluteTree.branch[j].y);

        if ( x_max == x_min && y_max == y_min ) {
          continue;
        }

        if ( pinMap.count(std::make_pair(fluteTree.branch[i].x, fluteTree.branch[i].y)) ) {
          firPin_name = pinMap[std::make_pair(fluteTree.branch[i].x, fluteTree.branch[i].y)];
        }
        else {
          firPin_name = steinerPinName + std::to_string(i - fluteTree.deg + 1);
          pinMap[std::make_pair(fluteTree.branch[i].x, fluteTree.branch[i].y)] = firPin_name;
          // store the steiner point pin information
          PIN *firPin = new PIN;
          firPin->fp.Set(fluteTree.branch[i].x, fluteTree.branch[i].y);
          firPin->term = 2;
          curNet->pin_map[firPin_name] = firPin;
        }

        if ( pinMap.count(std::make_pair(fluteTree.branch[j].x, fluteTree.branch[j].y)) ) {
          secPin_name = pinMap[std::make_pair(fluteTree.branch[j].x, fluteTree.branch[j].y)];
        }
        else {
          secPin_name = steinerPinName + std::to_string(j - fluteTree.deg + 1);
          pinMap[std::make_pair(fluteTree.branch[j].x, fluteTree.branch[j].y)] = secPin_name;
          // store the steiner point pin information
          PIN *secPin = new PIN;
          secPin->fp.Set(fluteTree.branch[j].x, fluteTree.branch[j].y);
          secPin->term = 2;
          curNet->pin_map[secPin_name] = secPin;
        }

        float wl = x_max - x_min + y_max - y_min;

        float cap = ( is_top ) ? wl * top_unit_cap / 2.f : wl * bot_unit_cap / 2.f;
        float res = ( is_top ) ? wl * top_unit_res : wl * bot_unit_res;

        ( pin2cap.find(firPin_name) == pin2cap.end() ) ? pin2cap[firPin_name] = cap : pin2cap[firPin_name] += cap;
        ( pin2cap.find(secPin_name) == pin2cap.end() ) ? pin2cap[secPin_name] = cap : pin2cap[secPin_name] += cap;

        ress.push_back(std::make_tuple(firPin_name, secPin_name, res));

      }

      for ( auto &[pin_name, cap] : pin2cap ) {
        caps.push_back(std::make_tuple(pin_name, "", cap));
      }
    }

    // printing
    // std::cout << curNet->Name() << std::endl;
    // for ( auto &[fpin, spin, cap] : caps ) {
    //   std::cout << fpin << ": " << cap << std::endl;
    // }

    // for ( auto &[fpin, spin, res] : ress ) {
    //   std::cout << fpin << " <-> " << spin << ": " << res << std::endl;
    // }
    // std::cout << std::endl;

  }

}

void UpdateTimingGrad(std::unordered_map<std::string, std::vector<ot::WireGradData>> &wire_gradients) {
  for ( int netID = 0; netID < netCNT; ++netID ) {
    NET *curNet = &netInstance[netID];

    curNet->timing_grad_map.clear();

    std::string netName = curNet->Name();

    if ( netName.substr(netName.size() >= 4 ? netName.size() - 4 : 0) == "_top" ) {
      continue;
    }

    // is 3D net
    if ( netName.substr(netName.size() >= 4 ? netName.size() - 4 : 0) == "_btm" ) {
      netName = netName.substr(0, netName.size() > 4 ? netName.size() - 4 : 0);

      NET *topNet = &netInstance[netID - 1];
      NET *btmNet = &netInstance[netID];

      std::string top_name = topNet->Name();
      std::string btm_name = btmNet->Name();


      if ( wire_gradients.find(netName) == wire_gradients.end() ) {
        printf("[WARN] Net %s is not found.\n", netName.c_str());
        continue;
      }
      auto &wire_grads = wire_gradients[netName];

      for ( auto &[pin1Name, pin2Name, gradient] : wire_grads ) {

        // handling exception (temporarily)
        if ( std::abs(gradient) > 1e3f ) {
          continue;
        }

        // std::cout << pin1Name << " <-> " << pin2Name << " = " << gradient << std::endl;

        bool is_top = false, is_btm = false;

        // pin 1
        PIN *pin1;
        if ( topNet->pin_map.find(pin1Name) != topNet->pin_map.end() ) {
          pin1 = topNet->pin_map[pin1Name];
          is_top = true;
        }
        else if ( btmNet->pin_map.find(pin1Name) != btmNet->pin_map.end() ) {
          pin1 = btmNet->pin_map[pin1Name];
          is_btm = true;
        }
        else {
          printf("[ERROR] Pin %s is not found in Net %s.\n", pin1Name.c_str(), netName.c_str());
          exit(1);
        }

        // pin 2
        PIN *pin2;
        if ( topNet->pin_map.find(pin2Name) != topNet->pin_map.end() ) {
          pin2 = topNet->pin_map[pin2Name];
          is_top = true;
        }
        else if ( btmNet->pin_map.find(pin2Name) != btmNet->pin_map.end() ) {
          pin2 = btmNet->pin_map[pin2Name];
          is_btm = true;
        }
        else {
          printf("[ERROR] Pin %s is not found in Net %s.\n", pin2Name.c_str(), netName.c_str());
          exit(1);
        }

        if ( is_top && is_btm ) {
          // ignore gradient propagation
          // printf("[INFO] Wire %s <-> %s is a 3D wire, ignore gradient propagation...\n", pin1Name.c_str(), pin2Name.c_str());
        }
        else if ( is_top ) {
          curNet->timing_grad_map.insert({ pin1, std::make_pair(pin2, gradient) });
          curNet->timing_grad_map.insert({ pin2, std::make_pair(pin1, gradient) });
        }
        else if ( is_btm ) {
          curNet->timing_grad_map.insert({ pin1, std::make_pair(pin2, gradient) });
          curNet->timing_grad_map.insert({ pin2, std::make_pair(pin1, gradient) });
        }
        else {
          printf("[ERROR] Wire %s <-> %s is neither in top die nor in bottom die.\n", pin1Name.c_str(), pin2Name.c_str());
          exit(1);
        }


      }


    }
    else {
      // not 3D net
      if ( wire_gradients.find(netName) == wire_gradients.end() ) {
        printf("[WARN] Net %s is not found.\n", netName.c_str());
        continue;
      }
      auto &wire_grads = wire_gradients[netName];

      for ( auto &[pin1Name, pin2Name, gradient] : wire_grads ) {

        // handling exception (temporarily)
        if ( std::abs(gradient) > 1e3f ) {
          continue;
        }

        // std::cout << pin1Name << " <-> " << pin2Name << " = " << gradient << std::endl;

        // pin 1
        PIN *pin1;
        if ( curNet->pin_map.find(pin1Name) != curNet->pin_map.end() ) {
          pin1 = curNet->pin_map[pin1Name];
        }
        else {
          printf("[ERROR] Pin %s is not found in Net %s.\n", pin1Name.c_str(), netName.c_str());
          exit(1);
        }

        // pin 2
        PIN *pin2;
        if ( curNet->pin_map.find(pin2Name) != curNet->pin_map.end() ) {
          pin2 = curNet->pin_map[pin2Name];
        }
        else {
          printf("[ERROR] Pin %s is not found in Net %s.\n", pin2Name.c_str(), netName.c_str());
          exit(1);
        }

        curNet->timing_grad_map.insert({ pin1, std::make_pair(pin2, gradient) });
        curNet->timing_grad_map.insert({ pin2, std::make_pair(pin1, gradient) });

      }


    }


  }
  // for ( int netID = 0; netID < netCNT; ++netID ) {
  //   NET *curNet = &netInstance[netID];
  //   printf("Net %s:\n", curNet->Name());
  //   for ( auto [pin1, grad_pair] : curNet->timing_grad_map ) {
  //     auto pin2 = grad_pair.first;
  //     auto grad = grad_pair.second;

  //     if ( pin1->term == 2 || pin2->term == 2 ) {
  //       printf("%f, %f <-> %f, %f : %f\n", pin1->fp.x, pin1->fp.y, pin2->fp.x, pin2->fp.y, grad);
  //     }
  //   }
  // }
  // exit(1);
}

void UpdateSteinerPoint(prec timing_phi_cof, prec alpha) {
  for ( int netID = 0; netID < netCNT; ++netID ) {
    NET *curNet = &netInstance[netID];

    for ( auto pin_pair: curNet->pin_map ) {
      PIN *curPin = pin_pair.second;
      if ( curPin->term != 2 ) {
        continue;
      }

      auto range = curNet->timing_grad_map.equal_range(curPin);
      // printf("len = %d\n", std::distance(range.first, range.second));
      for ( auto it = range.first; it != range.second; ++it ) {
        PIN *pin2 = it->second.first;
        prec gradient = it->second.second;

        prec x_diff = std::abs(curPin->fp.x - pin2->fp.x);
        prec y_diff = std::abs(curPin->fp.y - pin2->fp.y);

        if ( x_diff < 1e-6 && y_diff < 1e-6 ) {
          continue;
        }

        prec x_dir = ( x_diff < 1e-6 ) ? 0.f : ( curPin->fp.x < pin2->fp.x ? 1.f : -1.f );
        prec y_dir = ( y_diff < 1e-6 ) ? 0.f : ( curPin->fp.y < pin2->fp.y ? 1.f : -1.f );

        curPin->fp.x += timing_phi_cof * alpha * x_dir * gradient;
        curPin->fp.y += timing_phi_cof * alpha * y_dir * gradient;

        // printf("%f\n", timing_phi_cof * alpha * gradient);

      }
    }
  }

  // exit(0);
}

void timing_grad(int cell_idx, FPOS *grad) {
  grad->SetZero();

  CELL *cell = &gcell_st[cell_idx];
  PIN *pin = NULL;
  NET *net = NULL;
  FPOS net_grad;

  grad->SetZero();

  for ( int i = 0; i < cell->pinCNTinObject; i++ ) {
    pin = cell->pin[i];
    net = &netInstance[pin->netID];

    if ( net->pinCNTinObject <= 1 ) {
      continue;
    }

    auto range = net->timing_grad_map.equal_range(pin);
    for ( auto it = range.first; it != range.second; ++it ) {
      PIN *pin2 = it->second.first;
      prec gradient = it->second.second;

      prec x_diff = std::abs(pin->fp.x - pin2->fp.x);
      prec y_diff = std::abs(pin->fp.y - pin2->fp.y);

      if ( x_diff < 1e-6 && y_diff < 1e-6 ) {
        continue;
      }

      prec x_dir = ( x_diff < 1e-6 ) ? 0.f : ( pin->fp.x < pin2->fp.x ? 1.f : -1.f );
      prec y_dir = ( y_diff < 1e-6 ) ? 0.f : ( pin->fp.y < pin2->fp.y ? 1.f : -1.f );

      grad->x += x_dir * gradient;
      grad->y += y_dir * gradient;

    }
  }

}
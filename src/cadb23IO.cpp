#include "cadb23IO.h"
#include "bookShelfIO.h"

namespace cadb23 {

// Placement Info Container
std::map<std::string, Gate> gate_map;

std::map<std::string, Net> net_map;

std::map<std::string, std::map<std::string, Point> > top_cell_offset;
std::map<std::string, std::map<std::string, Point> > btm_cell_offset;

std::map<std::string, Point> top_cell_size;
std::map<std::string, Point> btm_cell_size;

int DieWidth, DieHeight;

int TopRowWidth, TopRowHeight, BtmRowWidth, BtmRowHeight, TopRowCount, BtmRowCount;

int HBT_Width, HBT_Height, HBT_Spacing, HBT_Cost, HBT_Count;

int MaxUtilTop, MaxUtilBtm;


void ParseCADB23_I(std::string cadb23inName) {
  printf("[INFO] cadbin file: %s\n", cadb23inName.c_str());
  std::ifstream istream(cadb23inName);
  if ( !istream ) {
    std::cerr << "[Error]: Failed to open file: " << cadb23inName << std::endl;
    exit(EXIT_FAILURE);
  }

  std::string buf;
  int cell_num, gate_num, net_num;

  std::string cell_type, pin_name, inst_name, net_name;
  int x, y, pin_num, w, h;

  istream >> buf >> buf >> buf >> buf >> cell_num;

  printf("[INFO] Top tech cells: %d\n", cell_num);

  for ( int c = 0; c < cell_num; ++c ) {
    istream >> buf >> buf >> cell_type >> w >> h >> pin_num;
    top_cell_size[cell_type] = std::make_pair(w, h);
    top_cell_offset[cell_type] = std::map<std::string, Point>();
    for ( int p = 0; p < pin_num; ++p ) {
      istream >> buf >> pin_name >> x >> y;
      top_cell_offset[cell_type][pin_name] = std::make_pair(x, y);
    }
  }

  istream >> buf >> buf >> cell_num;

  printf("[INFO] Bottom tech cells: %d\n", cell_num);

  for ( int c = 0; c < cell_num; ++c ) {
    istream >> buf >> buf >> cell_type >> w >> h >> pin_num;
    btm_cell_size[cell_type] = std::make_pair(w, h);
    btm_cell_offset[cell_type] = std::map<std::string, Point>();
    for ( int p = 0; p < pin_num; ++p ) {
      istream >> buf >> pin_name >> x >> y;
      btm_cell_offset[cell_type][pin_name] = std::make_pair(x, y);
    }
  }


  istream >> buf >> buf >> buf >> DieWidth >> DieHeight;
  printf("[INFO] Die size: %dx%d\n", DieWidth, DieHeight);

  istream >> buf >> MaxUtilTop;
  printf("[INFO] Top max util: %d%%\n", MaxUtilTop);

  istream >> buf >> MaxUtilBtm;
  printf("[INFO] Bottom max util: %d%%\n", MaxUtilBtm);

  istream >> buf >> buf >> buf >> TopRowWidth >> TopRowHeight >> TopRowCount;
  istream >> buf >> buf >> buf >> BtmRowWidth >> BtmRowHeight >> BtmRowCount;

  do {
    istream >> buf;
  }
  while ( buf != "TerminalSize" );

  istream >> HBT_Width >> HBT_Height >> buf >> HBT_Spacing >> buf >> HBT_Cost;
  printf("[INFO] HBT size: %dx%d\n", HBT_Width, HBT_Height);
  printf("[INFO] HBT spacing: %d\n", HBT_Spacing);
  printf("[INFO] HBT cost: %d\n", HBT_Cost);


  istream >> buf >> gate_num;
  printf("[INFO] Instance num: %d\n", gate_num);

  for ( int i = 0; i < gate_num; ++i ) {
    istream >> buf >> inst_name >> cell_type;
    Gate new_gate;
    new_gate.cell_type = cell_type;
    new_gate.inst_name = inst_name;

    gate_map[inst_name] = new_gate;
  }

  istream >> buf >> net_num;
  printf("[INFO] Net num: %d\n", net_num);

  for ( int n = 0; n < net_num; ++n ) {
    istream >> buf >> net_name >> pin_num;
    Net new_net;
    new_net.net_name = net_name;
    for ( int p = 0; p < pin_num; ++p ) {
      istream >> buf >> pin_name;
      new_net.pin_list.push_back(pin_name);

      size_t dpos = pin_name.find('/');
      std::string inst = pin_name.substr(0, dpos);
      std::string pin = pin_name.substr(dpos + 1);

      if ( pin.find("Z") != std::string::npos || pin.find("Q") != std::string::npos ) {
        new_net.pin_IO.push_back(OUTPUT);
      }
      else if ( ( gate_map[inst].cell_type.find("HA") != std::string::npos || gate_map[inst].cell_type.find("FA") != std::string::npos ) \
        && ( pin.find("S") != std::string::npos || pin.find("CO") != std::string::npos ) ) {
        new_net.pin_IO.push_back(OUTPUT);
      }
      else {
        new_net.pin_IO.push_back(INPUT);
      }
    }
    net_map[net_name] = new_net;
  }

  istream.close();
}

void ParseCADB23_O(std::string cadb23outName) {
  printf("[INFO] cadbout file: %s\n", cadb23outName.c_str());
  std::ifstream istream(cadb23outName);
  if ( !istream ) {
    std::cerr << "Error: Failed to open file: " << cadb23outName << std::endl;
    exit(EXIT_FAILURE);
  }

  std::string buffer;
  int top_gate_num, btm_gate_num;

  std::string inst_name, net_name;
  float x, y;

  istream >> buffer >> top_gate_num;

  printf("[INFO] Top insts: %d\n", top_gate_num);

  for ( int i = 0; i < top_gate_num; ++i ) {
    istream >> buffer >> inst_name >> x >> y >> buffer;
    // printf("%s: %d %d\n", inst_name.c_str(), x, y);

    gate_map[inst_name].is_top = true;
    gate_map[inst_name].pos = std::make_pair(x, y);
    gate_map[inst_name].reference_point = cadb23::Gate::BTMLEFT;
  }

  istream >> buffer >> btm_gate_num;

  printf("[INFO] Bottom insts: %d\n", btm_gate_num);

  for ( int i = 0; i < btm_gate_num; ++i ) {
    istream >> buffer >> inst_name >> x >> y >> buffer;
    // printf("%s: %d %d\n", inst_name.c_str(), x, y);

    gate_map[inst_name].is_top = false;
    gate_map[inst_name].pos = std::make_pair(x, y);
  }

  istream >> buffer >> HBT_Count;

  printf("[INFO] HBTs: %d\n", HBT_Count);

  for ( int i = 0; i < HBT_Count; ++i ) {
    istream >> buffer >> net_name >> x >> y;
    // printf("%s: %d %d\n", net_name.c_str(), x, y);

    net_map[net_name].HBT_pos = std::make_pair(x, y);
    net_map[net_name].is_3D = true;
  }

  istream.close();
}

void WriteBookShelf(std::string path) {

  std::ofstream ofs_top(path + "top_die.aux");
  ofs_top << "RowBasedPlacement : top_die.nodes top_die.nets top_die.wts top_die.pl top_die.scl" << std::endl;
  ofs_top.close();
  ofs_top.open(path + "top_die.wts");
  ofs_top << "UCLA wts 1.0" << std::endl;
  ofs_top.close();

  std::ofstream ofs_HBT(path + "HBT_layer.aux");
  ofs_HBT << "RowBasedPlacement : HBT_layer.nodes HBT_layer.nets HBT_layer.wts HBT_layer.pl HBT_layer.scl" << std::endl;
  ofs_HBT.close();
  ofs_HBT.open(path + "HBT_layer.wts");
  ofs_HBT << "UCLA wts 1.0" << std::endl;
  ofs_HBT.close();

  std::ofstream ofs_btm(path + "btm_die.aux");
  ofs_btm << "RowBasedPlacement : btm_die.nodes btm_die.nets btm_die.wts btm_die.pl btm_die.scl" << std::endl;
  ofs_btm.close();
  ofs_btm.open(path + "btm_die.wts");
  ofs_btm << "UCLA wts 1.0" << std::endl;
  ofs_btm.close();

  WriteBookShelf_pl(path);
  WriteBookShelf_scl(path);
  WriteBookShelf_nodes(path);
  WriteBookShelf_nets(path);
}

void WriteBookShelf_pl(std::string path) {
  std::ofstream ofs_top(path + "top_die.pl");
  std::ofstream ofs_HBT(path + "HBT_layer.pl");
  std::ofstream ofs_btm(path + "btm_die.pl");

  ofs_top << "UCLA pl 1.0" << std::endl << std::endl;
  ofs_HBT << "UCLA pl 1.0" << std::endl << std::endl;
  ofs_btm << "UCLA pl 1.0" << std::endl << std::endl;

  std::string line;

  for ( const auto &[gate_name, gate] : gate_map ) {
    if ( gate.is_top ) {
      ofs_top << gate_name << " " << gate.pos.first << " " << gate.pos.second << " : N" << std::endl;
    }
    else { 
      ofs_btm << gate_name << " " << gate.pos.first << " " << gate.pos.second << " : N" << std::endl;
    }
  }

  for ( const auto &[net_name, net] : net_map ) {
    if ( net.is_3D ) {
      float HBT_posX = net.HBT_pos.first - ( HBT_Width + HBT_Spacing ) / 2.f;
      float HBT_posY = net.HBT_pos.second - ( HBT_Height + HBT_Spacing ) / 2.f;
      ofs_top << net_name << "_HBT " << HBT_posX << " " << HBT_posY << " : N /FIXED" << std::endl;
      ofs_btm << net_name << "_HBT " << HBT_posX << " " << HBT_posY << " : N /FIXED" << std::endl;
      ofs_HBT << net_name << "_HBT " << HBT_posX << " " << HBT_posY << " : N" << std::endl;
      for ( const auto inst_pin : net.pin_list ) {
        std::string instName, pinName;
        size_t slash_pos = inst_pin.find('/');
        if ( slash_pos != std::string::npos ) {
          instName = inst_pin.substr(0, slash_pos);
          pinName = inst_pin.substr(slash_pos + 1);
        }
        else {
          PrintError("Invalid pin format: " + inst_pin);
        }

        auto &cur_gate = gate_map[instName];
        ofs_HBT << instName << " " << cur_gate.pos.first << " " << cur_gate.pos.second << " : N /FIXED" << std::endl;
      }
    }
  }

  
  ofs_top.close();
  ofs_HBT.close();
  ofs_btm.close();
}

void WriteBookShelf_scl(std::string path) {

  // Top die
  std::ofstream ofs_top(path + "top_die.scl");

  ofs_top << "UCLA scl 1.0" << std::endl << std::endl;
  ofs_top << "NumRows : " << TopRowCount << std::endl << std::endl;

  for ( int r = 0; r < TopRowCount; ++r ) {
    ofs_top << "CoreRow Horizontal" << std::endl;
    ofs_top << "  Coordinate : " << r * TopRowHeight << std::endl;
    ofs_top << "  Height : " << TopRowHeight << std::endl;
    ofs_top << "  Sitewidth : 1" << std::endl;
    ofs_top << "  Sitespacing : 1" << std::endl;
    ofs_top << "  Siteorient : N" << std::endl;
    ofs_top << "  Sitesymmetry : Y" << std::endl;
    ofs_top << "  SubrowOrigin : 0	NumSites : " << TopRowWidth << std::endl;
    ofs_top << "End" << std::endl;
  }
  ofs_top.close();

  // Bottom die
  std::ofstream ofs_btm(path + "btm_die.scl");

  ofs_btm << "UCLA scl 1.0" << std::endl << std::endl;
  ofs_btm << "NumRows : " << BtmRowCount << std::endl << std::endl;

  for ( int r = 0; r < BtmRowCount; ++r ) {
    ofs_btm << "CoreRow Horizontal" << std::endl;
    ofs_btm << "  Coordinate : " << r * BtmRowHeight << std::endl;
    ofs_btm << "  Height : " << BtmRowHeight << std::endl;
    ofs_btm << "  Sitewidth : 1" << std::endl;
    ofs_btm << "  Sitespacing : 1" << std::endl;
    ofs_btm << "  Siteorient : N" << std::endl;
    ofs_btm << "  Sitesymmetry : Y" << std::endl;
    ofs_btm << "  SubrowOrigin : 0	NumSites : " << BtmRowWidth << std::endl;
    ofs_btm << "End" << std::endl;
  }
  ofs_btm.close();

  // HBT layer
  int HBTRowWidth = DieWidth;
  int HBTRowHeight = HBT_Height + HBT_Spacing;
  int HBTRowCount = DieHeight / HBTRowHeight;

  std::ofstream ofs_HBT(path + "HBT_layer.scl");

  ofs_HBT << "UCLA scl 1.0" << std::endl << std::endl;
  ofs_HBT << "NumRows : " << HBTRowCount << std::endl << std::endl;

  for ( int r = 0; r < HBTRowCount; ++r ) {
    ofs_HBT << "CoreRow Horizontal" << std::endl;
    ofs_HBT << "  Coordinate : " << r * HBTRowHeight << std::endl;
    ofs_HBT << "  Height : " << HBTRowHeight << std::endl;
    ofs_HBT << "  Sitewidth : 1" << std::endl;
    ofs_HBT << "  Sitespacing : 1" << std::endl;
    ofs_HBT << "  Siteorient : N" << std::endl;
    ofs_HBT << "  Sitesymmetry : Y" << std::endl;
    ofs_HBT << "  SubrowOrigin : 0	NumSites : " << HBTRowWidth << std::endl;
    ofs_HBT << "End" << std::endl;
  }
  ofs_HBT.close();

}

void WriteBookShelf_nodes(std::string path) {

  int top_node_count = 0, btm_node_count = 0;

  std::vector<std::string> top_buffer, HBT_buffer, btm_buffer;
  std::unordered_set<std::string> HBT_pins;
  std::string line;

  for ( const auto &[gate_name, gate] : gate_map ) {
    if ( gate.is_top ) {
      line = gate.inst_name + " " + std::to_string(top_cell_size[gate.cell_type].first) + " " + std::to_string(top_cell_size[gate.cell_type].second);
      top_buffer.push_back(line);
      ++top_node_count;
    }
    else {
      line = gate.inst_name + " " + std::to_string(btm_cell_size[gate.cell_type].first) + " " + std::to_string(btm_cell_size[gate.cell_type].second);
      btm_buffer.push_back(line);
      ++btm_node_count;
    }
  }

  for ( const auto &[net_name, net] : net_map ) {
    if ( net.is_3D ) {
      line = net_name + "_HBT " + std::to_string(HBT_Width + HBT_Spacing) + " " + std::to_string(HBT_Height + HBT_Spacing);
      HBT_buffer.insert(HBT_buffer.begin(), line);
      line = net_name + "_HBT 0.0000 0.0000 terminal";
      top_buffer.push_back(line);
      btm_buffer.push_back(line);

      for ( const auto inst_pin : net.pin_list ) {
        std::string instName, pinName;
        size_t slash_pos = inst_pin.find('/');
        if ( slash_pos != std::string::npos ) {
          instName = inst_pin.substr(0, slash_pos);
          pinName = inst_pin.substr(slash_pos + 1);
        }
        else {
          PrintError("Invalid pin format: " + inst_pin);
        }

        if ( HBT_pins.find(instName) == HBT_pins.end() ) {
          line = instName + " 0.0000 0.0000 terminal";
          HBT_buffer.push_back(line);
          HBT_pins.emplace(instName);
        }
      }
    }
  }

  std::ofstream ofs_top(path + "top_die.nodes");
  std::ofstream ofs_HBT(path + "HBT_layer.nodes");
  std::ofstream ofs_btm(path + "btm_die.nodes");

  ofs_top << "UCLA nodes 1.0" << std::endl << std::endl
    << "NumNodes : " << top_node_count + HBT_Count << std::endl
    << "NumTerminals : " << HBT_Count << std::endl << std::endl;
  ofs_HBT << "UCLA nodes 1.0" << std::endl << std::endl
    << "NumNodes : " << HBT_Count + HBT_pins.size() << std::endl
    << "NumTerminals : " << HBT_pins.size() << std::endl << std::endl;
  ofs_btm << "UCLA nodes 1.0" << std::endl << std::endl
    << "NumNodes : " << btm_node_count + HBT_Count << std::endl
    << "NumTerminals : " << HBT_Count << std::endl << std::endl;

  for ( const std::string line : top_buffer ) {
    ofs_top << line << std::endl;
  }
  for ( const std::string line : HBT_buffer ) {
    ofs_HBT << line << std::endl;
  }
  for ( const std::string line : btm_buffer ) {
    ofs_btm << line << std::endl;
  }

  ofs_top.close();
  ofs_HBT.close();
  ofs_btm.close();
}

void WriteBookShelf_nets(std::string path) {
  int top_net_count = 0, btm_net_count = 0, HBT_net_count = 0;
  int top_pin_count = 0, btm_pin_count = 0, HBT_pin_count = 0;

  std::vector<std::string> top_buffer, HBT_buffer, btm_buffer;
  std::string line;

  for ( const auto &[net_name, net] : net_map ) {
    if ( net.is_3D ) {
      ++top_net_count; ++btm_net_count;
      HBT_net_count += 2;
      std::vector< std::pair<std::string, std::string> > top_gates, btm_gates;
      for ( const auto &inst_pin : net.pin_list ) {
        std::string instName, pinName;
        size_t slash_pos = inst_pin.find('/');
        if ( slash_pos != std::string::npos ) {
          instName = inst_pin.substr(0, slash_pos);
          pinName = inst_pin.substr(slash_pos + 1);
        }
        else {
          PrintError("Invalid pin format: " + inst_pin);
        }

        auto cur_gate = gate_map[instName];
        if ( cur_gate.is_top ) {
          top_gates.push_back(std::make_pair(instName, pinName));
        }
        else {
          btm_gates.push_back(std::make_pair(instName, pinName));
        }
      }

      // Top die
      top_pin_count += top_gates.size() + 1;
      HBT_pin_count += top_gates.size() + 1;
      line = "NetDegree : " + std::to_string(top_gates.size() + 1);
      top_buffer.push_back(line);
      HBT_buffer.push_back(line);
      for ( const auto &[instName, pinName] : top_gates ) {
        auto &top_gate = gate_map[instName];
        auto offset = top_cell_offset[top_gate.cell_type][pinName];
        auto size = top_cell_size[top_gate.cell_type];
        float center_offsetX = offset.first - size.first / 2.f;
        float center_offsetY = offset.second - size.second / 2.f;
        line = "  " + top_gate.inst_name + " I : " + std::to_string(center_offsetX) + " " + std::to_string(center_offsetY);

        top_buffer.push_back(line);
        HBT_buffer.push_back(line);
      }
      line = "  " + net_name + "_HBT I : 0 0";
      top_buffer.push_back(line);
      HBT_buffer.push_back(line);

      // Bottom die
      btm_pin_count += btm_gates.size() + 1;
      HBT_pin_count += btm_gates.size() + 1;
      line = "NetDegree : " + std::to_string(btm_gates.size() + 1);
      btm_buffer.push_back(line);
      HBT_buffer.push_back(line);
      for ( const auto &[instName, pinName] : btm_gates ) {
        auto &btm_gate = gate_map[instName];
        auto offset = btm_cell_offset[btm_gate.cell_type][pinName];
        auto size = btm_cell_size[btm_gate.cell_type];
        float center_offsetX = offset.first - size.first / 2.f;
        float center_offsetY = offset.second - size.second / 2.f;
        line = "  " + btm_gate.inst_name + " I : " + std::to_string(center_offsetX) + " " + std::to_string(center_offsetY);

        btm_buffer.push_back(line);
        HBT_buffer.push_back(line);
      }
      line = "  " + net_name + "_HBT I : 0 0";
      btm_buffer.push_back(line);
      HBT_buffer.push_back(line);

    }
    else {
      std::vector<std::string> line_buffer;
      bool is_top = false;

      line = "NetDegree : " + std::to_string(net.pin_list.size());
      line_buffer.push_back(line);
      for ( const auto &inst_pin : net.pin_list ) {
        std::string instName, pinName;
        size_t slash_pos = inst_pin.find('/');
        if ( slash_pos != std::string::npos ) {
          instName = inst_pin.substr(0, slash_pos);
          pinName = inst_pin.substr(slash_pos + 1);
        }
        else {
          PrintError("Invalid pin format: " + inst_pin);
        }

        auto cur_gate = gate_map[instName];
        is_top = cur_gate.is_top;

        float center_offsetX, center_offsetY;
        if ( is_top ) {
          auto offset = top_cell_offset[cur_gate.cell_type][pinName];
          auto size = top_cell_size[cur_gate.cell_type];
          center_offsetX = offset.first - size.first / 2.f;
          center_offsetY = offset.second - size.second / 2.f;
        }
        else {
          auto offset = btm_cell_offset[cur_gate.cell_type][pinName];
          auto size = btm_cell_size[cur_gate.cell_type];
          center_offsetX = offset.first - size.first / 2.f;
          center_offsetY = offset.second - size.second / 2.f;
        }

        line = "  " + cur_gate.inst_name + " I : " + std::to_string(center_offsetX) + " " + std::to_string(center_offsetY);
        
        line_buffer.push_back(line);
      }

      if ( is_top ) {
        top_buffer.insert(top_buffer.end(), line_buffer.begin(), line_buffer.end());
        ++top_net_count;
        top_pin_count += net.pin_list.size();
      }
      else {
        btm_buffer.insert(btm_buffer.end(), line_buffer.begin(), line_buffer.end());
        ++btm_net_count;
        btm_pin_count += net.pin_list.size();
      }
    }
  }

  std::ofstream ofs_top(path + "top_die.nets");
  std::ofstream ofs_HBT(path + "HBT_layer.nets");
  std::ofstream ofs_btm(path + "btm_die.nets");

  ofs_top << "UCLA nets 1.0" << std::endl << std::endl
    << "NumNets : " << top_net_count << std::endl
    << "NumPins : " << top_pin_count << std::endl << std::endl;
  ofs_HBT << "UCLA nets 1.0" << std::endl << std::endl
    << "NumNets : " << HBT_net_count << std::endl
    << "NumPins : " << HBT_pin_count << std::endl << std::endl;
  ofs_btm << "UCLA nets 1.0" << std::endl << std::endl
    << "NumNets : " << btm_net_count << std::endl
    << "NumPins : " << btm_pin_count << std::endl << std::endl;

  for ( const std::string line : top_buffer ) {
    ofs_top << line << std::endl;
  }
  for ( const std::string line : HBT_buffer ) {
    ofs_HBT << line << std::endl;
  }
  for ( const std::string line : btm_buffer ) {
    ofs_btm << line << std::endl;
  }

  ofs_top.close();
  ofs_HBT.close();
  ofs_btm.close();
}

void UpdateGateLocation() {
  printf("[INFO] updating gate locations from global placement..\n");
  for ( int i = 0; i < moduleCNT; i++ ) {
    MODULE *curModule = &moduleInstance[i];

    std::string modName = curModule->Name();
    std::string netName;

    if ( modName.length() >= 4 && modName.substr(modName.size() - 4) == "_HBT" ) {
      netName = modName.substr(0, modName.size() - 4);
      Net &curNet = net_map[netName];
      assert(curNet.is_3D);

      curNet.HBT_pos = std::make_pair(curModule->center.x, curModule->center.y);
    }
    else {
      Gate &curGate = gate_map[modName];
      curGate.pos = std::make_pair(curModule->pmin.x, curModule->pmin.y);
      curGate.reference_point = Gate::BTMLEFT;
    }

  }
}

}

std::map< std::string, int > moduleMap;

void Initialize3DGates() {
  PrintProcBegin("3D cells initialization");

  // Free existing memory if any before reinitialization
  delete[] moduleInstance;
  delete[] total_std_area_3D;
  delete[] moduleCNT_3D;
  moduleMap.clear();
  moduleNameStor.clear();

  // Container initialization
  moduleInstance = new MODULE[cadb23::gate_map.size() + cadb23::HBT_Count];
  total_std_area_3D = new prec[numLayer];
  moduleCNT_3D = new int[numLayer];
  for ( int i = 0; i < numLayer; ++i ) {
    total_std_area_3D[i] = 0;
    moduleCNT_3D[i] = 0;
  }

  MODULE *curModule = nullptr;

  placementMacroCNT = 0;

  // Initialize top and bottom gates
  int idx = 0;
  for ( const auto &[gate_name, curGate] : cadb23::gate_map ) {
    // No additional constructor needed for MODULE since "new" initializes it
    curModule = &moduleInstance[idx];

    if ( curGate.is_top ) {
      curModule->tier = 0; // Top tier
      curModule->size.Set(( prec ) cadb23::top_cell_size[curGate.cell_type].first,
        ( prec ) cadb23::top_cell_size[curGate.cell_type].second);
      ++moduleCNT_3D[0]; // Increment top tier count
    }
    else {
      curModule->tier = 2; // Bottom tier
      curModule->size.Set(( prec ) cadb23::btm_cell_size[curGate.cell_type].first,
        ( prec ) cadb23::btm_cell_size[curGate.cell_type].second);
      ++moduleCNT_3D[2]; // Increment bottom tier count
    }

    // set half_size
    curModule->half_size.Set(curModule->size.x / 2.f, curModule->size.y / 2.f);

    // pmin/pmax/center info update
    if ( curGate.reference_point == cadb23::Gate::CENTER ) {
      curModule->center.Set(curGate.pos.first, curGate.pos.second);
      curModule->pmin.Set(curModule->center.x - curModule->half_size.x,
        curModule->center.y - curModule->half_size.y);
      curModule->pmax.SetAdd(curModule->pmin, curModule->size);
    }
    else {
      curModule->pmin.Set(curGate.pos.first, curGate.pos.second);
      curModule->pmax.SetAdd(curModule->pmin, curModule->size);
      curModule->center.SetAdd(curModule->pmin, curModule->half_size);
    }
    

    // set area
    curModule->area = curModule->size.GetProduct();
    if ( curGate.is_top ) {
      total_std_area_3D[0] += curModule->area; // Top tier area
    }
    else {
      total_std_area_3D[2] += curModule->area; // Bottom tier area
    }

    // set Name
    moduleNameStor.push_back(gate_name);
    moduleMap[gate_name] = idx;

    // set Index
    curModule->idx = idx;

    // set flag
    curModule->flg = StdCell;

    ++idx;
  }

  moduleCNT = idx; // Update module count

  // Initialize HBTs
  for ( const auto &[net_name, curNet] : cadb23::net_map ) {
    if ( !curNet.is_3D ) {
      continue;
    }

    curModule = &moduleInstance[idx];

    curModule->tier = 1; // HBT tier
    curModule->size.Set(cadb23::HBT_Width + cadb23::HBT_Spacing, cadb23::HBT_Height + cadb23::HBT_Spacing);

    ++moduleCNT_3D[1]; // Increment HBT tier count

    curModule->half_size.Set(curModule->size.x / 2.f, curModule->size.y / 2.f);

    curModule->center.Set(curNet.HBT_pos.first, curNet.HBT_pos.second);

    curModule->pmin.Set(curModule->center.x - curModule->half_size.x,
      curModule->center.y - curModule->half_size.y);
    curModule->pmax.SetAdd(curModule->pmin, curModule->size);

    // set area
    curModule->area = curModule->size.GetProduct();
    total_std_area_3D[1] += curModule->area; // HBT tier area

    // set Name
    moduleNameStor.push_back(net_name + "_HBT");
    moduleMap[net_name + "_HBT"] = idx;

    // set Index
    curModule->idx = idx;

    // set flag
    curModule->flg = StdCell;

    ++idx;
  }

  printf("[INFO] Initialized %d Top cells.\n", moduleCNT_3D[0]);
  printf("[INFO] Initialized %d HBTs.\n", moduleCNT_3D[1]);
  printf("[INFO] Initialized %d Bottom cells.\n", moduleCNT_3D[2]);

  // Update moduleCNT to include HBTs
  moduleCNT = idx;

  PrintProcEnd("3D cells initialization");
}

void Initialize3DICNets() {
  PrintProcBegin("3D nets initialization");

  netCNT = cadb23::net_map.size() + cadb23::HBT_Count;
  pinCNT = 0;
  for ( auto &[net_name, curNet] : cadb23::net_map ) {
    pinCNT += curNet.pin_list.size();
  }
  pinCNT += 2 * cadb23::HBT_Count;

  // memory reservation
  delete[] netInstance; // Free existing memory if any
  delete[] pinInstance; // Free existing memory if any
  netInstance = new NET[netCNT];
  pinInstance = new PIN[pinCNT];

  MODULE *curModule = nullptr;
  NET *curNet = nullptr;
  PIN *curPin = nullptr;

  int pinIdx = 0;
  int netIdx = 0;

  tPinName.clear();  // terminals are not considered under current research
  mPinName.resize(moduleCNT);


  // Net initialization
  for ( auto &[net_name, net] : cadb23::net_map ) {

    if ( net.is_3D ) {
      std::string instName;
      FPOS curOffset;
      int io;

      // Partition the net into top_net and btm_net
      std::vector<std::string> pin_list_top, pin_list_btm;
      for ( auto &inst_pin : net.pin_list ) {
        std::string instName, pinName;
        size_t slash_pos = inst_pin.find('/');
        if ( slash_pos != std::string::npos ) {
          instName = inst_pin.substr(0, slash_pos);
          pinName = inst_pin.substr(slash_pos + 1);
        }
        else {
          PrintError("Invalid pin format: " + inst_pin);
        }

        const auto &curGate = cadb23::gate_map[instName];
        cadb23::Point offset;
        if ( curGate.is_top ) {
          pin_list_top.push_back(inst_pin);
        }
        else {
          pin_list_btm.push_back(inst_pin);
        }
      }

      // top net
      curNet = &netInstance[netIdx];
      std::string top_net_name = net_name + "_top";

      netNameStor.push_back(top_net_name);

      netNameMap[top_net_name] = netIdx;

      curNet->idx = netIdx;
      curNet->timingWeight = 0;

      curNet->terminalMin.Set(place.end);
      curNet->terminalMax.Set(place.org);

      curNet->pinCNTinObject = pin_list_top.size() + 1; // 1 for HBT
      curNet->pin = new PIN * [curNet->pinCNTinObject];

      for ( int i = 0; i < pin_list_top.size(); ++i ) {
        std::string instName, pinName;
        size_t slash_pos = pin_list_top[i].find('/');
        if ( slash_pos != std::string::npos ) {
          instName = pin_list_top[i].substr(0, slash_pos);
          pinName = pin_list_top[i].substr(slash_pos + 1);
        }
        else {
          PrintError("Invalid pin format: " + pin_list_top[i]);
        }

        const auto &curGate = cadb23::gate_map[instName];

        // Get Offset Infomation
        cadb23::Point offset = cadb23::top_cell_offset[curGate.cell_type][pinName];
        FPOS curOffset(( prec ) offset.first, ( prec ) offset.second);

        // Get IO information
        int io = ( net.pin_IO[i] == cadb23::INPUT ) ? 0 : 1; // 0 for input, 1 for output

        // pin Instnace mapping
        curPin = &pinInstance[pinIdx];
        curNet->pin[i] = curPin;

        // Add module information
        auto mtPtr = moduleMap.find(instName);
        if ( mtPtr == moduleMap.end() ) {
          cout << "** ERROR:  Net Instance ( " << instName
            << " ) does not exist in COMPONENTS/PINS statement "
            << "(moduleMap) " << endl;
          exit(1);
        }

        curModule = &moduleInstance[mtPtr->second];

        // save module pin Name into mPinName
        mPinName[mtPtr->second].push_back(pinName);

        AddPinInfoForModuleAndTerminal(
          &curModule->pin, &curModule->pof, curModule->pinCNTinObject++,
          curOffset, curModule->idx, netIdx, i, pinIdx++, io, false);

      }

      // HBT Pin
      instName = net_name + "_HBT";
      // Get Offset Infomation
      curOffset.Set(( prec ) ( cadb23::HBT_Width + cadb23::HBT_Spacing ) / 2.f,
        ( prec ) ( cadb23::HBT_Height + cadb23::HBT_Spacing ) / 2.f);

      // Get IO information
      io = 0; // 0 for input, 1 for output

      // pin Instnace mapping
      curPin = &pinInstance[pinIdx];
      curNet->pin[pin_list_top.size()] = curPin;

      // Add module information
      auto mtPtr_top = moduleMap.find(instName);
      if ( mtPtr_top == moduleMap.end() ) {
        cout << "** ERROR:  Net Instance ( " << instName
          << " ) does not exist in COMPONENTS/PINS statement "
          << "(moduleMap) " << endl;
        exit(1);
      }

      curModule = &moduleInstance[mtPtr_top->second];

      // save module pin Name into "TOP"
      mPinName[mtPtr_top->second].push_back("TOP");

      AddPinInfoForModuleAndTerminal(
        &curModule->pin, &curModule->pof, curModule->pinCNTinObject++,
        curOffset, curModule->idx, netIdx, pin_list_top.size(), pinIdx++, io, false);

      ++netIdx;

      // btm net
      curNet = &netInstance[netIdx];
      std::string btm_net_name = net_name + "_btm";

      netNameStor.push_back(btm_net_name);

      netNameMap[btm_net_name] = netIdx;

      curNet->idx = netIdx;
      curNet->timingWeight = 0;

      curNet->terminalMin.Set(place.end);
      curNet->terminalMax.Set(place.org);

      curNet->pinCNTinObject = pin_list_btm.size() + 1; // 1 for HBT
      curNet->pin = new PIN * [curNet->pinCNTinObject];

      for ( int i = 0; i < pin_list_btm.size(); ++i ) {
        std::string instName, pinName;
        size_t slash_pos = pin_list_btm[i].find('/');
        if ( slash_pos != std::string::npos ) {
          instName = pin_list_btm[i].substr(0, slash_pos);
          pinName = pin_list_btm[i].substr(slash_pos + 1);
        }
        else {
          PrintError("Invalid pin format: " + pin_list_btm[i]);
        }

        const auto &curGate = cadb23::gate_map[instName];

        // Get Offset Infomation
        cadb23::Point offset = cadb23::btm_cell_offset[curGate.cell_type][pinName];
        FPOS curOffset(( prec ) offset.first, ( prec ) offset.second);

        // Get IO information
        int io = ( net.pin_IO[i] == cadb23::INPUT ) ? 0 : 1; // 0 for input, 1 for output

        // pin Instnace mapping
        curPin = &pinInstance[pinIdx];
        curNet->pin[i] = curPin;

        // Add module information
        auto mtPtr = moduleMap.find(instName);
        if ( mtPtr == moduleMap.end() ) {
          cout << "** ERROR:  Net Instance ( " << instName
            << " ) does not exist in COMPONENTS/PINS statement "
            << "(moduleMap) " << endl;
          exit(1);
        }

        curModule = &moduleInstance[mtPtr->second];

        // save module pin Name into mPinName
        mPinName[mtPtr->second].push_back(pinName);

        AddPinInfoForModuleAndTerminal(
          &curModule->pin, &curModule->pof, curModule->pinCNTinObject++,
          curOffset, curModule->idx, netIdx, i, pinIdx++, io, false);

      }

      // HBT Pin
      instName = net_name + "_HBT";
      // Get Offset Infomation
      curOffset.Set(( prec ) ( cadb23::HBT_Width + cadb23::HBT_Spacing ) / 2.f,
        ( prec ) ( cadb23::HBT_Height + cadb23::HBT_Spacing ) / 2.f);

      // Get IO information
      io = 0; // 0 for input, 1 for output

      // pin Instnace mapping
      curPin = &pinInstance[pinIdx];
      curNet->pin[pin_list_btm.size()] = curPin;

      // Add module information
      auto mtPtr_btm = moduleMap.find(instName);
      if ( mtPtr_btm == moduleMap.end() ) {
        cout << "** ERROR:  Net Instance ( " << instName
          << " ) does not exist in COMPONENTS/PINS statement "
          << "(moduleMap) " << endl;
        exit(1);
      }

      curModule = &moduleInstance[mtPtr_btm->second];

      // save module pin Name into "BTM"
      mPinName[mtPtr_top->second].push_back("BTM");

      AddPinInfoForModuleAndTerminal(
        &curModule->pin, &curModule->pof, curModule->pinCNTinObject++,
        curOffset, curModule->idx, netIdx, pin_list_btm.size(), pinIdx++, io, false);

      ++netIdx;

      continue;
    }

    curNet = &netInstance[netIdx];

    netNameStor.push_back(net_name);

    netNameMap[net_name] = netIdx;

    curNet->idx = netIdx;
    curNet->timingWeight = 0;

    curNet->terminalMin.Set(place.end);
    curNet->terminalMax.Set(place.org);

    curNet->pinCNTinObject = net.pin_list.size();
    curNet->pin = new PIN * [curNet->pinCNTinObject];

    // if(net_name == "n263164") {
    //   printf("%d pins in total\n", curNet->pinCNTinObject);
    // }

    for ( int i = 0; i < curNet->pinCNTinObject; ++i ) {
      std::string instName, pinName;
      size_t slash_pos = net.pin_list[i].find('/');
      if ( slash_pos != std::string::npos ) {
        instName = net.pin_list[i].substr(0, slash_pos);
        pinName = net.pin_list[i].substr(slash_pos + 1);
      }
      else {
        PrintError("Invalid pin format in net: " + net_name);
      }

      const auto &curGate = cadb23::gate_map[instName];
      cadb23::Point offset;
      if ( curGate.is_top ) {
        offset = cadb23::top_cell_offset[curGate.cell_type][pinName];
      }
      else {
        offset = cadb23::btm_cell_offset[curGate.cell_type][pinName];
      }

      // Get Offset Infomation
      FPOS curOffset(( prec ) offset.first, ( prec ) offset.second);

      // Get IO information
      int io = ( net.pin_IO[i] == cadb23::INPUT ) ? 0 : 1; // 0 for input, 1 for output

      // pin Instnace mapping
      curPin = &pinInstance[pinIdx];
      curNet->pin[i] = curPin;

      // Add module information
      auto mtPtr = moduleMap.find(instName);
      if ( mtPtr == moduleMap.end() ) {
        cout << "** ERROR:  Net Instance ( " << instName
          << " ) does not exist in COMPONENTS/PINS statement "
          << "(moduleMap) " << endl;
        exit(1);
      }

      curModule = &moduleInstance[mtPtr->second];

      // save module pin Name into mPinName
      mPinName[mtPtr->second].push_back(pinName);

      // if ( net_name == "n263164" ) {
      //   printf("net id %d, mod id %d: %s %s\n", netIdx, curModule->idx, instName.c_str(), pinName.c_str());
      // }

      AddPinInfoForModuleAndTerminal(
        &curModule->pin, &curModule->pof, (curModule->pinCNTinObject)++,
        curOffset, curModule->idx, netIdx, i, pinIdx++, io, false);

    }

    ++netIdx;
  }


  // update instance number
  pinCNT = pinIdx;
  netCNT = netIdx;

  PrintInfoInt("NumNets", netCNT);
  PrintInfoInt("NumPins", pinCNT);


  // Update HPWL Information
  for ( int i = 0; i < netCNT; ++i ) {
    NET *net = &netInstance[i];
    net->min_x = net->terminalMin.x;
    net->min_y = net->terminalMin.y;
    net->max_x = net->terminalMax.x;
    net->max_y = net->terminalMax.y;

    for ( int j = 0; j < net->pinCNTinObject; j++ ) {
      PIN *pin = net->pin[j];

      MODULE *curModule = &moduleInstance[pin->moduleID];
      FPOS pof = curModule->pof[pin->pinIDinModule];
      FPOS pmin = curModule->pmin;

      FPOS fp;
      fp.x = pmin.x + pof.x;
      fp.y = pmin.y + pof.y;
      pin->fp = fp;

      net->min_x = std::min(net->min_x, fp.x);
      net->min_y = std::min(net->min_y, fp.y);
      net->max_x = std::max(net->max_x, fp.x);
      net->max_y = std::max(net->max_y, fp.y);

    }
  }

  PrintProcEnd("3D nets initialization");
}

void UpdateFromBookshelf(std::string bookshelf_path) {

  std::ifstream ifs_top(bookshelf_path + "top_die.lg.pl");
  std::ifstream ifs_HBT(bookshelf_path + "HBT_layer.lg.pl");
  std::ifstream ifs_btm(bookshelf_path + "btm_die.lg.pl");

  std::string line, buf, gate_name;
  prec x_pos, y_pos;

  std::getline(ifs_top, line);  // Skip header
  while ( std::getline(ifs_top, line) ) {
    // Stop if we encounter "/FIXED"
    if ( line.find("/FIXED") != std::string::npos ) {
      break;
    }

    std::istringstream iss(line);
    if ( iss >> gate_name >> x_pos >> y_pos >> buf >> buf ) {
      MODULE *curModule = &moduleInstance[moduleMap[gate_name]];
      curModule->pmin.Set(x_pos, y_pos);
      curModule->center.SetAdd(curModule->pmin, curModule->half_size);
      curModule->pmax.SetAdd(curModule->pmin, curModule->size);
    }
  }

  std::getline(ifs_HBT, line);  // Skip header
  while ( std::getline(ifs_HBT, line) ) {
    // Stop if we encounter "/FIXED"
    if ( line.find("/FIXED") != std::string::npos ) {
      break;
    }

    std::istringstream iss(line);
    if ( iss >> gate_name >> x_pos >> y_pos >> buf >> buf ) {
      MODULE *curModule = &moduleInstance[moduleMap[gate_name]];
      curModule->pmin.Set(x_pos, y_pos);
      curModule->center.SetAdd(curModule->pmin, curModule->half_size);
      curModule->pmax.SetAdd(curModule->pmin, curModule->size);
    }
  }

  std::getline(ifs_btm, line);  // Skip header
  while ( std::getline(ifs_btm, line) ) {
    // Stop if we encounter "/FIXED"
    if ( line.find("/FIXED") != std::string::npos ) {
      break;
    }

    std::istringstream iss(line);
    if ( iss >> gate_name >> x_pos >> y_pos >> buf >> buf ) {
      MODULE *curModule = &moduleInstance[moduleMap[gate_name]];
      curModule->pmin.Set(x_pos, y_pos);
      curModule->center.SetAdd(curModule->pmin, curModule->half_size);
      curModule->pmax.SetAdd(curModule->pmin, curModule->size);
    }
  }

  ifs_top.close();
  ifs_HBT.close();
  ifs_btm.close();

  // Update HPWL Information
  for ( int i = 0; i < netCNT; ++i ) {
    NET *net = &netInstance[i];
    net->min_x = net->terminalMin.x;
    net->min_y = net->terminalMin.y;
    net->max_x = net->terminalMax.x;
    net->max_y = net->terminalMax.y;

    for ( int j = 0; j < net->pinCNTinObject; j++ ) {
      PIN *pin = net->pin[j];

      MODULE *curModule = &moduleInstance[pin->moduleID];
      FPOS pof = curModule->pof[pin->pinIDinModule];
      FPOS pmin = curModule->pmin;

      FPOS fp;
      fp.x = pmin.x + pof.x;
      fp.y = pmin.y + pof.y;
      pin->fp = fp;

      net->min_x = std::min(net->min_x, fp.x);
      net->min_y = std::min(net->min_y, fp.y);
      net->max_x = std::max(net->max_x, fp.x);
      net->max_y = std::max(net->max_y, fp.y);

    }
  }

  // Update to Cadb format
  cadb23::UpdateGateLocation();
}
#ifndef __CADBIO__
#define __CADBIO__


#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <sstream>

#include "replace_private.h"
#include "lefdefIO.h"

extern std::map< std::string, int > moduleMap;

void Initialize3DGates();
void Initialize3DICNets();

void UpdateFromBookshelf(std::string bookshelf_path);

namespace cadb23 {
  
// Data Type Definition
typedef std::pair<float, float> Point;

enum Location { TOP, BTM, HBT };
enum IO { INPUT, OUTPUT };

struct WireSegment {
  std::string pin1_name, pin2_name;
  Point pin1_pos, pin2_pos;
  int wirelength;
  // bool is_top;
  Location location;
};

struct Gate {
  std::string inst_name;
  std::string cell_type;
  bool is_top;
  Point pos;
  enum { BTMLEFT, CENTER } reference_point;
};

struct Net {
  Net() { HBT_pos = std::make_pair(-1, -1); is_3D = false; }
  std::string net_name;
  std::vector<std::string> pin_list;
  std::vector<IO> pin_IO;
  Point HBT_pos;
  std::vector<WireSegment> wire_segments;
  bool is_3D;
  std::string HBT_top_name, HBT_btm_name;
};

// IO function
void ParseCADB23_I(std::string cadb23inName);
void ParseCADB23_O(std::string cadb23outName);
// void parse_pl_in(std::string cadb23inName);
void WriteBookShelf(std::string path);
void WriteBookShelf_pl(std::string path);
void WriteBookShelf_scl(std::string path);
void WriteBookShelf_nodes(std::string path);
void WriteBookShelf_nets(std::string path);

void UpdateGateLocation();

// Placement Info Container
extern std::map<std::string, Gate> gate_map;

extern std::map<std::string, Net> net_map;

extern std::map<std::string, std::map<std::string, Point> > top_cell_offset;
extern std::map<std::string, std::map<std::string, Point> > btm_cell_offset;

extern std::map<std::string, Point> top_cell_size;
extern std::map<std::string, Point> btm_cell_size;

extern int DieWidth, DieHeight;

extern int HBT_Width, HBT_Height, HBT_Spacing, HBT_Cost, HBT_Count;

extern int MaxUtilTop, MaxUtilBtm;

}

#endif
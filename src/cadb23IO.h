#ifndef __CADBIO__
#define __CADBIO__


#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>


namespace cadb23_io {
    // Data Type Definition
    typedef std::pair<int, int> Point;

    enum Location { TOP, BOT, HBT };
    enum IO { INPUT, OUTPUT };

    struct WireSegment {
        std::string pin1_name, pin2_name;
        Point pin1_pos, pin2_pos;
        int wirelength;
        // bool is_top;
        Location location;
    };

    struct Cell {
        std::string inst_name;
        std::string cell_type;
        bool is_top;
        Point pos;
    };

    struct Net {
        Net() { HBT_pos = std::make_pair(-1, -1); is_3D = false; }
        std::string net_name;
        std::vector<std::string> pin_list;
        std::vector<IO> pin_IO;
        Point HBT_pos;
        std::vector<WireSegment> wire_segments;
        bool is_3D;
        std::string HBT_top_name, HBT_bot_name;
    };

    // IO function
    void ParseCADB23_I(std::string cadb23inName);
    void ParseCADB23_O(std::string cadb23outName);
    // void parse_pl_in(std::string cadb23inName);
}

#endif
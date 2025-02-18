#include "cadb23IO.h"

namespace cadb23_io {
    // Placement Info Container
    std::map<std::string, Cell> cell_map;

    std::map<std::string, Net> net_map;

    std::map<std::string, std::map<std::string, Point> > top_cell_offset;

    std::map<std::string, std::map<std::string, Point> > bot_cell_offset;

    int DieWidth, DieHeight;

    int HBT_Width, HBT_Height, HBT_Spacing, HBT_Cost;
    
    int MaxUtilTop, MaxUtilBot;


    void ParseCADB23_I(std::string cadb23inName) {
        printf("[INFO] cadbin file: %s\n", cadb23inName.c_str());
        std::ifstream istream(cadb23inName);
        if ( !istream ) {
            std::cerr << "[Error]: Failed to open file: " << cadb23inName << std::endl;
            exit(EXIT_FAILURE);
        }

        std::string buf;
        int cell_num, inst_num, net_num;

        std::string cell_type, pin_name, inst_name, net_name;
        int x, y, pin_num;

        istream >> buf >> buf >> buf >> buf >> cell_num;

        printf("[INFO] Top tech cells: %d\n", cell_num);

        for ( int c = 0; c < cell_num; ++c ) {
            istream >> buf >> buf >> cell_type >> buf >> buf >> pin_num;
            top_cell_offset[cell_type] = std::map<std::string, Point>();
            for ( int p = 0; p < pin_num; ++p ) {
                istream >> buf >> pin_name >> x >> y;
                top_cell_offset[cell_type][pin_name] = std::make_pair(x, y);
            }
        }

        istream >> buf >> buf >> cell_num;

        printf("[INFO] Bottom tech cells: %d\n", cell_num);

        for ( int c = 0; c < cell_num; ++c ) {
            istream >> buf >> buf >> cell_type >> buf >> buf >> pin_num;
            bot_cell_offset[cell_type] = std::map<std::string, Point>();
            for ( int p = 0; p < pin_num; ++p ) {
                istream >> buf >> pin_name >> x >> y;
                bot_cell_offset[cell_type][pin_name] = std::make_pair(x, y);
            }
        }

        // do {
        //     istream >> buf;
        // }
        // while ( buf != "DieSize" );

        istream >> buf >> buf >> buf >> DieWidth >> DieHeight;
        printf("[INFO] Die size: %dx%d\n", DieWidth, DieHeight);

        istream >> buf >> MaxUtilTop;
        printf("[INFO] Top max util: %d%\n", MaxUtilTop);

        istream >> buf >> MaxUtilBot;
        printf("[INFO] Bot max util: %d%\n", MaxUtilBot);

        do {
            istream >> buf;
        }
        while ( buf != "TerminalSize" );

        istream >> HBT_Width >> HBT_Height >> buf >> HBT_Spacing >> buf >> HBT_Cost;
        printf("[INFO] HBT size: %dx%d\n", HBT_Width, HBT_Height);
        printf("[INFO] HBT spacing: %d\n", HBT_Spacing);
        printf("[INFO] HBT cost: %d\n", HBT_Cost);


        istream >> buf >> inst_num;
        printf("[INFO] Instance num: %d\n", inst_num);

        for ( int i = 0; i < inst_num; ++i ) {
            istream >> buf >> inst_name >> cell_type;
            Cell new_cell;
            new_cell.cell_type = cell_type;
            new_cell.inst_name = inst_name;

            cell_map[inst_name] = new_cell;
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
                else if ( ( cell_map[inst].cell_type.find("HA") != std::string::npos || cell_map[inst].cell_type.find("FA") != std::string::npos ) \
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
        int top_cell_num, bot_cell_num, HBT_num;

        std::string inst_name, net_name;
        int x, y;

        istream >> buffer >> top_cell_num;

        printf("[INFO] Top insts: %d\n", top_cell_num);

        for ( int i = 0; i < top_cell_num; ++i ) {
            istream >> buffer >> inst_name >> x >> y >> buffer;
            // printf("%s: %d %d\n", inst_name.c_str(), x, y);

            cell_map[inst_name].is_top = true;
            cell_map[inst_name].pos = std::make_pair(x, y);
        }

        istream >> buffer >> bot_cell_num;

        printf("[INFO] Bottom insts: %d\n", bot_cell_num);

        for ( int i = 0; i < bot_cell_num; ++i ) {
            istream >> buffer >> inst_name >> x >> y >> buffer;
            // printf("%s: %d %d\n", inst_name.c_str(), x, y);

            cell_map[inst_name].is_top = false;
            cell_map[inst_name].pos = std::make_pair(x, y);
        }

        istream >> buffer >> HBT_num;

        printf("[INFO] HBTs: %d\n", HBT_num);

        for ( int i = 0; i < HBT_num; ++i ) {
            istream >> buffer >> net_name >> x >> y;
            // printf("%s: %d %d\n", net_name.c_str(), x, y);

            net_map[net_name].HBT_pos = std::make_pair(x, y);
            net_map[net_name].is_3D = true;
        }

        istream.close();
    }
}





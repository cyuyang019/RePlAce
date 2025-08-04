create_clock -period 4.7 -name wb_clk_i [get_ports wb_clk_i]
create_clock -period 4.7 -name mtx_clk_pad_i [get_ports mtx_clk_pad_i]
create_clock -period 4.7 -name mrx_clk_pad_i [get_ports mrx_clk_pad_i]

set_input_delay 0 -min -rise [get_ports wb_rst_i] -clock clk
set_input_delay 0 -min -fall [get_ports wb_rst_i] -clock clk
set_input_delay 0 -max -rise [get_ports wb_rst_i] -clock clk
set_input_delay 0 -max -fall [get_ports wb_rst_i] -clock clk
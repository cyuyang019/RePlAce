create_clock -period 10 -name clk_i [get_ports clk_i]
create_clock -period 10 -name dwb_clk_i [get_ports dwb_clk_i]
create_clock -period 10 -name iwb_clk_i [get_ports iwb_clk_i]

set_input_delay 0 -min -rise [get_ports rst_i] -clock clk_i
set_input_delay 0 -min -fall [get_ports rst_i] -clock clk_i
set_input_delay 0 -max -rise [get_ports rst_i] -clock clk_i
set_input_delay 0 -max -fall [get_ports rst_i] -clock clk_i

set_input_delay 0 -min -rise [get_ports iwb_rst_i] -clock clk_i
set_input_delay 0 -min -fall [get_ports iwb_rst_i] -clock clk_i
set_input_delay 0 -max -rise [get_ports iwb_rst_i] -clock clk_i
set_input_delay 0 -max -fall [get_ports iwb_rst_i] -clock clk_i

set_input_delay 0 -min -rise [get_ports dwb_rst_i] -clock clk_i
set_input_delay 0 -min -fall [get_ports dwb_rst_i] -clock clk_i
set_input_delay 0 -max -rise [get_ports dwb_rst_i] -clock clk_i
set_input_delay 0 -max -fall [get_ports dwb_rst_i] -clock clk_i
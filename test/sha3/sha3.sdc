create_clock -period 2 -name clk [get_ports clk]

set_input_delay 0 -min -rise [get_ports reset] -clock clk
set_input_delay 0 -min -fall [get_ports reset] -clock clk
set_input_delay 0 -max -rise [get_ports reset] -clock clk
set_input_delay 0 -max -fall [get_ports reset] -clock clk
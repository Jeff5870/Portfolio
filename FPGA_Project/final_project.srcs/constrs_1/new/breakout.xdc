# Clock & Reset
set_property -dict {PACKAGE_PIN P17 IOSTANDARD LVCMOS33} [get_ports CLK_100M ]
set_property -dict {PACKAGE_PIN P15 IOSTANDARD LVCMOS33} [get_ports RESET ]


# VGA Sync
set_property -dict {PACKAGE_PIN D7 IOSTANDARD LVCMOS33} [get_ports vga_hs_pin]
set_property -dict {PACKAGE_PIN C4 IOSTANDARD LVCMOS33} [get_ports vga_vs_pin]

# VGA Red (4-bit)
set_property -dict {PACKAGE_PIN F5 IOSTANDARD LVCMOS33} [get_ports {vga_R_Data_pin[0]}]
set_property -dict {PACKAGE_PIN C6 IOSTANDARD LVCMOS33} [get_ports {vga_R_Data_pin[1]}]
set_property -dict {PACKAGE_PIN C5 IOSTANDARD LVCMOS33} [get_ports {vga_R_Data_pin[2]}]
set_property -dict {PACKAGE_PIN B7 IOSTANDARD LVCMOS33} [get_ports {vga_R_Data_pin[3]}]

# VGA Green (4-bit)
set_property -dict {PACKAGE_PIN B6 IOSTANDARD LVCMOS33} [get_ports {vga_G_Data_pin[0]}]
set_property -dict {PACKAGE_PIN A6 IOSTANDARD LVCMOS33} [get_ports {vga_G_Data_pin[1]}]
set_property -dict {PACKAGE_PIN A5 IOSTANDARD LVCMOS33} [get_ports {vga_G_Data_pin[2]}]
set_property -dict {PACKAGE_PIN D8 IOSTANDARD LVCMOS33} [get_ports {vga_G_Data_pin[3]}]

# VGA Blue (4-bit)
set_property -dict {PACKAGE_PIN C7 IOSTANDARD LVCMOS33} [get_ports {vga_B_Data_pin[0]}]
set_property -dict {PACKAGE_PIN E6 IOSTANDARD LVCMOS33} [get_ports {vga_B_Data_pin[1]}]
set_property -dict {PACKAGE_PIN E5 IOSTANDARD LVCMOS33} [get_ports {vga_B_Data_pin[2]}]
set_property -dict {PACKAGE_PIN E7 IOSTANDARD LVCMOS33} [get_ports {vga_B_Data_pin[3]}]

set_property IOSTANDARD LVCMOS33 [get_ports {LED[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {LED[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {LED[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {LED[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {LED[4]}]
set_property IOSTANDARD LVCMOS33 [get_ports {LED[5]}]
set_property IOSTANDARD LVCMOS33 [get_ports {LED[6]}]
set_property IOSTANDARD LVCMOS33 [get_ports {LED[7]}]

set_property PACKAGE_PIN K2 [get_ports {LED[0]}]
set_property PACKAGE_PIN J2 [get_ports {LED[1]}]
set_property PACKAGE_PIN J3 [get_ports {LED[2]}]
set_property PACKAGE_PIN H4 [get_ports {LED[3]}]
set_property PACKAGE_PIN J4 [get_ports {LED[4]}]
set_property PACKAGE_PIN G3 [get_ports {LED[5]}]
set_property PACKAGE_PIN G4 [get_ports {LED[6]}]
set_property PACKAGE_PIN F6 [get_ports {LED[7]}]

set_property -dict {PACKAGE_PIN V1 IOSTANDARD LVCMOS33} [get_ports btnL]
set_property -dict {PACKAGE_PIN R11 IOSTANDARD LVCMOS33} [get_ports btnR]
set_property -dict {PACKAGE_PIN U4 IOSTANDARD LVCMOS33} [get_ports start_btn]
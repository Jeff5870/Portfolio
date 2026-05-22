
module deb(
input wire clk, reset, sw,
output reg db );
// symbolic state declaration
localparam [2:0]  zero = 3'b000,
wait1_1 = 3'b001,
wait1_2 = 3'b010,
wait1_3 = 3'b011,
one = 3'b100,
wait0_1 = 3'b101,
wait0_2 = 3'b110,
wait0_3 = 3'b111;

// signal declaration
wire m_tick;
reg [2:0] state_reg, state_next;
// generation of a tick with period 10ms ~= 2^20 * 10ns
tick #(20) tick_10ms(clk, reset, m_tick);
// state register
always @(posedge clk, posedge reset)
if (reset)
state_reg <= zero;
else
state_reg <= state_next;

// next-state logic
always @*
begin
state_next = state_reg; // default state: the same
case (state_reg)
  zero: begin
    if (sw)
      state_next = wait0_1;
  end

  wait0_1: begin
    if (m_tick)
      state_next = wait0_2;
  end
  wait0_2: begin
    if (m_tick)
      state_next = wait0_3;
  end
  wait0_3: begin
    if (m_tick)
      state_next = one;
  end

  one: begin
    if (~sw)
      state_next = wait1_1;
  end

  wait1_1: begin
    if (m_tick)
      state_next = wait1_2;
  end
  wait1_2: begin
    if (m_tick)
      state_next = wait1_3;
  end
  wait1_3: begin
    if (m_tick)
      state_next = zero;
  end

  default: state_next = zero;
endcase

end

// output logic
always @*
begin
case (state_reg)
zero: db = 1'b0;
one: db = 1'b1;
wait1_1, wait1_2,wait1_3: db = 1'b0;
wait0_1, wait0_2,wait0_3: db = 1'b1;
default: db = 1'b0;
endcase
end
endmodule
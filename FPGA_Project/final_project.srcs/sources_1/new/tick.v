
module tick #(parameter N=21) (
input wire clock,
input wire reset,
output wire m_tick );
// signal declaration
reg [N-1:0] q_reg;
wire [N-1:0] q_next;
// register
always @(posedge clock, posedge reset)
if (reset)
q_reg <= 0;
else
q_reg <= q_next;
// next-state logic
assign q_next = q_reg + 1;
// output logic
assign m_tick = (q_reg==0) ? 1'b1 : 1'b0;
endmodule
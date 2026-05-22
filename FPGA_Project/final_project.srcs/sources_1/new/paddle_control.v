module paddle_control (
    input clk,
    input reset,
    input btnL,
    input btnR,
    output reg [9:0] PaddleX
);

parameter SCREEN_WIDTH = 640;
parameter PADDLE_WIDTH = 64;
parameter STEP = 6;
parameter M = 400_000;
parameter N = 19;

wire tick;
wire left_pressed  = ~btnL;  // Active-low
wire right_pressed = ~btnR;

// mod-M counter 控制移動速度
mod_m_counter #(.M(M), .N(N)) move_tick_gen (
    .clk(clk),
    .reset(reset),
    .max_tick(tick),
    .q()
);

always @(posedge clk or posedge reset) begin
    if (reset)
        PaddleX <= (SCREEN_WIDTH - PADDLE_WIDTH) / 2;
    else if (tick) begin
        if (left_pressed && PaddleX > STEP)
            PaddleX <= PaddleX - STEP;
        else if (right_pressed && PaddleX < SCREEN_WIDTH - PADDLE_WIDTH - STEP)
            PaddleX <= PaddleX + STEP;
    end
end

endmodule

module breakout_playfield(
	input clk,
	input [9:0] CounterX, ballX, PaddleX,
	input [8:0] CounterY, ballY,
    input ball_miss,
	output reg DrawBall, DrawBorder, DrawPaddle, 
	output DrawBrick,

	input BrickHit_now, RestoreBrickwall,
	output BrickHit_acq,
	output reg [7:0] score_led
);

parameter hDrawArea = 640;
parameter vDrawArea = 480;

always @(posedge clk) begin
	DrawBall <= (CounterX >= ballX) && (CounterX < ballX + 10'd16) && (CounterY >= ballY) && (CounterY < ballY + 9'd16);
	DrawBorder <= (CounterX[9:2] == 0) || (CounterX[9:2] == hDrawArea/4 - 1) || (CounterY[8:2] == 0) || (CounterY[8:2] == vDrawArea/4 - 1);
	DrawPaddle <= (CounterX >= PaddleX) && (CounterX <= PaddleX + 10'd64) && (CounterY >= vDrawArea - 9'd46) && (CounterY < vDrawArea - 9'd30);
end

// the brickwall starts at coordinates (16,48)
wire [9:0] BrickXo = CounterX - 10'd16;
wire [4:0] BrickX_H = BrickXo[9:5];
wire [4:0] BrickX_L = BrickXo[4:0];
wire [8:0] BrickYo = CounterY - 9'd48;
wire [4:0] BrickY_H = BrickYo[8:4];
wire [3:0] BrickY_L = BrickYo[3:0];
wire [9:0] BrickA = {BrickY_H, BrickX_H};  // 32 x 32 matrix
reg [1023:0] RAMbrickwall;
reg BrickPresent, BrickHit_nowR;
reg [7:0] score;

always @(posedge clk) begin
	if (BrickHit_now | RestoreBrickwall)
		RAMbrickwall[BrickA] <= RestoreBrickwall ? (BrickX_H < 19 && BrickY_H < 7) : 1'b0;

	BrickPresent <= RAMbrickwall[BrickA];
	BrickHit_nowR <= BrickHit_now;

	if (RestoreBrickwall)
		score <= 0; 
	else if (BrickHit_nowR & BrickPresent)
		score <= score + 1;
    else if (ball_miss && score > 0)
		score <= score - 1;
end


assign BrickHit_acq = BrickPresent & BrickHit_nowR;

reg BrickBody;

always @(posedge clk) begin
	BrickBody <= |BrickY_L[3:1] && |BrickX_L[4:1];  // leave two pixels between each brick
end

assign DrawBrick = BrickPresent & BrickBody;
always @(posedge clk) begin
	score_led <= score;
end

endmodule

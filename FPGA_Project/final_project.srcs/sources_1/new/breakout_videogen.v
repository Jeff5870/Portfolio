module breakout_videogen(
	input clk,
	input [9:0] PaddleX,
	output reg DrawArea, hSync, vSync, 
	output red, green, blue,
	output reg Collision, BrickHit,
	output [7:0] score_led 
);

localparam ballspeed = 2; // ball moves 8 pixels per frame
reg [9:0] ballX = 100;  // initial ball position
reg [8:0] ballY = 300;
reg ball_dirX, ball_dirY;

parameter hDrawArea = 640;
parameter hSyncPorch = 16;
parameter hSyncLen = 96;
parameter hFrameSize = 800;

parameter vDrawArea = 480;
parameter vSyncPorch = 10;
parameter vSyncLen = 2;
parameter vFrameSize = 525;

reg [9:0] CounterX;
reg [8:0] CounterY;

always @(posedge clk) begin
	CounterX <= (CounterX == hFrameSize-1) ? 10'd0 : CounterX + 10'd1;
	if(CounterX == hFrameSize-1)begin
		CounterY <= (CounterY == vFrameSize-1) ? 9'd0 : CounterY + 9'd1;
    end
	DrawArea <= (CounterX < hDrawArea) & (CounterY < vDrawArea);
	hSync <= (CounterX >= hDrawArea + hSyncPorch) & (CounterX < hDrawArea + hSyncPorch + hSyncLen);
	vSync <= (CounterY >= vDrawArea + vSyncPorch) & (CounterY < vDrawArea + vSyncPorch + vSyncLen);
end

wire DrawBall, DrawBorder, DrawPaddle, DrawBrick, BrickHit_now, BrickHit_acq;
wire [7:0] score_led_wire;
reg RestoreBrickwall = 1'b1;
reg MoveBall;
reg ball_miss;

breakout_playfield #(hDrawArea, vDrawArea) game(
	.clk(clk),
	.PaddleX(PaddleX),
	.CounterX(MoveBall ? ballX + {6'h00, {4{CounterX[0]}}} : CounterX),
	.CounterY(MoveBall ? ballY + {5'h00, {4{CounterX[1]}}} : CounterY),
	.ballX(ballX),
	.ballY(ballY),
	.ball_miss(ball_miss),
	.DrawBall(DrawBall), .DrawBorder(DrawBorder), .DrawPaddle(DrawPaddle), .DrawBrick(DrawBrick),
	.BrickHit_now(BrickHit_now), .BrickHit_acq(BrickHit_acq), .RestoreBrickwall(RestoreBrickwall),
	.score_led(score_led_wire)
);

wire FrameTick = (CounterX == hFrameSize-1) & (CounterY == vDrawArea-1);

always @(posedge clk) begin
	MoveBall <= MoveBall ? ~&CounterX[ballspeed+2:0] : FrameTick;
end

always @(posedge clk) begin
    if (FrameTick)
        ball_miss <= 0; // 每幀開始時清除 ball_miss
    else if (MoveBall && CounterX[2:0] == 3'h7 && ballY == 9'd459)
        ball_miss <= 1; // 球碰到底部時觸發
end

wire BounceableOject = DrawBorder | DrawPaddle | DrawBrick;
reg [3:0] HBC;

always @(posedge clk) begin
	HBC <= {BounceableOject, HBC[3:1]};
end

wire [15:0] updateDirX = 16'b01101101_10110110;
wire [15:0] updateDirY = 16'b01111001_10011110;

always @(posedge clk) begin
	if(MoveBall & CounterX[2:0] == 3'h5 & updateDirX[HBC])begin
		ball_dirX <= (~HBC[0] & HBC[1]) | (~HBC[2] & HBC[3]);
    end
	if(MoveBall & CounterX[2:0] == 3'h5 & updateDirY[HBC])begin
		ball_dirY <= (~HBC[0] & ~HBC[1]) | (HBC[2] & HBC[3]);
    end
	if(MoveBall & CounterX[2:0] == 3'h7)begin
		ballX <= ballX + {{9{ball_dirX}}, 1'b1};
    end
	if(MoveBall & CounterX[2:0] == 3'h7)begin
		ballY <= ballY + {{8{ball_dirY}}, 1'b1};
    end
end
reg [2:0] BHA;

always @(posedge clk) begin
	BHA <= {DrawBrick, BHA[2:1]};
end

assign BrickHit_now = MoveBall & CounterX[2] & BHA[0];

always @(posedge clk) begin
	if(FrameTick)
		BrickHit <= 1'b0;
	else if(BrickHit_now)
		BrickHit <= 1'b1;
end

reg [7:0] BrickHit_count = 0;

always @(posedge clk) begin
	BrickHit_count <= RestoreBrickwall ? 8'h00 : BrickHit_count + BrickHit_acq;
end

always @(posedge clk) begin
	if (RestoreBrickwall)
		RestoreBrickwall <= ~FrameTick;
	else
		RestoreBrickwall <= (BrickHit_count == 19*7) & ballY[8];
end

always @(posedge clk) begin
	if(FrameTick)
		Collision <= 1'b0;
	else if(MoveBall & CounterX[2] & HBC[1])
		Collision <= 1'b1;
end

wire DrawAll = DrawBall | DrawBorder | DrawPaddle | DrawBrick;
assign red = DrawBrick;
assign green = DrawAll;
assign blue = DrawAll;
assign score_led = score_led_wire;

endmodule

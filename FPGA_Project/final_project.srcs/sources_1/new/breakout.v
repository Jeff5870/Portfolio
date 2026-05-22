
module breakout(
    input CLK_100M,
    input RESET,
    input btnL, btnR,  // 按鈕輸入
    output vga_hs_pin, vga_vs_pin,
    output [3:0] vga_R_Data_pin,
    output [3:0] vga_G_Data_pin,
    output [3:0] vga_B_Data_pin,
    output [7:0] LED
);

    // 時脈與 reset 結構
    wire VGA_clk;
    wire locked;
    wire sys_resetn = RESET & locked;

    // Clock generator for 25.2MHz VGA
    VGAClk252 vga_clk_252 (
        .clk_in1(CLK_100M),
        .resetn(RESET),           // Clocking Wizard resetn 為 active-low
        .clk_out1(VGA_clk),
        .locked(locked)
    );

    // Debounced buttons（active-low 輸入 -> sw 反相）
    wire btnL_db, btnR_db;
    deb debL (.clk(VGA_clk), .reset(~sys_resetn), .sw(~btnL), .db(btnL_db));
    deb debR (.clk(VGA_clk), .reset(~sys_resetn), .sw(~btnR), .db(btnR_db));

    // Paddle control
    wire [9:0] PaddleX;
    paddle_control myPaddle (
        .clk(VGA_clk),
        .reset(~sys_resetn),
        .btnL(btnL_db),
        .btnR(btnR_db),
        .PaddleX(PaddleX)
    );

    // Video generation
    wire DrawArea, hSync, vSync, red, green, blue, Collision, BrickHit;
    breakout_videogen myVideoGen(
        .clk(VGA_clk),
        .PaddleX(PaddleX),
        .DrawArea(DrawArea),
        .hSync(hSync),
        .vSync(vSync),
        .red(red), .green(green), .blue(blue),
        .Collision(Collision), .BrickHit(BrickHit),
        .score_led(LED)
    );

    // VGA 輸出色彩轉換為 4-bit
    assign vga_R_Data_pin = {4{DrawArea & red}};
    assign vga_G_Data_pin = {4{DrawArea & green}};
    assign vga_B_Data_pin = {4{DrawArea & blue}};
    assign vga_hs_pin = ~hSync;
    assign vga_vs_pin = ~vSync;

endmodule

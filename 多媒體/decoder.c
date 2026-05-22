#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define BLOCK_SIZE 8

float PI = 3.14159265359;




/* 定義 Bitmap、ImgRGB 結構和其他相關函數，以保證 decoder 的正確性 */
typedef struct Bmpheader {
    char identifier[2];
    unsigned int filesize;
    unsigned short reserved;
    unsigned short reserved2;
    unsigned int bitmap_dataoffset;
    unsigned int bitmap_headersize;
    unsigned int width;
    unsigned int height;
    unsigned short planes;
    unsigned short bits_perpixel;
    unsigned int compression;
    unsigned int bitmap_datasize;
    unsigned int hresolution;
    unsigned int vresolution;
    unsigned int usedcolors;
    unsigned int importantcolors;
    unsigned int palette;
} Bitmap;

typedef struct RGB {
    unsigned char R;
    unsigned char G;
    unsigned char B;
} ImgRGB;

typedef struct YCbCr {
    float Y;
    float Cb;
    float Cr;
} ImgYCbCr;

void readheader(FILE* fp, Bitmap* x);
void InputData(FILE* fp, ImgRGB** array, int H, int W, int skip);
ImgRGB** malloc_2D(int row, int col);
ImgYCbCr** malloc_2D_YCbCr(int row, int col);
void YCbCr_to_RGB(ImgYCbCr** YCbCr, ImgRGB** RGB, int H, int W);
void output_bmp(ImgRGB** RGB, FILE* outfile, Bitmap bmpheader, int skip);
void free_2D(void** array, int rows);
void assign_to_YCbCr(ImgYCbCr** YCbCr, double blockY[BLOCK_SIZE][BLOCK_SIZE], double blockCb[BLOCK_SIZE][BLOCK_SIZE], double blockCr[BLOCK_SIZE][BLOCK_SIZE], int startX, int startY, int H , int W);
void inverse_dct_transform(double block[BLOCK_SIZE][BLOCK_SIZE]);
void inverse_quantize_block(double block[BLOCK_SIZE][BLOCK_SIZE], int quant_table[BLOCK_SIZE][BLOCK_SIZE]);
void process_block(double block[BLOCK_SIZE][BLOCK_SIZE], int quant_table[BLOCK_SIZE][BLOCK_SIZE], FILE *fp_eF);
void process_quant_block(FILE *fp_qF, double block[BLOCK_SIZE][BLOCK_SIZE]);
void shift(double blockY[BLOCK_SIZE][BLOCK_SIZE], double blockCb[BLOCK_SIZE][BLOCK_SIZE], double blockCr[BLOCK_SIZE][BLOCK_SIZE]);
void rle_decode_block_to_1D(FILE* fp, short output[BLOCK_SIZE * BLOCK_SIZE]);
void zigzag_scan_inverse(short input[BLOCK_SIZE * BLOCK_SIZE], double output[BLOCK_SIZE][BLOCK_SIZE]);
void dpcm_decode(double block[BLOCK_SIZE][BLOCK_SIZE]);
void rle_decode_block_to_1D_bin(FILE* fp, short output[BLOCK_SIZE * BLOCK_SIZE]);
void RGB_SQNR(ImgRGB** c0, ImgRGB** c1, int height, int width);

void usage(FILE *fp)
{
    fprintf(fp, "Wrong input!\n");
    return;
}

void decoder0(char  *fn_out ,char *fn_R, char *fn_G, char *fn_B, char *fn_dim)
{
    FILE* fp_R;
    FILE* fp_G;
    FILE* fp_B;
    FILE* fp_dim;
    FILE* fp_out;

    // 開啟 R.txt、G.txt、B.txt、dim.txt 以讀取數據
    fp_R = fopen(fn_R, "r");
    fp_G = fopen(fn_G, "r");
    fp_B = fopen(fn_B, "r");
    fp_dim = fopen(fn_dim, "r");

    // 檢查文件是否成功打開
    if (!fp_R || !fp_G || !fp_B || !fp_dim) {
        printf("Error opening input files.\n");
        exit(1);
    }

    // 讀取圖像大小信息
    int width, height;
    fscanf(fp_dim, "%d %d", &width, &height);


    int skip = (4 - (width * 3) % 4);
        if (skip == 4) skip = 0;

    // 配置記憶體以保存解碼的 RGB 數據
    ImgRGB** Decoded_RGB = malloc_2D(height, width);

    // 讀取 R、G、B 通道的值
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            fscanf(fp_R, "%hhu", &Decoded_RGB[i][j].R);
            fscanf(fp_G, "%hhu", &Decoded_RGB[i][j].G);
            fscanf(fp_B, "%hhu", &Decoded_RGB[i][j].B);
        }
    }

    // 關閉 R.txt、G.txt、B.txt、dim.txt 文件
    fclose(fp_R);
    fclose(fp_G);
    fclose(fp_B);
    fclose(fp_dim);

     // 打開輸出 BMP 文件以寫入解碼的像素值
    fp_out = fopen(fn_out, "wb");
    if (!fp_out) {
        printf("Error opening output file.\n");
        free_2D((void **)Decoded_RGB, height);
        exit(1);
    }

    // 寫入 BMP 標頭信息
    Bitmap bmpheader;
    strcpy(bmpheader.identifier, "BM");
    bmpheader.filesize = 54 + width * height * 3; // Assuming 24 bits per pixel
    bmpheader.reserved = 0;
    bmpheader.reserved2 = 0;
    bmpheader.bitmap_dataoffset = 54;
    bmpheader.bitmap_headersize = 40; // Assuming Windows V3 header size
    bmpheader.width = width;
    bmpheader.height = height;
    bmpheader.planes = 1;
    bmpheader.bits_perpixel = 24; // Assuming 24 bits per pixel
    bmpheader.compression = 0;    // No compression
    bmpheader.bitmap_datasize = width * height * 3; // Assuming 24 bits per pixel
    bmpheader.hresolution = 2835;
    bmpheader.vresolution = 2835;
    bmpheader.usedcolors = 0;
    bmpheader.importantcolors = 0;
    bmpheader.palette = 0;

    // Using the new output_bmp function
    output_bmp(Decoded_RGB, fp_out, bmpheader, 0);


    // 關閉輸出 BMP 文件
    fclose(fp_out);


    // 釋放記憶體
    free_2D((void **)Decoded_RGB, height);
}

void decoder1_a(char *fn_out,char *fn_in, char *fn_Qt_Y, char *fn_Qt_Cb, char *fn_Qt_Cr, char *fn_dim, char *fn_qF_Y, char *fn_qF_Cb, char *fn_qF_Cr) {
    FILE *fp_Qt_Y, *fp_Qt_Cb, *fp_Qt_Cr;
    FILE *fp_qF_Y, *fp_qF_Cb, *fp_qF_Cr;
    FILE *fp_dim, *fp_out, *fp_in;
   
    // 打開输入文件
    fp_Qt_Y = fopen(fn_Qt_Y, "r");
    fp_Qt_Cb = fopen(fn_Qt_Cb, "r");
    fp_Qt_Cr = fopen(fn_Qt_Cr, "r");
    fp_qF_Y = fopen(fn_qF_Y, "rb");
    fp_qF_Cb = fopen(fn_qF_Cb, "rb");
    fp_qF_Cr = fopen(fn_qF_Cr, "rb");
    fp_dim = fopen(fn_dim, "r");
    fp_in = fopen(fn_in, "rb");

    Bitmap bmpheader0;
    readheader(fp_in, &bmpheader0);


    

    // Allocate memory and read RGB 
    ImgRGB** Data_RGB_0 = malloc_2D(bmpheader0.height, bmpheader0.width);
    int skip = (4 - (bmpheader0.width * 3) % 4);
    if (skip == 4) skip = 0;
    
    
    InputData(fp_in, Data_RGB_0, bmpheader0.height, bmpheader0.width, skip);
    fclose(fp_in);

    int Y_quantization_table[8][8];
    int Cb_quantization_table[8][8];
    int Cr_quantization_table[8][8];

    if (!fp_qF_Y || !fp_qF_Cb || !fp_qF_Cr || !fp_dim) {
        printf("Error opening input files.\n");
        exit(1);
    }

    // 讀長、寬、量化表
    int width, height;
    fscanf(fp_dim, "%d %d", &width, &height);
    fclose(fp_dim);

    double blockY[BLOCK_SIZE][BLOCK_SIZE];
    double blockCb[BLOCK_SIZE][BLOCK_SIZE];
    double blockCr[BLOCK_SIZE][BLOCK_SIZE];

    for (int x = 0; x < BLOCK_SIZE; x++) {
        for (int y = 0; y < BLOCK_SIZE; y++) {
            int temp = 0;
            fscanf(fp_Qt_Y, "%d", &temp);
            Y_quantization_table[x][y] = temp;
            fscanf(fp_Qt_Cb, "%d", &temp);
            Cb_quantization_table[x][y] = temp;
            fscanf(fp_Qt_Cr, "%d", &temp);
            Cr_quantization_table[x][y] = temp;

        }
    }

    // Allocate memory 
    ImgYCbCr **Data_YCbCr = malloc_2D_YCbCr(height, width);
    ImgRGB **Data_RGB = malloc_2D(height, width);

    for (int x = 0; x < height; x += BLOCK_SIZE) {
        for (int y = 0; y < width; y += BLOCK_SIZE) {
            // 讀數據
            process_quant_block(fp_qF_Y, blockY);
            inverse_quantize_block(blockY, Y_quantization_table);
            process_quant_block(fp_qF_Cb, blockCb);  
            inverse_quantize_block(blockCb, Cb_quantization_table);  
            process_quant_block(fp_qF_Cr, blockCr);  
            inverse_quantize_block(blockCr, Cr_quantization_table);  

            // inverse dct
            
            inverse_dct_transform(blockY);
            inverse_dct_transform(blockCb);
            inverse_dct_transform(blockCr);

            // shift
            shift(blockY, blockCb, blockCr);

            // 賦值回 Data_YCbCr
            assign_to_YCbCr(Data_YCbCr, blockY, blockCb, blockCr, x, y, height, width);
             
        }
    }
    
    // close
    fclose(fp_Qt_Y);
    fclose(fp_Qt_Cb);
    fclose(fp_Qt_Cr);
    fclose(fp_qF_Y);
    fclose(fp_qF_Cb);
    fclose(fp_qF_Cr);
 
    

    //  YCbCr 2 RGB
    YCbCr_to_RGB(Data_YCbCr, Data_RGB, height, width);

    // free YCbCr 
    free_2D((void **)Data_YCbCr, height);

    fp_out = fopen(fn_out, "wb");
    if (!fp_out) {
        printf("Error opening output file.\n");
        exit(1);
    }

    // 寫入 BMP header
    Bitmap bmpheader;
    strcpy(bmpheader.identifier, "BM");
    bmpheader.filesize = 54 + width * height * 3; // Assuming 24 bits per pixel
    bmpheader.reserved = 0;
    bmpheader.reserved2 = 0;
    bmpheader.bitmap_dataoffset = 54;
    bmpheader.bitmap_headersize = 40; // Assuming Windows V3 header size
    bmpheader.width = width;
    bmpheader.height = height;
    bmpheader.planes = 1;
    bmpheader.bits_perpixel = 24; // Assuming 24 bits per pixel
    bmpheader.compression = 0;    // No compression
    bmpheader.bitmap_datasize = width * height * 3; // Assuming 24 bits per pixel
    bmpheader.hresolution = 2835;
    bmpheader.vresolution = 2835;
    bmpheader.usedcolors = 0;
    bmpheader.importantcolors = 0;
    bmpheader.palette = 1;

    // Using the new output_bmp function
    output_bmp(Data_RGB, fp_out, bmpheader, 0);

    RGB_SQNR(Data_RGB_0, Data_RGB, height, width);

    // 關閉輸出 BMP 文件
    fclose(fp_out);

    //free RGB 
    free_2D((void **)Data_RGB, height);
    free_2D((void **)Data_RGB_0, height);
}

void decoder1_b(char *fn_out, char *fn_Qt_Y, char *fn_Qt_Cb, char *fn_Qt_Cr, char *fn_dim, char *fn_qF_Y, char *fn_qF_Cb, char *fn_qF_Cr, char *fn_eF_Y, char *fn_eF_Cb, char *fn_eF_Cr) {
    FILE *fp_Qt_Y, *fp_Qt_Cb, *fp_Qt_Cr;
    FILE *fp_qF_Y, *fp_qF_Cb, *fp_qF_Cr;
    FILE *fp_eF_Y, *fp_eF_Cb, *fp_eF_Cr;
    FILE *fp_dim, *fp_out;
   
    // 打開输入文件
    fp_Qt_Y = fopen(fn_Qt_Y, "r");
    fp_Qt_Cb = fopen(fn_Qt_Cb, "r");
    fp_Qt_Cr = fopen(fn_Qt_Cr, "r");
    fp_qF_Y = fopen(fn_qF_Y, "rb");
    fp_qF_Cb = fopen(fn_qF_Cb, "rb");
    fp_qF_Cr = fopen(fn_qF_Cr, "rb");
    fp_eF_Y = fopen(fn_eF_Y, "rb");
    fp_eF_Cb = fopen(fn_eF_Cb, "rb");
    fp_eF_Cr = fopen(fn_eF_Cr, "rb");
    fp_dim = fopen(fn_dim, "r");

    int Y_quantization_table[8][8];
    int Cb_quantization_table[8][8];
    int Cr_quantization_table[8][8];

    if (!fp_qF_Y || !fp_qF_Cb || !fp_qF_Cr || !fp_eF_Y || !fp_eF_Cb || !fp_eF_Cr || !fp_dim) {
        printf("Error opening input files.\n");
        exit(1);
    }

    // 讀長、寬、量化表
    int width, height;
    fscanf(fp_dim, "%d %d", &width, &height);
    fclose(fp_dim);

    double blockY[BLOCK_SIZE][BLOCK_SIZE];
    double blockCb[BLOCK_SIZE][BLOCK_SIZE];
    double blockCr[BLOCK_SIZE][BLOCK_SIZE];

    for (int x = 0; x < BLOCK_SIZE; x++) {
        for (int y = 0; y < BLOCK_SIZE; y++) {
            int temp = 0;
            fscanf(fp_Qt_Y, "%d", &temp);
            Y_quantization_table[x][y] = temp;
            fscanf(fp_Qt_Cb, "%d", &temp);
            Cb_quantization_table[x][y] = temp;
            fscanf(fp_Qt_Cr, "%d", &temp);
            Cr_quantization_table[x][y] = temp;

        }
    }

    // Allocate memory 
    ImgYCbCr **Data_YCbCr = malloc_2D_YCbCr(height, width);
    ImgRGB **Data_RGB = malloc_2D(height, width);

    for (int x = 0; x < height; x += BLOCK_SIZE) {
        for (int y = 0; y < width; y += BLOCK_SIZE) {
            // 讀數據
            process_quant_block(fp_qF_Y, blockY);
            process_block(blockY, Y_quantization_table, fp_eF_Y);
            process_quant_block(fp_qF_Cb, blockCb);  
            process_block(blockCb, Cb_quantization_table, fp_eF_Cb);  
            process_quant_block(fp_qF_Cr, blockCr);  
            process_block(blockCr, Cr_quantization_table, fp_eF_Cr);  

            // inverse dct
            
            inverse_dct_transform(blockY);
            inverse_dct_transform(blockCb);
            inverse_dct_transform(blockCr);

            // shift
            shift(blockY, blockCb, blockCr);

            // 賦值回 Data_YCbCr
            assign_to_YCbCr(Data_YCbCr, blockY, blockCb, blockCr, x, y, height, width);
             
        }
    }
    
    // close
    fclose(fp_Qt_Y);
    fclose(fp_Qt_Cb);
    fclose(fp_Qt_Cr);
    fclose(fp_qF_Y);
    fclose(fp_qF_Cb);
    fclose(fp_qF_Cr);
    fclose(fp_eF_Y);
    fclose(fp_eF_Cb);
    fclose(fp_eF_Cr);
    

    //  YCbCr 2 RGB
    YCbCr_to_RGB(Data_YCbCr, Data_RGB, height, width);

    // free YCbCr 
    free_2D((void **)Data_YCbCr, height);

    fp_out = fopen(fn_out, "wb");
    if (!fp_out) {
        printf("Error opening output file.\n");
        exit(1);
    }

    // 寫入 BMP header
    Bitmap bmpheader;
    strcpy(bmpheader.identifier, "BM");
    bmpheader.filesize = 54 + width * height * 3; // Assuming 24 bits per pixel
    bmpheader.reserved = 0;
    bmpheader.reserved2 = 0;
    bmpheader.bitmap_dataoffset = 54;
    bmpheader.bitmap_headersize = 40; // Assuming Windows V3 header size
    bmpheader.width = width;
    bmpheader.height = height;
    bmpheader.planes = 1;
    bmpheader.bits_perpixel = 24; // Assuming 24 bits per pixel
    bmpheader.compression = 0;    // No compression
    bmpheader.bitmap_datasize = width * height * 3; // Assuming 24 bits per pixel
    bmpheader.hresolution = 2835;
    bmpheader.vresolution = 2835;
    bmpheader.usedcolors = 0;
    bmpheader.importantcolors = 0;
    bmpheader.palette = 1;

    // Using the new output_bmp function
    output_bmp(Data_RGB, fp_out, bmpheader, 0);



    // 關閉輸出 BMP 文件
    fclose(fp_out);

    //free RGB 
    free_2D((void **)Data_RGB, height);
}


void decoder2(char *fn_out, char *t, char *fn_rle) {
    int Y_quantization_table[8][8] = {
    {16, 11, 10, 16, 24, 40, 51, 61},
    {12, 12, 14, 19, 26, 58, 60, 55},
    {14, 13, 16, 24, 40, 57, 69, 56},
    {14, 17, 22, 29, 51, 87, 80, 62},
    {18, 22, 37, 56, 68, 109, 103, 77},
    {24, 35, 55, 64, 81, 104, 113, 92},
    {49, 64, 78, 87, 103, 121, 120, 101},
    {72, 92, 95, 98, 112, 100, 103, 99}
    };

    int Cb_quantization_table[8][8] = {
    {17, 18, 24, 47, 99, 99, 99, 99},
    {18, 21, 26, 66, 99, 99, 99, 99},
    {24, 26, 56, 99, 99, 99, 99, 99},
    {47, 66, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99}
    };

    int Cr_quantization_table[8][8] = {
    {17, 18, 24, 47, 99, 99, 99, 99},
    {18, 21, 26, 66, 99, 99, 99, 99},
    {24, 26, 56, 99, 99, 99, 99, 99},
    {47, 66, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99}
    };
    

    int mode = 0;
    mode = strcmp(t, "ascii");


    // Read the image size from the RLE file
    int width, height;
    FILE *fp_rle;  

    // 確認是ascii還是bin
    if (mode == 0) {
        fp_rle = fopen(fn_rle, "r");
        if (!fp_rle) {
            printf("Error opening RLE file.\n");
            exit(1);
        }
        fscanf(fp_rle, "%d %d", &height, &width);
    } else {
        fp_rle = fopen(fn_rle, "rb");
        if (!fp_rle) {
            printf("Error opening RLE file.\n");
            exit(1);
        }
        fread(&height, sizeof(int), 1, fp_rle);
        fread(&width, sizeof(int), 1, fp_rle);
    }

    // Allocate memory for YCbCr and RGB data
    ImgYCbCr** Data_YCbCr = malloc_2D_YCbCr(height, width);
    ImgRGB** Data_RGB = malloc_2D(height, width);

    // Data structures for holding decoded data
    double blockY[BLOCK_SIZE][BLOCK_SIZE], blockCb[BLOCK_SIZE][BLOCK_SIZE], blockCr[BLOCK_SIZE][BLOCK_SIZE];
    short rle_decoded_1D[BLOCK_SIZE * BLOCK_SIZE];
    double block[BLOCK_SIZE][BLOCK_SIZE];

  
    // Read and decode RLE data from the file
    for (int x = 0; x < height; x += BLOCK_SIZE) {
        for (int y = 0; y < width; y += BLOCK_SIZE) {
            // 讀數據
            // 讀取 RLE 數據並解碼到一維數組
            //做逆zigzag、逆dpcm
            if(mode == 0){
                rle_decode_block_to_1D(fp_rle, rle_decoded_1D);
            }
            else{
                rle_decode_block_to_1D_bin(fp_rle, rle_decoded_1D);
            }

            // for(int u = 0; u < 64; u++) {
            //     printf("%hd ",rle_decoded_1D[u]);
            // }
            // printf("\n ");

            zigzag_scan_inverse(rle_decoded_1D, blockY);
            dpcm_decode(blockY);
            

 

            if(mode == 0){
                rle_decode_block_to_1D(fp_rle, rle_decoded_1D);
            }
            else{
                rle_decode_block_to_1D_bin(fp_rle, rle_decoded_1D);
            }
            zigzag_scan_inverse(rle_decoded_1D, blockCb);
            dpcm_decode(blockCb);



            if(mode == 0){
                rle_decode_block_to_1D(fp_rle, rle_decoded_1D);
            }
            else{
                rle_decode_block_to_1D_bin(fp_rle, rle_decoded_1D);
            }
            zigzag_scan_inverse(rle_decoded_1D, blockCr);
            dpcm_decode(blockCr);


            // inverse quantize

            inverse_quantize_block(blockY, Y_quantization_table);
            inverse_quantize_block(blockCb, Cb_quantization_table);
            inverse_quantize_block(blockCr, Cr_quantization_table);
            

            // inverse dct
            
            inverse_dct_transform(blockY);
            inverse_dct_transform(blockCb);
            inverse_dct_transform(blockCr);

            // shift
            shift(blockY, blockCb, blockCr);

            // 賦值回 Data_YCbCr
            assign_to_YCbCr(Data_YCbCr, blockY, blockCb, blockCr, x, y, height, width);
             
        }
    }
    

    // Convert YCbCr to RGB
    YCbCr_to_RGB(Data_YCbCr, Data_RGB, height, width);

    // Write RGB data to BMP file
    FILE *fp_out = fopen(fn_out, "wb");
    if (!fp_out) {
        printf("Error opening output BMP file.\n");
        fclose(fp_rle);
        free_2D((void**)Data_YCbCr, height);
        free_2D((void**)Data_RGB, height);
        exit(1);
    }

    // Write BMP header
    Bitmap bmpheader;
    strcpy(bmpheader.identifier, "BM");
    bmpheader.filesize = 54 + width * height * 3; // Assuming 24 bits per pixel
    bmpheader.reserved = 0;
    bmpheader.reserved2 = 0;
    bmpheader.bitmap_dataoffset = 54;
    bmpheader.bitmap_headersize = 40; // Assuming Windows V3 header size
    bmpheader.width = width;
    bmpheader.height = height;
    bmpheader.planes = 1;
    bmpheader.bits_perpixel = 24; // Assuming 24 bits per pixel
    bmpheader.compression = 0;    // No compression
    bmpheader.bitmap_datasize = width * height * 3; // Assuming 24 bits per pixel
    bmpheader.hresolution = 2835;
    bmpheader.vresolution = 2835;
    bmpheader.usedcolors = 0;
    bmpheader.importantcolors = 0;
    bmpheader.palette = 1;
    

    // Write the RGB data to the BMP file
    output_bmp(Data_RGB, fp_out, bmpheader, 0);
    
            
    // Clean up
    fclose(fp_rle);
    fclose(fp_out);
    free_2D((void**)Data_YCbCr, height);
    free_2D((void**)Data_RGB, height);
}


/* A function to write RGB data into BMP file */
void OutputData(FILE* fp, ImgRGB** array, int H, int W) {
    int i, j;
    for (i = 0; i < H; i++) {
        for (j = 0; j < W; j++) {
            fputc(array[i][j].B, fp);
            fputc(array[i][j].G, fp);
            fputc(array[i][j].R, fp);
        }
    }
}

int main(int argc, char **argv)
{
    int option = 0;// 0 for demo0, 1 for demo1, etc
    
    if ( argc <=2 ) {
        usage(stderr);
        exit(1);
    }

    option = atoi(argv[1]);


    switch(option)
    {
        case 0:
        decoder0(argv[2], argv[3], argv[4], argv[5], argv[6]);
        break;
        
        case 1:
        if(argc == 11)
            decoder1_a(argv[2], argv[3], argv[4], argv[5], argv[6],argv[7], argv[8], argv[9], argv[10]);
        else
            decoder1_b(argv[2], argv[3], argv[4], argv[5], argv[6],argv[7], argv[8], argv[9], argv[10], argv[11], argv[12]);
        break;
        
        case 2:
        decoder2(argv[2], argv[3], argv[4]);
        break;

        default:
        break;
        // default statements
    }
    return 0; // 0: exit normally 1: exit with error
}

/* A function for making two dimensions memory locate array */
ImgRGB** malloc_2D(int row, int col) {
    ImgRGB** Array, *Data;
    int i;
    Array = (ImgRGB**)malloc(row * sizeof(ImgRGB*));
    Data = (ImgRGB*)malloc(row * col * sizeof(ImgRGB));
    for (i = 0; i < row; i++, Data += col) {
        Array[i] = Data;
    }
    return Array;
}

/* A function for making two dimensions memory locate array for YCbCr */
ImgYCbCr** malloc_2D_YCbCr(int row, int col) {
    ImgYCbCr** Array, *Data;
    int i;
    Array = (ImgYCbCr**)malloc(row * sizeof(ImgYCbCr*));
    Data = (ImgYCbCr*)malloc(row * col * sizeof(ImgYCbCr));
    for (i = 0; i < row; i++, Data += col) {
        Array[i] = Data;
    }
    return Array;
}


void free_2D(void** array, int rows) {
    free(array[0]);  // Free the data
    free(array);     // Free the array of pointers
}

void output_bmp(ImgRGB **RGB, FILE* outfile, Bitmap bmpheader, int skip) {
    char skip_buf[3] = {0, 0, 0};
    int x, y;
    fwrite(&bmpheader.identifier, sizeof(char), 2, outfile);  // Use sizeof(char) instead of sizeof(short)
    fwrite(&bmpheader.filesize, sizeof(int), 1, outfile);
    fwrite(&bmpheader.reserved, sizeof(unsigned short), 1, outfile);  // Use sizeof(unsigned short)
    fwrite(&bmpheader.reserved2, sizeof(unsigned short), 1, outfile);  // Use sizeof(unsigned short)
    fwrite(&bmpheader.bitmap_dataoffset, sizeof(int), 1, outfile);
    fwrite(&bmpheader.bitmap_headersize, sizeof(int), 1, outfile);
    fwrite(&bmpheader.width, sizeof(int), 1, outfile);
    fwrite(&bmpheader.height, sizeof(int), 1, outfile);
    fwrite(&bmpheader.planes, sizeof(unsigned short), 1, outfile);  // Use sizeof(unsigned short)
    fwrite(&bmpheader.bits_perpixel, sizeof(unsigned short), 1, outfile);  // Use sizeof(unsigned short)
    fwrite(&bmpheader.compression, sizeof(int), 1, outfile);
    fwrite(&bmpheader.bitmap_datasize, sizeof(int), 1, outfile);
    fwrite(&bmpheader.hresolution, sizeof(int), 1, outfile);
    fwrite(&bmpheader.vresolution, sizeof(int), 1, outfile);
    fwrite(&bmpheader.usedcolors, sizeof(int), 1, outfile);
    fwrite(&bmpheader.importantcolors, sizeof(int), 1, outfile);

    for (x = 0; x < bmpheader.height; x++) {
        for (y = 0; y < bmpheader.width; y++) {
            fwrite(&RGB[x][y].B, sizeof(unsigned char), 1, outfile);  // Use sizeof(unsigned char)
            fwrite(&RGB[x][y].G, sizeof(unsigned char), 1, outfile);  // Use sizeof(unsigned char)
            fwrite(&RGB[x][y].R, sizeof(unsigned char), 1, outfile);  // Use sizeof(unsigned char)
        }
        if (skip != 0) {
            fwrite(skip_buf, skip, 1, outfile);
        }
    }
}

//逆量化
void inverse_quantize_block(double block[BLOCK_SIZE][BLOCK_SIZE], int quant_table[BLOCK_SIZE][BLOCK_SIZE]) {
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            block[i][j] *= quant_table[i][j];
        }
    }
}

// inverse dct
void inverse_dct_transform(double block[BLOCK_SIZE][BLOCK_SIZE]) {
    double temp[BLOCK_SIZE][BLOCK_SIZE];
    double cu, cv, sum;

    for (int x = 0; x < BLOCK_SIZE; x++) {
        for (int y = 0; y < BLOCK_SIZE; y++) {
            sum = 0;
            for (int u = 0; u < BLOCK_SIZE; u++) {
                for (int v = 0; v < BLOCK_SIZE; v++) {
                    if (u == 0) cu = 1 / sqrt(2); else cu = 1;
                    if (v == 0) cv = 1 / sqrt(2); else cv = 1;

                    sum += cu * cv * block[u][v] *
                           cos((2 * x + 1) * u * PI / (2 * BLOCK_SIZE)) *
                           cos((2 * y + 1) * v * PI / (2 * BLOCK_SIZE));
                }
            }
            temp[x][y] = 0.25 * sum;
        }
    }

    // 回傳結果
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            block[i][j] = temp[i][j];
        }
    }
}

// 賦值回去
void assign_to_YCbCr(ImgYCbCr** YCbCr, double blockY[BLOCK_SIZE][BLOCK_SIZE], double blockCb[BLOCK_SIZE][BLOCK_SIZE], double blockCr[BLOCK_SIZE][BLOCK_SIZE], int startX, int startY, int H , int W) {
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            int x = startX + i;
            int y = startY + j;
            if (x < H && y < W) {
                YCbCr[x][y].Y = blockY[i][j] ;
                YCbCr[x][y].Cb = blockCb[i][j] ;
                YCbCr[x][y].Cr = blockCr[i][j] ;
            }
        }
    }
}

// YCbCr 2 RGB
void YCbCr_to_RGB(ImgYCbCr** YCbCr, ImgRGB** RGB, int H, int W) {
    for (int i = 0; i < H; i++) {
        for (int j = 0; j < W; j++) {
            float Y = YCbCr[i][j].Y;
            float Cb = YCbCr[i][j].Cb - 128;
            float Cr = YCbCr[i][j].Cr - 128;

            //YCbCr 2 RGB
            float R = round(Y- 0.000001218894189 * (Cb) + 1.401999589 * (Cr ));
            float G = round(Y - 0.3441356782 * (Cb ) - 0.7141361556 * (Cr ));
            float B = round(Y + 1.772000066 * (Cb ) + 0.0000004062980629 * (Cr));

            // 確保0-255
            RGB[i][j].R = (unsigned char)(fmin(255, fmax(0, R)));
            RGB[i][j].G = (unsigned char)(fmin(255, fmax(0, G)));
            RGB[i][j].B = (unsigned char)(fmin(255, fmax(0, B)));
        }
    }
}

// 賦值量化完
void process_quant_block(FILE *fp_qF, double block[BLOCK_SIZE][BLOCK_SIZE]) {
    short value;
    

    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            if (fread(&value, sizeof(short), 1, fp_qF) == 1) {    
                double value2 = (double)value;
                block[i][j] = (double)value2;      
            }  
        }    
    }
}

void process_block(double block[BLOCK_SIZE][BLOCK_SIZE], int quant_table[BLOCK_SIZE][BLOCK_SIZE], FILE *fp_eF) {
    float error;
    // 逆量化
    inverse_quantize_block(block, quant_table);

    // 加error
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            if (fread(&error, sizeof(float), 1, fp_eF) == 1) {
                block[i][j] += (double)error;
            } 
        }
    }

}
// shift 128
void shift(double blockY[BLOCK_SIZE][BLOCK_SIZE], double blockCb[BLOCK_SIZE][BLOCK_SIZE], double blockCr[BLOCK_SIZE][BLOCK_SIZE]){
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            blockY[i][j] += 128;
            blockCb[i][j] += 128;
            blockCr[i][j] += 128;   
        }
        
    }
}

// rle轉陣列(ascii)
void rle_decode_block_to_1D(FILE* fp, short output[BLOCK_SIZE * BLOCK_SIZE]) {
    // Reset the output array to zero
    memset(output, 0, sizeof(short) * BLOCK_SIZE * BLOCK_SIZE);

    // Skip the block header (e.g., "(0,0, Y)")
    char headerBuffer[10];
    if (fscanf(fp, "%s", headerBuffer) != 1) {
        printf("Error reading RLE file header.\n");
        return;
    }

    // Read the RLE data for the block
    int count;
    short value;
    int pos = 0;
    while (fscanf(fp, "%d %hd", &count, &value) == 2 && pos < BLOCK_SIZE * BLOCK_SIZE) {
        for (int i = 0; i < count && pos < BLOCK_SIZE * BLOCK_SIZE; i++) {
            output[pos++] = value;
        }
    }
}

// rle轉陣列(bin)
void rle_decode_block_to_1D_bin(FILE* fp, short output[BLOCK_SIZE * BLOCK_SIZE]) {
    // Reset the output array to zero
    memset(output, 0, sizeof(short) * BLOCK_SIZE * BLOCK_SIZE);

    // Read the RLE data for the block
    int count;
    short value;
    int pos = 0;
    
    while (pos < BLOCK_SIZE * BLOCK_SIZE &&
           fread(&count, sizeof(int), 1, fp) == 1 &&
           fread(&value, sizeof(short), 1, fp) == 1) {
        for (int i = 0; i < count && pos < BLOCK_SIZE * BLOCK_SIZE; i++) {
            output[pos++] = value;
            // printf("%hd ", value);
        }
    }
    // printf("\n\n");

    
}






// 解zigzag
void zigzag_scan_inverse(short input[BLOCK_SIZE * BLOCK_SIZE], double output[BLOCK_SIZE][BLOCK_SIZE]) {
    int i = 0, j = 0, k = 0;
    int total = BLOCK_SIZE * BLOCK_SIZE;
    double temp;
    while (k < total) {
        // Move upwards diagonally
        while (i >= 0 && j < BLOCK_SIZE) {
            temp = (double)input[k++];
            output[i][j] = temp;

            i--;
            j++;
        }

        // Adjust position after moving upwards
        if (i < 0 && j <= BLOCK_SIZE - 1) {
            i++;
        } else if (j == BLOCK_SIZE) {
            i += 2;
            j--;
        }

        // Move downwards diagonally
        while (j >= 0 && i < BLOCK_SIZE) {
            temp = (double)input[k++];
            output[i][j] = temp;

            i++;
            j--;
        }

        // Adjust position after moving downwards
        if (j < 0 && i <= BLOCK_SIZE - 1) {
            j++;
        } else if (i == BLOCK_SIZE) {
            j += 2;
            i--;
        }
    }
}

// 解dpcm
void dpcm_decode(double block[BLOCK_SIZE][BLOCK_SIZE]) {
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            if (i != 0 || j != 0) { // 跳過第一個值，因為它是絕對值
                if (j == 0) {
                    // 如果是每行的開頭，則與上一行的同列值相加
                    block[i][j] += block[i - 1][7];
                } else {
                    // 否則，與同行的前一個值相加
                    block[i][j] += block[i][j - 1];
                }
            }
        }
    }
}

void readheader(FILE* fp, Bitmap* x) {
    fread(&x->identifier, sizeof(x->identifier), 1, fp);
    fread(&x->filesize, sizeof(x->filesize), 1, fp);
    fread(&x->reserved, sizeof(x->reserved), 1, fp);
    fread(&x->reserved2, sizeof(x->reserved2), 1, fp);
    fread(&x->bitmap_dataoffset, sizeof(x->bitmap_dataoffset), 1, fp);
    fread(&x->bitmap_headersize, sizeof(x->bitmap_headersize), 1, fp);
    fread(&x->width, sizeof(x->width), 1, fp);
    fread(&x->height, sizeof(x->height), 1, fp);
    fread(&x->planes, sizeof(x->planes), 1, fp);
    fread(&x->bits_perpixel, sizeof(x->bits_perpixel), 1, fp);
    fread(&x->compression, sizeof(x->compression), 1, fp);
    fread(&x->bitmap_datasize, sizeof(x->bitmap_datasize), 1, fp);
    fread(&x->hresolution, sizeof(x->hresolution), 1, fp);
    fread(&x->vresolution, sizeof(x->vresolution), 1, fp);
    fread(&x->usedcolors, sizeof(x->usedcolors), 1, fp);
    fread(&x->importantcolors, sizeof(x->importantcolors), 1, fp);
}

void InputData(FILE* fp, ImgRGB** array, int H, int W, int skip) {
    int temp;
    char skip_buf[3];
    int i, j;
    for (i = 0; i < H; i++) {
        for (j = 0; j < W; j++) {
            temp = fgetc(fp);
            array[i][j].B = temp;
            temp = fgetc(fp);
            array[i][j].G = temp;
            temp = fgetc(fp);
            array[i][j].R = temp;
        }
        if (skip != 0) fread(skip_buf, skip, 1, fp);
    } 
}

// 算RGB SQNR
void RGB_SQNR(ImgRGB** c0, ImgRGB** c1, int height, int width){
    double sum_r = 0.0, sum_g = 0.0, sum_b = 0.0;
    double diff_R = 0.0;
    double diff_G = 0.0;
    double diff_B = 0.0;



    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            // Calculate differences
            diff_R += (c0[i][j].R - c1[i][j].R)*(c0[i][j].R - c1[i][j].R);
            diff_G += (c0[i][j].G - c1[i][j].G)*(c0[i][j].G - c1[i][j].G);
            diff_B += (c0[i][j].B - c1[i][j].B)*(c0[i][j].B - c1[i][j].B);


            // Calculate squared differences
            sum_r += c1[i][j].R * c1[i][j].R ;
            sum_g += c1[i][j].G * c1[i][j].G ;
            sum_b += c1[i][j].B * c1[i][j].B ;
        }
    }


    
    double sqnr_r = 10 * log10(sum_r / diff_R);
    double sqnr_g = 10 * log10(sum_g / diff_G);
    double sqnr_b = 10 * log10(sum_b / diff_B);

    printf("R's SQNR:%lf\nG's SQNR:%lf\nB's SQNR:%lf\n",sqnr_r,sqnr_g,sqnr_b);
}
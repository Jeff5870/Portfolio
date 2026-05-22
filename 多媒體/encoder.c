#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define BLOCK_SIZE 8

float PI = 3.14159265359;

// Standard Y Cb Cr component quantization table 
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

/*construct a structure of BMP header*/
typedef struct Bmpheader {
    char identifier[2]; // 0x0000
    unsigned int filesize; // 0x0002
    unsigned short reserved; // 0x0006
    unsigned short reserved2;
    unsigned int bitmap_dataoffset; // 0x000A
    unsigned int bitmap_headersize; // 0x000E
    unsigned int width; // 0x0012
    unsigned int height; // 0x0016
    unsigned short planes; // 0x001A
    unsigned short bits_perpixel; // 0x001C
    unsigned int compression; // 0x001E
    unsigned int bitmap_datasize; // 0x0022
    unsigned int hresolution; // 0x0026
    unsigned int vresolution; // 0x002A
    unsigned int usedcolors; // 0x002E
    unsigned int importantcolors; // 0x0032
    unsigned int palette; // 0x0036
} Bitmap;

/*construct a structure of RGB*/
typedef struct RGB {
    unsigned char R;
    unsigned char G;
    unsigned char B;
} ImgRGB;

/*construct a structure of YCbCr*/
typedef struct YCbCr {
    float Y;
    float Cb;
    float Cr;
} ImgYCbCr;

typedef struct {
    short value;
    int count;
} RLEPair;

void readheader(FILE* fp, Bitmap* x);
ImgRGB** malloc_2D(int row, int col);
ImgYCbCr** malloc_2D_YCbCr(int row, int col);
void InputData(FILE* fp, ImgRGB** array, int H, int W, int skip);
void RGB_to_YCbCr(ImgRGB** RGB, ImgYCbCr** YCbCr, int H, int W);
void free_2D(void** array, int rows);
void quantize_block(double block[BLOCK_SIZE][BLOCK_SIZE], int quant_table[BLOCK_SIZE][BLOCK_SIZE]);
void dct_transform(double block[BLOCK_SIZE][BLOCK_SIZE]);
void process_YCbCr_blocks(ImgYCbCr** YCbCr, int H, int W, FILE *quantizedYFile, FILE *quantizedCbFile, FILE *quantizedCrFile, FILE *errorYFile, FILE *errorCbFile, FILE *errorCrFile);
void process_block(double block[BLOCK_SIZE][BLOCK_SIZE], int quant_table[BLOCK_SIZE][BLOCK_SIZE], FILE* quantizedFile, FILE* errorFile, double signal_power[BLOCK_SIZE][BLOCK_SIZE], double noise_power[BLOCK_SIZE][BLOCK_SIZE]);
void print_SQNR( double signal_power[BLOCK_SIZE][BLOCK_SIZE], double noise_power[BLOCK_SIZE][BLOCK_SIZE]);
void process_YCbCr_blocks_rle(ImgYCbCr** YCbCr, int H, int W, FILE *rle, int mode) ;
void process_block_ne(double block[BLOCK_SIZE][BLOCK_SIZE], int quant_table[BLOCK_SIZE][BLOCK_SIZE]);
void zigzag_scan_modified(short input[BLOCK_SIZE][BLOCK_SIZE], short output[BLOCK_SIZE * BLOCK_SIZE]);
void dpcm_encode(double block[BLOCK_SIZE][BLOCK_SIZE], short encoded_block[BLOCK_SIZE][BLOCK_SIZE]) ;
int run_length_encode(short* input, RLEPair* output, int length);

void usage0(FILE *fp)
{
    fprintf(fp, "Wrong input!\n");
    return;
}



void demo0(char *fn_bmp, char *fn_R, char *fn_G, char *fn_B, char *fn_dim)
{
    int i, j, x, y;
    FILE* fp_in;
    FILE* fp_out_R;
    FILE* fp_out_G;
    FILE* fp_out_B;
    FILE* fp_out_dim;

    // 使用 strcpy 將 fn_bmp 的內容複製到 fn_in 陣列中
    char fn_in[FILENAME_MAX];
    strcpy(fn_in, fn_bmp);

    fp_in = fopen(fn_in, "rb");
    if (fp_in) {
        printf("Open %s as input file!\n", fn_in);

        // read header
        Bitmap bmpheader;
        readheader(fp_in, &bmpheader);

        // W:3024 x H:4032 for input.bmp
        int H = bmpheader.height;
        int W = bmpheader.width;

        // skip paddings at the end of each image line
        int skip = (4 - (bmpheader.width * 3) % 4);
        if (skip == 4) skip = 0;
        

        // allocate memory for RGB and YCbCr data
        ImgRGB** Data_RGB = malloc_2D(bmpheader.height, bmpheader.width);

        // Load BMP data
        InputData(fp_in, Data_RGB, bmpheader.height, bmpheader.width, skip);
        fclose(fp_in);

        // Open files for writing
        fp_out_R = fopen(fn_R, "w");
        fp_out_G = fopen(fn_G, "w");
        fp_out_B = fopen(fn_B, "w");
        fp_out_dim = fopen(fn_dim, "w");

        // Check if files are opened successfully
        if (fp_out_R && fp_out_G && fp_out_B && fp_out_dim) {
            // Write original and converted RGB values to text files
            for (x = 0; x < bmpheader.height; x++) {
                for (y = 0; y < bmpheader.width; y++) {
                    // Write RGB values to respective files
                    fprintf(fp_out_R, "%d ", Data_RGB[x][y].R);
                    fprintf(fp_out_G, "%d ", Data_RGB[x][y].G);
                    fprintf(fp_out_B, "%d ", Data_RGB[x][y].B);
                }
                // Move to the next line for each file
                fprintf(fp_out_R, "\n");
                fprintf(fp_out_G, "\n");
                fprintf(fp_out_B, "\n");
            }

            // Write dimensions to dim.txt
            fprintf(fp_out_dim, "%d %d\n", bmpheader.width, bmpheader.height);

            // Close all files
            fclose(fp_out_R);
            fclose(fp_out_G);
            fclose(fp_out_B);
            fclose(fp_out_dim);

            // Free allocated memory
            free_2D((void**)Data_RGB, bmpheader.height);
        } else {
            printf("Error opening files for writing.\n");
            fclose(fp_out_R);
            fclose(fp_out_G);
            fclose(fp_out_B);
            fclose(fp_out_dim);
            free_2D((void**)Data_RGB, bmpheader.height);  // Free memory even if file opening fails
        }
    } else {
        printf("Fail to open %s as input file!\n", fn_in);
    }
}



void demo1(char *fn_bmp, char *fn_Qt_Y, char *fn_Qt_Cb, char *fn_Qt_Cr, char *fn_dim, char *fn_qF_Y, char *fn_qF_Cb, char *fn_qF_Cr, char *fn_eF_Y, char *fn_eF_Cb, char *fn_eF_Cr) {
    FILE *fp_in, *fp_out_dim;
    FILE *fp_Qt_Y, *fp_Qt_Cb, *fp_Qt_Cr;
    FILE *quantizedYFile, *quantizedCbFile, *quantizedCrFile;
    FILE *errorYFile, *errorCbFile, *errorCrFile;
    

    // 打開輸入文件
    fp_in = fopen(fn_bmp, "rb");
    if (!fp_in) {
        printf("Failed to open input file %s\n", fn_bmp);
        return;
    }

    // read BMP header
    Bitmap bmpheader;
    readheader(fp_in, &bmpheader);


    

    // Allocate memory and read RGB 
    ImgRGB** Data_RGB = malloc_2D(bmpheader.height, bmpheader.width);
    int skip = (4 - (bmpheader.width * 3) % 4);
    if (skip == 4) skip = 0;
    
    
    InputData(fp_in, Data_RGB, bmpheader.height, bmpheader.width, skip);
    fclose(fp_in);

    //  RGB 2 YCbCr
    ImgYCbCr** Data_YCbCr = malloc_2D_YCbCr(bmpheader.height, bmpheader.width);
    RGB_to_YCbCr(Data_RGB, Data_YCbCr, bmpheader.height, bmpheader.width);
    
    // do YCbCr 
    fp_Qt_Y = fopen(fn_Qt_Y, "w");
    fp_Qt_Cb = fopen(fn_Qt_Cb, "w");
    fp_Qt_Cr = fopen(fn_Qt_Cr, "w");
    quantizedYFile = fopen(fn_qF_Y, "wb");
    quantizedCbFile = fopen(fn_qF_Cb, "wb");
    quantizedCrFile = fopen(fn_qF_Cr, "wb");
    errorYFile = fopen(fn_eF_Y, "wb");
    errorCbFile = fopen(fn_eF_Cb, "wb");
    errorCrFile = fopen(fn_eF_Cr, "wb");


    
    process_YCbCr_blocks(Data_YCbCr, bmpheader.height, bmpheader.width, quantizedYFile, quantizedCbFile, quantizedCrFile, errorYFile, errorCbFile, errorCrFile);

    // 寫入量化表
    for (int x = 0; x < BLOCK_SIZE; x++) {
        for (int y = 0; y < BLOCK_SIZE; y++) {
            int temp = 0;
            fprintf(fp_Qt_Y, "%d ", Y_quantization_table[x][y]);         
            fprintf(fp_Qt_Cb, "%d ", Cb_quantization_table[x][y]);
            fprintf(fp_Qt_Cr, "%d ", Cr_quantization_table[x][y] );
        }
        fprintf(fp_Qt_Y, "\n");  
        fprintf(fp_Qt_Cb, "\n"); 
        fprintf(fp_Qt_Cr, "\n"); 
    }
    
    // 關所有文件
    fclose(fp_Qt_Y);
    fclose(fp_Qt_Cb);
    fclose(fp_Qt_Cr);
    fclose(quantizedYFile);
    fclose(quantizedCbFile);
    fclose(quantizedCrFile);
    fclose(errorYFile);
    fclose(errorCbFile);
    fclose(errorCrFile);
    


    // free内存
    free_2D((void**)Data_RGB, bmpheader.height);
    free_2D((void**)Data_YCbCr, bmpheader.height);

    // 寫dim
    fp_out_dim = fopen(fn_dim, "w");
    if (fp_out_dim) {
        fprintf(fp_out_dim, "%d %d\n", bmpheader.width, bmpheader.height);
        fclose(fp_out_dim);
    } else {
        printf("Error opening dimension file for writing.\n");
    }
}

void demo2(char *fn_bmp, char *t, char *fn_rle_code) {
    FILE *fp_in;
    FILE *fp_rle_code;


    // 打開输入文件
    fp_in = fopen(fn_bmp, "rb");
    if (!fp_in) {
        printf("Failed to open input file %s\n", fn_bmp);
        return;
    }

    // read BMP header
    Bitmap bmpheader;
    readheader(fp_in, &bmpheader);


    

    // Allocate memory and read RGB 
    ImgRGB** Data_RGB = malloc_2D(bmpheader.height, bmpheader.width);
    int skip = (4 - (bmpheader.width * 3) % 4);
    if (skip == 4) skip = 0;
    
    
    InputData(fp_in, Data_RGB, bmpheader.height, bmpheader.width, skip);
    fclose(fp_in);

    //  RGB 2 YCbCr
    ImgYCbCr** Data_YCbCr = malloc_2D_YCbCr(bmpheader.height, bmpheader.width);
    RGB_to_YCbCr(Data_RGB, Data_YCbCr, bmpheader.height, bmpheader.width);

    int mode = 0 ;
    mode = strcmp(t, "ascii");

    // 確認是ascii還是bin
    if (mode == 0) {
        fp_rle_code = fopen(fn_rle_code, "w");
        
    } else {
        fp_rle_code = fopen(fn_rle_code, "wb");
       
    }

    process_YCbCr_blocks_rle(Data_YCbCr, bmpheader.height, bmpheader.width, fp_rle_code, mode);

    // free内存
    free_2D((void**)Data_RGB, bmpheader.height);
    free_2D((void**)Data_YCbCr, bmpheader.height);

 
}

int main(int argc, char **argv)
{
    int option = 0;// 0 for demo0, 1 for demo1, etc
    

    if ( argc <=2 ) {
        usage0(stderr);
        exit(1);
    }

    option = atoi(argv[1]);


    switch(option)
    {
        case 0:
        demo0(argv[2], argv[3], argv[4], argv[5], argv[6]);
        break;

        case 1:
        demo1(argv[2], argv[3], argv[4], argv[5], argv[6],argv[7], argv[8], argv[9], argv[10], argv[11], argv[12]);
        break;
        
        case 2:
        demo2(argv[2], argv[3], argv[4]);
        break;

        default:
        break;
        // default statements
    }
    printf("done");
    return 0; // 0: exit normally 1: exit with error
}


/* read header */
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

/* input data without header into RGB structure */
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


//RGB 2 YCbCr 
void RGB_to_YCbCr(ImgRGB** RGB, ImgYCbCr** YCbCr, int H, int W) {
    for (int i = 0; i < H; i++) {
        for (int j = 0; j < W; j++) {
            float R = RGB[i][j].R;
            float G = RGB[i][j].G;
            float B = RGB[i][j].B;

            //  RGB 2 YCbCr(範圍落在0-255)
            YCbCr[i][j].Y = fmin(255, fmax(0, 0.299 * R + 0.587 * G + 0.114 * B ));
            YCbCr[i][j].Cb = fmin(255, fmax(0, 128 - 0.168736 * R - 0.33124 * G + 0.5 * B));
            YCbCr[i][j].Cr = fmin(255, fmax(0, 128 + 0.5 * R - 0.418688 * G - 0.081312 * B));
        }
    }
}

void free_2D(void** array, int rows) {
    free(array[0]);  
    free(array);     
}

void process_YCbCr_blocks(ImgYCbCr** YCbCr, int H, int W, FILE *quantizedYFile, FILE *quantizedCbFile, FILE *quantizedCrFile, FILE *errorYFile, FILE *errorCbFile, FILE *errorCrFile) {
    double blockY[BLOCK_SIZE][BLOCK_SIZE];
    double blockCb[BLOCK_SIZE][BLOCK_SIZE];
    double blockCr[BLOCK_SIZE][BLOCK_SIZE];
    double signal_powerY[BLOCK_SIZE][BLOCK_SIZE] = {0};
    double noise_powerY[BLOCK_SIZE][BLOCK_SIZE] = {0};
    double signal_powerCb[BLOCK_SIZE][BLOCK_SIZE] = {0};
    double noise_powerCb[BLOCK_SIZE][BLOCK_SIZE] = {0};
    double signal_powerCr[BLOCK_SIZE][BLOCK_SIZE] = {0};
    double noise_powerCr[BLOCK_SIZE][BLOCK_SIZE] = {0};

    //分割8*8
    for (int x = 0; x < H; x += 8) {
        for (int y = 0; y < W; y += 8) {
            //  取值&補滿 8x8 blocks
            for (int i = 0; i < 8; i++) {
                for (int j = 0; j < 8; j++) {
                    int posX = x + i < H ? x + i : H - 1;
                    int posY = y + j < W ? y + j : W - 1;
                    blockY[i][j] = YCbCr[posX][posY].Y;
                    blockCb[i][j] = YCbCr[posX][posY].Cb;
                    blockCr[i][j] = YCbCr[posX][posY].Cr;
                }
            }

            // Process Y, Cb, Cr blocks with DCT transform and quantization
            process_block(blockY, Y_quantization_table, quantizedYFile, errorYFile, signal_powerY, noise_powerY);
            process_block(blockCb, Cb_quantization_table, quantizedCbFile, errorCbFile, signal_powerCb, noise_powerCb);
            process_block(blockCr, Cr_quantization_table, quantizedCrFile, errorCrFile, signal_powerCr, noise_powerCr);
        }
    }
    // 螢幕顯示 SQNR
    printf("Y's SQNR : \n");
    print_SQNR(signal_powerY, noise_powerY);
    printf("Cb's SQNR : \n");
    print_SQNR(signal_powerCb, noise_powerCb);
    printf("Cr's SQNR : \n");
    print_SQNR(signal_powerCr, noise_powerCr);
}



void process_block(double block[BLOCK_SIZE][BLOCK_SIZE], int quant_table[BLOCK_SIZE][BLOCK_SIZE], FILE* quantizedFile, FILE* errorFile, double signal_power[BLOCK_SIZE][BLOCK_SIZE], double noise_power[BLOCK_SIZE][BLOCK_SIZE]) {
    double preQuantizedBlock[BLOCK_SIZE][BLOCK_SIZE];

    // Shift 128
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            block[i][j] -= 128.0;
        }
    }

    //  DCT 
    dct_transform(block);

    // 先儲存值方便計算量化誤差
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            preQuantizedBlock[i][j] = block[i][j];
        }
    }

    // Quantize
    quantize_block(block, quant_table);

    // 存 quantized values &  quantization error & 算 SQNR
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            float error;
            short quantizedValue = (short)block[i][j];
            fwrite(&quantizedValue, sizeof(quantizedValue), 1, quantizedFile);

            error = preQuantizedBlock[i][j] - quantizedValue * quant_table[i][j];
            
            fwrite(&error, sizeof(float), 1, errorFile);
            signal_power[i][j] += preQuantizedBlock[i][j] * preQuantizedBlock[i][j];
            noise_power[i][j] += error * error;
        }
        
    }

    
}



//  DCT 
void dct_transform(double block[BLOCK_SIZE][BLOCK_SIZE]) {
    double temp[BLOCK_SIZE][BLOCK_SIZE];
    double cu, cv, sum;

    for (int u = 0; u < BLOCK_SIZE; u++) {
        for (int v = 0; v < BLOCK_SIZE; v++) {
            if (u == 0) cu = 1 / sqrt(2); else cu = 1;
            if (v == 0) cv = 1 / sqrt(2); else cv = 1;

            sum = 0;
            for (int x = 0; x < BLOCK_SIZE; x++) {
                for (int y = 0; y < BLOCK_SIZE; y++) {
                    sum += block[x][y] *
                           cos((2 * x + 1) * u * PI / (2 * BLOCK_SIZE)) *
                           cos((2 * y + 1) * v * PI / (2 * BLOCK_SIZE));
                }
            }
            temp[u][v] = 0.25 * cu * cv * sum;
        }
    }

    
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            block[i][j] = temp[i][j];
        }
    }
}

// quantization
void quantize_block(double block[BLOCK_SIZE][BLOCK_SIZE], int quant_table[BLOCK_SIZE][BLOCK_SIZE]) {
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            block[i][j] = round(block[i][j] / quant_table[i][j]);
        }
    }
}

//SQNR
void print_SQNR( double signal_power[BLOCK_SIZE][BLOCK_SIZE], double noise_power[BLOCK_SIZE][BLOCK_SIZE]){
    printf("{");
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            if (noise_power[i][j] != 0) {
                double sqnr = 10 * log10(signal_power[i][j] / noise_power[i][j]);
                printf("%lf ", sqnr);
            } else {
                printf("Infinity ");
            }
        }
        printf("\n");
    }
    printf("}\n");
    printf("\n");
}


void process_YCbCr_blocks_rle(ImgYCbCr** YCbCr, int H, int W , FILE *rle, int mode ) {
    double blockY[BLOCK_SIZE][BLOCK_SIZE];
    double blockCb[BLOCK_SIZE][BLOCK_SIZE];
    double blockCr[BLOCK_SIZE][BLOCK_SIZE];
    double count_y = 0, count_cb = 0,count_cr = 0;
    short dpcmY[BLOCK_SIZE][BLOCK_SIZE], dpcmCb[BLOCK_SIZE][BLOCK_SIZE], dpcmCr[BLOCK_SIZE][BLOCK_SIZE];
    short zigzagY[BLOCK_SIZE * BLOCK_SIZE], zigzagCb[BLOCK_SIZE * BLOCK_SIZE], zigzagCr[BLOCK_SIZE * BLOCK_SIZE];
    RLEPair rleY[BLOCK_SIZE * BLOCK_SIZE], rleCb[BLOCK_SIZE * BLOCK_SIZE], rleCr[BLOCK_SIZE * BLOCK_SIZE];
    int rleSizeY, rleSizeCb, rleSizeCr;

    if(mode == 0){
        fprintf(rle, "%d %d\n", H, W);
    }
    else{
        fwrite(&(H), sizeof(int), 1, rle);
        fwrite(&(W), sizeof(int), 1, rle);
    }
    // Process blocks
    for (int x = 0; x < H; x += BLOCK_SIZE) {
        for (int y = 0; y < W; y += BLOCK_SIZE) {
            // Fill 8x8 blocks
            for (int i = 0; i < BLOCK_SIZE; i++) {
                for (int j = 0; j < BLOCK_SIZE; j++) {
                    int posX = x + i < H ? x + i : H - 1;
                    int posY = y + j < W ? y + j : W - 1;
                    blockY[i][j] = YCbCr[posX][posY].Y;
                    blockCb[i][j] = YCbCr[posX][posY].Cb;
                    blockCr[i][j] = YCbCr[posX][posY].Cr;
                }
            }

            // DCT and Quantization
            process_block_ne(blockY, Y_quantization_table);
            process_block_ne(blockCb, Cb_quantization_table);
            process_block_ne(blockCr, Cr_quantization_table);

            // DPCM Encoding
            dpcm_encode(blockY, dpcmY);
            dpcm_encode(blockCb, dpcmCb);
            dpcm_encode(blockCr, dpcmCr);

            // Zigzag Scanning
            zigzag_scan_modified(dpcmY, zigzagY);
            zigzag_scan_modified(dpcmCb, zigzagCb);
            zigzag_scan_modified(dpcmCr, zigzagCr);

            // RLE
            rleSizeY = run_length_encode(zigzagY, rleY, BLOCK_SIZE * BLOCK_SIZE);
            rleSizeCb = run_length_encode(zigzagCb, rleCb, BLOCK_SIZE * BLOCK_SIZE);
            rleSizeCr = run_length_encode(zigzagCr, rleCr, BLOCK_SIZE * BLOCK_SIZE);

             // Calculate block indices
            int blockX = x / BLOCK_SIZE;
            int blockY = y / BLOCK_SIZE;

            count_y += rleSizeY;
            count_cb += rleSizeCb;
            count_cr += rleSizeCr;

            if(mode == 0){
                // Write RLE for Y component
                fprintf(rle, "(%d,%d,Y)", blockX, blockY);
                for (int i = 0; i < rleSizeY; i++) {
                    fprintf(rle, " %d %d", rleY[i].count, rleY[i].value);
                }
                fprintf(rle, "\n");

                // Write RLE for Cb component
                fprintf(rle, "(%d,%d,Cb)", blockX, blockY);
                for (int i = 0; i < rleSizeCb; i++) {
                    fprintf(rle, " %d %d", rleCb[i].count, rleCb[i].value);
                }
                fprintf(rle, "\n");

                // Write RLE for Cr component
                fprintf(rle, "(%d,%d,Cr)", blockX, blockY);
                for (int i = 0; i < rleSizeCr; i++) {
                    fprintf(rle, " %d %d", rleCr[i].count, rleCr[i].value);
                }
                fprintf(rle, "\n");
            }
            else{
                // Write RLE data
                for (int i = 0; i < rleSizeY; i++) {
                    fwrite(&(rleY[i].count), sizeof(int), 1, rle);
                    fwrite(&(rleY[i].value), sizeof(short), 1, rle);
                }

                // Write RLE data for Cb component
                for (int i = 0; i < rleSizeCb; i++) {
                    fwrite(&(rleCb[i].count), sizeof(int), 1, rle);
                    fwrite(&(rleCb[i].value), sizeof(short), 1, rle);
                }

                // Write RLE data for Cr component
                for (int i = 0; i < rleSizeCr; i++) {
                    fwrite(&(rleCr[i].count), sizeof(int), 1, rle);
                    fwrite(&(rleCr[i].value), sizeof(short), 1, rle);
                }

                
            }
        }
    }
    if (mode != 0){ 
        double total = count_y + count_cb + count_cr + 2;
        count_y = 1.0-(count_y*2/((double)H*(double)W));
        count_cb = 1.0-(count_cb*2/((double)H*(double)W));
        count_cr = 1.0-(count_cr*2/((double)H*(double)W));
        total = 1.0-(total *2/((double)H*(double)W));
        printf("Y's compression ratio:%lf\nCb's compression ratio:%lf\nCr's compression ratio:%lf\ntotal compression ratio:%lf\n",count_y,count_cb,count_cr,total);
    }
    
}

void process_block_ne(double block[BLOCK_SIZE][BLOCK_SIZE], int quant_table[BLOCK_SIZE][BLOCK_SIZE]) {
    // Shift 128
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            block[i][j] -= 128.0;
        }
    }
    //  DCT 
    dct_transform(block);
   
    // Quantize
    quantize_block(block, quant_table);
    
}

//dpcm encode
void dpcm_encode(double block[BLOCK_SIZE][BLOCK_SIZE], short encoded_block[BLOCK_SIZE][BLOCK_SIZE]) {
    double previous = 0;
    for (int i = 0; i < BLOCK_SIZE; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            double current = block[i][j];
            encoded_block[i][j] = (short)(current - previous);
            previous = current;
        }
    }
}

// zigzag(未modified已刪除)
void zigzag_scan_modified(short input[BLOCK_SIZE][BLOCK_SIZE], short output[BLOCK_SIZE * BLOCK_SIZE]) {
    int i = 0, j = 0, k = 0;
    int total = BLOCK_SIZE * BLOCK_SIZE;

    while (k < total) {
        // Move upwards diagonally
        while (i >= 0 && j < BLOCK_SIZE) {
            output[k++] = input[i][j];
            i--;
            j++;
        }

        // Move to the next row or column
        if (i < 0 && j <= BLOCK_SIZE - 1) {
            i++;
        } else if (j == BLOCK_SIZE) {
            i += 2;
            j--;
        }

        // Move downwards diagonally
        while (j >= 0 && i < BLOCK_SIZE) {
            output[k++] = input[i][j];
            i++;
            j--;
        }

        // Move to the next row or column
        if (j < 0 && i <= BLOCK_SIZE - 1) {
            j++;
        } else if (i == BLOCK_SIZE) {
            j += 2;
            i--;
        }
    }
}

//run length encode
int run_length_encode(short* input, RLEPair* output, int length) {
    int k = 0, i = 0;
    while (i < length) {
        output[k].value = input[i];
        output[k].count = 1;

        while (i + 1 < length && input[i] == input[i + 1]) {
            output[k].count++;
            i++;
        }

        k++;
        i++;
    }
    return k; 
}

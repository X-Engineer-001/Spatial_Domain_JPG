#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <algorithm>
#include <iostream>
using namespace std;

#pragma pack(2)
struct BmpFileHeader {
    uint16_t bfTybe;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};
struct BmpInfoHeader {
    uint32_t biSize;
    uint32_t biWidth;
    uint32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biImageSize;
    uint32_t biXPelsPerMeter;
    uint32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack()
#pragma pack(1)
struct rgb_t {
    uint8_t r=0,g=0,b=0;
};
struct brush_t {
    uint8_t width = 0, height = 0;
    rgb_t color;
};
#pragma pack()
static brush_t brush;

struct img_t {
    uint32_t width, height;
    uint16_t bits;
    uint8_t type;
    rgb_t* pixel;
};
static img_t img;
#define RGB 3
#define RGBA 4
#define GRAY 1
#define UNKNOW 0

inline const int index(const int w, int i, int j){
    return i*w+j;
}

inline rgb_t* Index_img(int i, int j){
    return (img.pixel+i*img.width+j);
}

void deCompression(const char*name){
    FILE*fp = fopen(name, "rb+");
    fread((char*)&img.width, sizeof(uint32_t), 1, fp);
    fread((char*)&img.height, sizeof(uint32_t), 1, fp);

    const int width = img.width, height = img.height;
    const int bits = img.bits = 24, type = img.type = RGB;
    rgb_t*pixel = img.pixel = (rgb_t*)calloc(((size_t)width)*((size_t)height), sizeof(rgb_t));
    bool*done = (bool*)calloc(((size_t)width)*((size_t)height), sizeof(bool));
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            if(!(*(done+index(width,i,j)))){
                fread((char*)&brush, sizeof(uint8_t), sizeof(brush_t), fp);
                rgb_t pivot = brush.color;
                for(int x = 0; x < brush.height; x++){
                    for(int y = 0; y < brush.width; y++){
                        rgb_t*tmp = Index_img(i+x,j+y);
                        *tmp = pivot;
                        *(done+index(width,i+x,j+y)) = true;
                    }
                }
            }
        }
    }
    fclose(fp);
}

void Write(const char* name) {
    const uint32_t width = img.width, height = img.height;
    const uint16_t bits = img.bits;
    const uint8_t type = img.type;
    const rgb_t*pixel = img.pixel;
    struct BmpFileHeader file_h = {0x4d42, 54+width*height*bits/8, 0, 0, 54};
    if(type == GRAY) {file_h.bfSize += 1024; file_h.bfOffBits += 1024;}

    struct BmpInfoHeader info_h = {40, width, height, 1, bits, 0, width*height * bits/8, 0, 0, 0, 0};
    if(type == GRAY) {info_h.biClrUsed = 256;}

    FILE *fp = fopen(name,"wb+");
    fwrite((char*)&file_h, sizeof(uint8_t), sizeof(file_h), fp);
    fwrite((char*)&info_h, sizeof(uint8_t), sizeof(info_h), fp);

    if(type == GRAY) {
        for(int i = 0; i < 256; i++) {
            uint8_t c = i;
            fwrite((char*)&c, sizeof(uint8_t), 1, fp);
            fwrite((char*)&c, sizeof(uint8_t), 1, fp);
            fwrite((char*)&c, sizeof(uint8_t), 1, fp);
            fwrite("", sizeof(uint8_t), 1, fp);
        }
    }

    const long img_align = (width*bits/8*type) % 4;
    const uint8_t full = 255;
    for(int i = height-1; i >= 0; i--) {
        for(int j = 0; j < width; j++) {
            const rgb_t*c = Index_img(i,j);
            if(type == RGB) {
                fwrite((char*)&(c->b), sizeof(uint8_t), 1, fp);
                fwrite((char*)&(c->g), sizeof(uint8_t), 1, fp);
                fwrite((char*)&(c->r), sizeof(uint8_t), 1, fp);
            }else if(type == RGBA) {
                fwrite((char*)&(c->b), sizeof(uint8_t), 1, fp);
                fwrite((char*)&(c->g), sizeof(uint8_t), 1, fp);
                fwrite((char*)&(c->r), sizeof(uint8_t), 1, fp);
                fwrite((char*)&full, sizeof(uint8_t), 1, fp);
            }else if(type == GRAY) {
                fwrite((char*)&(c->b), sizeof(uint8_t), 1, fp);
            }
        }

        for(long i = 0; i < img_align; i++) {
            fwrite("", sizeof(uint8_t), 1, fp);
        }
    }
    fclose(fp);
}

int main() {
    string str;
    getline(cin, str);
    if(str.length() <= 6 || str.substr(str.length()-6,6) != ".sdjpg"){str+=".sdjpg";}
    deCompression(str.c_str());
    Write((str.substr(0,str.rfind("."))+"_decomp.bmp").c_str());
    return 0;
}

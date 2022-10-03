#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <algorithm>
#include <iostream>
#include <math.h>
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

struct ycbcr_t{
    uint8_t y=0;
    int8_t cb=0,cr=0;
};
inline ycbcr_t rgb_ycbcr(const rgb_t input){
    const double r = (double)input.r, g = (double)input.g, b = (double)input.b;
    ycbcr_t c;
    c.y = (uint8_t)round(0.299*(r-g)+g+0.114*(b-g));
    c.cb = (int8_t)round(0.5643*(b-c.y));
    c.cr = (int8_t)round(0.7133*(r-c.y));
    return c;
}
inline rgb_t ycbcr_rgb(const ycbcr_t input){
    const double y = (double)input.y, cb = (double)input.cb, cr = (double)input.cr;
    rgb_t c;
    c.b = (uint8_t)round(cb/0.5643+y);
    c.r = (uint8_t)round(cr/0.7133+y);
    c.g = (uint8_t)round(y-0.3443*cb-0.7137*cr);
    return c;
}

inline ycbcr_t YCbCr_min(const ycbcr_t c1, const ycbcr_t c2){
    ycbcr_t c;
    c.y = min(c1.y, c2.y);
    c.cb = min(c1.cb, c2.cb);
    c.cr = min(c1.cr, c2.cr);
    return c;
}
inline ycbcr_t YCbCr_max(const ycbcr_t c1, const ycbcr_t c2){
    ycbcr_t c;
    c.y = max(c1.y, c2.y);
    c.cb = max(c1.cb, c2.cb);
    c.cr = max(c1.cr, c2.cr);
    return c;
}


static double Yrule=4, CBrule=50, CRrule=50;
inline bool YCbCr_close(const ycbcr_t c1, const ycbcr_t c2){
    double Dark = (max(abs(c1.y-255), abs(c2.y-255))+255)/255;
    double Yclose = abs(c1.y-c2.y);
    double CBclose = abs(c1.cb-c2.cb);
    double CRclose = abs(c1.cr-c2.cr);
    return ((Yclose < Yrule*Dark) && (CBclose < 0.5643*CBrule*Dark) && (CRclose < 0.7133*CRrule*Dark));
}

inline const int index(const int w, int i, int j){
    return i*w+j;
}

inline rgb_t* Index_img(int i, int j){
    return (img.pixel+i*img.width+j);
}

void Read(const char* name) {
    struct BmpFileHeader file_h;
    struct BmpInfoHeader info_h;
    FILE*fp = fopen(name, "rb+");
    fread((char*)&file_h, sizeof(uint8_t), sizeof(file_h), fp);
    fread((char*)&info_h, sizeof(uint8_t), sizeof(info_h), fp);
    const int width = img.width = info_h.biWidth;
    const int height = img.height = info_h.biHeight;
    const int bits = img.bits = info_h.biBitCount;
    if(bits == 24){
        img.type = RGB;
    }else if(bits == 32){
        img.type = RGBA;
    }else if(img.bits == 8){
        img.type = GRAY;
    }else{
        img.type = UNKNOW;
    }
    const int type = img.type;
    rgb_t*pixel = img.pixel = (rgb_t*)calloc(((size_t)width)*((size_t)height), sizeof(rgb_t));

    fseek(fp, file_h.bfOffBits, SEEK_SET);
    const long img_align = (width*bits/8*type) % 4;
    char trash;
    for(int i = height-1; i >= 0; i--) {
        for(int j = 0; j < width; j++) {
            rgb_t*c = Index_img(i,j);
            if(type == RGB) {
                fread((char*)&(c->b), sizeof(uint8_t), 1, fp);
                fread((char*)&(c->g), sizeof(uint8_t), 1, fp);
                fread((char*)&(c->r), sizeof(uint8_t), 1, fp);
            }else if(type == RGBA) {
                fread((char*)&(c->b), sizeof(uint8_t), 1, fp);
                fread((char*)&(c->g), sizeof(uint8_t), 1, fp);
                fread((char*)&(c->r), sizeof(uint8_t), 1, fp);
                fread((char*)&trash, sizeof(uint8_t), 1, fp);
            }else if(type == GRAY) {
                fread((char*)&(c->b), sizeof(uint8_t), 1, fp);
                c->r = c->g = c->b;
            }
        }
        fseek(fp , img_align , SEEK_CUR);
    }
    fclose(fp);
}

inline bool cmp(rgb_t a, rgb_t b){
    return rgb_ycbcr(a).y < rgb_ycbcr(b).y;
}
void Median_flower(){
    const int width = img.width, height = img.height;
    const int bits = img.bits, type = img.type;
    rgb_t*pixel = img.pixel;
    rgb_t*newP = (rgb_t*)calloc(((size_t)width)*((size_t)height), sizeof(rgb_t));
    rgb_t arr[5];
    const int dir[5][2] = {{0,0},{0,-1},{0,1},{-1,0},{1,0}};
    for(int i = 0; i < height; i++) {
        for(int j = 0; j < width; j++) {
            for(int d = 0; d < 5; d++){
                int x = dir[d][0], y = dir[d][1];
                arr[d] = *Index_img(min(max(i+x,0),height-1),min(max(j+y,0),width-1));
            }
            sort(arr,arr+5,cmp);
            *(newP+i*width+j) = arr[2];
        }
    }
    free(pixel);
    img.pixel = newP;
}

void Compress(const char* name){
    const int width = img.width, height = img.height;
    const int bits = img.bits, type = img.type;
    rgb_t*pixel = img.pixel;
    bool*done = (bool*)calloc(((size_t)width)*((size_t)height), sizeof(bool));

    FILE *fp = fopen(name,"wb+");
    fwrite((char*)&width, sizeof(uint32_t), 1, fp);
    fwrite((char*)&height, sizeof(uint32_t), 1, fp);

    for(int i = 0; i < height; i++) {
        for(int j = 0; j < width; j++) {
            if(!(*(done+index(width,i,j)))){
                ycbcr_t RminYCbCr, RmaxYCbCr, DminYCbCr, DmaxYCbCr, c;
                DmaxYCbCr = DminYCbCr = RmaxYCbCr = RminYCbCr = rgb_ycbcr(*Index_img(i,j));
                uint8_t sqr = 0, right = 0, down = 0, rightB = 0, downB = 0;
                int sqrF = 1, rightF = 1, downF = 1;
                while(sqrF || rightF || downF){
                    sqr += sqrF;
                    right += rightF;
                    down += downF;
                    if(rightF && (j+right >= width || right >= 255)){rightF = 0; sqrF = 0;}
                    if(downF && (i+down >= height || down >= 255)){downF = 0; sqrF = 0;}
                    if(rightF){
                        for(int edge = 0; edge < sqr; edge++){
                            if(rightB == 0 && *(done+index(width, i+edge, j+right))){rightB = right;}
                            c = rgb_ycbcr(*Index_img(i+edge, j+right));
                            RminYCbCr = YCbCr_min(RminYCbCr, c);
                            RmaxYCbCr = YCbCr_max(RmaxYCbCr, c);
                        }
                        if(!YCbCr_close(RminYCbCr,RmaxYCbCr)){rightF = 0; sqrF = 0;}
                    }
                    if(downF){
                        for(int edge = 0; edge < sqr; edge++){
                            if(downB == 0 && *(done+index(width, i+down, j+edge))){downB = down;}
                            c = rgb_ycbcr(*Index_img(i+down, j+edge));
                            DminYCbCr = YCbCr_min(DminYCbCr, c);
                            DmaxYCbCr = YCbCr_max(DmaxYCbCr, c);
                        }
                        if(!YCbCr_close(DminYCbCr,DmaxYCbCr)){downF = 0; sqrF = 0;}
                    }
                    if(sqrF){
                        c = rgb_ycbcr(*Index_img(i+sqr,j+sqr));
                        if(!(*(done+index(width, i+sqr, j+sqr))) && YCbCr_close(RminYCbCr,c) && YCbCr_close(RmaxYCbCr,c)){
                            DminYCbCr = RminYCbCr = YCbCr_min(RminYCbCr, c);
                            DmaxYCbCr = RmaxYCbCr = YCbCr_max(RmaxYCbCr, c);
                            DminYCbCr = RminYCbCr = YCbCr_min(RminYCbCr, DminYCbCr);
                            DmaxYCbCr = RmaxYCbCr = YCbCr_max(RmaxYCbCr, DmaxYCbCr);
                        }else{sqrF = 0;}
                    }
                }
                if(rightB>0){right = (right+rightB)/2;}
                if(downB>0){down = (down+downB)/2;}
                if(right>=down){
                    brush.height = sqr;
                    brush.width = right;
                }else{
                    brush.height = down;
                    brush.width = sqr;
                }
                for(int x = 0; x < brush.height; x++){
                    for(int y = 0; y < brush.width; y++){
                        *(done+index(width,i+x,j+y)) = true;
                    }
                }
                brush.color = *Index_img(i+brush.height/2,j+brush.width/2);
                fwrite((char*)&brush, sizeof(uint8_t), sizeof(brush_t), fp);
            }
        }
    }
    fclose(fp);
}

int main() {
    string str;
    getline(cin, str);
    if(str.length() <= 4 || str.substr(str.length()-4,4) != ".bmp"){str+=".bmp";}
    Read(str.c_str());
    Median_flower();
    Compress((str.substr(0,str.rfind("."))+".sdjpg").c_str());
    return 0;
}

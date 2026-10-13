// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdint.h>
#include <stddef.h>
extern "C" int snprintf(char*,size_t,const char*,...);
#include "glcdfont.c"
template<class T> T min(T a,T b) { return a<b?a:b; }
template<class T> T max(T a,T b) { return a>b?a:b; }
template<class T> T constrain(T v,T lo,T hi) { return min(max(v,lo),hi); }

// Run the firmware layout functions with GFX drawing backed by host pixels.
class GFXcanvas16 {
 public:
  GFXcanvas16(int,int) {}
  uint16_t* getBuffer() { return pixels_; }
  void fillScreen(uint16_t c) { for(auto& p:pixels_) p=c; }
  void setFont(const void*) {}
  void setTextSize(int s) { size_=s; }
  void setTextWrap(bool) {}
  void setTextColor(uint16_t c) { color_=c; }
  void setCursor(int x,int y) { x_=x; y_=y; }
  void print(const char* text) {
    while(*text) {
      const unsigned char ch=*text++;
      for(int x=0;x<5;++x) for(int y=0;y<8;++y)
        if(font[ch*5+x]&(1<<y)) fillRect(x_+x*size_,y_+y*size_,size_,size_,color_);
      x_+=6*size_;
    }
  }
  void pixel(int x,int y,uint16_t c) { if(x>=0&&x<160&&y>=0&&y<80) pixels_[y*160+x]=c; }
  void fillRect(int x,int y,int w,int h,uint16_t c) { for(int j=0;j<h;++j) for(int i=0;i<w;++i) pixel(x+i,y+j,c); }
  void drawFastHLine(int x,int y,int w,uint16_t c) { fillRect(x,y,w,1,c); }
  void drawFastVLine(int x,int y,int h,uint16_t c) { fillRect(x,y,1,h,c); }
  void drawRect(int x,int y,int w,int h,uint16_t c) {
    drawFastHLine(x,y,w,c); drawFastHLine(x,y+h-1,w,c); drawFastVLine(x,y,h,c); drawFastVLine(x+w-1,y,h,c);
  }
  void drawLine(int x0,int y0,int x1,int y1,uint16_t c) {
    int dx=x1>x0?x1-x0:x0-x1, sx=x0<x1?1:-1;
    int dy=y1>y0?y0-y1:y1-y0, sy=y0<y1?1:-1, error=dx+dy;
    for(;;) { pixel(x0,y0,c); if(x0==x1&&y0==y1) break; int e=error*2; if(e>=dy) { error+=dy; x0+=sx; } if(e<=dx) { error+=dx; y0+=sy; } }
  }
  void fillCircle(int x,int y,int r,uint16_t c) { for(int dy=-r;dy<=r;++dy) for(int dx=-r;dx<=r;++dx) if(dx*dx+dy*dy<=r*r) pixel(x+dx,y+dy,c); }
  void fillTriangle(int x0,int y0,int x1,int y1,int x2,int y2,uint16_t c) {
    int loX=min(x0,min(x1,x2)),hiX=max(x0,max(x1,x2)),loY=min(y0,min(y1,y2)),hiY=max(y0,max(y1,y2));
    for(int y=loY;y<=hiY;++y) for(int x=loX;x<=hiX;++x) {
      const int a=(x1-x0)*(y-y0)-(y1-y0)*(x-x0),b=(x2-x1)*(y-y1)-(y2-y1)*(x-x1),d=(x0-x2)*(y-y2)-(y0-y2)*(x-x2);
      if((a>=0&&b>=0&&d>=0)||(a<=0&&b<=0&&d<=0)) pixel(x,y,c);
    }
  }
 private:
  uint16_t pixels_[160*80];
  int x_=0,y_=0,size_=1; uint16_t color_=0xffff;
};

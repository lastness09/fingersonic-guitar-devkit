#include "GameRuntime.h"
#include "GameSprites.h"
#include "config.h"
#ifdef PEA_HOST_TEST
#include "tests/CanvasMock.h"
#else
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#endif
#include <string.h>

namespace pea {
namespace {
constexpr uint16_t black=0x0000,white=0xffff,dim=0x8c51,red=0xf9e7,cyan=0x3edf,yellow=0xff04;
constexpr uint16_t grassA=0x2247,grassB=0x2a68,leaf=0x6f24,line=0x39ca;
GFXcanvas16 canvas(160,80);
const char* const difficultyNames[]={"EASY","NORMAL","HARD"};
const char* const menuNames[]={"DIFFICULTY","SOUND","LED SHOW","RESTART","UPLOADER"};
uint16_t color(char pixel) {
  switch(pixel) {
    case 'g': return 0x1265; case 'G': return leaf; case 'o': return 0x0983;
    case 'l': return 0x34a8; case 's': return 0xb649; case 'w': return white;
    case 'k': return black; case 'j': return 0x638f; case 'p': return 0x2926;
    case 'r': return red; case 'v': return 0x9bfa; default: return black;
  }
}
void sprite(const char* const* rows,int height,int x,int y,int scale=1,uint16_t tint=0) {
  for(int row=0;row<height;++row) for(int col=0;rows[row][col];++col)
    if(rows[row][col]!='.') canvas.fillRect(x+col*scale,y+row*scale,scale,scale,tint?tint:color(rows[row][col]));
}
void text(int x,int y,const char* label,uint16_t c=white,int size=1) {
  canvas.setFont(nullptr); canvas.setTextWrap(false); canvas.setTextSize(size); canvas.setTextColor(c); canvas.setCursor(x,y); canvas.print(label);
}
void centered(int y,const char* label,uint16_t c=white,int size=1) { text((160-int(strlen(label))*6*size)/2,y,label,c,size); }
void right(int edge,int y,const char* label,uint16_t c=white) { text(edge-int(strlen(label))*6,y,label,c); }
void battery(const Frame& f) {
  const uint16_t c=f.full||f.charging?leaf:(f.batteryPercent<20?red:dim);
  canvas.drawRect(143,3,13,7,c); canvas.fillRect(156,5,2,3,c);
  canvas.fillRect(145,5,f.batteryPercent*9/100,3,c);
}
void header(const Frame& f) {
  canvas.fillRect(0,0,160,13,black); text(2,3,"PEA",leaf);
  char label[16]; snprintf(label,sizeof(label),"W%02d",f.game.wave); text(28,3,label,cyan);
  for(int i=0;i<3;++i) sprite(heart,5,55+i*8,4,1,i<f.game.lives?0:line);
  snprintf(label,sizeof(label),"%05lu",(unsigned long)f.game.score); right(136,3,label,white);
  battery(f); canvas.drawFastHLine(0,12,160,line);
}
void board(const Frame& f) {
  canvas.fillScreen(black);
  for(int lane=0;lane<6;++lane) {
    const int y=14+lane*9;
    canvas.fillRect(10,y,150,8,lane%2?grassA:grassB);
    for(int x=31+lane%3;x<160;x+=16) canvas.drawFastHLine(x,y+6,2,0x3288);
    if(lane==f.selected) canvas.drawRect(0,y-1,9,9,yellow);
    char label[2]={char('1'+lane),0}; text(2,y,label,lane==f.selected?white:dim);
    sprite(plant,8,12,y,1,f.game.hurtFlash[lane]?red:0);
    if(f.game.shotFlash[lane]) canvas.fillRect(21,y+2,2,3,yellow);
  }
  for(const auto& enemy:f.game.enemies) if(enemy.active) {
    const int x=enemy.x/256,y=14+enemy.lane*9;
    const char* const* image=enemy.kind==Kind::Helmet?helmet:(enemy.kind==Kind::Runner?runner:walker);
    sprite(image,8,x,y,1,enemy.flash?white:0);
    // Two leg poses; freezing and pausing retain the current animation phase.
    if((f.game.time/(enemy.kind==Kind::Runner?100:180))%2) {
      canvas.fillRect(x+1,y+7,6,1,enemy.lane%2?grassA:grassB);
      canvas.fillRect(x+1,y+7,2,1,0x2926); canvas.fillRect(x+5,y+7,2,1,0x2926);
    }
    if(enemy.kind==Kind::Helmet) canvas.drawFastHLine(x+2,y,enemy.hp+2,yellow);
    if(f.game.time<f.game.freezeUntil) canvas.drawRect(x-1,y-1,10,9,cyan);
  }
  for(const auto& bean:f.game.beans) if(bean.active) {
    const int x=bean.x/256,y=18+bean.lane*9;
    canvas.fillCircle(x,y,1,leaf); canvas.drawFastHLine(x-2,y,1,yellow);
  }
  for(const auto& b:f.game.bursts) if(b.life) {
    const int y=18+b.lane*9,r=b.big?4:2;
    const uint16_t c=b.life%2?yellow:white;
    canvas.drawFastHLine(b.x-r,y,2*r+1,c); canvas.drawFastVLine(b.x,y-r,2*r+1,c);
    if(b.big) { canvas.fillRect(b.x-3,y-3,1,1,leaf); canvas.fillRect(b.x+3,y+3,1,1,leaf); }
  }
  header(f);
}
void footer(const Frame& f) {
  canvas.fillRect(0,70,160,10,black); canvas.drawFastHLine(0,70,160,line);
  if(f.inputStalled) { text(2,72,"INPUT WAIT",red); return; }
  if(f.notice[0]) { text(2,72,f.notice,yellow); return; }
  if(!f.touchReady) { text(2,72,"TOUCH!",red); }
  else {
    // Use freeze and bomb icons to preserve space on the small display.
    canvas.drawFastHLine(2,75,7,cyan); canvas.drawFastVLine(5,72,7,cyan);
    canvas.drawLine(3,73,7,77,cyan); canvas.drawLine(3,77,7,73,cyan);
    const int recharge=f.game.time>=f.game.freezeReadyAt?14:14-int((f.game.freezeReadyAt-f.game.time)*14/8000);
    canvas.drawFastHLine(12,75,14,line); canvas.drawFastHLine(12,75,recharge,cyan);
    canvas.fillCircle(37,75,3,yellow); canvas.fillRect(37,71,2,2,red);
    char label[4]; snprintf(label,sizeof(label),"%d",f.game.bombs); text(44,72,label,white);
  }
  if(f.game.combo>1) { char label[8]; snprintf(label,sizeof(label),"x%d",f.game.combo); text(70,72,label,yellow); }
  char label[12]; snprintf(label,sizeof(label),"H%05lu",(unsigned long)f.game.best); right(158,72,label,dim);
}
void band(int y,int h) { canvas.fillRect(25,y,135,h,0x1082); }
void overlay(const Frame& f) {
  if(f.game.mode==Mode::Ready) {
    band(27,30); centered(30,"READY",white,2); centered(49,difficultyNames[f.config.difficulty],yellow);
  } else if(f.game.mode==Mode::Paused) { band(30,24); centered(35,"PAUSED",yellow,2); }
  else if(f.game.mode==Mode::Over) {
    band(25,34); centered(27,"GAME OVER",red,1); centered(43,"AGAIN",white,2);
  } else if(f.game.time<f.game.waveBannerUntil) {
    char label[16]; snprintf(label,sizeof(label),"WAVE %02d",f.game.wave);
    canvas.fillRect(48,29,86,13,0x1082); text(53,32,label,yellow);
  }
}
void menu(const Frame& f) {
  canvas.fillRect(0,13,160,57,black);
  const int first=min(int(f.option),2);
  for(int row=0;row<3;++row) {
    const int option=first+row,y=22+row*15;
    if(option==f.option) canvas.fillRect(0,y-3,2,13,yellow);
    text(5,y,menuNames[option],option==f.option?white:dim);
    const char* value="GO";
    if(option==0) value=difficultyNames[f.config.difficulty];
    if(option==1) value=f.config.sound?"ON":"OFF";
    if(option==2) value=f.config.led?"ON":"OFF";
    if(option>=3&&f.confirm&&option==f.option) value="CONFIRM";
    right(158,y,value,option==f.option?yellow:dim);
  }
}
void standby(const Frame& f) {
  canvas.fillScreen(black); centered(7,"PEA DEFENSE",leaf);
  const uint16_t c=f.full?leaf:white;
  canvas.drawRect(40,25,76,30,c); canvas.fillRect(116,34,4,12,c);
  const int solid=f.batteryPercent*3/100,animation=(f.now/1000)%4;
  for(int i=0;i<3;++i) if(f.full||i<solid||(f.charging&&i<solid+animation)) canvas.fillRect(44+i*23,29,20,22,c);
  centered(65,f.inputStalled?"INPUT WAIT":(f.full?"FULL":(f.charging?"CHARGING":"STANDBY")),f.inputStalled?red:dim);
}
void boot(const Frame& f) {
  canvas.fillScreen(black); centered(8,"PEA DEFENSE",white,2);
  sprite(plant,8,15,33,3); sprite(walker,8,112,33,3);
  canvas.fillCircle(53+(f.now%1000)*45/1000,42,3,leaf);
  centered(66,"FINGERSONIC",dim);
}
void draw(const Frame& f) {
  if(f.booting) { boot(f); return; }
  if(!f.active) { standby(f); return; }
  board(f); if(f.menu) menu(f); else overlay(f); footer(f);
}
#ifndef PEA_HOST_TEST
class Panel:public Adafruit_ST7735 {
 public:
  Panel():Adafruit_ST7735(hw::tftCs,hw::tftDc,hw::tftReset) {}
  void offset() { setColRowStart(24,0); }
};
Panel panel;
uint16_t previous[160*80],strip[160*8]; bool first=true;
void flush() {
  const uint16_t* pixels=canvas.getBuffer();
  for(int y=0;y<80;y+=8) {
    int left=160,rightEdge=-1;
    for(int row=y;row<y+8;++row) for(int x=0;x<160;++x)
      if(first||pixels[row*160+x]!=previous[row*160+x]) { left=min(left,x); rightEdge=max(rightEdge,x); }
    if(rightEdge<left) continue;
    const int width=rightEdge-left+1;
    for(int row=0;row<8;++row) memcpy(strip+row*width,pixels+(y+row)*160+left,width*2);
    panel.startWrite(); panel.setAddrWindow(left,y,width,8); panel.writePixels(strip,width*8); panel.endWrite();
    for(int row=0;row<8;++row) memcpy(previous+(y+row)*160+left,pixels+(y+row)*160+left,width*2);
    taskYIELD();
  }
  first=false;
}
void task(void*) {
  pinMode(hw::tftBacklight,OUTPUT); digitalWrite(hw::tftBacklight,LOW);
  SPI.begin(hw::tftClock,-1,hw::tftMosi,hw::tftCs);
  panel.initR(INITR_MINI160x80); panel.offset(); panel.setRotation(3); panel.setSPISpeed(40000000);
  const uint8_t madctl=ST77XX_MADCTL_MX|ST77XX_MADCTL_MV|ST7735_MADCTL_BGR;
  panel.sendCommand(ST77XX_MADCTL,&madctl,1); panel.invertDisplay(false); panel.fillScreen(black);
  digitalWrite(hw::tftBacklight,HIGH);
  Frame frame;
  for(;;) {
    if(runtimeFrame(frame)) { draw(frame); flush(); }
    vTaskDelay(pdMS_TO_TICKS(33));
  }
}
#endif
}
#ifndef PEA_HOST_TEST
void displayBegin() {
  if(!canvas.getBuffer()) { Serial.println("[PEA] Canvas allocation failed"); return; }
  if(xTaskCreatePinnedToCore(task,"pea_display",4096,nullptr,1,nullptr,1)!=pdPASS) Serial.println("[PEA] Display task failed");
}
#else
extern "C" __declspec(dllexport) const uint16_t* pea_preview(int scene) {
  Frame f{}; f.active=true; f.booting=false; f.touchReady=true; f.audioReady=true; f.batteryPercent=77;
  f.game.mode=Mode::Playing; f.game.score=1230; f.game.best=3450; f.game.wave=5; f.game.time=1400;
  for(int l=0;l<6;++l) {
    f.game.enemies[l]={uint16_t((85+l*9)*256),40,uint8_t(l),uint8_t(l%3==2?3:1),0,Kind(l%3),true};
    f.game.beans[l]={uint16_t((40+l*5)*256),uint8_t(l),true};
  }
  f.selected=2;
  if(scene==1) {
    f.game.mode=Mode::Ready; f.game.wave=1; f.game.score=0;
    for(auto& enemy:f.game.enemies) enemy.active=false;
    for(auto& bean:f.game.beans) bean.active=false;
  }
  if(scene==2) f.game.mode=Mode::Paused;
  if(scene==3) { f.game.mode=Mode::Over; f.game.lives=0; }
  if(scene==4) { f.menu=true; f.option=1; }
  if(scene==5) { f.active=false; f.charging=true; f.now=2000; }
  if(scene==6) { f.booting=true; f.now=600; }
  if(scene==7) f.game.freezeUntil=4000;
  draw(f); return canvas.getBuffer();
}
extern "C" __declspec(dllexport) const uint16_t* pea_motion(int ticks,int paused) {
  static Game demo;
  demo.init(1); demo.start(); demo.debugState().enemies[0]={uint16_t(90*256),40,0,1,0,Kind::Walker,true};
  demo.fire(0); if(paused) demo.pause();
  for(int i=0;i<ticks;++i) demo.update(20);
  Frame f{}; f.active=true; f.booting=false; f.touchReady=true; f.game=demo.state();
  draw(f); return canvas.getBuffer();
}
#endif
}

#pragma once
#include <stdint.h>
namespace pea {
// Original eight-row pixel sprites; dots are transparent, within each nine-pixel lane.
static const char* const plant[8]={
  "..gggg....",".gGGGGg...","gGGwGGGooo","gGGGGGGooo",".ggGGgg...","...gg.....",".llggll...","ll.ggl.ll."
};
static const char* const walker[8]={
  "..sssss.","..swksk.","..sssss.","...ss...",".jjjjjj.",".sjjsjs.","..pppp..","..p..p.."
};
static const char* const runner[8]={
  "..sssss.","..swksk.","..sssss.","...ss...",".vvvvvv.",".svvs.s.","..pppp..",".p....p."
};
static const char* const helmet[8]={
  "..rrrrr.",".rrrrrrr","..swksk.","..sssss.",".jjjjjj.",".sjjsjs.","..pppp..","..p..p.."
};
static const char* const heart[5]={".r.r.","rrrrr","rrrrr",".rrr.","..r.."};
}

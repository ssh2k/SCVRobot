#pragma once
#include <cstdint>
#include <cstdlib>
#include "PathRunner.h"

// grid[y][x]: 0 = 통로, 1 = 장애물 (5x5 고정)
struct AStarResult {
  bool ok;
  uint16_t n; // out 경로 길이
};

// 성공 시 out[0]=(sx,sy) ... out[n-1]=(gx,gy)
// 실패 시 ok=false, n=0
AStarResult planAstar5x5(
  const bool grid[5][5],
  int sx, int sy, int gx, int gy,
  PathRunner::Node* out, uint16_t maxOut
);

// ---- 경로 유효성 검사 (템플릿: x,y 멤버만 있으면 어느 타입이나 허용) ----
template <typename NodeT>
static inline bool validatePath5x5(
  const bool grid[5][5],
  const NodeT* p, uint16_t n
){
  auto inBounds = [](int x,int y){ return (0<=x && x<5 && 0<=y && y<5); };
  if (!p || n==0) return false;
  for (uint16_t i=0;i<n;++i){
    int x=p[i].x, y=p[i].y;
    if (!inBounds(x,y)) return false;
    if (grid[y][x])     return false; // 장애물 위
    if (i+1<n){
      int dx = p[i+1].x - x;
      int dy = p[i+1].y - y;
      if (abs(dx)+abs(dy) != 1) return false; // 비인접
    }
  }
  return true;
}

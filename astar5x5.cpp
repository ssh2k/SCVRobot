#include "astar5x5.h"
#include <cstdlib>

static inline int manhattan(int x1,int y1,int x2,int y2){
  return abs(x1-x2)+abs(y1-y2);
}
static inline bool inBounds(int x,int y){ return (0<=x && x<5 && 0<=y && y<5); }

AStarResult planAstar5x5(
  const bool grid[5][5],
  int sx, int sy, int gx, int gy,
  PathRunner::Node* out, uint16_t maxOut
){
  AStarResult res{false, 0};
  if (!inBounds(sx,sy) || !inBounds(gx,gy)) return res;
  if (grid[sy][sx] || grid[gy][gx]) return res; // 시작/목표 막힘

  const int N = 25;
  auto idx = [](int x,int y){ return y*5 + x; };

  bool closed[25]{}; bool open[25]{};
  int  cameX[25]; int  cameY[25];
  int  g[25]; int  f[25];

  for (int i=0;i<N;++i){ closed[i]=false; open[i]=false; g[i]=1e9; f[i]=1e9; }

  const int sidx = idx(sx,sy);
  g[sidx] = 0;
  f[sidx] = manhattan(sx,sy,gx,gy);
  cameX[sidx] = -1; cameY[sidx] = -1;
  open[sidx] = true;

  auto pushOrImprove = [&](int cx,int cy,int nx,int ny){
    if (!inBounds(nx,ny)) return;
    if (grid[ny][nx]) return;
    int ci = idx(cx,cy), ni = idx(nx,ny);
    if (closed[ni]) return;
    int tentative_g = g[ci] + 1; // 한 칸 비용=1
    if (!open[ni] || tentative_g < g[ni]){
      cameX[ni] = cx; cameY[ni] = cy;
      g[ni] = tentative_g;
      f[ni] = tentative_g + manhattan(nx,ny,gx,gy);
      open[ni] = true;
    }
  };

  const int goalIdx = idx(gx,gy);
  bool found = false;

  while (true) {
    int cur = -1; int bestF = 1e9;
    for (int i=0;i<N;++i){
      if (open[i] && f[i] < bestF){ bestF = f[i]; cur = i; }
    }
    if (cur < 0) break;
    open[cur] = false; closed[cur] = true;
    if (cur == goalIdx){ found = true; break; }

    int cx = cur % 5, cy = cur / 5;
    // 4-이웃 (y+1 = 위)
    pushOrImprove(cx,cy, cx+1,cy); // RIGHT
    pushOrImprove(cx,cy, cx-1,cy); // LEFT
    pushOrImprove(cx,cy, cx,cy+1); // UP
    pushOrImprove(cx,cy, cx,cy-1); // DOWN
  }

  if (!found) return res;

  // 역추적
  PathRunner::Node tmp[25];
  int n = 0;
  for (int cur = goalIdx; cur >= 0 && n < 25; ){
    int x = cur % 5, y = cur / 5;
    tmp[n++] = {x,y};
    int px = cameX[cur], py = cameY[cur];
    if (px < 0) break; // 시작점
    cur = py*5 + px;
  }

  if ((uint16_t)n > maxOut) return res;
  for (int i=0;i<n;++i) out[i] = tmp[n-1-i];

  res.ok = true;
  res.n  = (uint16_t)n;
  return res;
}

bool validatePath5x5(const bool grid[5][5],
                     const PathRunner::Node* p, uint16_t n)
{
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

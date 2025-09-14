// PathRunner.h
#ifndef PATH_RUNNER_H
#define PATH_RUNNER_H

#include <cstdint>
#include "gridMove.h"

class PathRunner {
public:
  struct Node { int x; int y; };
  static constexpr uint16_t MAX_POINTS = 64;

  explicit PathRunner(gridMove& mover, uint16_t dwell_ms = 150);

  void loadPath(const Node* points, uint16_t count);
  void start();
  void update();

  bool isFinished() const;
  uint16_t pathLength() const;
  uint16_t segmentIndex() const;

  void forceStop(); // 비상 정지 함수
  void setDwellMs(uint16_t ms);

private:
  gridMove&  mover_;
  Node       path_[MAX_POINTS]{};
  uint16_t   n_{0};
  uint16_t   i_{0};
  bool       started_{false};
  bool       inDwell_{false};
  uint32_t   dwellEndMs_{0};
  uint16_t   dwellMs_{150};
  bool       wasBusy_{false};
};

#endif // PATH_RUNNER_H


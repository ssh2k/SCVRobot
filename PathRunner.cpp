// PathRunner.cpp
#include "PathRunner.h"
#include <Arduino.h>

PathRunner::PathRunner(gridMove& mover, uint16_t dwell_ms)
: mover_(mover), dwellMs_(dwell_ms) {}

void PathRunner::loadPath(const Node* points, uint16_t count) {
  if (!points || count == 0 || count > MAX_POINTS) {
    n_ = 0;
  } else {
    for (uint16_t k = 0; k < count; ++k) {
      path_[k] = points[k];
    }
    n_ = count;
  }
  // 상태 초기화
  i_ = 0;
  started_ = false;
  inDwell_ = false;
  wasBusy_ = false;
}

void PathRunner::start() {
  if (n_ > 0) {
    started_ = true;
    i_ = 0;
    inDwell_ = false;
    wasBusy_ = false;
  }
}

void PathRunner::update() {
  mover_.update(); // 하위 모듈 먼저 업데이트

  if (!started_ || isFinished()) {
    return;
  }

  const bool busyNow = !mover_.isIdle();

  // 이동 세그먼트가 방금 끝났는지 확인
  if (wasBusy_ && !busyNow) {
    // 마지막 세그먼트가 아니라면, 칸 사이 정지(dwell) 시작
    if (i_ < n_ - 1) {
      inDwell_ = true;
      dwellEndMs_ = millis() + dwellMs_;
    }
  }
  wasBusy_ = busyNow;

  // Dwell 상태 처리
  if (inDwell_) {
    if ((int32_t)(millis() - dwellEndMs_) >= 0) {
      inDwell_ = false; // Dwell 시간 종료
    } else {
      return; // 아직 Dwell 중
    }
  }

  // 다음 이동 세그먼트 시작
  if (!mover_.isIdle() || inDwell_) {
    return; // 아직 이전 동작이 진행 중이거나 Dwell 중이면 대기
  }

  if (i_ < n_ - 1) {
    const Node& curr = path_[i_];
    const Node& next = path_[i_ + 1];
    mover_.stepTo(curr.x, curr.y, next.x, next.y);
    i_++; // 다음 세그먼트로 인덱스 이동
  }
}

void PathRunner::forceStop() {
    // 하위 모듈 즉시 정지
    mover_.stopMotors();
    while(mover_.hasQueued()) {
      mover_.popQueued();
    }

    // PathRunner 상태 초기화
    n_ = 0;
    i_ = 0;
    started_ = false;
    inDwell_ = false;
    wasBusy_ = false;
}

bool PathRunner::isFinished() const {
  return !started_ || (i_ >= n_ - 1 && mover_.isIdle() && !inDwell_);
}

uint16_t PathRunner::pathLength()  const { return n_; }
uint16_t PathRunner::segmentIndex() const { return i_; }
void PathRunner::setDwellMs(uint16_t ms) { dwellMs_ = ms; }


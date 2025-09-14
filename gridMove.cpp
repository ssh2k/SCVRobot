// gridMove.cpp
#include "gridMove.h"
#include <Arduino.h>

// ───────── 핀 매핑 ─────────
static constexpr uint8_t RIGHT_DIR_PIN = 2;
static constexpr uint8_t RIGHT_PWM_PIN = 3;
static constexpr uint8_t LEFT_DIR_PIN  = 4;
static constexpr uint8_t LEFT_PWM_PIN  = 5;

// ───────── DIR 논리 ─────────
// DIR=LOW → Forward, DIR=HIGH → Backward (요구사항)
static constexpr bool RIGHT_DIR_FORWARD_HIGH = false;
static constexpr bool LEFT_DIR_FORWARD_HIGH  = false;

gridMove::gridMove() {
  pinMode(RIGHT_DIR_PIN, OUTPUT);
  pinMode(RIGHT_PWM_PIN, OUTPUT);
  pinMode(LEFT_DIR_PIN,  OUTPUT);
  pinMode(LEFT_PWM_PIN,  OUTPUT);
  stopMotors();
}

// ----------------- Public API -----------------
void gridMove::stepTo(int currX, int currY, int nextX, int nextY) {
  // 이미 동작 중이면 무시(또는 큐만 쌓고 싶다면 정책 변경 가능)
  if (!isIdle()) return;

  const int dx = nextX - currX;
  const int dy = nextY - currY;

  Direction targetDir = currentDirection;
  if      (dx == 1 && dy == 0)  targetDir = Direction::RIGHT;
  else if (dx == -1 && dy == 0) targetDir = Direction::LEFT;
  else if (dx == 0 && dy == 1)  targetDir = Direction::UP;
  else if (dx == 0 && dy == -1) targetDir = Direction::DOWN;
  else {
    // 인접 아님 → 무시
    return;
  }

  // 1) 최소회전 예약  2) 직진 예약
  scheduleRotateTo(targetDir);
  enqueue(Action::Forward);

  // 즉시 첫 액션 시작
  if (action_ == Action::Idle && hasQueued()) {
    startAction(popQueued());
  }
}

void gridMove::update() {
  if (action_ == Action::Idle) {
    // 큐가 남아 있으면 다음 시작
    if (hasQueued()) startAction(popQueued());
    return;
  }

  // 시간 도달 시 액션 종료
  if ((int32_t)(millis() - actionEndMs_) >= 0) {
    finishAction();
    // 다음 액션이 있으면 즉시 시작
    if (hasQueued()) startAction(popQueued());
  }
}

bool gridMove::isIdle() const { return action_ == Action::Idle && qlen_ == 0; }
gridMove::Action gridMove::currentAction() const { return action_; }
gridMove::Direction gridMove::getDirection() const { return currentDirection; }

void gridMove::startForward()   { enqueue(Action::Forward);   if (action_==Action::Idle) startAction(popQueued()); }
void gridMove::startBackward()  { enqueue(Action::Backward);  if (action_==Action::Idle) startAction(popQueued()); } // ★ 신규
void gridMove::startRotateCW()  { enqueue(Action::RotateCW);  if (action_==Action::Idle) startAction(popQueued()); }
void gridMove::startRotateCCW() { enqueue(Action::RotateCCW); if (action_==Action::Idle) startAction(popQueued()); }

void gridMove::setForwardDurationMs(uint32_t ms){ forwardDurationMs = ms; }
void gridMove::setBackwardDurationMs(uint32_t ms){ backwardDurationMs = ms; } // ★ 신규
void gridMove::setRotateDurationMs(uint32_t ms) { rotateDurationMs  = ms; }

void gridMove::setForwardPWMs(int l, int r)     { forwardLeftPWM=l;  forwardRightPWM=r; }
void gridMove::setBackwardPWMs(int l, int r)    { backwardLeftPWM=l; backwardRightPWM=r; } // ★ 신규
void gridMove::setRotatePWMs(int l, int r)      { rotateLeftPWM=l;   rotateRightPWM=r;  }

// ----------------- Internal helpers -----------------
void gridMove::scheduleRotateTo(Direction target) {
  if (target == currentDirection) {
    // 회전 불필요
    return;
  }
  // 최소 회전: 현재→목표
  switch (currentDirection) {
    case Direction::UP:
      if      (target == Direction::RIGHT) enqueue(Action::RotateCW);
      else if (target == Direction::LEFT)  enqueue(Action::RotateCCW);
      else if (target == Direction::DOWN)  { enqueue(Action::RotateCW); enqueue(Action::RotateCW); }
      break;
    case Direction::DOWN:
      if      (target == Direction::LEFT)  enqueue(Action::RotateCW);
      else if (target == Direction::RIGHT) enqueue(Action::RotateCCW);
      else if (target == Direction::UP)    { enqueue(Action::RotateCW); enqueue(Action::RotateCW); }
      break;
    case Direction::LEFT:
      if      (target == Direction::UP)    enqueue(Action::RotateCW);
      else if (target == Direction::DOWN)  enqueue(Action::RotateCCW);
      else if (target == Direction::RIGHT) { enqueue(Action::RotateCW); enqueue(Action::RotateCW); }
      break;
    case Direction::RIGHT:
      if      (target == Direction::DOWN)  enqueue(Action::RotateCW);
      else if (target == Direction::UP)    enqueue(Action::RotateCCW);
      else if (target == Direction::LEFT)  { enqueue(Action::RotateCW); enqueue(Action::RotateCW); }
      break;
  }
  // 실제 currentDirection 갱신은 회전 완료 시점(finishAction)에서 수행
}

void gridMove::startAction(Action a) {
  action_ = a;
  const uint32_t now = millis();

  switch (a) {
    case Action::Forward:
      driveMotors(forwardLeftPWM, forwardRightPWM);
      actionEndMs_ = now + forwardDurationMs;
      break;

    case Action::Backward: // ★ 신규: 회전 없이 바로 후진
      driveMotors(backwardLeftPWM, backwardRightPWM);
      actionEndMs_ = now + backwardDurationMs; // 기본은 forward와 동일
      break;

    case Action::RotateCW:
      driveMotors(rotateLeftPWM, rotateRightPWM);
      actionEndMs_ = now + rotateDurationMs;
      break;

    case Action::RotateCCW:
      driveMotors(-rotateLeftPWM, -rotateRightPWM);
      actionEndMs_ = now + rotateDurationMs;
      break;

    case Action::Idle:
    default:
      stopMotors();
      actionEndMs_ = now;
      break;
  }
}

void gridMove::finishAction() {
  // 액션 종료 처리
  stopMotors();

  // 방향 갱신은 회전 완료 시점에만
  if (action_ == Action::RotateCW || action_ == Action::RotateCCW) {
    auto cw = [](Direction d)->Direction {
      switch (d) {
        case Direction::UP:    return Direction::RIGHT;
        case Direction::RIGHT: return Direction::DOWN;
        case Direction::DOWN:  return Direction::LEFT;
        case Direction::LEFT:  return Direction::UP;
      }
      return Direction::UP;
    };
    auto ccw = [](Direction d)->Direction {
      switch (d) {
        case Direction::UP:    return Direction::LEFT;
        case Direction::LEFT:  return Direction::DOWN;
        case Direction::DOWN:  return Direction::RIGHT;
        case Direction::RIGHT: return Direction::UP;
      }
      return Direction::UP;
    };
    currentDirection = (action_ == Action::RotateCW) ? cw(currentDirection)
                                                     : ccw(currentDirection);
  }

  action_ = Action::Idle;
}

void gridMove::enqueue(Action a) {
  if (qlen_ < 3) {
    q_[qlen_++] = a;
  }
}

bool gridMove::hasQueued() const { return qlen_ > 0; }

gridMove::Action gridMove::popQueued() {
  if (qlen_ == 0) return Action::Idle;
  
  Action a = q_[0]; // 첫 번째 동작을 변수에 저장
  qlen_--; // 큐 길이를 먼저 줄임

  // 남은 항목들을 앞으로 한 칸씩 이동
  for (uint8_t i = 0; i < qlen_; i++) {
    q_[i] = q_[i + 1];
  }

  return a; // 저장해둔 첫 번째 동작을 반환
}

// ----------------- Low-level motor -----------------
void gridMove::driveMotors(int leftPWM, int rightPWM) {
  // 좌측
  const bool leftForward  = (leftPWM >= 0);
  const uint8_t leftDuty  = (uint8_t)constrain(abs(leftPWM), 0, 255);
  const bool leftDirLevel = LEFT_DIR_FORWARD_HIGH ? leftForward : !leftForward;
  digitalWrite(LEFT_DIR_PIN, leftDirLevel);
  analogWrite(LEFT_PWM_PIN, leftDuty);

  // 우측
  const bool rightForward  = (rightPWM >= 0);
  const uint8_t rightDuty  = (uint8_t)constrain(abs(rightPWM), 0, 255);
  const bool rightDirLevel = RIGHT_DIR_FORWARD_HIGH ? rightForward : !rightForward;
  digitalWrite(RIGHT_DIR_PIN, rightDirLevel);
  analogWrite(RIGHT_PWM_PIN, rightDuty);
}

void gridMove::stopMotors() {
  analogWrite(LEFT_PWM_PIN,  0);
  analogWrite(RIGHT_PWM_PIN, 0);
}

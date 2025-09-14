// gridMove.h
#ifndef GRIDMOVE_H
#define GRIDMOVE_H

#include <cstdint>

class gridMove {
public:
    enum class Direction { UP, DOWN, LEFT, RIGHT };
    enum class Action { Idle, RotateCW, RotateCCW, Forward, Backward };

    gridMove();

    void stepTo(int currX, int currY, int nextX, int nextY);
    void update();

    bool isIdle() const;
    Action currentAction() const;
    Direction getDirection() const;

    void startForward();
    void startBackward();
    void startRotateCW();
    void startRotateCCW();

    void setForwardDurationMs(uint32_t ms);
    void setBackwardDurationMs(uint32_t ms);
    void setRotateDurationMs(uint32_t ms);
    void setForwardPWMs(int leftPWM, int rightPWM);
    void setBackwardPWMs(int leftPWM, int rightPWM);
    void setRotatePWMs(int leftPWM, int rightPWM);
    
    // PathRunner가 접근할 수 있도록 public으로 변경된 함수들
    void stopMotors();
    bool hasQueued() const;
    Action popQueued();

private:
    void scheduleRotateTo(Direction target);
    void startAction(Action a);
    void finishAction();
    void driveMotors(int leftPWM, int rightPWM);
    void enqueue(Action a);

private:
    Direction currentDirection{Direction::RIGHT};
    Action    action_{Action::Idle};
    uint32_t  actionEndMs_{0};

    // --- [BUG FIX] 180도 회전(2) + 전진(1)을 위해 큐 크기를 3으로 늘림 ---
    Action    q_[3]{Action::Idle, Action::Idle, Action::Idle};
    uint8_t   qlen_{0};
    
    uint32_t forwardDurationMs{5373};
    uint32_t backwardDurationMs{5373};
    uint32_t rotateDurationMs{1970};
    int forwardLeftPWM{73};
    int forwardRightPWM{63};
    int backwardLeftPWM{-73};
    int backwardRightPWM{-63};
    int rotateLeftPWM{73};
    int rotateRightPWM{-63};
};

#endif // GRIDMOVE_H

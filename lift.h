// lift.h
#ifndef LIFT_H
#define LIFT_H

#include <Arduino.h>

// 전역 상수 선언
extern const float LIFT_MIN_HEIGHT_CM;
extern const float LIFT_MAX_HEIGHT_CM;

class Lift {
public:
    // 공개적으로 사용할 상태와 함수들
    enum class LiftState { IDLE, MOVING_UP, MOVING_DOWN };
    float height_cm;

    Lift();
    void begin();
    void update(); // 메인 루프에서 계속 호출될 비블로킹 업데이트 함수
    void stop();
    void upFor(unsigned long ms);
    void downFor(unsigned long ms);
    LiftState getState() const;

private:
    // 클래스 내부에서만 사용할 상태 변수와 함수들
    LiftState _state = LiftState::IDLE;
    unsigned long _actionEndMs = 0;
    bool _powerOn = false;

    void setPower(bool on);
    long readUltrasonicCM();
    void stepPulse(bool dir);
    void updateHeight(); // 높이 측정 함수 선언 추가
};

#endif // LIFT_H


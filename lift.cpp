// lift.cpp
#include "lift.h"

// ----- 핀 매핑 -----
static constexpr int PIN_DIR   = 10;
static constexpr int PIN_STEP  = 11;
static constexpr int PIN_EN    = 12;
static constexpr int PIN_RELAY = 13;
static constexpr int PIN_TRIG  = 8;
static constexpr int PIN_ECHO  = 9;

// ----- 타이밍 파라미터 -----
static constexpr unsigned STEP_HIGH_US      = 500;
static constexpr unsigned STEP_LOW_US       = 500;
static constexpr unsigned DIR_SETUP_US      = 2;
static constexpr unsigned ULTRASONIC_TOUT_US= 30000;

// ----- 전역 상수 정의 -----
const float LIFT_MIN_HEIGHT_CM = 1.0f;
const float LIFT_MAX_HEIGHT_CM = 4.0f;

// ----- 생성자 -----
Lift::Lift() : height_cm(0.0f) {}

// ----- 초기화 -----
void Lift::begin() {
    pinMode(PIN_DIR, OUTPUT);
    pinMode(PIN_STEP, OUTPUT);
    pinMode(PIN_EN, OUTPUT);
    pinMode(PIN_RELAY, OUTPUT);
    pinMode(PIN_TRIG, OUTPUT);
    pinMode(PIN_ECHO, INPUT);

    digitalWrite(PIN_TRIG, LOW);
    _powerOn = false;
    digitalWrite(PIN_EN, HIGH);
    digitalWrite(PIN_RELAY, LOW);

    updateHeight(); // 시작 시 높이 갱신
}

// ----- 릴레이/EN 제어 -----
void Lift::setPower(bool on) {
    if (_powerOn == on) return;
    if (on) {
        digitalWrite(PIN_RELAY, HIGH);
        digitalWrite(PIN_EN, LOW);
    } else {
        digitalWrite(PIN_EN, HIGH);
        digitalWrite(PIN_RELAY, LOW);
    }
    _powerOn = on;
}

// ----- 초음파 센서 읽기 -----
long Lift::readUltrasonicCM() {
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);
    unsigned long duration = pulseIn(PIN_ECHO, HIGH, ULTRASONIC_TOUT_US);
    return static_cast<long>(duration * 0.034f / 2.0f);
}

// ----- private 멤버 함수 -----
void Lift::updateHeight() {
    height_cm = static_cast<float>(readUltrasonicCM());
}

// ----- 한 스텝 펄스 -----
void Lift::stepPulse(bool dir) {
    digitalWrite(PIN_DIR, dir ? HIGH : LOW);
    delayMicroseconds(DIR_SETUP_US);
    digitalWrite(PIN_STEP, HIGH);
    delayMicroseconds(STEP_HIGH_US);
    digitalWrite(PIN_STEP, LOW);
    delayMicroseconds(STEP_LOW_US);
}

// ----- 비블로킹 시간 제어 함수 -----
void Lift::upFor(unsigned long ms) {
    setPower(true);
    _state = LiftState::MOVING_UP;
    _actionEndMs = millis() + ms;
}

void Lift::downFor(unsigned long ms) {
    setPower(true);
    _state = LiftState::MOVING_DOWN;
    _actionEndMs = millis() + ms;
}

// ----- 정지 함수 -----
void Lift::stop() {
    setPower(false);
    _state = LiftState::IDLE;
}

// ----- 비블로킹 update 함수 -----
void Lift::update() {
    if (_state == LiftState::IDLE) {
        return;
    }

    updateHeight(); // 내부에서 private 함수 호출
    bool timeIsUp = (millis() >= _actionEndMs);
    bool limitReached = (_state == LiftState::MOVING_UP   && height_cm >= LIFT_MAX_HEIGHT_CM) ||
                        (_state == LiftState::MOVING_DOWN && height_cm <= LIFT_MIN_HEIGHT_CM);

    if (timeIsUp || limitReached) {
        stop();
        return;
    }

    if (_state == LiftState::MOVING_UP) {
        stepPulse(true);
    } else if (_state == LiftState::MOVING_DOWN) {
        stepPulse(false);
    }
}

Lift::LiftState Lift::getState() const {
    return _state;
}


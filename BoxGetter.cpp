// BoxGetter.cpp
#include "BoxGetter.h"
#include <Arduino.h>

BoxGetter::BoxGetter(gridMove& mover, Lift& lift)
: mover_(mover), lift_(lift) {}

void BoxGetter::startGetBox() {
    state_ = State::Orient;
    plannedTurnQueued_ = false;
    forwardIssued_ = false;
    backwardIssued_ = false;
}

void BoxGetter::update() {
    // 모든 상태에서 mover_와 lift_는 계속 업데이트 되어야 합니다.
    mover_.update();
    lift_.update();

    switch (state_) {
    case State::Idle:
        break;

    case State::Orient:
        stepOrient_();
        break;

    case State::Forward:
        if (!forwardIssued_) {
            mover_.startForward();
            forwardIssued_ = true;
        }
        if (mover_.isIdle()) {
            state_ = State::Raise;
        }
        break;

    case State::Raise:
        // ======================================================================
        // ▼▼▼▼▼ [BUG FIX] 비블로킹 Lift 클래스 사용하도록 로직 변경 ▼▼▼▼▼
        // ======================================================================
        if (lift_.getState() == Lift::LiftState::IDLE) {
            // 아직 올리기 시작 안했으면 시작
            lift_.upFor(2000); // 2초간 올리기 시작
        }
        
        // 올리는 동작이 끝났으면 (IDLE 상태로 돌아왔으면) 다음으로
        if (lift_.getState() == Lift::LiftState::IDLE && forwardIssued_) {
             state_ = State::Backward;
        }
        // forwardIssued_를 플래그로 재활용하여 Raise 시작을 한 번만 하도록 함
        if(lift_.getState() == Lift::LiftState::MOVING_UP) forwardIssued_ = true;
        break;

    case State::Backward:
        if (!backwardIssued_) {
            mover_.startBackward();
            backwardIssued_ = true;
        }
        if (mover_.isIdle()) {
            state_ = State::Lower;
        }
        break;

    case State::Lower:
        // ======================================================================
        // ▼▼▼▼▼ [BUG FIX] 비블로킹 Lift 클래스 사용하도록 로직 변경 ▼▼▼▼▼
        // ======================================================================
        if (lift_.getState() == Lift::LiftState::IDLE) {
            lift_.downFor(2000); // 2초간 내리기 시작
        }

        // 내리는 동작이 끝났으면 (IDLE 상태로 돌아왔으면) 종료
        if (lift_.getState() == Lift::LiftState::IDLE && backwardIssued_) {
            state_ = State::Done;
        }
        if(lift_.getState() == Lift::LiftState::MOVING_DOWN) backwardIssued_ = true;
        break;

    case State::Done:
        break;
    }
}

bool BoxGetter::isBusy() const { return state_ != State::Idle && state_ != State::Done; }
bool BoxGetter::isFinished() const { return state_ == State::Done; }
BoxGetter::State BoxGetter::state() const { return state_; }

void BoxGetter::stepOrient_() {
    if (!plannedTurnQueued_) {
        using D = gridMove::Direction;
        switch (mover_.getDirection()) {
            case D::DOWN:
                break;
            case D::UP:
                mover_.startRotateCW();
                mover_.startRotateCW();
                break;
            case D::LEFT:
                mover_.startRotateCW();
                break;
            case D::RIGHT:
                mover_.startRotateCCW();
                break;
        }
        plannedTurnQueued_ = true;
    }
    
    // mover_.update()는 메인 update()에서 호출되므로 여기서 호출할 필요 없음

    if (mover_.getDirection() == gridMove::Direction::DOWN && mover_.isIdle()) {
        state_ = State::Forward;
    }
}

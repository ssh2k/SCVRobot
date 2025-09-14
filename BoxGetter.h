// BoxGetter.h
#ifndef BOXGETTER_H
#define BOXGETTER_H

#include "gridMove.h"
#include "lift.h"

class BoxGetter {
public:
    enum class State {
        Idle,
        Orient,
        Forward,
        Raise,
        Backward,
        Lower,
        Done
    };

    BoxGetter(gridMove& mover, Lift& lift);

    void startGetBox();  // 시퀀스 시작
    void update();       // 매 loop()마다 호출

    bool isBusy() const;
    bool isFinished() const;
    State state() const;

private:
    void stepOrient_();

    gridMove& mover_;
    Lift& lift_;
    State state_{State::Idle};

    bool plannedTurnQueued_{false};
    bool forwardIssued_{false};
    bool backwardIssued_{false};
};

#endif // BOXGETTER_H

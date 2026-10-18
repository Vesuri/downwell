#ifndef DOWNWELL_CORE_ENCOUNTER_BOUNDARY_H
#define DOWNWELL_CORE_ENCOUNTER_BOUNDARY_H

namespace downwell {
// One frontend session, not a room-local owner. Retry/room changes take place
// inside an update; neither may clear a terminal callback failure. Existing
// event scopes nest inside the body, while recursively updating/drawing the
// whole session is a binding error. No registry/catalogue walk is required.
class EncounterBoundary {
public:
    enum class Phase { idle, startup, update, draw, retired };
    EncounterBoundary()=default;
    EncounterBoundary(const EncounterBoundary&)=delete;
    EncounterBoundary& operator=(const EncounterBoundary&)=delete;

    bool succeeded()const{return good_;}
    Phase phase()const{return phase_;}
    bool fail(){good_=false;return false;}
    bool retire(){
        if(phase_!=Phase::idle&&phase_!=Phase::retired)return fail();
        phase_=Phase::retired;return good_;
    }
    template<class Body> bool start(Body body){
        if(started_)return fail();
        started_=true;return execute(Phase::startup,body);
    }
    template<class Body> bool update(Body body){
        return started_?execute(Phase::update,body):fail();
    }
    template<class Body> bool draw(Body body){
        return started_?execute(Phase::draw,body):fail();
    }
private:
    template<class Body> bool execute(Phase next,Body body){
        if(!good_||phase_!=Phase::idle)return fail();
        phase_=next;
        struct Leave {Phase& phase;~Leave(){phase=Phase::idle;}} leave{phase_};
        const bool okay=body();
        // A nested callback may have failed even when its caller forgot to
        // return that result. Do not turn that terminal failure back to success.
        good_=good_&&okay;return good_;
    }
    Phase phase_=Phase::idle;
    bool started_=false,good_=true;
};
}
#endif

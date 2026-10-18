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
    // Split-loop frontends need a lexical owner across update and Draw without
    // moving hardware staging into game logic. An early return/break cannot
    // silently close a successful frame: completion is explicit.
    class Frame {
    public:
        explicit Frame(EncounterBoundary& boundary):boundary_(boundary){
            if(!boundary_.started_||!boundary_.good_||boundary_.phase_!=Phase::idle){
                boundary_.fail();return;
            }
            owns_=true;boundary_.phase_=Phase::update;
        }
        Frame(const Frame&)=delete;
        Frame& operator=(const Frame&)=delete;
        ~Frame(){
            if(owns_){boundary_.fail();boundary_.phase_=Phase::idle;}
        }
        bool valid()const{return owns_&&boundary_.good_;}
        bool beginDraw(){
            if(!valid()||boundary_.phase_!=Phase::update)return boundary_.fail();
            boundary_.phase_=Phase::draw;return true;
        }
        bool drawing()const{return valid()&&boundary_.phase_==Phase::draw;}
        bool finish(){
            if(!owns_)return boundary_.fail();
            owns_=false;boundary_.phase_=Phase::idle;return boundary_.good_;
        }
        bool fail(){return boundary_.fail();}
    private:
        EncounterBoundary& boundary_;
        bool owns_=false;
    };
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

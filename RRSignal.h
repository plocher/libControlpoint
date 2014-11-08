#ifndef RRSIGNAL_H
#define RRSIGNAL_H
#include <Arduino.h>

class RRSignal {
public:
    enum State     { UNKNOWN, LEFT, RIGHT, ALLSTOP, TIME, ERROR };
    enum Stick     { NONE, FLEET, ER };
    enum Timer     { NOTIMER, RUNNING, EXPIRED };
    
	RRSignal(const char *name)                     { _init(name, NULL); };
    RRSignal(const char *name,  TrackCircuit *t)   { _init(name, t); };
	

    boolean is(State s)               { return (_reported == s); };
    boolean isSafe(State s)           { if (s  == _reported) { return true; }
                                        if (!is(RRSignal::ALLSTOP)) { return false; }
										_safestate = s;
										return true;
                                      };
    boolean isSafe(int n, int r)      { return isSafe(toState(n,r)); }
	void  doSafe(void)				  { set(_safestate); _safestate = UNKNOWN; }
    
    State commanded(void)             { return _commanded;};
    void set(State s)                 { if ((_commanded != RRSignal::ALLSTOP) && (_commanded != s)) { setTime(10, s); } else { _commanded = s; } };
    void set(int n, int r)            { set(toState(n,r)); };
    
	boolean knockdown(void)           { if (_commanded != RRSignal::ALLSTOP) {
											_commanded = RRSignal::ALLSTOP;
											return true;
										}
										return false;
									   }
	
	
	// boolean  knockdown()              { if ((_reported == RRSignal::ALLSTOP) && ((_track) &&_track->isOccupied())) { _commanded = RRSignal::ALLSTOP; return true;} return false; };
	boolean route()					  { return _routeOK; }
	void route(boolean OK)			  { _routeOK = OK; }
	boolean approach()				  { return _approach; }
	void approach(boolean OK)		  { _approach = OK; }
	boolean advancedapproach()		  { return _advancedapproach; }
	void advancedapproach(boolean OK) { _advancedapproach = OK; }
    
    void report(void)                 { _reported = _commanded; }
    State reported(void)              { return _reported;};  // differs when running time
    State toState(int l, int r)       { return (
                                            (((l) == 1) && ((r) == 0)) ? RRSignal::LEFT :
                                            (((l) == 0) && ((r) == 1)) ? RRSignal::RIGHT : 
                                            (((l) == 0) && ((r) == 0)) ? RRSignal::UNKNOWN :   
                                            (((l) == 1) && ((r) == 1)) ? RRSignal::ALLSTOP :   
                                            RRSignal::ERROR
                                        );
                                      }
    
    boolean isRunningTime(void)       { return (_timer != RRSignal::NOTIMER); }
    boolean isExpiredTime(void)       { return (_timer == RRSignal::EXPIRED); }
    void setTime(int seconds, State s){
                                        // Dispatcher is knocking down signal, need to spin up timer before changing signal...
                                            _timestarted = millis();  
                                            _time2end = (seconds *1000); 
                                            _timer = RRSignal::RUNNING;
                                            _nextcommanded = s;
                                            _commanded = TIME; // knock it down now, but don't let the plant change for xxx seconds...
                                      }
    Timer runTime(void)                {
                                        if (isRunningTime()) {
                                            unsigned int timeNow = millis();
                                            if (timeNow - _timestarted > _time2end) { // timer expired
                                                _timer = RRSignal::EXPIRED;
                                            }
                                        } 
                                        if (_timer == RRSignal::EXPIRED) {
                                          _commanded = _nextcommanded;
                                          _timestarted = _time2end = 0;
                                          _timer = RRSignal::NOTIMER;
                                        }  
                                        return _timer;  
                                      }
    
    Stick stick(void)                 { return _stick;};
    void stick(Stick s)               { _stick = s; };
    byte leftindication()             { return ((_reported == LEFT)  ? 0 : 1 ); } 
    byte rightindication()            { return ((_reported == RIGHT) ? 0 : 1 ); } 
    boolean isOOC(void)               { return (_reported != _commanded); }
    boolean named(char *n)            { return strcmp(n, _name) == 0; }
    void print(void)                  { 
#ifdef DEBUG
                                        for(int x = 7-strlen(_name); x > 0; x--) { Serial.print(" "); }
                                        Serial.print(_name); Serial.print(" rpt:"); Serial.print(toString(_reported));Serial.print(" cmd: "); Serial.print(toString(_commanded));
										Serial.print( _advancedapproach ? "AA" : "--");
										Serial.print( _approach         ? "A"  : "-");
										Serial.print( _routeOK          ? "R"  : "-");
										Serial.print(" -> ");
										if (_track == NULL) {
											Serial.print("No TrackCircuit");
										} else {										
											_track->print();
										}
#endif
                                      };
private:
	void _init(const char *name, TrackCircuit *tk) { 
										_name = name; 
										_track = tk;
										_stick = RRSignal::NONE;
										_reported = _commanded = RRSignal::UNKNOWN; 
										_timer = RRSignal::NOTIMER; 
										_routeOK=false;
										_approach = false; 
										_advancedapproach = false; 
										_safestate = UNKNOWN;
										_nextcommanded = UNKNOWN;
									  };
	
	const char *toString(State s) {
		switch (s) {
            case RRSignal::UNKNOWN:  return "   UNKNOWN";
            case RRSignal::LEFT:     return "      LEFT";
            case RRSignal::RIGHT:    return "     RIGHT";
            case RRSignal::ALLSTOP:  return "   ALLSTOP";
            case RRSignal::TIME:     return "      TIME";
            default:
            case RRSignal::ERROR:    return "     ERROR";
        }
	}
    const char *_name;
	TrackCircuit  *_track;
    State _reported;   
    State _commanded;       
    State _safestate;       
    State _nextcommanded;       
    Stick _stick;
    Timer _timer;
	boolean _routeOK;
	boolean _advancedapproach;
	boolean _approach;
    unsigned int _timestarted;
    unsigned int _time2end;
    boolean runningTime;

};


#endif


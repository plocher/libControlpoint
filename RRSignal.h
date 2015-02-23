/*
 *    Railroad Signal Mast abstrction
 *
 *    Copyright (c) 2013-2015 John Plocher
 *    Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)
 *
 */

#ifndef RRSIGNAL_H
#define RRSIGNAL_H
#include <Arduino.h>
#include <elapsedMillis.h>
#include <ControlPoint.h>
#include <SPCoast.h>

class RRSignal {
public:
	// MUST be the SAME as RRSignalHead's version (work around for an Arduino limitation)
    enum Aspects   { CLEAR, LIMITED_CLEAR, ADVANCED_APPROACH, APPROACH, RESTRICTING, STOP, DARK };
    //                G       (G)              (Y)              Y           (R)        R
    enum State     { UNKNOWN, LEFT, RIGHT, ALLSTOP, TIME, ERROR };
    enum Stick     { NONE, FLEET = 1, ER = 2, FLEET_ER = 3 };
    enum Timer     { NOTIMER, RUNNING, EXPIRED };
    
	RRSignal(const char *name)                     { _init(name, NULL); };
	

    boolean is(State s)               { return (_reported == s); };
    boolean isSafe(State s)           { if (s  == _reported) { return true; }
                                        if (!is(RRSignal::ALLSTOP)) { return false; }
										_safestate = s;
										return true;
                                      };
    boolean isSafe(int n, int r)      { return isSafe(toState(n,r)); }
	void  doSafe(void)				  { if (_safestate != UNKNOWN) { set(_safestate); _safestate = UNKNOWN; } }

    boolean commanded(State s)        { return (_commanded == s); };
    State commanded(void)             { return _commanded;};
    void set(State s)                 { if (_commanded == s)                      { /* NO OP */ }
	                                    else if (_commanded == RRSignal::ALLSTOP) {  _commanded = s;}
									    else                                      { setTime(10, s); }
									  };
    void set(int n, int r)            { set(toState(n,r)); };
    
	boolean knockdown(void)           { if ((_reported != RRSignal::ALLSTOP) && (_commanded != RRSignal::ALLSTOP)) {
											_wascommanded = _commanded;        // FLEET triggers off of this...
											_reported     = RRSignal::ALLSTOP;
											return true;
										}
										return false;
									  }    
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
                                            _delaytime = 0;  
                                            _time2end = (seconds *1000); 
                                            _timer = RRSignal::RUNNING;
                                            _nextcommanded = s;
                                            _commanded = TIME; // knock it down now, but don't let the plant change for xxx seconds...
                                      }
    Timer runTime(void)               {
                                        if (isRunningTime()) {
                                            if (_delaytime > _time2end) { // timer expired
                                                _timer = RRSignal::EXPIRED;
                                            }
                                        } 
                                        if (_timer == RRSignal::EXPIRED) {
                                          _commanded    = _nextcommanded;  
                                          _time2end     = 0;
                                          _timer = RRSignal::NOTIMER;
                                        }  
                                        return _timer;  
                                      }
    
    Stick stick(void)                 { return _stick;};
    void stick(Stick s)               { _stick = s; };
    boolean isStick(Stick s)          { return ((_stick | (s)) == (s)); };

    boolean local(void)               { return _localControl;};
    void local(boolean b)             { _localControl = b; };

    Aspects LeftAspect(void) {
	                                  return (is(LEFT)                             ? CLEAR 
                                            : commanded(LEFT) && (isStick(FLEET))  ? CLEAR
                                            : (isStick(ER))                        ? RESTRICTING
                                            : _localControl 	                   ? RESTRICTING 
											: STOP);
                                      }
    Aspects RightAspect(void) {
							          return (is(RIGHT)                            ? CLEAR 
							                : commanded(RIGHT) && (isStick(FLEET)) ? CLEAR
							                : (isStick(ER))                        ? RESTRICTING
							                : _localControl 	                   ? RESTRICTING 
											: STOP);
                                      }
    byte leftindication()             { return ((_reported == LEFT)  ? 0 : 1 ); } 	// for K#SG 
    byte rightindication()            { return ((_reported == RIGHT) ? 0 : 1 ); } 	// and K#NG indications

    boolean named(char *n)            { return strcmp(n, _name) == 0; }
    void print(void)                  { 
                                        for(int x = 7-strlen(_name); x > 0; x--) { Serial.print(" "); }
                                        Serial.print(_name); Serial.print(" rpt:"); Serial.print(toString(_reported));Serial.print(" cmd: "); Serial.print(toString(_commanded));
										Serial.print( _advancedapproach ? "AA" : "--");
										Serial.print( _approach         ? "A"  : "-");
										Serial.print( _routeOK          ? "R"  : "-");
                                      };
private:
	void _init(const char *name, TrackCircuit *tk) { 
										_name = name; 
										_stick = RRSignal::NONE;
										_wascommanded = _reported = _commanded = RRSignal::UNKNOWN; 
										_timer = RRSignal::NOTIMER; 
										_routeOK=false;
										_approach = false; 
										_advancedapproach = false; 
										_safestate = UNKNOWN;
										_nextcommanded = UNKNOWN;
										_localControl = false;
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
    State _reported;   
    State _wascommanded;   
    State _commanded;       
    State _safestate;       
    State _nextcommanded;       
    Stick _stick;
    Timer _timer;
	boolean _routeOK;
	boolean _advancedapproach;
	boolean _approach;
	boolean _localControl;
	
	elapsedMillis _delaytime;
    unsigned int _time2end;
    boolean runningTime;

};


#endif


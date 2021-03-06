/*
 *    Railroad Turnout (aka Switch) abstrction
 *
 *    Copyright (c) 2013-2015 John Plocher
 *    Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)
 *
 */

#ifndef SWITCH_H
#define SWITCH_H
#include <Arduino.h>
#include <elapsedMillis.h>
#include "RRSignal.h"
#include "TrackCircuit.h"

/*
 * A turnout on the layout
 *
 * Can be Normal, Reverse, in motion, or failed.
 *
 * Implements both Signal Locking and Track Circuit locking
 *
 * Flow:  
 *    cTc asks if safe to change, remember the potential new state
 *    if all components are safe to change, use validated state 
 *        - timer stashes new state in _nextcommanded, runs for several seconds, sets _commanded.  
 *    else ignore control packet
 *    Layout written using commanded state
 *    Layout read, into _real
 *    Indications come from real state
 */
 
class Switch {
public:
    enum State { UNKNOWN, NORMAL, REVERSE, TIME, ERROR };
    enum Timer { NOTIMER, RUNNING, EXPIRED };

    //Switch(const char *name, RRSignal *s)                    { _init(name, s,    NULL); };
    //Switch(const char *name, RRSignal *s, TrackCircuit *t)   { _init(name, s,    t); };
    
    Switch(char *name) {
		_init(name, NULL, NULL, NULL, 0, 0, 0); 
	};

	Switch(char *name, I2Cextender *m, int bitposN, int bitposR, int bitposM) { 
		_init(name, NULL, NULL, m, bitposN, bitposR, bitposM); 
	};
	Switch(char *name, State (*getFunction)(const char *), void (*setFunction)(const char *, State)) {
		_init(name, getFunction, setFunction, NULL, 0, 0, 0); 
	};
	
	void unpack(State s) {
		_real = s; 
	}
	void unpack(I2Cextender *m, int bitposN, int bitposR) {
		unpack(readLayout(m, bitposN, bitposR));
	}
	void unpack(void) {
		unpack(readLayout());
	}
	// grab the actual state from the field feedback data
	State readLayout(I2Cextender *m, int bitposN, int bitposR) {
		int n = (bitRead((*m).current(), bitposN) == 0);
		int r = (bitRead((*m).current(), bitposR) == 0);
		return (toState(n,r));
	}
	State readLayout(void) {
		if (_getState) {
			return _getState(_name);
		} else if (_m && (_bitposN == -1)) {
			return _real; 	// no feedback from layout, use last commanded state...
		} else if (_m) {
			return readLayout(_m, _bitposN, _bitposR);
		} else {
			return (ERROR);
		}
	}
   
	// push the current state out to the field
	void pack(I2Cextender *m, int bitposM, int bit) {
		bitWrite((*m).next, bitposM, bit); 
	}
	void pack(void) {
		if (_setState) {
			_setState(_name, _real);
		} else if (_m) {
			//Serial.print("Packing "); print(); Serial.println();
			pack(_m, _bitposM, fieldcommand());
		}
	}
	
    State   is(void)                  { return _real;};       // actual layout state
    boolean is(State s)               { return (_real == s); };
	void sig(RRSignal *s)             { _my_signal = s; }
	void trk(TrackCircuit *tc)        { _my_track = tc; }
	
	
	// user callable functions to manage the state of the turnout - everything after this needs to be safe
	// Safe to change when no change needed or if signal is at STOP and track UNOCCUPIED
    boolean isS(State s)               { return (_safestate == s); };

	boolean isSafe(void) {
		return isSafe(readLayout());
	}
	boolean isSafe(int* controls, int n, int r) {
		return isSafe(bitRead((*controls), n), bitRead((*controls), r));
	}
    boolean isSafe(int n, int r)      { return isSafe(toState(n,r)); }
	boolean isSafe(State s) { 
		if (s  == Switch::ERROR)   { return false; }
		_safestate = s;
		return true;
	};
	void  doSafe(void)				  { if (_safestate != UNKNOWN) { set(_safestate); _safestate = UNKNOWN; } }
	
    boolean isC(State s)               { return (_commanded == s); };
    State commanded(void)             { return _commanded;};  // From the dispatcher/cTc machine

	void  set(State s)                { if ( (s == NORMAL) || (s == REVERSE)) { _nextcommanded = _commanded = s; } };
    void  set(int n, int r)           { set(toState(n,r)); };

	// Use state from earlier isSafe call...
    State toState(int n, int r)       { return (
                                            (((r) == 1) && ((n) == 0)) ? Switch::REVERSE :
                                            (((r) == 0) && ((n) == 1)) ? Switch::NORMAL : 
                                            (((r) == 0) && ((n) == 0)) ? Switch::UNKNOWN :   
                                            Switch::ERROR
                                        );
                                      }

    byte fieldcommand(void)           { return ((_commanded == Switch::NORMAL) ? 0 : 1 ); }    // control bit - 0 = normal, 1 - reverse

    boolean isRunning(void)           { return (_timer != Switch::NOTIMER); }
    boolean isExpired(void)           { return (_timer == Switch::EXPIRED); }
    void setSlowMotion(int seconds, State s){
                                            // set a timer to simulate the time it takes a switch to throw...
                                            _delaytime = 0;  
                                            _time2end = (seconds *1000); 
                                            _timer = Switch::RUNNING;
                                            _nextcommanded = s;
                                            _commanded = TIME; // register the change as happening, but don't let the plant change for xxx seconds...
											// Serial.print("setSloMo: "); print(); Serial.println(); 

                                      }
    Timer runSlowMotion(void)               {
                                        if (isRunning()) {
                                            if (_delaytime > _time2end) { // timer expired
                                                _timer = Switch::EXPIRED;
                                            }
											// Serial.print("runningSloMo: "); print(); Serial.println(); 

                                        } 
                                        if (_timer == Switch::EXPIRED) {
                                          _real = _commanded = _nextcommanded;
                                          _timer = Switch::NOTIMER;
										  //Serial.print("doneSloMo: "); print(); Serial.println(); 
										  return Switch::EXPIRED;
                                        }  
                                        return _timer;  
                                      }
                                      
    char * name(void)                 { return _name; }
	boolean named(char *n)            { return strcmp(n, _name) == 0; }
    void print(void)                  { 
                                        for(int x = 7-strlen(_name); x > 0; x--) { Serial.print(" "); }
                                        Serial.print(_name); Serial.print(" real:"); 
										Serial.print(toString(_real));
                                        Serial.print(" cmd:"); 
										Serial.print(toString(_commanded));
                                        Serial.print(" ncmd:"); 
										Serial.print(toString(_nextcommanded));
                                        Serial.print(" time:");
										Serial.print(_timer == Switch::RUNNING? "RUN TIME" : (_timer == Switch::EXPIRED) ? " EXPIRED" : "NO TIMER");
                                        Serial.print(" safe:");
										Serial.print(toString(_safestate));
										Serial.print(" field:"); 
										Serial.print(fieldcommand(), BIN);
                                        Serial.print(" ");
                                      };
private:
	void _init(char *name, State (*getFunction)(const char *), void (*setFunction)(const char *, State), I2Cextender *m, int bitposN, int bitposR, int bitposM) { 
		_name      = (char *)name; 
		_getState = getFunction;
		_setState = setFunction;
		_m = m;
		_bitposN = bitposN;
		_bitposR = bitposR;
		_bitposM = bitposM;
		_nextcommanded = _commanded = _real = _safestate = Switch::UNKNOWN; 
		_timer = Switch::NOTIMER;; 
	};
    

    const char * toString(State s) {
	    switch (s) {
            case Switch::UNKNOWN:  return "UNKNOWN"; 
            case Switch::NORMAL:   return " NORMAL";
            case Switch::REVERSE:  return "REVERSE";
            default:
            case Switch::ERROR:    return "  ERROR";
		};
	};
	char *_name;
    State _commanded;      // from cTc
    State _nextcommanded;  // delayed, from commanded
    State _safestate;      // delayed, from safe test
    State _real;           // from layout to cTc

	I2Cextender *_m;
	State       (*_getState)(const char *);	
	void        (*_setState)(const char *, State);
	int 		_bitposN;
	int 		_bitposR;
	int 		_bitposM;

    Timer _timer;
	elapsedMillis _delaytime;
	unsigned int _time2end;
    boolean runningTime;
    RRSignal *_my_signal;
    TrackCircuit *_my_track;
};

#endif


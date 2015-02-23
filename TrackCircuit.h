/*
 *    Track Circuit abstrction
 *
 *    Copyright (c) 2013-2015 John Plocher
 *    Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)
 *
 */

#ifndef TRACKCIRCUIT_H
#define TRACKCIRCUIT_H
#include <Arduino.h>
#include <I2Cextender.h>

class TrackCircuit {
public:
    enum State { UNKNOWN, EMPTY, OCCUPIED, ERROR };

	TrackCircuit(const char *name)                                         { _init(name, NULL, NULL, 0); };
	TrackCircuit(const char *name, I2Cextender *m, int bitpos)             { _init(name, NULL, m, bitpos); };
	TrackCircuit(const char *name, State (*setFunction)(const char *))     { _init(name, setFunction, NULL, 0); };
    
    boolean is()                      { return (_real); };
    boolean is(State s)               { return (_real == s); };
    boolean isOccupied()              { return (_real == TrackCircuit::OCCUPIED); };
    const char* name(void)            { return  _name; };
    boolean named(char *n)            { return strcmp(n, _name) == 0; }

	void unpack(State s)              { _real = s; }
	void unpack(I2Cextender *m, int bitpos) {
		_real = bitRead((*(m)).current(), (bitpos))  ? TrackCircuit::EMPTY : TrackCircuit::OCCUPIED;
									  }
	void unpack()					  {
								        if (_setState) {
											unpack(_setState(_name));
									    } else if (_m) {
											unpack(_m, _bitpos);
										} else unpack(ERROR);
									  }
    void print(void)                  { 
										const char *s;
                                        for(int x = 7-strlen(_name); x > 0; x--) { Serial.print(" "); }
                                        Serial.print(_name); Serial.print(":");
                                        switch (_real) {
                                            case TrackCircuit::UNKNOWN:  s = "   UNKNOWN"; break;
                                            case TrackCircuit::EMPTY:    s = "     EMPTY"; break;
                                            case TrackCircuit::OCCUPIED: s = "  OCCUPIED"; break;
                                            default:
                                            case TrackCircuit::ERROR:    s = "     ERROR"; break;
                                        }
                                        Serial.print(s);
                                      };
private:
	void _init(const char *name, State (*setState)(const char *), I2Cextender *m, int bitpos) {
		_name = name; 
		_real = TrackCircuit::UNKNOWN;
		_setState = setState;
		_m = m;
		_bitpos = bitpos;
	}
    const char  *_name;
	I2Cextender *_m;
	int         _bitpos;
	State       (*_setState)(const char *);	
    State       _real;       
};


#endif


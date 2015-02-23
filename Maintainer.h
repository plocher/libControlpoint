#ifndef MAINTAINER_H
#define MAINTAINER_H
#include <Arduino.h>

/*
 *   Maintainer Call abstraction
 *
 *    Copyright (c) 2013-2015 John Plocher
 *    Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)
 *
 */
class Maintainer {
public:
    enum State { UNKNOWN, OFF, ON, ERROR };
    
	Maintainer(const char *name, void (*setFunction)(const char*, State))      { _init(name, setFunction, NULL, 0);      };
	Maintainer(const char *name, I2Cextender *m, int bitpos)                   { _init(name, NULL,        m,    bitpos); };
    
    State   is(void)            { return _commanded; };	// From cTc
    boolean is(State s)         { return (_commanded == s); };

	// push the current state out to the field
	void pack(I2Cextender *m, int bitpos, int bit) {
		bitWrite((*m).next, bitpos, bit); 
	}
    void pack(void)				{
									if (_setFunction) {
										_setFunction(_name, _commanded);
									} else if (_m) {
										pack(_m, _bitpos, is(ON)? 1 : 0);
									}
								}

    void   set(State s)         { _commanded = s; };
    void   set(int n)           { _commanded = (((n) == 1) ? Maintainer::ON  :
											    ((n) == 0) ? Maintainer::OFF :   
															 Maintainer::ERROR);
    }
    boolean named(char *n)      { return strcmp(n, _name) == 0; }
    void print(void)            {
	 									const char *s;
                                        for(int x = 7-strlen(_name); x > 0; x--) { Serial.print(" "); }
                                        Serial.print(_name); Serial.print(":"); 
                                        switch (_commanded) {
                                            case Maintainer::UNKNOWN:  s = "   UNKNOWN"; break;
                                            case Maintainer::ON:       s = "        ON"; break;
                                            case Maintainer::OFF:      s = "       OFF"; break;
                                            default:
                                            case Maintainer::ERROR:    s = "     ERROR"; break;
                                        }
                                        Serial.print(s);
                                       };
    
private:
	void _init(const char *name, void (*setFunction)(const char*, State), I2Cextender *m, int bitpos) { 
		_name = name;
		_setFunction = setFunction;
		_m = m;
		_bitpos = bitpos;
		_commanded = Maintainer::UNKNOWN;
	};
    
    const char *_name;
    State _commanded;
	I2Cextender *_m;
	int         _bitpos;
	void (*_setFunction)(const char*, State);
};


#endif


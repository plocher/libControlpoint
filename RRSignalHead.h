/*
 *    Railroad Signal Head abstrction
 *
 *    Copyright (c) 2013-2015 John Plocher
 *    Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)
 *
 */

#ifndef RRSIGNALHEAD_H
#define RRSIGNALHEAD_H
#include <Arduino.h>
#include <avr/pgmspace.h>
#include <elapsedMillis.h>
#include <ControlPoint.h>
#include <SPCoast.h>

class RRSignalHead {
public:	
	// MUST be the SAME as RRSignal's version (work around for an Arduino limitation)
    enum Aspects   { CLEAR, LIMITED_CLEAR, ADVANCED_APPROACH, APPROACH, RESTRICTING, STOP, DARK };
    //                G       (G)              (Y)              Y           (R)        R
    
    RRSignalHead(const char *name) {
		_init(name, NULL, NULL, NULL, 0, 0);
	};
    RRSignalHead(const char *name, RRSignal *sig) {
		_init(name, sig, NULL, NULL, 0, 0);
	};
    RRSignalHead(const char *name, RRSignal *sig, void (*setFunction)(const char *, Aspects, int, int)) {
		_init(name, sig, setFunction, NULL, 0, 0);
	};
	RRSignalHead(const char *name, RRSignal *sig, I2Cextender *m, int bitpos1, int bitpos2) { 
		_init(name, sig, NULL, m, bitpos1, bitpos2);; 
	};
	RRSignalHead(const char *name, I2Cextender *m, int bitpos1, int bitpos2) { 
		_init(name, NULL, NULL, m, bitpos1, bitpos2); 
	};

	void aspect2twobitindication( int* bit1, int* bit2) {
	    *bit1 = 1;  // default to
	    *bit2 = 0;  // restrictive STOP
	    int blinking = 0;
	    switch (_commanded) {                            // 00 = green, 11 = dark, 01 = yellow, 10 = red
	      case RRSignalHead::CLEAR:                *bit1 = 0; *bit2 = 0; blinking = 0; break;  //  G
	      case RRSignalHead::LIMITED_CLEAR:        *bit1 = 0; *bit2 = 0; blinking = 1; break;  // (G)
	      case RRSignalHead::ADVANCED_APPROACH:    *bit1 = 1; *bit2 = 0; blinking = 1; break;  // (Y)
	      case RRSignalHead::APPROACH:             *bit1 = 1; *bit2 = 0; blinking = 0; break;  //  Y
	      case RRSignalHead::RESTRICTING:          *bit1 = 0; *bit2 = 1; blinking = 1; break;  // (R)
	      default:
	      case RRSignalHead::STOP:                 *bit1 = 0; *bit2 = 1; blinking = 0; break;  //  R
	      case RRSignalHead::DARK:                 *bit1 = 1; *bit2 = 1; blinking = 0; break;  //-dark-
	    }
	    if (blinker > 900) {
	      blinker = 0;
	      blinkstate = (blinkstate == 0 ? 1 : 0);
	    }
	    if (blinking && blinkstate) {
	      *bit1 = *bit2 = 1;  // dark
	    }
	}

	// push the current state out to the field
	void pack(I2Cextender *m, int bitpos1, int bitpos2, int bit1, int bit2) {
		bitWrite((*m).next, bitpos1, bit1); 
		bitWrite((*m).next, bitpos2, bit2); 
	}
    void pack(void) {
		int bit1, bit2;
		aspect2twobitindication(&bit1, &bit2);
		if (_setAspect) {
			_setAspect(_name, _commanded, bit1, bit2);
		} else if (_m) {
			pack(_m, _bitpos1, _bitpos2, bit1, bit2);
		}
	}
    void setRoutes(void *str) {
		_routes = str;
	};
	const char* const* getRoutes() {
		return (char* const*)_routes;
	}
    static Aspects mostRestrictive(Aspects a1, Aspects a2)  { 
		return (((int) a1) < ((int) a2)) ? (a2) : (a1);
	};
    static Aspects leastRestrictive(Aspects a1, Aspects a2)  { 
		return (((int) a1) > ((int) a2)) ? (a2) : (a1);
	};

    Aspects is(void)             	  { return (_commanded); };
    boolean is(Aspects s)             { return (_commanded == s); };
	boolean named(char *n)            { return strcmp(n, _name) == 0; };
    void set(Aspects s)               { _commanded = s; };
	//boolean hasSig()				  { return _sig ? true : false; }
	//void setWithSig(void)			  { 
	//									if (_sig) { set((*_sig).is(RRSignal::ALLSTOP) ? STOP: CLEAR); }
	//								  }
    void print(void) {
#ifdef DEBUG
	    const char *s;
	    for(int x = 7-strlen(_name); x > 0; x--) { Serial.print(" "); }
	    Serial.print(_name); Serial.print(":"); 
		Serial.print(toString(_commanded));
#endif
    };


private:
	void _init(const char *name, RRSignal *sig, void (*setFunction)(const char*, Aspects, int, int), I2Cextender *m, int bitpos1, int bitpos2) { 
		_name = name;
		_sig = sig;
		_commanded = RRSignalHead::STOP;
		_setAspect = setFunction;
		_m = m;
		_bitpos1   = bitpos1;
		_bitpos2   = bitpos2;
		blinkstate = 0;
		_routes    = NULL;
	};
	
	const char *toString(Aspects a) {
        switch (a) {
          case CLEAR:                  return "      CLEAR";  // green
          case LIMITED_CLEAR:          return "   LIMCLEAR";  // flash green
          case ADVANCED_APPROACH:      return "  AAPPROACH";  // flash yellow
          case APPROACH:               return "   APPROACH";  // yellow
          case RESTRICTING:            return "RESTRICTING";  // flash red
          default:
          case STOP:                   return "       STOP";  // red
        }
    }

	elapsedMillis blinker;
	boolean blinkstate;
    const char    *_name;
	RRSignal      *_sig;
	I2Cextender   *_m;
	int           _bitpos1;
	int           _bitpos2;
	Aspects       _commanded;  
	void (*_setAspect)(const char*, Aspects, int, int); 
	void*  _routes; 
};


#endif


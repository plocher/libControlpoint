
#ifndef LIGHTING_H
#define LIGHTING_H

#include <Arduino.h>
#include <RGBdriver.h>
#include <elapsedMillis.h>

/*
 * Lighting commands for ambient and overgead lights
 *
 *  The fading pixels class works like this:
 *      call set(district, color); when you want to change the color for the district
 *		repeatedly call fade() in loop();
 */


   /* 
	*  The fading pixels class works like this:
	*       call set(r,g,b,i);
	*		repeatedly call fade() in loop();
	*		if fade() returns 1, use the r(), g(), b() and i() values to drive a LED
	*
	*  Imagine a crossfade that moves 
	*   the red LED from  0 to 10,    (up by 10)
	*   the green   from  0 to  5,    (up by 5) and 
	*   the blue    from 10 to  7,    (down by 3) in ten steps.
	*   We'd want to count the 10 steps and increase or 
	*   decrease color values in evenly stepped increments.
	*   Imagine a + indicates raising a value by 1, and a -
	*   equals lowering it. Our 10 step fade would look like:
	* 
	*   1 2 3 4 5 6 7 8 9 10
	* R + + + + + + + + + +
	* G   +   +   +   +   +
	* B     -     -     -
	* 
	* The red rises from 0 to 10 in ten steps, the green from 
	* 0-5 in 5 steps, and the blue falls from 10 to 7 in three steps.
	* 
	* In the code below, the color (in 0-255 values) is changed in 1020 steps (255*4).
	* 
	* To figure out how big a step there should be between one up- or
	* down-tick of one of the LED values, we call calculateStep(), 
	* which calculates the absolute gap between the start and end values, 
	* and then divides that gap by 1020 to determine the size of the step  
	* between adjustments in the value.
	*/
class Pixel {
public:
	enum State { INIT_FADE, ITERATE_FADE, WAIT_FADE, FINALIZE_FADE, DONE_FADE };
	Pixel(void) { 
		_r = desiredr = prevr = 0;
		_g = desiredg = prevg = 0;
		_b = desiredb = prevb = 0;
		_i = desiredi = previ = 255;
		_state = FINALIZE_FADE;
	}
	byte r() { return _r; };
	byte g() { return _g; };
	byte b() { return _b; };
	byte i() { return _i; };
	// Set color, gradually fade from current value to desired value.
	void set(byte R, byte G, byte B, byte I) { 
		desiredr = R; desiredg = G; desiredb = B; desiredi = I; _state = INIT_FADE; 
#if defined(DEBUG)
		if (0) {
			Serial.print("Pixel.set( rgbi= ");
			Serial.print(R, DEC); Serial.print(",");
			Serial.print(G, DEC); Serial.print(",");
			Serial.print(B, DEC); Serial.print(") ... ");
			Serial.print(desiredr, DEC); Serial.print(",");
			Serial.print(desiredg, DEC); Serial.print(",");
			Serial.print(desiredb, DEC); Serial.print(")\n");
		}
#endif
		};
	void set(byte R, byte G, byte B)         { set(R, G, B, 255); };
	
	// immediate color set, ignoring fade routine
	void force(byte R, byte G, byte B, byte I) { 
		desiredr = R; desiredg = G; desiredb = B; desiredi = I; 
		_r = R; _g = G; _b = B; _i = I; 
		_state = FINALIZE_FADE;
	}
	void force(byte R, byte G, byte B)       { force(R, G, B, 255); };

	/* fade() 
	 * once a new value is set, this iterator loops 1020 times, checking to see if  
	*  the value needs to be updated each time.  If something changes, it returns 1, else 0
	*/
	int need2fade(void) { return (_state != DONE_FADE); };
	
	int fade(int verbose) {
	  while (true) {
		  switch (_state) {
		    case INIT_FADE:
		    	idx = 0;
				if ((prevr == desiredr) &&
					(prevg == desiredg) &&
					(prevb == desiredb) &&
					(previ == desiredi) ) {
						_state = DONE_FADE;
						break;
				}
				stepr = calculateStep(prevr, desiredr);
				stepg = calculateStep(prevg, desiredg);
				stepb = calculateStep(prevb, desiredb);
				stepi = calculateStep(previ, desiredi);
#if defined(DEBUG)
				if (0 && verbose) { 
					Serial.print("INIT_FADE idx="); Serial.print(idx, DEC);
					Serial.print(", current color = (");
					Serial.print(_r, DEC); Serial.print(",");
					Serial.print(_g, DEC); Serial.print(",");
					Serial.print(_b, DEC); Serial.print("), ");
					Serial.print(", old color = (");
					Serial.print(prevr, DEC); Serial.print(",");
					Serial.print(prevg, DEC); Serial.print(",");
					Serial.print(prevb, DEC); Serial.print("), ");
					Serial.print("new color = (");
					Serial.print(desiredr, DEC); Serial.print(",");
					Serial.print(desiredg, DEC); Serial.print(",");
					Serial.print(desiredb, DEC); Serial.print("), ");
					Serial.print("step = (");
					Serial.print(stepr, DEC); Serial.print(",");
					Serial.print(stepg, DEC); Serial.print(",");
					Serial.print(stepb, DEC); Serial.print(")\n");
				}
#endif
		        _state = ITERATE_FADE;
		        break;
		    case ITERATE_FADE:
		        if (idx > 1020) {
		          _state = FINALIZE_FADE;
		          break;
		        }
		        _r = calculateVal(stepr, _r, idx);
		        _g = calculateVal(stepg, _g, idx);
		        _b = calculateVal(stepb, _b, idx);
		        _i = calculateVal(stepi, _i, idx);
#if defined(DEBUG)
				if (0 && verbose && ((idx == 1) || (idx % 64 == 0) || (idx == 1020))) { 
					Serial.print("ITERATE_FADE idx="); Serial.print(idx, DEC);
					Serial.print(" d[0]=(");
					Serial.print(_r, DEC); Serial.print(",");
					Serial.print(_g, DEC); Serial.print(",");
					Serial.print(_b, DEC); Serial.print(")\n");
				}
#endif
		        idx++;
		        _timeout = 0;
		        // _state = WAIT_FADE;
		        _state = ITERATE_FADE;
		        return 1;
		    case WAIT_FADE:
		        if (_timeout > 1) {
					_state = ITERATE_FADE;
#if defined(DEBUG)
					if (0 && verbose) { Serial.print("exit waiting, idx="); Serial.println(idx, DEC); }
#endif
					break;
				} else return 0;
		    case FINALIZE_FADE:
		        // Update current values for next loop
		        prevr = _r; 
		        prevg = _g; 
		        prevb = _b;
		        previ = _i;
				_state = DONE_FADE;
				return 1;
			case DONE_FADE:
				return 0;
			}
		}
	};
	
private:
	int idx;
	byte _r, desiredr, prevr; int stepr; 
	byte _g, desiredg, prevg; int stepg; 
	byte _b, desiredb, prevb; int stepb; 
	byte _i, desiredi, previ; int stepi; 
	enum State _state;
	elapsedMillis _timeout;

	int calculateStep(byte prevValue, byte endValue) {
	  int step = (int)endValue - (int)prevValue; // What's the overall gap?
	  if (step != 0) step = 1020 / step;
#if defined(DEBUG)
	  // Serial.print("Calc step: prev="); Serial.print(prevValue, DEC); Serial.print(", end="); Serial.print(endValue, DEC);Serial.print(", step="); Serial.print(step, DEC); Serial.println();
#endif
	  return (step);
	};

	/*  When the loop value, idx, reaches the step size appropriate for one
	 *  of the colors, it increases or decreases the value of that color by 1. 
	 *  (R, G, B and I (intensity) are each calculated separately.)
	 */

	int calculateVal(int step, int val, int idx) {
	  if ((step) && idx % step == 0) {		// If step is non-zero and its time to change a value,
	    if (step > 0)      { val += 1; }    //   increment the value if step is positive...
	    else if (step < 0) { val -= 1; }    //   ...or decrement it if step is negative
	  }
	  // Defensive driving: make sure val stays in the range 0-255
	  return (val > 255) ? 255 : (val < 0) ? 0 : val;
	};	
};

class Lighting {
public:

    enum District  { BACKDROP, HORIZON, OVERHEADW, OVERHEADRGB, NUMDISTRICTS };  
	enum Color     { OFF, ON, DAWN, MORNING, LATEMORNING, NOON, AFTERNOON, EVENING, SUNSET, NIGHT, MIDNIGHT, NUMCOLORS }; // up to 16 values (4 bits)
	
	Lighting(int c = 5, int d = 6) : lampDriver(c,d) { 
		for (int x = 0; x < NUMDISTRICTS; x++) {
			set(x, OFF);
		} 
	};
	int districts(void) { return NUMDISTRICTS; };
	
	void force(int district, int r, int g, int b) {
		set(district,r,g,b,1);
	}
	
	void set(int district, int r, int g, int b, int f) {
#if defined(DEBUG)
		if (0) {
			Serial.print("Lighting.set( d=");
			Serial.print(district, DEC); Serial.print(", rgb=");
			Serial.print(r, DEC); Serial.print(",");
			Serial.print(g, DEC); Serial.print(",");
			Serial.print(b, DEC); Serial.print(")\n");
		}
#endif
		if (f)
			d[district].force(r,g,b, 255);
		else
			d[district].set(r,g,b, 255);
	};	
	
	void set(int district, int r, int g, int b) {
		set(district,r,g,b,0);
	}
	
	void set(int district, int c) {
#if defined(DEBUG)
		if (0) {
			Serial.print("Lighting.set( d=");
			Serial.print(district, DEC); Serial.print(", (int)c= ");
			Serial.print(c, DEC); Serial.print(")\n");
		}
#endif
		set(district, (Color) c);
	};
	void set(int district, Color c) {
		set(district, c, 0);
	}
	void set(int district, Color c, int f) {
#if defined(DEBUG)
		if (0) { 
			Serial.print("Lighting.set( d=");
			Serial.print(district, DEC); Serial.print(", (Color)c= ");
			Serial.print(c, DEC); Serial.print(")\n");
		}
#endif
		if (district >= 0 && district < NUMDISTRICTS) {
			// define what colors each strand should display for this time of day...
			switch (c) {
	            case OFF:       	  set(district,      0,   0,   0, f);  return;
	            case ON:        	  set(district,    255, 255, 255, f);  return;
	            case DAWN:      	  
					switch(district) {
					case BACKDROP:    set(BACKDROP,      0,   0,   0, f);  return;		// Off 
					case HORIZON:     set(HORIZON,      45,   0,  17, f);  return;		// red/purple glow
					case OVERHEADW:   set(OVERHEADW,     0,   0,   0, f);  return;		// Off
					case OVERHEADRGB: set(OVERHEADRGB,   7,   0, 100, f);  return;		// Blue
					} return;
	            case MORNING:
					switch(district) {
					case BACKDROP:    set(BACKDROP,      0,   0,   0, f);  return;		// Off
					case HORIZON:     set(HORIZON,      45,   0, 125, f);  return;		// bright pastel blue with a tinge of red
					case OVERHEADW:   set(OVERHEADW,     3,   3,   3, f);  return;		// dim
					case OVERHEADRGB: set(OVERHEADRGB,  15,  30,  60, f);  return;		// morning sky tinge blue, warmer
					} return;
	            case LATEMORNING:    
					switch(district) {
					case BACKDROP:    set(BACKDROP,     40,  40,  40, f);  return;		// white
					case HORIZON:     set(HORIZON,      45,  25,  55, f);  return;		// light, still with some color
					case OVERHEADW:   set(OVERHEADW,    90,  90,  90, f);  return;		// brighter
					case OVERHEADRGB: set(OVERHEADRGB,  70,  70,  90, f);  return;		// full spectrum
					} return;
	            case NOON:    
					switch(district) {
					case BACKDROP:    set(BACKDROP,    100, 100, 100, f);  return;		// bright white
					case HORIZON:     set(HORIZON,      25,  25,  25, f);  return;		// nondescript
					case OVERHEADW:   set(OVERHEADW,   120, 120, 120, f);  return;		// Grey
					case OVERHEADRGB: set(OVERHEADRGB, 140, 140, 220, f);  return;		// bright
					} return;
	            case AFTERNOON: 
					switch(district) {
					case BACKDROP:    set(BACKDROP,     60,  60,  60, f);  return;		// white
					case HORIZON:     set(HORIZON,      25,  12,  15, f);  return;		// dimmer, warming to red
					case OVERHEADW:   set(OVERHEADW,   100, 100, 100, f);  return;		// bright
					case OVERHEADRGB: set(OVERHEADRGB, 100, 100, 150, f);  return;		// dimmer, but still full spectrum
					} return;
	            case EVENING: 
					switch(district) {
					case BACKDROP:    set(BACKDROP,     40,  40,  40, f);  return;		// Grey
					case HORIZON:     set(HORIZON,      35,  25,  12, f);  return;		// Orange
					case OVERHEADW:   set(OVERHEADW,    60,  60,  60, f);  return;		// Grey
					case OVERHEADRGB: set(OVERHEADRGB,  90,  70, 110, f);  return;		// blending to purple...
					} return;
	            case SUNSET:  
					switch(district) {
					case BACKDROP:    set(BACKDROP,      5,   5,   5, f);  return;	    // Off
					case HORIZON:     set(HORIZON,     120,  20,  65, f);  return;	    // red with purple
					case OVERHEADW:   set(OVERHEADW,    20,  20,  20, f);  return;	    // starting to dim
					case OVERHEADRGB: set(OVERHEADRGB,  40,  30, 125, f);  return;	    // more and more blue
					} return;
	            case NIGHT:
					switch(district) {
					case BACKDROP:    set(BACKDROP,      2,   2,   2, f);  return;	
					case HORIZON:     set(HORIZON,       0,   0, 140, f);  return;	
					case OVERHEADW:   set(OVERHEADW,     0,   0,   0, f);  return;	
					case OVERHEADRGB: set(OVERHEADRGB,   0,   0, 150, f);  return;	
					} return;
	            case MIDNIGHT: 
					switch(district) {
					case BACKDROP:    set(BACKDROP,      0,   0,   0, f);  return;	
					case HORIZON:     set(HORIZON,       0,   0, 120, f);  return;	
					case OVERHEADW:   set(OVERHEADW,     0,   0,   0, f);  return;	
					case OVERHEADRGB: set(OVERHEADRGB,   0,   0, 140, f);  return;	
					} return;
	            default:        	set(district,  10,   5,   5, f);  return;		// PINK!
	        }
		}
	};
	
	void send(void) {
	    lampDriver.begin();
	    for (int district = 0; district < NUMDISTRICTS; district++) {
	        lampDriver.SetColor(d[district].r(),d[district].b(),d[district].g()); // strings are in Red Green Blue order
	    }
	    lampDriver.end();
	};
	
	int need2fade(void) {
		int changed = 0;
		for (int district = 0; district < NUMDISTRICTS; district++) {
			changed += d[district].need2fade();
	    }
		return (changed);
	};
		
	void fade(void) {
		int changed = 0;
		for (int district = 0; district < NUMDISTRICTS; district++) {
#if defined(DEBUG)
			changed += d[district].fade(district == 0); // only enable debugging output on district 0 to avoid overwhelming printf() output
#else
			changed += d[district].fade(0); // no debugging output 
#endif
	    }
		if (changed) {
#if defined(DEBUG)
			// Serial.print("d[0]=("); Serial.print(d[0].r(), DEC); Serial.print(","); Serial.print(d[0].g(), DEC); Serial.print(","); Serial.print(d[0].b(), DEC); Serial.print(")\n");
#endif
			send();
		}
	};
	
	private:
		Pixel    d[NUMDISTRICTS];
		RGBdriver lampDriver;
};

#endif


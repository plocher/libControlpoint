/* 
 *    Copyright (c) 2013-2015 John Plocher
 *    Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)
 */

#ifndef CONTROLPOINT_H
#define CONTROLPOINT_H
#define DEBUG
#include <Arduino.h>
#include <LocoNet.h>

#include "TrackCircuit.h"
#include "Switch.h"
#include "RRSignal.h"
#include "RRSignalHead.h"
#include "Maintainer.h"


// defined in the main sketch...
extern int getNumPorts(void);
extern int getNumTrackCircuits(void);
extern int getNumSwitches(void);
extern int getNumSignals(void);
extern int getNumHeads(void);
extern int getNumCalls(void);

extern I2Cextender		m[];
extern TrackCircuit		track[];
extern RRSignal			sig[];
extern RRSignalHead		head[];
extern Switch			sw[];
extern Maintainer		mc[];

class ControlPoint {
public:
	//static RRSignalHead::Aspects Evaluate(char *input);
	//static RRSignalHead::Aspects mostRestrictive(RRSignalHead::Aspects current, RRSignalHead::Aspects desired);
	static void 			 initializeCodeLine(int lnrx, int lntx);
	static int               sendCodeLine(int from, int to, int *indications);
	static boolean           readall(void);
	static void              writeall(void);	
	static int               LnPacket2Controls(int *src, int *dst, int *controls);
	static int               freeRam (void);
	static void              setup(void);
	static void              savestate(int *controls);
	static void              restorestate(void);
	
#ifdef DEBUG
	static void              printEverything(void);	
	static void              printControls(int from, int to, int *controls);
	static void              printIndications(int from, int to, int *indications);
	static void              printPacket(char *name, int from, int to, int *packet);	
	static void              printLnPacket(lnMsg *LnPacket);	
	static void              printBin(byte x);	
	static void              printBinReverse(byte x);	
	static void              printBinOriginal(byte x);	
#endif
private:
	static int						getSignal(char *name);
	static int						getSwitch(char *name);
	static int						getHead(char *name);
	static int						getTrack(char *name);
	static RRSignalHead::Aspects	A_Switch(char *name, char *token);
	static RRSignalHead::Aspects	A_Signal(char *name, char *token);
	static RRSignalHead::Aspects	A_Approach(char *name, char *token);
	static RRSignalHead::Aspects	A_Track(char *name, char*token);
	
};
  

#endif




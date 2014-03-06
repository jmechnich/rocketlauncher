#ifndef LAUNCHER_HH
#define LAUNCHER_HH

#include "Common.hh"
#include "Command.hh"

#include <sstream>
#include <cstring>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <unistd.h>

class Action;
class Command;

template<class MsgIface, class UserIface>
class Launcher
{
public:
  Launcher( int vendorID, int deviceID)
          : _mi(vendorID,deviceID,MSG_STATUS) {init();}
  // read status (non-blocking)
  int  update_status();
  // wait for status bit 'cmd' (blocking)
  int  wait(char cmd);
  // send cmd (non-blocking)
  int  move(char cmd);
  // move to endpoint in given direction (blocking)
  int  moveHome( char cmd);
  // move for dt in seconds (blocking)
  void moveTimed( char cmd, double dt);
  // move relative to current position (blocking)
  int  moveRel( double theta, double phi);
  // move to absolute coordinates (blocking)
  int  moveAbs( double theta, double phi);
  // start firing and stop after status bit flipped (blocking)
  int  fire();
  // start firing and stop after timeout (blocking)
  int  fireTimeout(double timeout);
  // process event loop
  bool process();
  // trigger dialog for moveRel arguments
  void goRel();
  // trigger dialog for moveAbs arguments
  void goAbs();
  // go to upper-right endpoints, calibrating (0,0)
  void goHome();
  // run calibration pattern for measuring theta/phi pos/neg times
  void calibrate();
  // connect to USB device and start UI
  int  connect();
  // disconnect from USB device and stop UI
  int  disconnect();
  // print positional variables status
  void printStatusPV();
  // print move variables status
  void printStatusMV();
  // print key bindings
  void printHelp();

  int  stop() {_start =-1;return 0;}
  double thetaMin() const {return _thetaMin;}
  double thetaMax() const {return _thetaMax;}
  void   setThetaMinMax( double min, double max)
        {_thetaMin = min;_thetaMax = max;}
  double phiMin()   const {return _phiMin;}
  double phiMax()   const {return _phiMax;}
  void   setPhiMinMax( double min, double max)
        {_phiMin = min;_phiMax = max;}
  double thetaRange() const {return _thetaMax-_thetaMin;}
  double phiRange()   const {return _phiMax-_phiMin;}
  double thetaPos()   const {return _thetaPos;}
  double thetaNeg()   const {return _thetaNeg;}
  void   setThetaPosNeg( double pos, double neg)
        {_thetaPos = pos;_thetaNeg=neg;}
  double phiPos()   const {return _phiPos;}
  double phiNeg()   const {return _phiNeg;}
  void   setPhiPosNeg( double pos, double neg)
        {_phiPos = pos;_phiNeg=neg;}
  double theta()    const {return _theta;}
  double phi()      const {return _phi;}
  bool   calibrated() const
        {return minMaxValid() && posValid() && speedValid();}
  bool   minMaxValid() const
        {return _thetaMin>-1 && _thetaMax>-1 && _phiMin>-1 && _phiMax>-1;}
  bool   posValid() const
        {return _thetaMin<=_theta && _theta<=_thetaMax &&
              _phiMin<=_phi && _phi<=_phiMax;}
  bool   speedValid() const
        {return _thetaPos>0 && _thetaNeg>0 && _phiPos>0 && _phiNeg>0;}
  void   setDebug( bool debug)
        {_debug=debug;_mi.setDebug(debug);_ui.setDebug(debug);}
  void   addAction( const Action& a, Command* c) {_ui.addAction(a,c);}
private:
  void adjust(char cmd, double dt);
  void init();
  
  char     _current;
  char     _statusOld;
  double   _start;
  double   _theta;
  double   _thetaMin;
  double   _thetaMax;
  double   _phi;
  double   _phiMin;
  double   _phiMax;
  double   _thetaPos;
  double   _thetaNeg;
  double   _phiPos;
  double   _phiNeg;
  MsgIface  _mi;
  UserIface _ui;
  bool _debug;
  
  Timer _timer;
};

#include "Launcher.icc"

#endif

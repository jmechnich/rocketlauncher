#include "Launcher.hh"

#include "CursesInterface.hh"
typedef CursesInterface ControlInterface;

#ifdef HAVE_LIBUSB10
#include "LibUSB10Interface.hh"
typedef LibUSB10Interface USBInterface;
#endif

#ifdef HAVE_LIBUSB
#include "LibUSBInterface.hh"
typedef LibUSBInterface USBInterface;
#endif

#ifdef HAVE_IOKIT
#include "IOKitInterface.hh"
typedef IOKitInterface USBInterface;
#endif

int main()
{
  typedef Launcher<USBInterface,ControlInterface> MyLauncher;
  // create launcher for given vendor and device ids
  MyLauncher l(0x0a81, 0x0701);
  // set times for moving between the two endpoints of each angular direction
  // produced by Launcher::calibrate
  l.setThetaPosNeg( 2.95986, 2.76801);
  l.setPhiPosNeg( 19.5367, 19.857);
  // define key shortcuts/actions (see Command.hh)
  l.addAction( Action( 'a', "Move left",  0,-3, true, MSG_LEFT),
               makeTrigger_1(l,&MyLauncher::move,char(MSG_LEFT)));
  l.addAction( Action( 'd', "Move right", 0, 3, true, MSG_RIGHT),
               makeTrigger_1(l,&MyLauncher::move,char(MSG_RIGHT)));
  l.addAction( Action( 'w', "Move up",   -2, 0, true, MSG_UP),
               makeTrigger_1(l,&MyLauncher::move,char(MSG_UP)));
  l.addAction( Action( 's', "Move down",  2, 0, true, MSG_DOWN),
               makeTrigger_1(l,&MyLauncher::move,char(MSG_DOWN)));
  l.addAction( Action( ' ', "Stop"),
               makeTrigger_1(l, &MyLauncher::move,char(MSG_STOP)));
  // approximately 5.5s needed for charging and releasing the air
  l.addAction( Action( 'f', "Single-shot fire, blocking"),
               makeTrigger_0(l, &MyLauncher::fire));
  l.addAction( Action( 'F', "Single-shot fire, stopped by timeout"),
               makeTrigger_1(l, &MyLauncher::fireTimeout,5.5));
  l.addAction( Action( 'E', "Single-shot fire, stopped by status update"),
               makeTrigger_1(l, &MyLauncher::move,char(MSG_FIRE)));
  l.addAction( Action( '1', "Move to kitchen"),
               makeTrigger_2(l, &MyLauncher::moveAbs, 65.,110.));
  l.addAction( Action( '2', "Move to couch"),
               makeTrigger_2(l, &MyLauncher::moveAbs, 90.,170.));
  l.addAction( Action( '3', "Move to bed"),
               makeTrigger_2(l, &MyLauncher::moveAbs, 90.,235.));
  l.addAction( Action( '4', "Move to desk"),
               makeTrigger_2(l, &MyLauncher::moveAbs, 90.,285.));
  l.addAction( Action( 'h', "Go home"),
               makeTrigger_0(l, &MyLauncher::goHome));
  l.addAction( Action( 'c', "Calibrate"),
               makeTrigger_0( l, &MyLauncher::calibrate));
  l.addAction( Action( 'p', "Print position parameters"),
               makeTrigger_0( l, &MyLauncher::printStatusPV));
  l.addAction( Action( 'm', "Print movement parameters"),
               makeTrigger_0( l, &MyLauncher::printStatusMV));
  l.addAction( Action( 'q', "Quit"),
               makeTrigger_0( l, &MyLauncher::stop));
  l.addAction( Action( 'g', "Go relative"),
               makeTrigger_0( l, &MyLauncher::goRel));
  l.addAction( Action( 'z', "Go absolute"),
               makeTrigger_0( l, &MyLauncher::goAbs));
  l.addAction( Action( '?', "Print help"),
               makeTrigger_0( l, &MyLauncher::printHelp));

  // for debug mode: 'mknod errpipe p' and start with
  // './rocketlauncher 2>errpipe'
  // 'tail -f errpipe' in a 2nd terminal
#ifdef DEBUG
  l.setDebug( true);
#endif

  // connect to launcher and update the event loop once every 50ms
  int ret;
  if((ret=l.connect())) return ret;
  while((ret=l.process())) {usleep(50000);}
  if((ret=l.disconnect())) return ret;
    
  return 0;
}

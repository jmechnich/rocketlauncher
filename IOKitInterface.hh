#ifndef IOKITINTERFACE_HH
#define IOKITINTERFACE_HH

#include <IOKit/IOReturn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <string>

#include "Common.hh"

class IOKitInterface
{
  enum{XFER_BULK,XFER_INT};
  struct SendCmd
  {
    int RequestType;
    int Request;
    int Value;
    int Index;
    int Timeout;
  };
  struct RecvCmd
  {
    int Type;
    int Endpoint;
    int Timeout;
  };
  
public:
  IOKitInterface( int vendor, int product, char statusMsg)
          : _dev(0), _interface(0), _vendor(vendor), _product(product),
            _statusMsg(statusMsg), _debug(false)
        {
          // The hex values passed into the control_msg() method define how the
          // USB interface passes the control byte on to the controller
          if( _vendor == 0x0a81 && _product == 0x0701)
          {
            // 0x21 bmRequestType
            //      LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE (?)
            // 0x09 bRequest LIBUSB_REQUEST_SET_CONFIGURATION
            // 0x01 wValue   should be bConfigurationValue
            //               (0x200 in other implementations)
            // 0x00 wIndex
            // 1000 timeout
            SendCmd send_cmd = {0x21,0x09,0x01,0x00,1000};
            _send_cmd=send_cmd;
            //      type     XFER_INT or XFER_BULK, lsusb says Interrupt
            // 0x81 ep       LIBUSB_ENDPOINT_IN | 0x1 (?), lsusb says 0x81
            // 1000 timeout
            RecvCmd recv_cmd = {XFER_INT,1,1000};
            _recv_cmd=recv_cmd;
          }
          else
          {
            SendCmd send_cmd = {0};_send_cmd=send_cmd;
            RecvCmd recv_cmd = {0};_recv_cmd=recv_cmd;
          }
        }
  IOReturn open();
  IOReturn openDevice();
  IOReturn openInterface();
  IOReturn setConfiguration();
  IOReturn close();
  IOReturn send( char msg);
  IOReturn print_settings();
  IOReturn resetPipe(UInt8 pipeRef);
  IOReturn checkPipe(UInt8 pipeRef);
  IOReturn read( char* status);
  void     setDebug(bool debug) {_debug=debug;}
private:
  IOUSBDeviceInterface300**    _dev;
  IOUSBInterfaceInterface300** _interface;
  int     _vendor;
  int     _product;
  char    _statusMsg;
  bool    _debug;
  
  SendCmd _send_cmd;
  RecvCmd _recv_cmd;

  IOReturn    print_error( const std::string& name, IOReturn code);
  const char* str_return(IOReturn err);
};

#endif

#ifndef LIBUSB10INTERFACE_HH
#define LIBUSB10INTERFACE_HH

#include <libusb.h>
#include <iostream>
#include <cstring>

class LibUSB10Interface
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
  LibUSB10Interface( int vendor, int product, char statusMsg)
          : _dev(0), _interface(0), _vendor(vendor), _product(product),
            _statusMsg(statusMsg), _debug(false), _init(false)
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
            RecvCmd recv_cmd = {XFER_INT,LIBUSB_ENDPOINT_IN|0x1,1000};
            _recv_cmd=recv_cmd;
          }
          else
          {
            SendCmd send_cmd = {0};_send_cmd=send_cmd;
            RecvCmd recv_cmd = {0};_recv_cmd=recv_cmd;
          }
        }
  int open()
        {
          int ret=0;
          if(!_send_cmd.RequestType)
          {
            std::cerr << "send commands not initialized" << std::endl;
            return -1;
          }
          if(!_recv_cmd.Endpoint)
          {
            std::cerr << "recv commands not initialized" << std::endl;
            return -1;
          }
          ret=libusb_init(0);
          if(ret)
          {
            std::cerr << "libusb_init failed with code " << ret
                      << " (" << libusb_error_name(ret) << ")" << std::endl;
            return ret;
          }
          if(_debug) libusb_set_debug(0,2);
          _dev=libusb_open_device_with_vid_pid(0,_vendor,_product);
          if(!_dev)
          {
            std::cerr << "libusb_open_device_with_vid_pid failed" << std::endl;
            libusb_exit(0);
            return -1;
          }
          // try to claim
          if(libusb_kernel_driver_active(_dev,_interface)==1)
          {
            if(_debug) std::cerr << "Unloading kernel driver" << std::endl;
            ret=libusb_detach_kernel_driver(_dev,_interface);
            if(ret<0)
            {
              std::cerr << "libusb_detach_kernel_driver failed with code "
                        << ret << " (" << libusb_error_name(ret) << ")"
                        << std::endl;
              libusb_close(_dev);_dev=0;
              libusb_exit(0);
              return ret;
            }
          }
          // seems to break the connection (requires replugging)
          // ret = libusb_set_configuration(_dev, 0);
          // if(ret)
          // {
          //   std::cerr << "libusb_set_configuration failed with code " << ret
          //             << " (" << libusb_error_name(ret) << ")" << std::endl;
          //   return ret;
          // }
          ret = libusb_claim_interface(_dev, _interface);
          if(ret)
          {
            std::cerr << "libusb_claim_interface failed with code " << ret
                      << " (" << libusb_error_name(ret) << ")" << std::endl;
            
            libusb_close(_dev);_dev=0;
            libusb_exit(0);
            return ret;
          }
          _init=true;
          ret = libusb_set_interface_alt_setting(_dev,_interface,0);
          if(ret)
          {
            std::cerr << "libusb_set_interface_alt_setting failed with code "
                      << ret << " (" << libusb_error_name(ret) << ")"
                      << std::endl;
            return ret;
          }
          if(_debug) std::cerr << "Testing USB send" << std::endl;
          ret=send(0x0);
          if(ret)
          {
            std::cerr << "send(0x0) failed with code "
                      << ret << " (" << libusb_error_name(ret) << ")"
                      << std::endl;
            return ret;
          }
          if(_debug) std::cerr << "Testing USB read" << std::endl;
          ret=read(0);
          if(ret)
          {
            std::cerr << "read() failed with code "
                      << ret << " (" << libusb_error_name(ret) << ")"
                      << std::endl;
            return ret;
          }
          return ret;
        }
  int close()
        {
          if(!_init) return 0;
          int ret = libusb_release_interface(_dev,_interface);
          if(ret)
          {
            std::cerr << "libusb_release_interface failed with code " << ret
                      << " (" << libusb_error_name(ret) << ")" << std::endl;
            return ret;
          }
          libusb_close(_dev);
          _dev=0;
          libusb_exit(0);
          return ret;
        }
  int send( char msg)
        {
          if(!_dev) return -1;
          const int bufsize=8;
          static unsigned char buf[bufsize];
          memset(buf,0,bufsize);
          buf[0]=msg;
          int ret=libusb_control_transfer(_dev,_send_cmd.RequestType,
                                          _send_cmd.Request,_send_cmd.Value,
                                          _send_cmd.Index,buf,bufsize,
                                          _send_cmd.Timeout);
          if(ret<0)
          {
            std::cerr << "libusb_control_transfer failed with code " << ret
                      << " (" << libusb_error_name(ret) << ")" << std::endl;
            return ret;
          }
          return 0;
        }
  int read( char* status)
        {
          if(!_dev) return -1;
          int ret = send(_statusMsg);
          if(ret<0) return ret;
          unsigned char tmp;
          int actual_xfer=0;
          if( _recv_cmd.Type==XFER_BULK)
              ret=libusb_bulk_transfer(_dev,_recv_cmd.Endpoint,&tmp,1,
                                       &actual_xfer,_recv_cmd.Timeout);
          else if(_recv_cmd.Type==XFER_INT)
              ret=libusb_interrupt_transfer(_dev,_recv_cmd.Endpoint,&tmp,1,
                                            &actual_xfer,_recv_cmd.Timeout);
          if(ret)
          {
            std::cerr << "libusb_bulk_transfer failed with code " << ret
                      << " (" << libusb_error_name(ret) << ")" << std::endl;
            return ret;
          }
          if(status) *status = tmp;
          return ret;
        }
  void setDebug(bool debug) {_debug=debug;}
private:
  libusb_device_handle* _dev;
  int     _interface;
  int     _vendor;
  int     _product;
  char    _statusMsg;
  bool    _debug;
  bool    _init;
  
  SendCmd _send_cmd;
  RecvCmd _recv_cmd;
};

#endif

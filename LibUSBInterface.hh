#ifndef LIBUSBINTERFACE_HH
#define LIBUSBINTERFACE_HH

#include <usb.h>
#include <iostream>
#include <cstring>

class LibUSBInterface
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
  LibUSBInterface( int vendor, int product, char statusMsg)
          : _dev(0), _interface(0), _vendor( vendor), _product(product),
            _statusMsg( statusMsg), _debug(false), _init(false)
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
            // 0x81 ep       USB_ENDPOINT_IN | 0x1 (?), lsusb says 0x81
            // 1000 timeout
            RecvCmd recv_cmd = {XFER_INT,USB_ENDPOINT_IN|0x1,1000};
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
          usb_init();
          usb_find_busses();
          usb_find_devices();
          struct usb_bus *bus = usb_get_busses();
          for( ; bus && !_dev; bus = bus->next) {
            for( struct usb_device *dev = bus->devices; dev; dev = dev->next) {
              if (dev->descriptor.idVendor == _vendor &&
                  dev->descriptor.idProduct == _product) {
                _dev = usb_open(dev);
                if(!_dev)
                {
                  std::cerr << "usb_open failed" << std::endl;
                  return -1;
                }
                break;
              }
            }
          }
          if(!_dev)
          {
            std::cerr << "device not found, exiting" << std::endl;
            return -1;
          }
          // try to claim, if it fails detach the driver using the device
          int ret = usb_claim_interface(_dev, _interface);
          if(ret < 0)
          {
            if(_debug)
            {
              std::cerr << "usb_claim_interface failed with code "
                        << ret << " (" << strerror(-ret) << ")" << std::endl;
              std::cerr << "trying to detach driver" << std::endl;
            }
#ifdef LIBUSB_HAS_GET_DRIVER_NP
            int buflen = 255;
            char buf[buflen+1];
            ret = usb_get_driver_np(_dev, _interface, buf, buflen);
            if(ret < 0)
            {
              std::cerr << "usb_get_driver_np failed with code "
                        << ret << " (" << strerror(-ret) << ")" << std::endl;
              usb_close(_dev);
              _dev=0;
              return ret;
            }
#endif
#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
            ret = usb_detach_kernel_driver_np( _dev, _interface);
            if(ret < 0)
            {
              std::cerr << "usb_detach_kernel_driver_np failed with code "
                        << ret << " (" << strerror(-ret) << ")" << std::endl;
              usb_close(_dev);
              _dev=0;
              return ret;
            }
#endif
            ret = usb_claim_interface(_dev,_interface);
            if(ret < 0)
            {
              std::cerr << "usb_claim_interface failed with code " << ret
                        << " (" << strerror(-ret) << ")" << std::endl;
              usb_close(_dev);
              _dev=0;
              return ret;
            }
          }
          _init=true;
          if(_debug) std::cerr << "Testing USB send" << std::endl;
          ret=send(0x0);
          if(ret<0)
          {
            std::cerr << "send(0x0) failed with code " << ret
                      << " (" << strerror(-ret) << ")"
                      << std::endl;
            return ret;
          }
          if(_debug) std::cerr << "Testing USB read" << std::endl;
          ret=read(0);
          if(ret<0)
          {
            std::cerr << "read() failed with code " << ret
                      << " (" << strerror(-ret) << ")"
                      << std::endl;
            return ret;
          }
          return 0;
        }
  int close()
        {
          if(!_init) return 0;
          int ret = usb_release_interface(_dev,_interface);
          if(ret<0)
          {
            std::cerr << "usb_release_interface failed with code " << ret
                      << " (" << strerror(-ret) << ")" << std::endl;
            return ret;
          }
          usb_close(_dev);
          if(ret<0) return ret;
          _dev=0;
          return ret;
        }
  int send( char msg)
        {
          if(!_dev) return -1;
          const int bufsize=8;
          static char buf[bufsize];
          memset(buf, 0, bufsize);
          buf[0] = msg;
          int ret=usb_control_msg(_dev,_send_cmd.RequestType,_send_cmd.Request,
                                  _send_cmd.Value,_send_cmd.Index,buf,bufsize,
                                  _send_cmd.Timeout);
          if(ret<0)
          {
            std::cerr << "usb_control_msg failed with code " << ret
                      << " (" << strerror(-ret) << ")" << std::endl;
            return ret;
          }
          return ret;
        }
  int read( char* status)
        {
          if(!_dev) return -1;
          int ret = send(_statusMsg);
          if(ret<0) return ret;
          char tmp;
          ret = usb_interrupt_read(_dev,_recv_cmd.Endpoint,&tmp,1,
                                   _recv_cmd.Timeout);
          if(ret<0)
          {
            std::cerr << "usb_interrupt_read failed with code " << ret
                      << " (" << strerror(-ret) << ")" << std::endl;
            return ret;
          }
          if( status) *status = tmp;
          return ret;
        }
  void setDebug(bool debug) {_debug=debug;}
private:
  usb_dev_handle* _dev;
  int             _interface;
  int             _vendor;
  int             _product;
  char            _statusMsg;
  bool            _debug;
  bool            _init;
  
  SendCmd _send_cmd;
  RecvCmd _recv_cmd;
};

#endif

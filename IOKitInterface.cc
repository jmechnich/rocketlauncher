#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/USBSpec.h>

#include <iostream>

#include "IOKitInterface.hh"

IOReturn IOKitInterface::open()
{
  IOReturn ret=0;
  if(!_send_cmd.RequestType) {
    std::cerr << "send commands not initialized" << std::endl;
    return -1;
  }
  if(!_recv_cmd.Endpoint) {
    std::cerr << "recv commands not initialized" << std::endl;
    return -1;
  }
  if(_debug) std::cerr << "Opening device" << std::endl;
  ret=openDevice();
  if(ret) return print_error("openDevice()",ret);
  if(_debug) std::cerr << "Setting configuration" << std::endl;
  ret=setConfiguration();
  if(ret) return print_error("setConfiguration()",ret);
  if(_debug) std::cerr << "Opening interface" << std::endl;
  ret=openInterface();
  if(ret) return print_error("openInterface()",ret);

  // if(_debug) std::cerr << "Checking pipe IN" << std::endl;
  // ret=checkPipe(1);
  // if(ret) return print_error("checkPipe(1)",ret);

  // if(_debug) std::cerr << "Resetting pipe IN" << std::endl;
  // ret=resetPipe(1);
  // if(ret) return print_error("resetPipe(1)",ret);

  if(_debug) std::cerr << "Testing USB send" << std::endl;
  if(_debug) std::cerr << "print_settings" << std::endl;
  ret=print_settings();
  if(ret) return print_error("print_settings",ret);

  ret=send(MSG_STOP);
  if(ret) return print_error("send(MSG_STOP)",ret);
  ret=send(MSG_STOP);
  if(ret) return print_error("send(MSG_STOP)",ret);

  if(_debug) std::cerr << "Testing USB read" << std::endl;
  ret=read(0);
  if(ret) return print_error("read()",ret);
  return 0;
}

IOReturn IOKitInterface::openDevice()
{
  if(_dev) return -1;
  IOReturn ret=0;
  // try finding device
  CFMutableDictionaryRef matchingDictionary =
      IOServiceMatching(kIOUSBDeviceClassName);
  SInt32 idVendor  = _vendor;
  SInt32 idProduct = _product;
  CFDictionaryAddValue(
      matchingDictionary, CFSTR(kUSBVendorID),
      CFNumberCreate(kCFAllocatorDefault,kCFNumberSInt32Type,&idVendor));
  CFDictionaryAddValue(
      matchingDictionary, CFSTR(kUSBProductID),
      CFNumberCreate(kCFAllocatorDefault,kCFNumberSInt32Type,&idProduct));
  io_iterator_t iterator = 0;
  IOServiceGetMatchingServices(
      kIOMasterPortDefault,matchingDictionary, &iterator);
  io_service_t usbRef = IOIteratorNext(iterator);
  IOObjectRelease(iterator);
  if(!usbRef) {
    std::cerr << "Device not found" << std::endl;
    return kIOReturnNoDevice;
  }
  // try opening device
  SInt32 score;
  IOCFPlugInInterface** plugin;
  IOCreatePlugInInterfaceForService(
      usbRef, kIOUSBDeviceUserClientTypeID,
      kIOCFPlugInInterfaceID, &plugin, &score);
  IOObjectRelease(usbRef);
  (*plugin)->QueryInterface(
      plugin, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID300),
      (LPVOID*)&_dev);
  (*plugin)->Release(plugin);
  if(!_dev) {
    if(_debug) {
      std::cerr << "Could not retrieve device" << std::endl;
    }
    return kIOReturnNoDevice;
  }
  ret=(*_dev)->USBDeviceOpen(_dev);
  if(ret==kIOReturnSuccess) {
    if(_debug) {
      std::cerr << "Got access to device" << std::endl;
    }
  }
  else if(ret==kIOReturnExclusiveAccess) {
    if(_debug) {
      std::cerr << "Got exclusive access to device" << std::endl;
    }
  }
  else {
            
    (*_dev)->Release(_dev);
    return print_error("USBDeviceOpen",ret);
  }
  // // try setting configuration
  // IOUSBConfigurationDescriptorPtr config;
  // // set first configuration as active
  // ret = (*_dev)->GetConfigurationDescriptorPtr(_dev, 0, &config);
  // if(ret != kIOReturnSuccess) {
  //   return print_error("GetConfigurationDescriptorPtr",ret);
  // }
  // ret = (*_dev)->SetConfiguration(_dev,config->bConfigurationValue);
  // if(ret != kIOReturnSuccess) {
  //   return print_error("SetConfiguration",ret);
  // }
  return ret;
}
    
IOReturn IOKitInterface::openInterface()
{
  if(_interface) return -1;
  IOReturn ret=0;
  // try finding interface
  IOUSBFindInterfaceRequest interfaceRequest;
  interfaceRequest.bInterfaceClass = kIOUSBFindInterfaceDontCare;
  interfaceRequest.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
  interfaceRequest.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
  interfaceRequest.bAlternateSetting = kIOUSBFindInterfaceDontCare;
  io_iterator_t iterator = 0;
  (*_dev)->CreateInterfaceIterator(_dev, &interfaceRequest, &iterator);
  io_service_t usbRef = IOIteratorNext(iterator);
  IOObjectRelease(iterator);
  SInt32 score;
  IOCFPlugInInterface** plugin;
  IOCreatePlugInInterfaceForService(
      usbRef,kIOUSBInterfaceUserClientTypeID,kIOCFPlugInInterfaceID,
      &plugin, &score);
  IOObjectRelease(usbRef);
  (*plugin)->QueryInterface(
      plugin, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID300),
      (LPVOID*)&_interface);
  (*plugin)->Release(plugin);
  if(!_interface) {
    std::cerr << "Could not retrieve interface" << std::endl;
    return -1;
  }
  // try opening interface
  ret = (*_interface)->USBInterfaceOpen(_interface);
  if(ret != kIOReturnSuccess) {
    (*_interface)->Release(_interface);
    return print_error("USBInterfaceOpen",ret);
  }
  return ret;
}

IOReturn IOKitInterface::setConfiguration() 
{
  if(!_dev || _interface) return kIOReturnNoDevice;
  IOReturn ret=0;
  UInt8 nconf;
  ret=(*_dev)->GetNumberOfConfigurations(_dev, &nconf);
  if(ret!=kIOReturnSuccess) {
    return print_error("GetNumberOfConfigurations",ret);
  }
  UInt8 c=0;
  for(int i=0; i< nconf; ++i)
  {
    std::cerr << "Configuration " << i << ":" << std::endl;
    IOUSBConfigurationDescriptorPtr desc;
    ret=(*_dev)->GetConfigurationDescriptorPtr(_dev,i,&desc);
    if(ret!=kIOReturnSuccess)
        return print_error("GetConfigurationDescriptorPtr",ret);
    std::cerr << std::hex;
    std::cerr << "bLength:             " << int(desc->bLength)
              << std::endl;
    std::cerr << "bDescriptorType:     " << int(desc->bDescriptorType)
              << std::endl;
    std::cerr << "wTotalLength:        " << desc->wTotalLength
              << std::endl;
    std::cerr << "bNumInterfaces:      "
              << int(desc->bNumInterfaces) << std::endl;
    std::cerr << "bConfigurationValue: "
              << int(desc->bConfigurationValue) << std::endl;
    std::cerr << "iConfiguration:      "
              << int(desc->iConfiguration) << std::endl;
    std::cerr << "bmAttributes:        "
              << int(desc->bmAttributes) << std::endl;
    std::cerr << "MaxPower:            "
              << int(desc->MaxPower) << std::endl;
    std::cerr << std::dec;
    c=desc->bConfigurationValue;
  }
  if(_debug) std::cerr << "Selected Configuration: " << int(c) << std::endl;
  ret=(*_dev)->SetConfiguration(_dev,c);
  if(ret!=kIOReturnSuccess)
      return print_error("SetConfiguration",ret);
  return ret;
}
  
IOReturn IOKitInterface::close()
{
  if(_interface) {
    (*_interface)->USBInterfaceClose(_interface);
    (*_interface)->Release(_interface);
    _interface=0;
  }
  if(_dev) {
    (*_dev)->USBDeviceClose(_dev);
    (*_dev)->Release(_dev);
    _dev=0;
  }
          
  return 0;
}

IOReturn IOKitInterface::send( char msg)
{
  if(!_dev) return -1;
  const int bufsize=8;
  UInt8 buf[bufsize];
  buf[0]=msg;
  IOUSBDevRequest request;
  //request.bmRequestType = 0x21;
  request.bmRequestType=USBmakebmRequestType(kUSBOut, kUSBClass, kUSBInterface); 
  request.bRequest=0x09;
  //request.wValue  =0x1;
  request.wValue  =0x200;
  request.wIndex  =0;
  request.wLength =bufsize;
  request.pData   =buf;
  IOReturn ret=(*_dev)->DeviceRequest(_dev,&request);
  return ret;
}

IOReturn IOKitInterface::print_settings()
{
  IOReturn ret = 0;
  if(!_interface)
      return print_error("print_settings",kIOReturnNoDevice);
  UInt8 alt=0;
  ret=(*_interface)->GetAlternateSetting(_interface,&alt);
  std::cerr << "Alternate setting " << int(alt) << std::endl;
  UInt8 nep=0;
  ret=(*_interface)->GetNumEndpoints(_interface,&nep);
  std::cerr << "Number of EP      " << int(nep) << std::endl;
  // Information   Pipe   EP   Description
  // direction        1    1   0 OUT, 1 IN
  // number           1    -   
  // Transfer Type    3    3   0 Control, 1 Iso, 2 Bulk, 3 Interrupt
  // Max Packet Size  1    1
  // Interval        20   20   in ms
  for(UInt8 i=1;i<=nep;++i)
  {
    std:: cerr << " EP " << int(i) << std::endl;
    UInt8 direction=0;
    UInt8 number=0;
    UInt8 transferType=0;
    UInt16 maxPacketSize=0;
    UInt8 interval=0;
    ret = (*_interface)->GetPipeProperties(
        _interface,nep, &direction,&number, &transferType,
        &maxPacketSize, &interval);
    if(ret != kIOReturnSuccess) return ret;
    std::cerr << " Pipe properties " << int(ret) << std::endl;
    std::cerr << " Direction       " << int(direction) << std::endl;
    std::cerr << " Number          " << int(number) << std::endl;
    std::cerr << " Transfer Type   " << int(transferType) << std::endl;
    std::cerr << " Max Packet Size " << int(maxPacketSize) << std::endl;
    std::cerr << " Interval        " << int(interval) << std::endl;
    ret = (*_interface)->GetEndpointProperties(
        _interface, alt, i, direction, &transferType, &maxPacketSize, &interval);
    if(ret != kIOReturnSuccess) return ret;
    std::cerr << " EP properties   " << int(ret) << std::endl;
    std::cerr << " Transfer Type   " << int(transferType) << std::endl;
    std::cerr << " Max Packet Size " << int(maxPacketSize) << std::endl;
    std::cerr << " Interval        " << int(interval) << std::endl;
  }
  return ret;
}
  
IOReturn IOKitInterface::resetPipe(UInt8 pipeRef)
{
  IOReturn ret=0; 
  // std::cerr << "AbortPipe " << ret << std::endl;
  // ret = (*_interface)->AbortPipe(_interface,pipeRef);
  // if(ret!=kIOReturnSuccess) return ret;
  std::cerr << "SetPipePolicy " << ret << std::endl;
  ret = (*_interface)->SetPipePolicy(_interface,pipeRef,1,2);
  if(ret!=kIOReturnSuccess) return print_error("SetPipePolicy",ret);
  std::cerr << "ResetPipe " << ret << std::endl;
  ret = (*_interface)->ResetPipe(_interface,pipeRef);
  if(ret!=kIOReturnSuccess) return print_error("ResetPipe",ret);
  return ret;
}

IOReturn IOKitInterface::checkPipe(UInt8 pipeRef)
{
  IOReturn ret=0;
  std::cerr << "GetPipeStatus " << ret << std::endl;
  ret = (*_interface)->GetPipeStatus(_interface,pipeRef);
  if(ret==kIOReturnSuccess) {
  }
  else if(ret==kIOUSBPipeStalled) {
            
    std::cerr << "Clear pipe both ends" << ret << std::endl;
    ret = (*_interface)->ClearPipeStallBothEnds(_interface,pipeRef);
    if(ret) return print_error("ClearPipeStallBothEnds",ret);
  }
  else {
    return print_error("GetPipeStatus",ret);
  }
  return ret;
}
  
IOReturn IOKitInterface::read( char* status)
{
  if(!_interface) return -1;
  UInt8 pipeRef = 1;
  IOReturn ret=0;
  ret = send(MSG_STATUS);
  if(ret!=kIOReturnSuccess) return print_error("send",ret);
  const UInt32 bufsize=1;
  unsigned char buf[bufsize];
  UInt32 actual_xfer=bufsize;
  if(_debug) std::cerr << "Run loop" << std::endl;
  bool returnAfterSourceHandled = false;
  CFTimeInterval seconds = 100;
  CFStringRef mode = kCFRunLoopDefaultMode;
  CFRunLoopRunInMode(mode, seconds, returnAfterSourceHandled);  
  if(_debug) std::cerr << "Reading from pipe" << ret << std::endl;
  ret = (*_interface)->ReadPipe(_interface,pipeRef,&buf,&actual_xfer);
  if(ret!=kIOReturnSuccess) return print_error("ReadPipe",ret);
  if(actual_xfer != bufsize)
      std::cerr << "Read " << actual_xfer << "/" << bufsize
                << " bytes" << std::endl;
  if(status) *status = *buf;
  return ret;
}

IOReturn IOKitInterface::print_error( const std::string& name, IOReturn code)
{
  std::cerr << name << " failed with code " << code 
            << " (" << str_return(code) << ")" << std::endl;
  return code;
}

const char* IOKitInterface::str_return(IOReturn err)
{
  switch(err)
  {
  case(kIOReturnSuccess):
    return "kIOReturnSuccess";
  case(kIOReturnError):
    return "kIOReturnError";
  case(kIOReturnNoMemory):
    return "kIOReturnNoMemory";
  case(kIOReturnNoResources):
    return "kIOReturnNoResources";
  case(kIOReturnIPCError):
    return "kIOReturnIPCError";
  case(kIOReturnNoDevice):
    return "kIOReturnNoDevice";
  case(kIOReturnNotPrivileged):
    return "kIOReturnNotPrivileged";
  case(kIOReturnBadArgument):
    return "kIOReturnBadArgument";
  case(kIOReturnLockedRead):
    return "kIOReturnLockedRead";
  case(kIOReturnLockedWrite):
    return "kIOReturnLockedWrite";
  case(kIOReturnExclusiveAccess):
    return "kIOReturnExclusiveAccess";
  case(kIOReturnBadMessageID):
    return "kIOReturnBadMessageID";
  case(kIOReturnUnsupported):
    return "kIOReturnUnsupported";
  case(kIOReturnVMError):
    return "kIOReturnVMError";
  case(kIOReturnInternalError):
    return "kIOReturnInternalError";
  case(kIOReturnIOError):
    return "kIOReturnIOError";
  case(kIOReturnCannotLock):
    return "kIOReturnCannotLock";
  case(kIOReturnNotOpen):
    return "kIOReturnNotOpen";
  case(kIOReturnNotReadable):
    return "kIOReturnNotReadable";
  case(kIOReturnNotWritable):
    return "kIOReturnNotWritable";
  case(kIOReturnNotAligned):
    return "kIOReturnNotAligned";
  case(kIOReturnBadMedia):
    return "kIOReturnBadMedia";
  case(kIOReturnStillOpen):
    return "kIOReturnStillOpen";
  case(kIOReturnRLDError):
    return "kIOReturnRLDError";
  case(kIOReturnDMAError):
    return "kIOReturnDMAError";
  case(kIOReturnBusy):
    return "kIOReturnBusy";
  case(kIOReturnTimeout):
    return "kIOReturnTimeout";
  case(kIOReturnOffline):
    return "kIOReturnOffline";
  case(kIOReturnNotReady):
    return "kIOReturnNotReady";
  case(kIOReturnNotAttached):
    return "kIOReturnNotAttached";
  case(kIOReturnNoChannels):
    return "kIOReturnNoChannels";
  case(kIOReturnNoSpace):
    return "kIOReturnNoSpace";
  case(kIOReturnPortExists):
    return "kIOReturnPortExists";
  case(kIOReturnCannotWire):
    return "kIOReturnCannotWire";
  case(kIOReturnNoInterrupt):
    return "kIOReturnNoInterrupt";
  case(kIOReturnNoFrames):
    return "kIOReturnNoFrames";
  case(kIOReturnMessageTooLarge):
    return "kIOReturnMessageTooLarge";
  case(kIOReturnNotPermitted):
    return "kIOReturnNotPermitted";
  case(kIOReturnNoPower):
    return "kIOReturnNoPower";
  case(kIOReturnNoMedia):
    return "kIOReturnNoMedia";
  case(kIOReturnUnformattedMedia):
    return "kIOReturnUnformattedMedia";
  case(kIOReturnUnsupportedMode):
    return "kIOReturnUnsupportedMode";
  case(kIOReturnUnderrun):
    return "kIOReturnUnderrun";
  case(kIOReturnOverrun):
    return "kIOReturnOverrun";
  case(kIOReturnDeviceError):
    return "kIOReturnDeviceError";
  case(kIOReturnNoCompletion):
    return "kIOReturnNoCompletion";
  case(kIOReturnAborted):
    return "kIOReturnAborted";
  case(kIOReturnNoBandwidth):
    return "kIOReturnNoBandwidth";
  case(kIOReturnNotResponding):
    return "kIOReturnNotResponding";
  case(kIOReturnIsoTooOld):
    return "kIOReturnIsoTooOld";
  case(kIOReturnIsoTooNew):
    return "kIOReturnIsoTooNew";
  case(kIOReturnNotFound):
    return "kIOReturnNotFound";
  case(kIOReturnInvalid):
    return "kIOReturnInvalid";
  }
  return "";
}

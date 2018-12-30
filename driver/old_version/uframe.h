#ifndef __UFRAME__
#define __UFRAME__

#define  DEVICE_NAME "uframe"

#define TYPE_CONTROL 0
#define TYPE_BULK 1
#define TYPE_INTERRUPT 2

#define DIR_IN 0
#define DIR_OUT 1

struct uframe_dev {
    struct uframe_endpoint *endpoints;
    struct usb_device *udev;
    struct usb_interface *interface ;
    int endpoint_count;
    int minor;
    int VID;
    int PID;
};

static struct usb_device_id usb_table [] ={	{USB_DEVICE(0x16c0, 0x04dc)},	{USB_DEVICE(0x16c0, 0x05dc)},{}};

static int devCnt = 2;

static struct uframe_dev uframe_devices [2] = {{.VID = 0x16c0, .PID = 0x04dc}, {.VID = 0x16c0, .PID = 0x05dc}, };

struct uframe_endpoint {
    //order is important -> ioctl return all descriptors
    int type; 
    int dir;
    int endpointaddr;
    int interval;
    int buffer_size; // if input determine the size
    char * data;
    struct kref kref; 
    struct cdev cdev;
};

void uframe_delete(struct kref *);

#endif
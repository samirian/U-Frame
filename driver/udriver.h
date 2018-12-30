#ifndef __UFRAME_CHAR__
#define __UFRAME_CHAR__

#define IOCTL_INTERRUPT_INTERVAL 0
#define IOCTL_CONTROL_READ 1
#define IOCTL_ENDPOINTS_DESC 3

#define  DEVICE_NAME "uframe"

#define TYPE_CONTROL 0
#define TYPE_BULK 1
#define TYPE_INTERRUPT 2

#define DIR_IN 0
#define DIR_OUT 1

/*file operations*/
ssize_t uframe_write(struct file *, const char __user *, size_t , loff_t *);
ssize_t uframe_read(struct file *, char __user *, size_t , loff_t *);
int uframe_release(struct inode *node, struct file *filp);
int uframe_open(struct inode *node, struct file *filp);
long uframe_ioctl(struct file *f, unsigned int cmd, unsigned long arg);

/*usb operations*/
void uframe_delete(struct kref *);
static int uframe_probe(struct usb_interface *interface, const struct usb_device_id *id);
static void uframe_disconnect(struct usb_interface *interface);
static int findDeviceIndexByMinor(int minor);
static int findDeviceIndexByDevice(struct usb_device *device);
static void setup_cdev(struct cdev * ,int );

static int uframe_major;
static int uframe_minor;

static int devices_count = 0;

dev_t uframe_devno;

struct __endpoint_desc {
    int type; 
    int dir;
    int endpointaddr;
    int interval;
    int buffer_size; 
};

struct control_params {
    uint8_t 	request;
    uint8_t 	request_type;
    uint16_t 	value;
    uint16_t 	index;
    uint16_t 	size;
};

struct uframe_dev {
    struct 	uframe_endpoint *endpoints;
    struct 	usb_device *udev;
    struct 	usb_interface *interface ;
    int 	endpoint_count;
    int 	minor;
    int 	VID;
    int 	PID;
};

struct uframe_endpoint {
    //order is important -> ioctl return all descriptors
    int 	type; 
    int 	dir;
    int 	endpointaddr;
    int 	interval;
    int 	buffer_size; // if input determine the size
    char 	*data;
    struct 	kref kref; 
    struct 	cdev cdev;
};

static struct usb_device_id usb_table [] ={	{USB_DEVICE(0x16c0, 0x04dc)},	{USB_DEVICE(0x16c0, 0x05dc)},{}};

static int devCnt = 2;

static struct uframe_dev uframe_devices [2] = {{.VID = 0x16c0, .PID = 0x04dc}, {.VID = 0x16c0, .PID = 0x05dc}, };

#endif
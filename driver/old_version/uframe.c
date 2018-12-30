#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/uaccess.h>          // Required for the copy to user function
#include <linux/cdev.h>
#include <linux/usb.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include "uframe.h"
#include "uframechar.h"

 
MODULE_LICENSE("GPL");          
MODULE_AUTHOR("Mahmoud Nagy"); 
MODULE_DESCRIPTION("A generic kernel driver for UFrame"); 
MODULE_VERSION("0.5");     


int uframe_major;
int uframe_minor;

/*
struct uframeDeviceDiscriptor uframe_dev_dis[] = {

}
*/
static int devices_count = 0;

dev_t uframe_devno;

static int findDeviceIndexByDevice(struct usb_device *device);
static int uframe_probe(struct usb_interface *interface, const struct usb_device_id *id);
static void uframe_disconnect(struct usb_interface *interface);
    
static void setup_cdev(struct cdev * ,int );

struct file_operations uframe_fops = 
{
    .owner          = THIS_MODULE,
    .read           = uframe_read,
    .write          = uframe_write,
    .open           = uframe_open,
    .release        = uframe_release,
    .unlocked_ioctl = uframe_ioctl,
};

static struct usb_driver uframe_driver =
{
    .name       = DEVICE_NAME,
    .id_table   = usb_table,
    .probe      = uframe_probe,
    .disconnect = uframe_disconnect,
};

void uframe_delete(struct kref *krf)
{	
    struct uframe_endpoint *ep = container_of(krf, struct uframe_endpoint, kref);
    kfree (ep->data); //free data allocated
}

static int __init uframe_init(void)
{
    int result;
    int i;

    result = usb_register(&uframe_driver);
    if(result)
	   pr_err("%s: Can't register driver, errno %d\n", DEVICE_NAME, result);

    result = alloc_chrdev_region(&uframe_devno
                                    , 0
                                    , devCnt
                                    , DEVICE_NAME);

    if(result < 0)
    {
        printk(KERN_WARNING "%s: can't allocate major number\n", DEVICE_NAME);
        return result;
    }

    uframe_major = MAJOR(uframe_devno); //major fixed for all devices
    uframe_minor = MINOR(uframe_devno); //first minor in the requested range
    printk(KERN_WARNING "%s: Major : %d, first Minor :%d\n", DEVICE_NAME, uframe_major, uframe_minor);

    //giving minor for each device
    for( i = 0; i<devCnt ; i++){
        uframe_devices[i].minor = uframe_minor + 1;
        uframe_devices[i].endpoint_count = 0; //initialize it to zero
    }
    
    return 0;
}

//good
static void __exit uframe_exit(void)
{  
    unregister_chrdev_region(uframe_devno, devCnt);
    usb_deregister(&uframe_driver);
}

static int uframe_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    struct usb_host_interface *iface_desc;
    struct usb_endpoint_descriptor *endpoint;
    int i;
    int index;
    int retval;
    
    devices_count++;
    printk(KERN_ALERT "%s: USB CONNECTED, %d connected devices\n", DEVICE_NAME, devices_count);

    index = findDeviceIndexByDevice(usb_get_dev(interface_to_usbdev(interface)));

    printk(KERN_ALERT "%s: index %d\n", DEVICE_NAME, index);
    if(index < 0){
        printk(KERN_ALERT "%s: can not get index in probe\n", DEVICE_NAME);
        return -1;
    }
    iface_desc = interface->cur_altsetting;

    uframe_devices[index].endpoint_count = iface_desc->desc.bNumEndpoints + 1 ; // don't forget there is also the control ep
    uframe_devices[index].interface = interface;
    uframe_devices[index].udev = usb_get_dev(interface_to_usbdev(interface));
    uframe_devices[index].endpoints = kmalloc(sizeof(struct uframe_endpoint) * uframe_devices[index].endpoint_count, GFP_KERNEL);

    if(!uframe_devices[index].endpoints)
    {
    	printk(KERN_ERR"%s: Could not allocate uframe_devs\n", DEVICE_NAME);
    	retval = -ENOMEM;
    	goto error;
    }

    memset(uframe_devices[index].endpoints, 0, sizeof(struct uframe_endpoint) * uframe_devices[index].endpoint_count);

    uframe_devices[index].endpoints[0].type = TYPE_CONTROL;
    uframe_devices[index].endpoints[0].endpointaddr = 0;
    
    setup_cdev(&uframe_devices[index].endpoints[0].cdev, uframe_devices[index].minor); // register the control endpoint
    // start from 1 since ep 0 is control
    for (i = 1; i < iface_desc->desc.bNumEndpoints+1; ++i)
    {
        //register the rest of the endpoints
    	endpoint = &iface_desc->endpoint[i-1].desc; // ep starts from 0 
    	kref_init(&uframe_devices[index].endpoints[i].kref);
    	uframe_devices[index].endpoints[i].endpointaddr = endpoint->bEndpointAddress; // set it for later
    	if(endpoint->bEndpointAddress & USB_DIR_IN)
    	{//in
    	    uframe_devices[index].endpoints[i].dir = DIR_IN;
    	    uframe_devices[index].endpoints[i].buffer_size = endpoint->wMaxPacketSize;
    	    uframe_devices[index].endpoints[i].interval = endpoint->bInterval;
    	    uframe_devices[index].endpoints[i].data = kmalloc(uframe_devices[index].endpoints[i].buffer_size,GFP_KERNEL);
    	    if (!uframe_devices[index].endpoints[i].data)
    	    {
        		printk(KERN_ERR"%s: Could not allocate data attribute\n",DEVICE_NAME);
        		retval = -ENOMEM;
        		goto error;
    	    }
    	}
    	else //out
    	    uframe_devices[index].endpoints[i].dir = DIR_OUT;

    	switch (endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
    	{
        	case USB_ENDPOINT_XFER_BULK:
        	    uframe_devices[index].endpoints[i].type = TYPE_BULK;
        	    break;
        	case USB_ENDPOINT_XFER_CONTROL:
        	    uframe_devices[index].endpoints[i].type = TYPE_CONTROL;
        	    break;
        	case USB_ENDPOINT_XFER_INT:
        	    uframe_devices[index].endpoints[i].type = TYPE_INTERRUPT;
        	    break;
        	default:
        	    printk(KERN_INFO"%s: This type not supported \n", DEVICE_NAME); 
    	}
    	printk(KERN_INFO "%s: Detected endpoint dir:%d type:%d\n"
                ,DEVICE_NAME
                ,uframe_devices[index].endpoints[i].dir
                ,uframe_devices[index].endpoints[i].type);
    	setup_cdev(&uframe_devices[index].endpoints[i].cdev, uframe_minor + i); // setup corresponding cdev
    }
    
    return 0;

error:
    if (uframe_devices[index].endpoints)
    	for(i = 0 ; i < uframe_devices[index].endpoint_count ; i++)
    	    kref_put(&uframe_devices[index].endpoints[i].kref, uframe_delete);
    kfree(uframe_devices[index].endpoints);
    usb_put_dev(uframe_devices[index].udev);
    return retval; 
}

static void uframe_disconnect(struct usb_interface *interface)
{
    int i;
    int index;
    devices_count--;
    printk(KERN_ALERT "%s: USB drive removed, %d connected devices.\n",DEVICE_NAME, devices_count);

    index = findDeviceIndexByDevice(usb_get_dev(interface_to_usbdev(interface)));
    
    if(index< 0){
        printk(KERN_ALERT "%s: can not get index in disconnect\n",DEVICE_NAME);
    }
    if(uframe_devices[index].endpoints)
	for(i=0; i < uframe_devices[index].endpoint_count; i++)
	{
	    cdev_del(&uframe_devices[index].endpoints[i].cdev);
	    kref_put(&uframe_devices[index].endpoints[i].kref, uframe_delete);
	}
    
    usb_put_dev(uframe_devices[index].udev);
    kfree(uframe_devices[index].endpoints);
    uframe_devices[index].endpoint_count = 0;
}

static void setup_cdev(struct cdev *dev ,int minor)
{
    int err,devno = MKDEV(uframe_major, minor);
    cdev_init(dev,&uframe_fops);
    dev->ops = &uframe_fops;
    dev->owner = THIS_MODULE;
    err = cdev_add(dev, devno, 1);
    if(err)
	   printk(KERN_NOTICE "%s: Error %d adding node%d", DEVICE_NAME, err, minor);
}

int findDeviceIndexByDevice(struct usb_device *device){
    int i;
    printk(KERN_ALERT"findDeviceIndexByDevice");
    for(i = 0 ; i < devCnt ;i++){
        printk(KERN_ALERT" %d", i);
        if(uframe_devices[i].VID == device->descriptor.idVendor && uframe_devices[i].PID == device->descriptor.idProduct)
            return i;
    }
    return -1;
}

module_init(uframe_init);
module_exit(uframe_exit);

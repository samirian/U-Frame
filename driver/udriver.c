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
#include <linux/types.h>
#include <linux/time.h>
#include "udriver.h"

 
MODULE_LICENSE("GPL");          
MODULE_AUTHOR("Mahmoud Nagy"); 
MODULE_DESCRIPTION("A generic kernel driver for UFrame"); 
MODULE_VERSION("0.5");     


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

//good
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

    //giving minor for each device
    for( i = 0; i<devCnt ; i++){
        uframe_devices[i].minor = uframe_minor + i;
        printk(KERN_WARNING "%s: device : %d, Minor :%d\n", DEVICE_NAME, i, uframe_devices[i].minor);
        uframe_devices[i].endpoint_count = 0; //initialize it to zero
    }
    
    return 0;
}

static void __exit uframe_exit(void)
{  
    printk(KERN_WARNING"%s : life is not fair!.", DEVICE_NAME);
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
    printk(KERN_WARNING "%s: USB CONNECTED, %d connected devices\n", DEVICE_NAME, devices_count);

    index = findDeviceIndexByDevice(usb_get_dev(interface_to_usbdev(interface)));

    if(index < 0){
        printk(KERN_WARNING "%s: can not get index in probe\n", DEVICE_NAME);
        return -1;
    }

    printk(KERN_WARNING "%s: minor : %d\n",DEVICE_NAME, uframe_devices[index].minor);
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
    printk(KERN_WARNING "%s: USB drive removed, %d connected devices.\n",DEVICE_NAME, devices_count);

    index = findDeviceIndexByDevice(usb_get_dev(interface_to_usbdev(interface)));
    
    if(index< 0){
        printk(KERN_WARNING "%s: can not get index in disconnect\n",DEVICE_NAME);
    }else
        printk(KERN_WARNING "%s: minor : %d\n",DEVICE_NAME, uframe_devices[index].minor);
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

module_init(uframe_init);
module_exit(uframe_exit);

int uframe_open(struct inode *inode, struct file *filp)
{
    struct uframe_endpoint *endpoint;
    endpoint = container_of(inode->i_cdev, struct uframe_endpoint, cdev);
    
    if(endpoint->type != TYPE_CONTROL && endpoint->dir == DIR_OUT && (filp->f_flags & O_ACCMODE) != O_WRONLY) 
        return -EACCES;
    else if(endpoint->type != TYPE_CONTROL && endpoint->dir == DIR_IN && (filp->f_flags & O_ACCMODE) != O_RDONLY)
        return -EACCES;
    
    filp->private_data = endpoint; // for other methods
    kref_get(&endpoint->kref);
    return 0;
}

int uframe_release(struct inode *inode, struct file *filp)
{
    struct uframe_endpoint *endpoint;
    endpoint = container_of(inode->i_cdev, struct uframe_endpoint, cdev);
    kref_put(&endpoint->kref,uframe_delete);
    return 0;
}

ssize_t uframe_read(struct file *filp, char __user *u_buffer, size_t count, loff_t *offp)
{
    struct uframe_endpoint *endpoint;
    int retval = 0;
    int d_size;
    int minor;
    int index;

    endpoint = filp->private_data;

    minor = MINOR(filp->f_path.dentry->d_inode->i_rdev);
    printk( "reading on minor number %d\n", minor);

    index = findDeviceIndexByMinor(minor);

    printk(KERN_INFO"%s: device : READ From endpoint %d, type %d, dir %d\n",DEVICE_NAME, endpoint->endpointaddr, endpoint->type, endpoint->dir);

    if(!count)
        return 0;
    printk(KERN_INFO "%s: device : --------------starting transfer--------------\n", DEVICE_NAME);
    d_size = min(endpoint->buffer_size ,(int) count);
    switch(endpoint->type)
    {
        case TYPE_CONTROL:
            break;
        case TYPE_BULK:
            retval = usb_bulk_msg(uframe_devices[index].udev
                                    ,usb_rcvbulkpipe(uframe_devices[index].udev, endpoint->endpointaddr)
                                    ,endpoint->data
                                    ,d_size
                                    ,(int *) &count, HZ*10);
            break;
        case TYPE_INTERRUPT:
            retval = usb_interrupt_msg(uframe_devices[index].udev
                                        ,usb_rcvintpipe(uframe_devices[index].udev
                                        ,endpoint->endpointaddr)
                                        ,endpoint->data
                                        ,d_size
                                        ,(int *) &count, HZ*10);
            break;
    }

    printk(KERN_INFO "%s:  device : read Data %s\n",DEVICE_NAME, endpoint->data);
    /* if the read was successful, copy the data to userspace */
    if (!retval)
    {
        printk(KERN_INFO "%s: device : Copying buffer to user\n",DEVICE_NAME);
        if (copy_to_user(u_buffer, endpoint->data, d_size))
        {
            printk(KERN_ERR"%s: device : Error Copying data to user\n",DEVICE_NAME);
            retval = -EFAULT;
        }
        else
            retval = d_size;
    }
    printk(KERN_INFO "%s: device : --------------done transfer--------------\n", DEVICE_NAME);
    return retval;

}

ssize_t uframe_write(struct file *filp, const char __user *u_buffer, size_t frame_len, loff_t *offp)
{
    struct uframe_endpoint *endpoint;
    int retval;
    char *k_buffer;
    char *d_buffer;
    struct control_params *cparams;
    size_t offset;
    size_t i;
    int index;
    int minor;
    
    k_buffer = kmalloc(frame_len,GFP_KERNEL);
    retval = 0;
    endpoint = filp->private_data;   

    minor = MINOR(filp->f_path.dentry->d_inode->i_rdev);
    printk( "writing to minor number %d\n", minor);

    index = findDeviceIndexByMinor(minor);

    printk(KERN_INFO"%s: device : WRITE From endpoint %d, type %d, dir %d\n",DEVICE_NAME, endpoint->endpointaddr, endpoint->type, endpoint->dir);
    printk(KERN_INFO "%s: device : --------------starting transfer--------------\n", DEVICE_NAME);
    if (!frame_len)
        return 0;
    
    if (copy_from_user(k_buffer, u_buffer, frame_len))
        return -EFAULT;
        
    switch(endpoint->type)
    {
        case TYPE_CONTROL:
            printk(KERN_INFO "%s: device : control type\n", DEVICE_NAME);
            cparams = (struct control_params *) u_buffer;

            printk(KERN_INFO"%s: device : request %d request_type %d value %d index %d size %d",DEVICE_NAME, 
                   cparams->request, cparams->request_type, cparams->value, cparams->index, cparams->size);

            d_buffer = kmalloc(cparams->size,GFP_KERNEL);
            offset = sizeof(cparams);
            for(i = 0 ; i < cparams->size ; i++){
                d_buffer[i] = k_buffer[i + offset];
            }

            retval = usb_control_msg(uframe_devices[index].udev
                                        ,usb_sndctrlpipe(uframe_devices[index].udev,endpoint->endpointaddr)
                                        ,cparams->request
                                        ,cparams->request_type
                                        ,cparams->value
                                        ,cparams->index
                                        ,d_buffer
                                        ,cparams->size
                                        ,0);
            break;
        case TYPE_BULK:
            printk(KERN_INFO "%s: device : bulk type\n", DEVICE_NAME);
            retval = usb_bulk_msg(uframe_devices[index].udev, usb_sndbulkpipe(uframe_devices[index].udev, endpoint->endpointaddr), k_buffer, (int) frame_len, (int *) &frame_len, HZ*10);
            break;
        case TYPE_INTERRUPT:
            printk(KERN_INFO "%s: device : interrupt type\n", DEVICE_NAME);
            retval = usb_interrupt_msg(uframe_devices[index].udev, usb_sndintpipe(uframe_devices[index].udev, endpoint->endpointaddr), k_buffer, (int) frame_len, (int *) &frame_len, HZ*10);
        break;
    }
    printk(KERN_INFO "%s: device : --------------done transfer--------------\n", DEVICE_NAME);
    return frame_len;
}

long uframe_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct uframe_endpoint *endpoint;
    int retval;
    struct control_params *cparams = NULL;
    char *data;
    struct __endpoint_desc _endpoint_desc;
    int index;
    int minor;
    
    cparams = kmalloc(sizeof(struct control_params),GFP_KERNEL);
    retval = 0;
    endpoint = filp->private_data;    

    minor = MINOR(filp->f_path.dentry->d_inode->i_rdev);
    printk( "IOCTL on minor number %d\n", minor);

    index = findDeviceIndexByMinor(minor);

    if (index < 0){
        printk(KERN_INFO"can not get index");
        return -EFAULT;
    }

    printk(KERN_INFO"%s: device : IOCTL From endpoint %d, type %d, dir %d\n", DEVICE_NAME, endpoint->endpointaddr, endpoint->type, endpoint->dir);

    switch(cmd)
    {
        case IOCTL_INTERRUPT_INTERVAL:
            printk(KERN_INFO"IOCTL interrupt interval %d", IOCTL_INTERRUPT_INTERVAL);
            if(endpoint->type == TYPE_INTERRUPT)
            {
                if(copy_to_user((int __user *) arg, &endpoint->interval, sizeof(int)))
                    return -EFAULT;

            }else
                return -ENOTTY;
            break;

        case IOCTL_CONTROL_READ:
            printk(KERN_INFO"IOCTL control read %d", IOCTL_CONTROL_READ);
            if(copy_from_user(cparams, (struct control_params __user*) arg, sizeof(cparams)))
                return -EFAULT;
            
            printk(KERN_INFO"%s: device : request %d request_type %d value %d index %d size %d",DEVICE_NAME, 
                   cparams->request, cparams->request_type, cparams->value, cparams->index, cparams->size);
            
            data = kmalloc(cparams->size,GFP_KERNEL);
            printk(KERN_INFO"reched");
            retval = usb_control_msg(uframe_devices[index].udev
                                        ,usb_rcvctrlpipe(uframe_devices[index].udev, endpoint->endpointaddr)
                                        ,cparams->request
                                        ,cparams->request_type
                                        ,cparams->value
                                        ,cparams->index
                                        ,data
                                        ,cparams->size
                                        ,0);

            printk(KERN_INFO"reched");
            if(retval < 0)
            {
                printk(KERN_ERR"%s: couldn't control message to read\n",DEVICE_NAME);
                return retval;
            }
            if(cparams->size)
            {
                if(copy_to_user((char __user*) (arg + sizeof(struct control_params)), data, cparams->size))
                return -EFAULT;
            }

            printk(KERN_INFO"reched");
            printk(KERN_INFO"%s", data);
            break;

        case IOCTL_ENDPOINTS_DESC:
            printk(KERN_INFO"IOCTL interrupt interval %d", IOCTL_ENDPOINTS_DESC);
            _endpoint_desc.type = endpoint->type;
            _endpoint_desc.interval = endpoint->interval;
            _endpoint_desc.dir = endpoint->dir;
            _endpoint_desc.endpointaddr = endpoint->endpointaddr;
            _endpoint_desc.buffer_size = endpoint->buffer_size;
            printk(KERN_INFO"%s: endpoints type %d interval %d dir %d endpointaddr %d size %d \n",DEVICE_NAME, _endpoint_desc.type,
            _endpoint_desc.interval, _endpoint_desc.dir, _endpoint_desc.endpointaddr, _endpoint_desc.buffer_size);
            if(copy_to_user((struct __endpoint_desc  __user *) arg, &_endpoint_desc, sizeof(struct __endpoint_desc))) 
                return -EFAULT;

            retval = 0;
            break;
            default: // not defined
            return -ENOTTY;
    }

    printk(KERN_INFO"reched");
    return retval;
}

void uframe_delete(struct kref *krf)
{   
    struct uframe_endpoint *ep = container_of(krf, struct uframe_endpoint, kref);
    kfree (ep->data); //free data allocated
}

int findDeviceIndexByDevice(struct usb_device *device){
    int i;
    for(i = 0 ; i < devCnt ;i++){
        if(uframe_devices[i].VID == device->descriptor.idVendor && uframe_devices[i].PID == device->descriptor.idProduct)
            return i;
    }
    return -1;
}

int findDeviceIndexByMinor(int minor){
    int i;
    for (i = 0 ; i < devCnt ; i++){
        if(uframe_devices[i].minor == minor)
            return i;
    }
    return -1;
}
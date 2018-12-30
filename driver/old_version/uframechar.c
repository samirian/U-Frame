#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/uaccess.h>          // Required for the copy to user function
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/usb.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/time.h>
#include "uframechar.h"
#include "uframe.h"

struct control_params {
    uint8_t request;
    uint8_t request_type;
    uint16_t value;
    uint16_t index;
    uint16_t size;
};

static int findDeviceIndexByMinor(int minor);

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

struct __endpoint_desc {
    int type; 
    int dir;
    int endpointaddr;
    int interval;
    int buffer_size; 
};

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

int findDeviceIndexByMinor(int minor){
	int i;

	printk(KERN_ALERT"findDeviceIndexByMinor %d", minor);
	for (i = 0 ; i < devCnt ; i++){
		printk(KERN_ALERT"%d", uframe_devices[i].minor);
		if(uframe_devices[i].minor == minor)
			return i;
	}
	return -1;
}
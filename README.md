# U-Frame
**note:** *supported language is python only.*

**note:** *only linux systems are supported till now, windows systems are intended to be supported in the near future.*
## For Linux systems:
1. Connect your device and get its vendor ID and product ID, you can use **lsusb** command.
1. Go to driver folder and run the **manage.py** script passing to it the vendor ID and product ID of your device. During this step, the **manage.py** script will check the device discriptor and find all its interfaces and endpoints, then it will create a node for each endpoint in the folder **/dev/U-Frame/vid/pid/interface**. You can take a look at **deviceDiscriptor.txt** and **endpoints.txt** files created in driver folder after running **manage.py** script.
![how to use manage.py script](https://github.com/samirian/U-Frame/blob/master/Software-Documents/images/how%20to%20use%20manage.py%20script.png)
1. Now the driver is loaded to the kernel, Horaaaaaay you are ready to go! you can start making your program.

## Simple example:
So, assume that we got bored of the normal use of the mouse buttons and we want to create our own use, for example : if we pressed the middle button it opens a certain program, Hmmmm, sounds interesting is not it?, lets see how to do it.
### Steps:
1. First things first, since we are working on a usb HID device, a **mouse**, which has a module named **usbhid** already loaded, then we have first to unload it using the command **rmmod** in order to replace it with our driver, but do not worry, this command will not delete the driver or anything like that it will just unload it from the kernel in order that our driver can communicate with the device and we will reload it back later after we finish, but also note that at this point any usb connected mouse or keyboard will not work temporarly until we reload the **usbhid** module again, if you are lucky and your are working on a laptop then its the best time to make advantge of its keyboard and touchpad!.
![unload usbhid](https://github.com/samirian/U-Frame/blob/master/Software-Documents/images/unload%20usbhid.png)
1. Type the command **dmesg** in the terminal to check if the driver is connected to the device or not, if its connected it will show a message like the one in the picture, and if its not ,all you have to do is to unplug your device and replug it again.
![dmesg](https://github.com/samirian/U-Frame/blob/master/Software-Documents/images/dmesg.png)
1. Copy the contents of the folder **ulib** to the folder where you want to create your project (**test** for this example).
1. Create your project file (**test.py** for this example). **note:** *for the mouse I am using in this example, it happens that it sends maximum 8 bytes for the interrupt word and the middle button set the sencond byte to 4 when it is clicked.*
![test code](https://github.com/samirian/U-Frame/blob/master/Software-Documents/images/test%20code.png)
1. Run the **test.py** file, now if you click on the middle button it will open the **gedit** program, well done you have completed your first program, go ahead and create your own more complex applications, but until you that do not forget to unload the **uframe** driver and reload the **usbhid** using the commands **rmmod**, **insmod**, you can use the command **locate** to locate **usbhid.ko** module. **note:** *after every restart for the system the uframe driver will be unloaded and the usbhid module will be reloaded automatically.*
![reloading usbhid](https://github.com/samirian/U-Frame/blob/master/Software-Documents/images/reloading%20usbhid.png)

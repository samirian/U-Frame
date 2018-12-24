# U-Frame
**note:** *supported language is python only.*

**note:** *only linux systems are supported till now, windows systems are intended to be supported in the near futur.*
## For Linux systems:
1. Connect your device and get its vendor ID and product ID, you can use **lsusb** command.
1. Go to driver folder and run the **manage.py** script passing to it the vendor ID and product ID of your device.During this step, the **manage.py** script will check the device descriptor and find all its interfaces and endpoints, then it will create a node for each endpoint in the folder **/dev/U-Frame/vid/pid/interface**. You can take a look at **deviceDiscrptor.txt** and **endpoints.txt** files created in driver folder after running **manage.py** script.
1. Now the driver is loaded to the kernel,Horaaaaaay you are ready to go! you can start making your program.

![](https://github.com/samirian/U-Frame/blob/master/Software-Documents/images/how%20to%20use%20manage%20script.png)

## Simple example:
So, assume that we got bored of the normal use of the mouse buttons and we want to create our own use, for example : if we pressed the middle button it opens a certain program, Hmmmm, sounds interesting isnt it?, lets see how to do it.
### Steps:
1. Copy the contents of the folder ulib to the folder where you want to create your project (**test** for this example).
1. Create your project file (**test.py** for this example).
![](https://github.com/samirian/U-Frame/blob/master/Software-Documents/images/how%20to%20use%20manage%20script.png)
1. d

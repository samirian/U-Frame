#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stropts.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>

struct control_params {
    uint8_t request;
    uint8_t request_type;
    uint16_t value;
    uint16_t index;
    uint16_t size;
    char  data[256];
};

//Tested using our device
void testControlWithData()
{
    int fd;
    int retval;
    fd = open("/dev/uframe/command",O_RDWR);
    assert(fd > 0);
    
    struct control_params p;
    p.size = sizeof(p.data);
    p.request = 1;
    p.request_type = 0xc0;
    p.value = 0;
    p.index = 0;
    printf("Open Led \n");
    
    retval = write(fd, &p, sizeof(p)); // open led
    printf("retval : %d\n",retval);
    assert(retval == sizeof(p));

    sleep(2);
    printf("Close Led \n");
    p.request = 0;
    retval = write(fd, &p, sizeof(p)); // close led
    assert(retval == sizeof(p));

    
    printf("Read data\n");
    p.request = 2;
    p.size = 16;
    retval = ioctl(fd, 1, &p); // read data out
    printf("retval : %d\n",retval);
    assert(retval == p.size);
    assert(!strncmp(p.data,"Hello, USB!",11));
    printf("data: %s\n",p.data);

    printf("Over write data\n");
    p.request = 3;
    p.value = 'T' + ('E' <<8);
    p.index = 'S' + ('T' <<8);
    retval = write(fd, &p, sizeof(p)); // close led
    assert(retval == sizeof(p));

    printf("Read Data\n");
    p.request = 2;
    p.size = 16;
    retval = ioctl(fd, 1, &p); // read data out
    assert(retval == p.size);
    assert(!strncmp(p.data,"Hello, TEST",11));
    
    retval = close(fd);
    assert(retval ==0);
    
}

void testControlWithoutData()
{
    int fd;
    int retval;
    fd = open("/dev/uframe/test/control",O_RDWR);
    assert(fd > 0);

    struct control_params p;
    memset(&p,0,sizeof(p));
    p.request_type = 0xc0;
    p.value = 0;
    p.index = 0;
    p.size = 0;
    p.request = 2;
    
    retval = ioctl(fd, 1, &p);
    assert(retval == 0);

    retval = close(fd);
    assert(retval ==0);

    
}


void testInterruptWrite()
{
    int fd;
    int retval ;
    fd = open("/dev/uframe/test/interruptout",O_RDWR); //open read and write to test both direction
    assert(fd == -1);
    assert(errno == -EACCES);

    fd = open("/dev/uframe/test/interruptout",O_WRONLY);
    assert(fd > 0);
    
    char data[255] = "test data";

    retval = write(fd, data, 255);
    assert(retval == 255);

    retval = write(fd, data, 0); // zero data
    assert(retval == 0);
    
    retval = read(fd, data, 255); // read in out endpoint
    assert(retval == -1);
    assert(errno == -EACCES);

    int interval;
    retval = ioctl(fd, 0, &interval);
    assert(retval == 0);
    
}

//tested using a mouse
void testInterruptRead()
{
    int fd;
    int retval ;
    
    fd = open("/dev/uframe/bulk",O_RDWR); //open for read and write
    assert(fd == -1);
    assert(errno == EACCES);
    
    printf("errno : %d\n",errno);
    fd = open("/dev/uframe/bulk",O_RDONLY);
    assert(fd > 0);
   
    unsigned char data[255];
    int i,j;
   

    
    int interval;
    retval = ioctl(fd, 0, &interval);
    assert(retval == 0);
    assert(interval == 100); 
    printf("Interval %d \n", interval);

    
    for(j = 0 ; j < 10 ; j++)
    {
	retval = read(fd, data, 8);
	printf("%d\n",retval);
	assert(retval == 4);
	for(i =0; i < retval ; i++)
	    printf("byte %d: %x\n", i, data[i]);
	sleep(interval/1000);
    }
    
    retval = read(fd, data, 0); // zero data
    assert(retval == 0);
    
    retval = write(fd, data, 255); // read in out endpoint
    assert(retval == -1);
    assert(errno == EBADF);

   
}

void testBulkWrite()
{
    int fd;
    int retval ;
    fd = open("/dev/uframe/test/bulkout",O_RDWR); //open read and write to test both direction
    assert(fd == -1);
    assert(errno == EACCES);

    fd = open("/dev/uframe/test/bulkout",O_WRONLY);
    assert(fd > 0);
    
    char data[255] = "test data";

    retval = write(fd, data, 255);
    assert(retval == 255);

    retval = write(fd, data, 0); // zero data
    assert(retval == 0);
    
    retval = read(fd, data, 255); // read in out endpoint
    assert(retval == -1);
    assert(errno == EBADF);
    
}

void testBulkRead()
{
    int fd;
    int retval ;
    
    fd = open("/dev/uframe/test/bulkin",O_RDWR); //open for read and write
    assert(fd == -1);
    assert(errno == EACCES);
    
    fd = open("/dev/uframe/test/bulkin",O_RDONLY);
    assert(fd > 0);
    
    char data[255];

    retval = read(fd, data, 255);
    assert(retval == 255);

    retval = read(fd, data, 0); // zero data
    assert(retval == 0);
    
    retval = write(fd, data, 255); // read in out endpoint
    assert(retval == -1);
    assert(errno == EBADF);

}

void testIOCTL()
{
    int fd,i, j, retval;
    int buff[5];
    memset(buff, 0, 5 *sizeof(int));
    fd = open("/dev/uframe/interrupt",O_RDONLY);
    assert(fd > 0);

    printf("IOCTL READ ENDPOINTS\n");
    retval = ioctl(fd, 3,buff);
    assert(retval == 0);
    for(j =0 ; j < 5; j++)
	printf("data %d: %d\n",j,buff[j]);
    close(fd);
}

int main()
{
    testControlWithData();
    //testControlWithoutData();
    testInterruptRead();
    testIOCTL();
}

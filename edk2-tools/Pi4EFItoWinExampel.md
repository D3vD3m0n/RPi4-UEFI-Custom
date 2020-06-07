RPi 4 Custom UEFI Firmware Exampels
===================================
---

###RPi 4 Working Mailbox example

######Code: Select all
```c
#include <iomanip>

/* Register access helper */
#define REG(x)     (*(volatile unsigned int *)(x))
/* Mailbox 0 base address (Read by ARM) */
#define MBOX0_BASE 0x3F00B880
/* Mailbox 1 base address (Read by GPU) */
#define MBOX1_BASE 0x3F00B8A0

/* Mailbox Controller Registers*/
/* I/O register is at the base address */
#define MBOX_RDWR(x)    REG((x))
/* Status register at the offset 0x18 from base */
#define MBOX_STATUS(x)  REG((x) + 0x18)
/* Status register value when mailbox is full/empty */
#define   MBOX_FULL     0x80000000
#define   MBOX_EMPTY    0x40000000
/*  Interrupt configuration register */
#define MBOX_CONFIG(x)    REG((x) + 0x1C)
/* Configuration register mask to enable mailbox data IRQ */
#define   MBOX_DATAIRQEN  0x00000001

using namespace std;

int main (void)
{
//      uint32_t ui32_value;
//      while (MBOX_STATUS(MBOX0_BASE) & MBOX_EMPTY);
//      ui32_value = MBOX_RDWR(MBOX0_BASE);
//      do
//      {
//              cout<<"test"<<endl;
//              while (MBOX_STATUS(MBOX0_BASE) & MBOX_EMPTY);
////            ui32_value = MBOX_RDWR(MBOX0_BASE);
//      }
//      while ((ui32_val & 0xF) != channel)

        uint32_t ui32_value4 = MBOX_STATUS(MBOX0_BASE) & MBOX_EMPTY;

}
```

The code you are showing is baremetal code .. >>>> AKA NOT LINUX <<<<<

######Code: Select all
```c
#define BCM2708_PERI_BASE       0x20000000
```

Apparently the Base is 0xFE000000.

Still feel like I am going down a rabbit hole. The only reference I can find to the GPU temperature is in reference to mailboxes. The only code I can find has to do with GPIO. I can't find anything that ties them together except the base peripheral 
address of 0xFE000000 (the are actually showing 0x20000000 from 
https://www.pieter-jan.com/node/15 and https://github.com/raspberrypi/firmware ... -mailboxes.

 The peripherals start at 0xFE000000 for the RPI4, but you don't need the Mailbox to access these are per https://www.pieter-jan.com/node/15, you can just mmap them out and read the data out of the memory you have allocated. The mailboxes though have to be read in a different way completely.

Am I totally missing the point here? The userland code creates a program called vgencmd which has a parameter "measure_temp", so it can be done. I can't understand their code, I can't even figure out how they match the "measure_temp" parameter.

How about this, I am going to place a call to mmap with the address 0xFE000000 + 0xB880 using the block size of 4*1024, and print out the data in hex. NO that can't be right. The mailboxes must reside higher in memory than the GPIO, right???

What about the TAG for Get Temperature 0x00030006, perhaps I can read something from (0xFE000000 + 0x00030006) FE03006.

No you have not understood. I will try and slice and dice so code on the fly here so there may be some errors but the general thrust is correct.
So this is the mailbox structure in code form (it is correct I have cut and paste it from working code)

######Code: Select all
```c
struct MailBoxRegisters {
	const uint32_t Read0;	  // 0x00         Read data from VC to ARM
	uint32_t Unused[3];       // 0x04-0x0F
	uint32_t Peek0;	       // 0x10
	uint32_t Sender0;      // 0x14
  	uint32_t Status0;	// 0x18         Status of VC to ARM
	uint32_t Config0;	// 0x1C        
	uint32_t Write1;	// 0x20         Write data from ARM to VC
	uint32_t Unused2[3];	// 0x24-0x2F
	uint32_t Peek1;	   // 0x30
	uint32_t Sender1;  // 0x34
	uint32_t Status1;	  // 0x38         Status of ARM to VC
	uint32_t Config1;	  // 0x3C 
};
```

It lives at an offset .. yes from the GPIO address .. so create a pointer to it.

######Code: Select all
```c
#define MAILBOX ((volatile __attribute__((aligned(4))) struct MailBoxRegisters*)(uintptr_t)( 0xFE000000 + 0xB880))
```

Now you use the registers to send messages backward and forward to the VC6 .. think of it as something like a uart channel between the two processors. Now the exchange is described by https://github.com/raspberrypi/firmware ... -interface but basically you need a block of memory that is 16 byte aligned you setup the data into there and send the address + channel thru the mail box. The VC6 will fill in the memory block with responses.

######Code: Select all
```c
#include <stdarg.h>
#define MAIL_EMPTY	0x40000000		/* Mailbox Status Register: Mailbox Empty */
#define MAIL_FULL	0x80000000		/* Mailbox Status Register: Mailbox Full  */

bool mailbox_tag_message (uint32_t* response_buf,	 // Pointer to response buffer
	                     uint8_t data_count,		// Number of uint32_t variadic data following
	                     ...)					// Variadic uint32_t values for call
{
	uint32_t __attribute__((aligned(16))) message[data_count + 3];    // Must be 16 byte aligned
	va_list list;
	va_start(list, data_count);				// Start variadic argument
	message[0] = (data_count + 3) * 4;		// Size of message needed
	message[data_count + 2] = 0;			// Set end pointer to zero
	message[1] = 0;						// Zero response message
	for (int i = 0; i < data_count; i++) {
		message[2 + i] = va_arg(list, uint32_t);	// Fetch next variadic
	}
	va_end(list);							// variadic cleanup	
    
        while ((MAILBOX->Status1 & MAIL_FULL) != 0) {};    // Make sure ARM to VC mailbox is not full .. wait if so
	MAILBOX->Write1 = ((uint32_t)(uintptr_t)&message | 0x8);  // Write message address + channel 8 (tags) to mailbox
       while ((MAILBOX->Status0 & MAIL_EMPTY) != 0){}; // Make sure VC to ARM is not empty .. wait if so
       volatile uint32_t value = MAILBOX->Read0;     // Clear the response
	if (message[1] == 0x80000000) {			// Check success flag
		if (response_buf) {					// If buffer NULL used then don't want response
			for (int i = 0; i < data_count; i++)
				response_buf[i] = message[2 + i];	// Transfer out each response message
		}
		return true;	// message success
	}
	return false;	   // Message failed
}
```

Okay finally lets look at how we use the code .. the tag you are interested in is 0x00030006, it has a request length of 4 and a response length of 8. Those are in bytes so you are providing 1 x uint32_t and it gives back 2 x uint32_t. So we will need to exchange 5 entries being the tag id, the request length, the response length, a buffer (prefilled with our request) it will overwrite with resposne 1, a uint32_t it will fill as response 2. I don't know what "temperature id" you want, so I will leave it as that but it's going to be 0,1,2,3 etc you might have to scrounge thru linux code to find what the id's number refer to. There will be a couple of ID's for different parts of the SOC.

######Code: Select all
```c
uint32_t Buffer[5] = { 0 };
if (mailbox_tag_message(&Buffer[0], 5, 0x00030006, 4, 8, temperature_id, 0))
{
       // If you get in here the mailbox exchange was successful
       // buffer[4] has the return value of whatever you asked
}
```

    The userland code creates a program called vgencmd which has a parameter "measure_temp", so it can be done. I can't understand their code, I can't even figure out how they match the "measure_temp" parameter. 

 vcgencmd doesn't handle any of the commands. It passes the command to the firmware via the mailbox interface and then gets the result back. 

If you want to examine the mailboxes then you can use the 
/opt/vc/bin/vcmailbox utility. 
There is no error checking so be careful with what you send.

#####Here is my gpu_temp.cc
```c
#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include "gpu_temp.h"

using namespace std;

bool mailbox_tag_message (uint32_t* response_buf, uint8_t data_count, ...);

int main (int argc, char** argv)
{
// Create some variables
float temperature;
struct bcm2835_peripheral *block;
block=new struct bcm2835_peripheral;
int high = 60;
int critical = 70;

// Deal with argc and argv
if(argc>=4 || argc == 2){return -1;}
if(argc==3){
critical = atoi(argv[2]);
high = atoi(argv[1]);
}

// Open a file descriptor to block of memory to store results
if ((block->mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0)
{
printf("Failed to open /dev/mem, try checking permissions.\n");
return -1;
}

// Map out block
block->map = mmap(
NULL,
BLOCK_SIZE,
PROT_READ|PROT_WRITE,
MAP_SHARED,
block->mem_fd, // File descriptor to physical memory virtual file '/dev/mem'
block->addr_p // Address in physical map that we want this memory block to expose
);

// If we can't map return error
if (block->map == MAP_FAILED)
{
perror("mmap");
return -1;
}

// Otherwise we have a pointer to the mapped space
block->addr = (volatile unsigned int *)block->map;


// Do something with it


// NEW
uint32_t Buffer[5] = { 0 };

if (mailbox_tag_message(&Buffer[0], 5, TEMPERATURE_TAG, REQUEST_LENGTH, RESPONSE_LENGTH, TEMPERATURE_ID, 0))
{
// If you get in here the mailbox exchange was successful
// buffer[4] has the return value of whatever you asked
cout<<Buffer[4]<<endl;
}

// Unmap the memory space
munmap(block->map, BLOCK_SIZE);
close(block->mem_fd);

// Temporary to make function work
temperature=34.50;

// Output the values in the Nagios format
cout << "Temp OK - GPU Temperature is "<<std::setprecision(4)<<temperature<<
" | gputemp="<<std::setprecision(4)<<temperature<<";"<<high<<";"<<critical<<endl;
}

bool mailbox_tag_message (uint32_t* response_buf, // Pointer to response buffer
uint8_t data_count, // Number of uint32_t variadic data following
...) // Variadic uint32_t values for call
{
uint32_t __attribute__((aligned(16))) message[data_count + 3]; // Must be 16 byte aligned
va_list list;
va_start(list, data_count); // Start variadic argument
message[0] = (data_count + 3) * 4; // Size of message needed
message[data_count + 2] = 0; // Set end pointer to zero
message[1] = 0; // Zero response message
for (int i = 0; i < data_count; i++)
{
message[2 + i] = va_arg(list, uint32_t);// Fetch next variadic
}
va_end(list); // variadic cleanup

// Not sure why, but we are writing to Mailbox 1 Channel 8 (ARM -> VC)
// Make sure ARM to VC mailbox is not full .. wait if so
while ((MAILBOX->Status1 & MAIL_FULL) != 0) {};

MAILBOX->Write1 = ((uint32_t)(uintptr_t)&message | 0x8); // Write message address + channel 8 (tags) to mailbox

// Now we read from Mailbox
// Make sure VC to ARM is not empty .. wait if so
while ((MAILBOX->Status0 & MAIL_EMPTY) != 0){};

volatile uint32_t value = MAILBOX->Read0; // Clear the response

// Check success flag
if (message[1] == SUCCESS )
{
// If buffer NULL used then don't want response
if (response_buf)
{
for (int i = 0; i < data_count; i++)
response_buf = message[2 + i]; // Transfer out each response message
}
return true; // message success
}
return false; // Message failed
}

And corresponding gpu_temp.h

#include <sys/mman.h>
#include <unistd.h>
#include <stdarg.h>

//#define GPIO_BASE (BCM2708_PERI_BASE + 0x200000) // GPIO controller
#define BLOCK_SIZE (4*1024)
#define BCM2708_PERI_BASE_1 0x20000000 // Peripheral Base for RPi1
#define BCM2708_PERI_BASE_2 0x3F000000 // Peripheral Base for RPi2
#define BCM2708_PERI_BASE_3 0x3F000000 // Peripheral Base for RPi3
#define BCM2708_PERI_BASE_4 0xFE000000 // Peripheral Base for RPi4
#define BCM2708_PERI_BASE BCM2708_PERI_BASE_4 // We are compiling for RPi4
#define MAIL_BASE 0xB880 // Base address for the mailbox registers
#define MAILBOX ((volatile __attribute__((aligned(4))) struct MailBoxRegisters*)(uintptr_t)( BCM2708_PERI_BASE + MAIL_BASE))
#define BLOCK_SIZE (4*1024)
#define MAIL_EMPTY 0x40000000 /* Mailbox Status Register: Mailbox Empty */
#define MAIL_FULL 0x80000000 /* Mailbox Status Register: Mailbox Full */
//#define STATUS0 0x18
//#define READ0 0x00
//#define STATUS1 0x38
//#define WRITE1 0x20
#define SUCCESS 0x80000000
#define TEMPERATURE_ID 0
#define TEMPERATURE_TAG 0x00030006
#define REQUEST_LENGTH 4
#define RESPONSE_LENGTH 8


struct MailBoxRegisters
{
const uint32_t Read0; // 0x00 Read data from VC to ARM
uint32_t Unused[3]; // 0x04-0x0F
uint32_t Peek0; // 0x10
uint32_t Sender0; // 0x14
uint32_t Status0; // 0x18 Status of VC to ARM
uint32_t Config0; // 0x1C
uint32_t Write1; // 0x20 Write data from ARM to VC
uint32_t Unused2[3]; // 0x24-0x2F
uint32_t Peek1; // 0x30
uint32_t Sender1; // 0x34
uint32_t Status1; // 0x38 Status of ARM to VC
uint32_t Config1; // 0x3C
};

// Structure to place kernal memory
struct bcm2835_peripheral
{
unsigned long addr_p;
int mem_fd;
void *map;
volatile unsigned int *addr;
};
```

LdB if you see something real obvious or some concept I am still not getting please let me know. I greatly appreciate your assistance so far
You lost me with this you want a pointer to the -mapped space

######Code: Select all
```c
// Otherwise we have a pointer to the mapped space
block->addr = (volatile unsigned int *)block->map;
```
You have to access the area via the map address not it's real address .. linux demands it

Okay so you have a definition of the mailbox

######Code: Select all
```c
struct MailBoxRegisters
{
const uint32_t Read0; // 0x00 Read data from VC to ARM
uint32_t Unused[3]; // 0x04-0x0F
uint32_t Peek0; // 0x10
uint32_t Sender0; // 0x14
uint32_t Status0; // 0x18 Status of VC to ARM
uint32_t Config0; // 0x1C
uint32_t Write1; // 0x20 Write data from ARM to VC
uint32_t Unused2[3]; // 0x24-0x2F
uint32_t Peek1; // 0x30
uint32_t Sender1; // 0x34
uint32_t Status1; // 0x38 Status of ARM to VC
uint32_t Config1; // 0x3C
};
```
so just map it like so .. the file handle is to /dev/mem obviously

######Code: Select all
```c
volatile struct MailBoxRegisters* MAILBOX = (volatile struct MailBoxRegisters*)mmap(
                0,
		BLOCK_SIZE,
		PROT_READ | PROT_WRITE,
		MAP_SHARED,
		"your_file_handle_to_/dev/mem",
		0xFE000000 + 0xB880);
```

The real address you are chasing is mapped by linux and gives you back a virtual pointer to the structure. The block size will be 4096 because that is the smallest virtual table block size .That is all there is to it

I used capitals ("MAILBOX") simply to make it match up with the original baremetal code so your linux mapped pointer is the same as "#define MAILBOX ((volatile __attribute__((aligned(4))) struct MailBoxRegisters*)(uintptr_t)( 0xFE000000 + 0xB880))"

That pointer will now look like and work like the true baremetal code (except it has a funny address) because the mailbox has been virtualized to there by linux.

So the code sequence is
#####  1) Open handle to /dev/mem
#####  2) map a pointer to mailbox
#####  3) Use mailbox pointer to do exchange as per baremetal using that pointer
#####  4) unmap pointer
#####  5) close handle


Cleaned up and simplified the code to only run the parts that are necessary for access to the mailbox


```c
int main (int argc, char** argv)
{
// 1> Open handle to /dev/mem

struct bcm2835_peripheral *block; // Create a pointer to a bcm2835_peripheral struct called block
block=new struct bcm2835_peripheral; // Allocate a new struct that block points to

if ((block->mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) // Open a read/write "handle" to /dev/mem
{
printf("Failed to open /dev/mem, try checking permissions.\n");
return -1;
}

block->map = mmap( // Copy a 4096 block of memory starting at 0xFE00B880 to the struct pointed to by block
NULL,
BLOCK_SIZE,
PROT_READ|PROT_WRITE,
MAP_SHARED,
block->mem_fd,
0xFE000000 + 0xB880
);

if (block->map == MAP_FAILED) // If mmap returned MAP_FAILED, bail
{
perror("mmap");
return -1;
}
// 2> map a pointer to mailbox
// 3> Use mailbox pointer to do exchange as per baremetal using that pointer
// 4> unmap pointer
// 5> close handle
munmap(block->map, BLOCK_SIZE); // Return the 4096 byte block of memory
}
```

You lose me at step 2. There is something subtle here going on that I am not getting. I have mapped a pointer to a copy of the protected space in memory.
What am I doing in step 2 and 4?
The thing you are calling "block->map" is the mailbox address under linux .. that is the thing you want and the only thing you need.
So trying to follow your original code it goes round in circles.
Your code above is correct but I have these two points

1.) My confusion is you have block->map as a void* .. why?
2.) You can't use it like that cast it to a volatile mailbox pointer because that is what it is.

This makes it a lot easier for me to follow

######Code: Select all
```c
struct bcm2835_peripheral
{
unsigned long addr_p;
int mem_fd;
volatile struct MailBoxRegisters* map; // changed
volatile unsigned int *addr; // what is this thing?
};
```
On that struct I am confused by these 2 points

1.) Then you have this other weird thing called addr ... why?
2.) I can't understand what this is supposed to be a pointer to an unsigned int called address and its volatile?

Moving on to step 4

unmap uses a void* so you can pass block->map straight back to it without even casting when you need to unmap.
So this line of code works no matter what pointer type block->map (harking back to why the hell is block->map a void*)

######Code: Select all
```c
// 5> close handle
munmap(block->map, BLOCK_SIZE); // Return the 4096 byte block of memory
```

So in your code block->map is equivalent to MAILBOX in the original baremetal code, use it as such which is step 3.
As long as the code of step 3 is under your global definition of block and you make the type change to map you can literally #define it as such

######Code: Select all
```c
#define MAILBOX block->map
```


Your partial sentences dosed with disdain and sarcasm are exacerbated by your ESL.

Yes I am ESL, sorry I cant help where I was born.
All I can offer is to put a sample version out on github and you can try and work out what I did.
Principal Software Engineer at Raspberry Pi (Trading) Ltd.
Contrary to popular belief, humorous signatures are allowed. 
Posted, should be enough code to do steps 1,2,4,5. I can compile this, but it gets caught on the assertions.

######Code: Select all
```c
/* test.cc */
#include "test.h"

int main (int argc, char** argv)
{
// 1. Open handle to /dev/mem
bcm2835_peripheral *block;
block=new struct bcm2835_peripheral;

if ((block->mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0)
{
printf("Failed to open /dev/mem, try checking permissions.\n");
return -1;
}

// 2. map a pointer to mailbox
void* mmappedData = mmap(
NULL,
BLOCK_SIZE,
PROT_READ|PROT_WRITE,
MAP_SHARED,
block->mem_fd,
MAILBOX_BASE
);

assert(mmappedData != MAP_FAILED);

// 3. Use mailbox pointer to do exchange as per baremetal using that pointer

// 4. unmap pointer
int rc=munmap(mmappedData, BLOCK_SIZE);
assert(mmappedData != MAP_FAILED);

// 5. close handle
close(block->mem_fd);
}

/* test.h */

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <assert.h>

#define BLOCK_SIZE (4*1024)
#define MAILBOX_BASE 0xFE000000 + 0xB880

struct bcm2835_peripheral
{
unsigned long addr_p;
int mem_fd;
volatile struct MailBoxRegisters* map;
}
```
// Note that I haven't defined MailBoxRegisters anywhere. I would have thought the compile would fail


Fri Sep 13, 2019 10:44 am
Okay thank you for that simplifying now can see it better

Ok this fails for 2 reasons

######Code: Select all
```c
void* mmappedData = mmap(
NULL,
BLOCK_SIZE,
PROT_READ|PROT_WRITE,
MAP_SHARED,
block->mem_fd,
MAILBOX_BASE
);
```

Somehow we both forgot the 0x200000 offset from Peripheral base To GPIO base

######Code: Select all
```c
#define MAILBOX_BASE 0xFE200000 + 0xB880
```
However secondary issue is 0x880 is not 4K aligned and from what I know of /dev/mem everything must be 4K align
I thought that would have come up in the various online resources and I had not really thought about the mapping as I left that to you.

So suggestion make the block size 0x10000 (64K) and use a 4K aligned physical address

######Code: Select all
```c
#define BLOCK_SIZE (0x10000)
#define MAILBOX_BASE 0xFE200000 + 0xB000
```

That will make the mailbox 0x880 offset from the pointer returned but well within that 64K mapped block.

So try that and it should get past that assert and you should get a valid virtual page.
Now I also warn you that /dev/mem requires root privilege so you will have to "sudo" execution of the command file you create if you are normal user

---
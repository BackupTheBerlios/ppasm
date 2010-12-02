#include "loader.h"
#include "util.h"
#include "assemble.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <time.h>
#include <fcntl.h>
#include <sched.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <poll.h>

/*
Thanks all the folks on parallax forum for documenting the serial protocol
check out here
http://forums.parallax.com/showthread.php?t=106314&highlight=serial+lfsr
I used the chip's documentation, and took a look at the code of other people in the forums,
I used the lfsr algorithm but split the recieve bit function and redid it with poll() for
efficency. It schould autmatically detect avalible data, so it schould improve the detection rate
*/

static u8 lfsr = 'P'; /* lfsr to communicate with propeller, initialized with 'P' */
static int fd; /* serial port file descriptor */
static struct termios oldtio; /* saved serial port settings to restore after the use of serial port */

/*****************************************************************\
*                                                                 *
*   Returns least significant lfsr bit and iterates a step.       *
*                                                                 *
\*****************************************************************/
static unsigned lfsr_step()
{
    unsigned result = lfsr & 0x01;
    lfsr = lfsr << 1 & 0xFE | (lfsr >> 7 ^ lfsr >> 5 ^ lfsr >> 4 ^ lfsr >> 1) & 1;
    return result;
}

/**********************************************************************************\
*                                                                                  *
*   Encodes @param data as a series of short/long pulses whith 3 bits/bye in       *
*   11 bytes of @param buff, ready to be send to propeller.                        *
*                                                                                  *
\**********************************************************************************/
void encode(u8* buff, u32 data)
{
    /* 0x92 = 10010010 */
    /*         ^  ^  ^ */
    unsigned i = 0;
    for (; i < 10; i++)
    {
        buff[i] = 0x92 | (data & 1) | (data & 2) << 2 | (data & 4) << 4;
        data >>= 3; /* process next 3 bits of u32 data */
    }
    /* the second to last bit in u8 is unused so 0xF2 instead of 0x92 and
    (data & 4) << 4 is skipped since it is the 33rd bit of data */
    buff[10] = 0xF2 | data & 1 | (data & 2) << 2;
}

/*****************************************************************\
*                                                                 *
*   Enables the dtr line pointed to @param fd.                    *
*                                                                 *
\*****************************************************************/
void enable_dtr()
{
    #ifdef ALT_SERIAL_IOCTL
    int controlbits, result;
    result = ioctl(fd, TIOCMGET, &controlbits);
    if(result == -1)
        sys_error("enable_dtr: error getting serial port options");

    controlbits |= TIOCM_DTR;
    result = ioctl(fd, TIOCMSET, &controlbits);
    if(result == -1)
        sys_error("enable_dtr: error setting serial port options");
    #else
    int result, controlbits = TIOCM_DTR;
    result = ioctl(fd, TIOCMBIS, &controlbits);
    if(result < 0)
        sys_error("enable_dtr: error getting serial port options");
    #endif
}

/*****************************************************************\
*                                                                 *
*   Disables the dtr line pointed to @param fd.                   *
*                                                                 *
\*****************************************************************/
void disable_dtr()
{
    #ifdef ALT_SERIAL_CTL
    int controlbits, result;
    result = ioctl(fd, TIOCMGET, &controlbits);
    if(!result)
        sys_error("disable_dtr: error getting serial port options");

    controlbits &= ~TIOCM_DTR;
    result = ioctl(fd, TIOCMSET, &controlbits);
    if(!result)
        sys_error("disable_dtr: error setting serial port options");
    #else
    int result, controlbits = TIOCM_DTR;
    result = ioctl(fd, TIOCMBIC, &controlbits);
    if(result < 0)
        sys_error("enable_dtr: error getting serial port options");
    #endif
}

/*****************************************************************\
*                                                                 *
*   Resets propeller                                              *
*                                                                 *
\*****************************************************************/
void prop_reset()
{
    enable_dtr(fd);
    sleep_msec(25);
    disable_dtr(fd);
}

/*****************************************************************\
*                                                                 *
*   Writes @param buffer of @param size bytes to serial port fd.  *
*                                                                 *
\*****************************************************************/
void serial_write_buffer(u8* buffer, size_t size)
{
    size_t bytes_send = write(fd, buffer, size);
    if(bytes_send != size)
        sys_error("failed to write to serial");
}

/*****************************************************************\
*                                                                 *
*   Writes u32 @param data to propeller.                          *
*                                                                 *
\*****************************************************************/
void prop_send_u32(u32 data)
{
    static u8 buff[11];
    encode(buff, data);
    serial_write_buffer(buff, 11);
}

/*****************************************************************\
*                                                                 *
*   Recieves a bit of data from propeller.                        *
*                                                                 *
\*****************************************************************/
u8 recieve(int timeout)
{
    size_t num_bytes;
    u8 byte;

    struct pollfd fds = { fd, POLLIN | POLLPRI, 0 };
    num_bytes = poll(&fds, 1, timeout);
    if(num_bytes < 0)
        fatal("poll failed");

    if((fds.revents & POLLIN) || (fds.revents & POLLPRI))
    {
        num_bytes = read(fd, &byte, 1);
        if(num_bytes < 1)
            sys_error("recv1_select() serial read failed");
    }
    else if((fds.revents & POLLERR) || (fds.revents & POLLHUP) || (fds.revents & POLLHUP))
        fatal("error in poll");
    else
        fatal("%s: timed out, propeller hardware not found.", __FUNCTION__);

    return byte - 0xFE;
}

/************************************************************************************\
*                                                                                    *
*   Recieves propellers'answers while sending 0xF9 each 25 msec.                     *
*   @param timeout to wait.                                                          *
*                                                                                    *
\************************************************************************************/
u8 recieve_pinging(int timeout)
{
    size_t num_bytes, t1 = get_time_ms();
    u8 byte, f9 = 0xF9;
    struct pollfd fds = { fd, POLLIN | POLLPRI, 0 };

    while(get_time_ms() - t1 < timeout)
    {
        write(fd, &f9, 1);
        poll(&fds, 1, 25);

        if((fds.revents & POLLIN) || (fds.revents & POLLPRI))
        {
            num_bytes = read(fd, &byte, 1);
            if(num_bytes < 1)
                sys_error("recv_echo: serial read failed!");
            return byte - 0xFE;
        }
        else if((fds.revents & POLLERR) || (fds.revents & POLLHUP) || (fds.revents & POLLHUP))
            fatal("error in poll");
    }

    fatal("%s: propeller timed out", __FUNCTION__);
    return 0;
}


/*******************************************************************************\
*                                                                               *
*   Connects to propeller on device @param fd and sends a @param command to it. *
*   @return propeller version                                                   *
*                                                                               *
\********************************************************************************/
u8 prop_connect()
{
    u8 buff[258];
    prop_reset();
    sleep_msec(95);

    lfsr = 'P'; /* resetting lfsr */

    *buff = 0xF9; /* Timing calibration */

    /* 250 bytes of lfsr data */
    for(unsigned i = 1; i < 251; i++)
    {
        buff[i] = (lfsr_step() | 0xFE);
    }
    serial_write_buffer(buff, 251);

    /* Send 258 timing calibrations */
    for(unsigned i = 0; i < 258; i++)
    {
        buff[i] = 0xF9;
    }
    serial_write_buffer(buff, 258);
    fsync(fd);

    /* Recieve 250 bytes of LFSR data */
    for(unsigned i = 0; i < 250; i++)
    {
        if(recieve(100) != lfsr_step())
            fatal("recieved wrong LFSR, lost hardware connection?");
    }

    u8 version = 0;
    for(unsigned i = 0; i < 8; ++i)
        version |= (recieve(200) << i);

    return version;
}

/*****************************************************************\
*                                                                 *
*   Sets serial port @param fd settings.                          *
*                                                                 *
\*****************************************************************/
void set_serial()
{
    int result = tcgetattr(fd, &oldtio); /* save current serial port settings */
    if(result < 0)
        sys_error("failed to retrieve serial port attributes");

    struct termios tio;
    memset(&tio, 0, sizeof(tio)); /* clear struct for new port settings */

    tio.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
    cfsetispeed(&tio, B115200);
    tio.c_iflag = 0;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    tio.c_cc[VMIN]     = 1;   /* blocking read until 1 char received */

    result = tcflush(fd, TCIFLUSH);
    if(result < 0)
        sys_error("tcflush failed");

    result = tcsetattr(fd, TCSANOW, &tio);
    if(result < 0)
        sys_error("failed to set serial port attributes");
}


/*****************************************************************\
*                                                                 *
*   Restores serial port @param fd  attributes.                   *
*                                                                 *
\*****************************************************************/
void restore_serial()
{
    if(!tcsetattr(fd,TCSANOW, &oldtio))
        sys_error("failed to restore serial port attributes");
}

/*****************************************************************\
*                                                                 *
*   Sets realtime priority on the current process.                *
*                                                                 *
\*****************************************************************/
void set_realtime_priority()
{
    struct sched_param sparam;
    sparam.sched_priority = 10;
    if(sched_setscheduler(0, SCHED_FIFO, &sparam))
        fprintf(stderr, "failed to set realtime priority\n");
}

/*****************************************************************\
*                                                                 *
*   TODO write a proper serial port finder                        *
*                                                                 *
\*****************************************************************/
int find_serial()
{
    int fd = -1;
    char port[] = "/dev/ttyUSB_";
    for(unsigned i = 0; i < 10; i++)
    {
        port[11] = i + 0x30;
        fd = open(port, O_RDWR | O_NOCTTY);
        if(!fd)
        {
            /* TODO try contact prop */
            return fd;
        }
    }

    /* ok, trying /dev/ttyS0 - 9 */
    port[8] = 'S';
    port[10] = 0;

    for(unsigned i = 0; i < 10; i++)
    {
        port[9] = i + 0x30;
        fd = open(port, O_RDWR | O_NOCTTY);
        if(!fd)
        {
            /* TODO try contact prop */
            break;
        }
    }

    return fd;
}

/************************************************************************************\
*                                                                                    *
*   Sends a stream of instructions @param prog of length @param progsz to propeller. *
*                                                                                    *
\************************************************************************************/
static void prop_send_program(instruction_t* prog, size_t progsz)
{
    size_t imgsz = progsz * 4 + PREAMBLE_SIZE;
    u8* image = malloc(imgsz);
    mlock(image, imgsz);
    #ifdef PPASM_LITTLE_ENDIAN
    create_preamble(image, (u8*)prog, progsz);
    memcpy(image + PREAMBLE_SIZE, (u8*)prog, imgsz - PREAMBLE_SIZE);
    #else
    create_preamble(image, &prog[0].byte[3], progsz);
    memcpy(image + PREAMBLE_SIZE, (u8*)&prog[0].byte[3], imgsz - PREAMBLE_SIZE);
    #endif

    u32 s;
    unsigned num_u32 = (imgsz) / 4;
    prop_send_u32(num_u32); /* transmit the number of u32 in the image */
    printf("sending %lu bytes %lu longs\n", imgsz, imgsz / 4);
    for(unsigned i = 0; i < num_u32; i ++)
    {
        unsigned j = i << 2;
        s = (u32)(image[j + 0]) |
             (u32)(image[j + 1] << 8) |
             (u32)(image[j + 2] << 16) |
             (u32)(image[j + 3] << 24);

        prop_send_u32(s);

    }

    /* Read a bit indicating whether checksum failed */
    if(recieve_pinging(16000))
        fatal("ram checksum failed");

    munlock(image, imgsz);
    free(image);
}

/**********************************************************************\
*                                                                      *
*   Tries to execute a @param command on the propeller, connected to   *
*   @param device                                                      *
*                                                                      *
\**********************************************************************/
void prop_action(const char* device, u32 command)
{
    fd = open(device, O_RDWR | O_NOCTTY);
    if(fd < 3)
        sys_error("failed to open serial port");

    if(opt_verbose > 4)
        fprintf(vfile, "opened %s r/w fd: %i command: %u\n", device, fd, command);

    mlock(&program, num_ops * 4);
    set_serial();

    set_realtime_priority();

    u8 version = prop_connect();

    if(version != 1)
        fatal("wrong propeller version");

    prop_send_u32(command);

    switch(command)
    {
        case CMD_SHUTDOWN:
            printf("found propeller version %u\n", version);
            break;

        case CMD_RAM_RUN:
            prop_send_program(program, num_ops);
            fprintf(stdout, "program downloaded successfuly");
            break;

        default:
            fatal("FIXME: only show version/load to ram and run is supported for now");
    }

    munlockall();
    restore_serial();
}

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


/*
the source code here is mostly ported from here
http://forums.parallax.com/showthread.php?t=106314&highlight=serial+lfsr
I will rework it in the near future with select if I get this to work
*/

static u8 lfsr = 'P'; /* lfsr to communicate with propeller */
static int fd; /* serial port file descriptor */

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
*   Encodes @param data in 11 bytes of @param buff, ready to be send to propeller. *
*                                                                                  *
\**********************************************************************************/
void encode(u8 buff[], u32 data)
{
    u8 i = 0;
    for(; i < 10; i++)
    {
        buff[i] = 0b10010010 | data & 0b00000001 | (data & 0b00000010) << 2 | (data & 0b00000100) << 4;
        data >>= 3;
    }
    buff[i] = 0b11110010 | (data & 0b00000001) | (data & 0b00000010) << 2 ;
}

/*****************************************************************\
*                                                                 *
*   Enables the dtr line pointed to @param fd.                    *
*                                                                 *
\*****************************************************************/
void enable_dtr()
{
    int result, controlbits = TIOCM_DTR;
    result = ioctl(fd, TIOCMBIS, &controlbits);
    if(result < 0)
            sys_error("enable_dtr: error getting serial port options");

    /*
    int controlbits, result;
    result = ioctl(fd, TIOCMGET, &controlbits);
    if(result == -1)
            sys_error("enable_dtr: error getting serial port options");

    controlbits |= TIOCM_DTR;
    result = ioctl(fd, TIOCMSET, &controlbits);
    if(result == -1)
            sys_error("enable_dtr: error setting serial port options");

    */
}

/*****************************************************************\
*                                                                 *
*   Disables the dtr line pointed to @param fd.                   *
*                                                                 *
\*****************************************************************/
void disable_dtr()
{
    int result, controlbits = TIOCM_DTR;
    result = ioctl(fd, TIOCMBIC, &controlbits);
    if(result < 0)
            sys_error("enable_dtr: error getting serial port options");

    /* alternative style
    int controlbits, result;
    result = ioctl(fd, TIOCMGET, &controlbits);
    if(!result)
            sys_error("disable_dtr: error getting serial port options");

    controlbits &= ~TIOCM_DTR;
    result = ioctl(fd, TIOCMSET, &controlbits);
    if(!result)
            sys_error("disable_dtr: error setting serial port options");

*/
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
*   Recieves a bit from propeller on device @param fd.            *
*                                                                 *
\*****************************************************************/
u8 prop_recieve_bit(unsigned echo_on, unsigned timeout)
{
    ulong t1 = get_time_ms();
    u8 f9 = 0xF9;
    u8 bit;
    int result;

    while(!echo_on || get_time_ms() - t1 < timeout)
    {
        if(echo_on)
        {
            serial_write_buffer(&f9, 1);
            sleep_msec(25);
        }
        if(!echo_on || (result = read(fd, &bit, 1)) > 0)
        {
            if(result > 0)
            {
                bit -= 0xFE;

                if (bit > 1)
                    fatal("Receiving bit failed!");

                return bit == 1;
            }
        }
    }
    fatal("%s timed out!", __FUNCTION__);
    return 0;
}

u8 recv1(ulong timeout)
{
    ulong t1 = get_time_ms();
    u8 byte;

    do
    {
        size_t num_bytes = read(fd, &byte, 1);
        if(num_bytes == 1)
            return byte - 0xFE;
        else if(num_bytes < 0)
            sys_error("error while reading Propeller's data");
    }
    while(get_time_ms() - t1 < timeout);

    fatal("recv1 timeout, no hardware?");
    return 0;
}

u8 recv1_select(ulong timeout)
{
    u8 byte;
    size_t num_bytes;
    int result;
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = timeout * 1000;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    result = select(fd + 1, &readfds, NULL, NULL, &tv);
    if(result < 0)
        fatal("select() on fd failed");

    if(FD_ISSET(fd, &readfds))
    {
        num_bytes = read(fd, &byte, 1);
        if(result < 1)
            sys_error("serial read failed");
    }
    else
        fatal("%s: timeout, propeller hardware not found.", __FUNCTION__);

    return byte - 0xFE;
}

/*******************************************************************************\
*                                                                               *
*   Connects to propeller on device @param fd and sends a @param command to it. *
*   @return propeller version                                                   *
*                                                                               *
\********************************************************************************/
u8 prop_init()
{
    u8 buff[509];
    prop_reset();
    sleep_msec(95);


    lfsr = 'P'; /* resetting lfsr */

    *buff = 0xF9;

    /* Transmit timing Calibration 0xF9 and 250 bytes lfsr data */
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
        if(recv1_select(100) != lfsr_step())
            fatal("recieved wrong LFSR, lost hardware connection?");
    }

    u8 version = 0;
    for(unsigned i = 0; i < 8; ++i)
        version |= (recv1_select(200) << i);

    return version;
}

/*****************************************************************\
*   Sets serial port @param fd settings.                          *
\*****************************************************************/
static struct termios oldtio, tio;

void set_serial()
{
    int result;

    result = tcgetattr(fd, &oldtio); /* save current serial port settings */
    if(result < 0)
        sys_error("failed to get serial attributes");


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
        sys_error("failed to set serial attributes");
}


/*****************************************************************\
*                                                                 *
*   Restores serial port @param fd  attributes.                   *
*                                                                 *
\*****************************************************************/
void restore_serial()
{
    int result = tcsetattr(fd,TCSANOW, &oldtio);
    if(result < 0)
        sys_error("failed to restore serial attributes");
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
        fprintf(stderr, "error setting realtime priority\n");
}

/*****************************************************************\
*                                                                 *
*   TODO write a proper serial port finder               *
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

u8 recieve_while_pinging(ulong timeout)
{
    u8 byte, f9 = 0xF9;
    size_t num_bytes;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 25 * 1000;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    ulong t = get_time_ms();
    while(get_time_ms() - t < timeout)
    {
        write(fd, &f9, 1); /* ping */
        select(fd + 1, &readfds, NULL, NULL, &tv); /* waiting for pong */

        if(FD_ISSET(fd, &readfds))
        {
            num_bytes = read(fd, &byte, 1);
            if(num_bytes < 1)
                sys_error("serial read failed");

            printf("read %c\n", byte);

            return byte - 0xFE;
        }
    }

    fatal("%s: waiting for propeller confirmation timed out.", __FUNCTION__);
    return 0;
}


/************************************************************************************\
*                                                                                    *
*   Sends a stream of instructions @param prog of length @param progsz to propeller. *
*                                                                                    *
\************************************************************************************/
static void prop_send_program(instruction_t* prog, size_t progsz)
{
    FILE* file = tmpfile();
    assemble(file);
    size_t imgsz = ftell(file);
    fseek(file, 0, SEEK_SET);
    u8* image = malloc(imgsz);
    fread(image, 1, imgsz, file);

    u32 op;
    prop_send_u32(imgsz); /* transmit the number of u32 in the image */

    for (unsigned i = 0; i < imgsz; i += 4)
    {
        op = (u32)(image[i] |
                   image[i + 1] << 8 |
                   image[i + 2] << 16 |
                   image[i + 3] << 24);

        prop_send_u32(op);
    }
    fsync(fd);

    /* Read a bit indicating whether checksum failed */
    if(recieve_while_pinging(8000))
        fatal("ram checksum failed");
}

/**********************************************************************\
*                                                                      *
*   Tries to execute a @param command on the propeller, connected to   *
*   @param device                                                      *
*                                                                      *
\**********************************************************************/
void prop_action(const char* device, u32 command)
{
    //fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    fd = open(device, O_RDWR | O_NOCTTY);
    if(fd < 3)
        sys_error("failed to open serial port");

    if(opt_verbose > 4)
        fprintf(vfile, "opened %s r/w fd: %i\n", device, fd);
    //fcntl(fd, F_SETFL, O_NONBLOCK); /* make the reads non-blocking */

    set_serial();

    mlockall(MCL_FUTURE);
    set_realtime_priority();

    u8 version = prop_init();
    printf("found propeller ver %u\n", version);

    prop_send_u32(command);

    prop_send_program(program, num_ops);

    munlockall();
    restore_serial(fd);
}

#include "loader.h"
#include "util.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <time.h>
#include <fcntl.h>
#include <sched.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

/*
the source code here is mostly ported from here
http://forums.parallax.com/showthread.php?t=106314&highlight=serial+lfsr
I will rework it in the near future with select or poll if I get this to work
*/

static u8 lsfr = 80;
static int fd;

static void reset_lsfr()
{
    lsfr = 80;
}

/*****************************************************************\
*                                                                 *
*   Returns least significant lfsr bit and iterates a step.       *
*                                                                 *
\*****************************************************************/
static int lfsr_step()
{
    int r = (lsfr & 1);
    lsfr = (lsfr << 1) | ((lsfr >> 7) ^ (lsfr >> 5) ^ (lsfr >> 4) ^ (lsfr >> 1) & 1);
    return r;
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
    if(result == -1)
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
    if(result == -1)
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
    sleep_msec(10);
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
void prop_write_u32(u32 data)
{
    static u8 buff[11];
    encode(buff, data);
    serial_write_buffer(buff, 11);
    sync();
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


/*******************************************************************************\
*                                                                               *
*   Connects to propeller on device @param fd and sends a @param command to it. *
*   @return propeller version                                                   *
*                                                                               *
\********************************************************************************/
u8 prop_init(int command)
{
    u8 buff[258];

    prop_reset(fd);
    sleep_msec(100);
    reset_lsfr();

    /* Transmit timing Calibration 0xF9 and 250 bytes lfsr data */
    *buff = 0xF9;
    for(unsigned i = 1; i < 251; i++)
    {
        buff[i] = 0xFE | lfsr_step();
    }

    serial_write_buffer(buff, 251);

    /* Send 258 timing calibrations */
    for(unsigned i = 0; i < 258; i++)
    {
        buff[i] = 0xF9;
    }

    serial_write_buffer(buff, 258);
    sync();

    /* Recieve 250 bytes of LFSR data */
    for(unsigned i = 0; i < 250; i++)
    {
        if(prop_recieve_bit(0, 100) != lfsr_step())
            fatal("error recieving LFSR");
    }

    u8 version = 0;

    for(unsigned i = 0; i < 8; ++i)
        version |= (prop_recieve_bit(0, 100) << i);

    prop_write_u32(command);

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

    tcflush(fd, TCIFLUSH);
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
    sparam.sched_priority = 0;
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

/************************************************************************************\
*                                                                                    *
*   Sends a stream of instructions @param prog of length @param progsz to propeller. *
*                                                                                    *
\************************************************************************************/
static void prop_send_program(instruction_t* prog, size_t progsz)
{
    u32 op;
    prop_write_u32(progsz); /* transmit the number of u32 in the image */

    for (unsigned i = 0; i < progsz; i++)
    {
        op = (u32)(prog[i].byte[0] |
                   prog[i].byte[1] << 8 |
                   prog[i].byte[2] << 16 |
                   prog[i].byte[3] << 24);

        prop_write_u32(op);
    }
    sync();

    /* Read a bit indicating whether checksum failed */
    if (prop_recieve_bit(1, 2500))
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
    fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(fd < 0)
        sys_error("failed to open serial port");

    set_serial(fd);

    mlockall(MCL_FUTURE);
    set_realtime_priority();

    u8 version = prop_init(command);
    prop_send_program(program, num_ops);

    munlockall();
    restore_serial(fd);
}

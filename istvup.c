/**
 * Detects if a TV is attached by looking for
 * the HDCP i2c port 0x3a. 
 *
 * Borrowed code from i2cdetect from the lm-sensors
 * project.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

/* dbus stuff */
#include "dbus/dbus.h"

#define HDCP_I2C_PORT 0x3a

/* thanks to lm-sensors */
__s32 i2c_smbus_access(int file, char read_write, __u8 command, int size, union i2c_smbus_data *data)
{
    struct i2c_smbus_ioctl_data args;
    __s32 err;

    args.read_write = read_write;
    args.command = command;
    args.size = size;
    args.data = data;

    err = ioctl(file, I2C_SMBUS, &args);
    if (err == -1)
        err = -errno;
    return err;
}

__s32 i2c_smbus_write_quick(int file, __u8 value)
{
    return i2c_smbus_access(file, value, 0, I2C_SMBUS_QUICK, NULL);
}

int main(int argc, char *argv[])
{
    int fd, ret, started = 1, timeout = 100000;
    char filename[20];
    DBusConnection *conn;
    DBusError err;
    DBusMessage *msg, *response, *startmsg, *stopmsg;
    const char *unit_name = "xbmc.service";
    const char *unit_mode = "replace";

    if(argc < 2) {
        fprintf(stderr, "usage: %s [i2c-dev]\n", argv[0]);
        exit(1);
    }

    snprintf(filename, 19, "/dev/i2c-%s", argv[1]);
    fd = open(filename, O_RDWR);
    if(fd < 0) {
        fprintf(stderr, "failed to open %s\n", filename);
        exit(1);
    }

    if(ioctl(fd, I2C_SLAVE, HDCP_I2C_PORT) < 0) {
        fprintf(stderr, "failed to set i2c slave\n");
        close(fd);
        exit(1);
    }

    /* setup dbus */
    dbus_error_init(&err);
    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if(dbus_error_is_set(&err)) {
        fprintf(stderr, "failed to open dbus: %s\n", err.message);
        close(fd);
        exit(1);
    }
    msg = dbus_message_new_method_call("org.freedesktop.systemd1", "/org/freedesktop/systemd1", "org.freedesktop.systemd1.Manager", "GetUnit");
    if(!msg) {
        fprintf(stderr, "failed to create dbus message\n");
        close(fd);
        exit(1);
    }
    dbus_message_append_args(msg, DBUS_TYPE_STRING, &unit_name, DBUS_TYPE_INVALID);

    response = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);
    if(dbus_error_is_set(&err)) {
        fprintf(stderr, "failed to get systemd unit: %s\n", err.message);
        close(fd);
        exit(1);
    }

    stopmsg = dbus_message_new_method_call("org.freedesktop.systemd1", "/org/freedesktop/systemd1", "org.freedesktop.systemd1.Manager", "StopUnit");
    if(!stopmsg) {
        fprintf(stderr, "failed to create dbus message\n");
        close(fd);
        exit(1);
    }
    dbus_message_append_args(stopmsg, DBUS_TYPE_STRING, &unit_name, DBUS_TYPE_STRING, &unit_mode, DBUS_TYPE_INVALID);
    
    startmsg = dbus_message_new_method_call("org.freedesktop.systemd1", "/org/freedesktop/systemd1", "org.freedesktop.systemd1.Manager", "StartUnit");
    if(!startmsg) {
        fprintf(stderr, "failed to create dbus message\n");
        close(fd);
        exit(1);
    }
    dbus_message_append_args(startmsg, DBUS_TYPE_STRING, &unit_name, DBUS_TYPE_STRING, &unit_mode, DBUS_TYPE_INVALID);

    ret = 0;

    while(1) {
        /* probe by writing */
        ret = i2c_smbus_write_quick(fd, I2C_SMBUS_WRITE);
        if(ret < 0) {
            /* tv is not on */
            if(started > 0) {
                /* stop xbmc */
                response = dbus_connection_send_with_reply_and_block(conn, stopmsg, -1, &err);
                if(dbus_error_is_set(&err)) {
                    fprintf(stderr, "failed to get systemd unit: %s\n", err.message);
                    close(fd);
                    exit(1);
                } else {
                    fprintf(stderr, "stopped xbmc\n");
                }
                started = 0;
                timeout = 10000; // sleep less
            }
        } else {
            if(started == 0) {
                /* tv is on, start xbmc */
                response = dbus_connection_send_with_reply_and_block(conn, startmsg, -1, &err);
                if(dbus_error_is_set(&err)) {
                    fprintf(stderr, "failed to get systemd unit: %s\n", err.message);
                    close(fd);
                    exit(1);
                } else {
                    fprintf(stderr, "started xbmc\n");
                }
                started = 1;
                timeout = 100000; // sleep more
            }
        }
        
        /* sleep a while */
        fprintf(stderr, "ping\n");
        usleep(1000 * timeout);
    }

    close(fd);
    exit(0);
}

istvup
======

Way too complicated piece of code to check the HDCP chip i2c address availability. In my case the chip drops from the i2c bus when the TV is put into standby. Combined with some basic dBus code it either starts or stops the xbmc.service through systemd.

Notes
-----
It's all pretty specific code, but it should run if your Linux distro happens to use systemd. You might be able to use this for other init systems. I simply do not know if reading/writing the HDCP chip like this harms the TV in any way! The code to check the address was taken from the lm-sensors project.

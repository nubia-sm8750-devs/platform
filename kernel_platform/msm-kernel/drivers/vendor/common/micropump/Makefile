# default file name is zte_pump.ko
ifeq ($(CONFIG_BOS1921_MICRO_PUMP), m)
	obj-m += zte_pump.o
	zte_pump-objs += bos1921.o
endif
ifeq ($(CONFIG_AW86320_MICRO_PUMP), m)
	obj-m += zte_pump.o
	zte_pump-objs += aw86320.o
endif

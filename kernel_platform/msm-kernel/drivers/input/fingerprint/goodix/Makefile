
ifeq ($(ZTE_CONFIG_FINGERPRINT_EXTCMD),true)
	 ccflags-y += -DZTE_CONFIG_FINGERPRINT_EXTCMD
endif

obj-m += zte_fingerprint.o

zte_fingerprint-y := gf_spi.o platform.o netlink.o

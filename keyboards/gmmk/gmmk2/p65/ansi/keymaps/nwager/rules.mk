# set to yes if you want console debugging
DEBUG = no

ifeq ($(DEBUG), yes)
	MOUSEKEY_ENABLE = no        # Mouse keys.
	EXTRAKEY_ENABLE = no        # Audio control and System control.
	NKRO_ENABLE = no            # USB Nkey Rollover.
	CONSOLE_ENABLE = yes        # Console for debug.
endif

# Set VERBOSE make variable to 1 to output all tool commands.
VERBOSE?=0
ifeq "$(VERBOSE)" "0"
Q=@
else
Q=
endif


PROJECT         := linuxcnc-passthru
DEVICES         := LPC1768
GCC4MBED_DIR    := /home/morris/Stuff/reprap/gcc4mbed
NO_FLOAT_SCANF  := 1
NO_FLOAT_PRINTF := 1
GCC4MBED_TYPE   := Debug
MRI_BREAK_ON_INIT := 0
MRI_UART_BAUD   := 9600
MRI_UART        := MRI_UART_0
#MRI_UART        := MRI_UART_MBED_USB MRI_UART_SHARE
MRI_SEMIHOST_STDIO := 0
MRI_ENABLE      := 1
NEWLIB_NANO     := 1

include $(GCC4MBED_DIR)/build/gcc4mbed.mk

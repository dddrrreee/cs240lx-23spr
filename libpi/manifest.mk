#BUILD_DIR := ./objs
#LIB := libpi.a
#START := ./staff-start.o

include $(CS240LX_2023_PATH)/libpi/defs.mk

ifeq ($(USE_FP),1)
    BUILD_DIR := ./fp-objs
    LIB := libpi-fp.a
    #LIBM := libm
    START := $(LPP)/staff-start-fp.o
    STAFF_OBJS := $(foreach o, $(STAFF_OBJS), $(dir $o)/fp/$(notdir $o))
else
    BUILD_DIR := ./objs
    LIB := libpi.a
    START := $(LPP)/staff-start.o
endif


# set this to the path of your ttyusb device if it can't find it
# automatically
TTYUSB = 

# the string to extract for checking
GREP_STR := 'HASH:\|ERROR:\|PANIC:\|PASS:\|TEST:'

# set if you want the code to automatically run after building.
RUN = 1
# set if you want the code to automatically check after building.
#CHECK = 0

DEPS += ./src

include $(CS240LX_2023_PATH)/libpi/mk/Makefile.lib.template

all:: $(START)

$(LPP)/staff-start.o: $(BUILD_DIR)/staff-start.o
	cp $(BUILD_DIR)/staff-start.o staff-start.o

$(LPP)/staff-start-fp.o: $(LPP)/fp-objs/staff-start.o
	cp $(LPP)/fp-objs/staff-start.o staff-start-fp.o


clean::
	rm -f staff-start.o
	rm -f staff-start-fp.o
	make -C  libc clean
	make -C  staff-src clean
ifneq ($(USE_FP),1)
	make clean USE_FP=1
	make -C libm clean
endif


.PHONY : libm test

ATT_ROOT := $(CURDIR)/../../../../
include $(ATT_ROOT)/att_patterns/scripts/config.mk

TARGET := p05_rx

SOURCES := $(wildcard *.c) $(wildcard ../*.c)  $(wildcard ../mp_lib/*.c)


INCLUDES += -I$(ATT_ROOT)/att_patterns/include -I$(ATT_ROOT)/att_patterns/include/soc-include -I$(ATT_ROOT)/att_patterns/include/arm -I$(ATT_ROOT)/att_patterns/include/gcc-include
INCLUDES += -I$(ATT_ROOT)/att_patterns/include/sdk-include  -I$(ATT_ROOT)/
INCLUDES +=   -I../  -I../mp_lib/ -I$(ATT_ROOT)/att_patterns/lib/

LIBS += $(ATT_ROOT)/att_patterns/lib/$(IC_TYPE)/libatt.$(LIBSUFFIX)
LIBS += $(ATT_ROOT)/att_patterns/lib/$(IC_TYPE)/socatp/libatpatt.$(LIBSUFFIX)

include $(ATT_ROOT)/att_patterns/scripts/$(IC_TYPE).mk
$(eval $(call MAKE_PROG,$(TARGET),$(SOURCES),$(LDFILE)))


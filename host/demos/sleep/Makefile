CS_API_PATH=../../snia_cs_api
CC=gcc
CFLAGS+=-g -O3
CPPFLAGS+=-I$(CS_API_PATH) -MMD -MP -D_GNU_SOURCE
LDLIBS+=-lnvme
LDFLAGS+=-O3
# -MMD Like -MD except mention only user header files, not system header files
# -MP add phony target for each header to prevent errors when header is missing

C_SOURCES = $(wildcard *.c)
DEPENDENCIES := $(C_SOURCES:.c=.d)

all : sleep

# Do not include the depency rules for "clean"
ifneq ($(MAKECMDGOALS),clean)
-include $(DEPENDENCIES)
endif

sleep : sleep.o $(CS_API_PATH)/cs_api_nvme_tsp.o $(CS_API_PATH)/cs_utils.o

clean :
	rm -f *.o *.d $(CS_API_PATH)/*.o $(CS_API_PATH)/*.d sleep

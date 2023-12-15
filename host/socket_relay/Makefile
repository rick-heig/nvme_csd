CS_API_PATH=../snia_cs_api

CC=gcc
CFLAGS+=-g -O3
CPPFLAGS+=-I$(CS_API_PATH) -MMD -MP -D_GNU_SOURCE
LDLIBS += -lnvme

all : relay

relay : main
	mv main relay

main : main.o $(CS_API_PATH)/cs_api_nvme_tsp.o

clean :
	rm -f main relay main.o main.d $(CS_API_PATH)/cs_api_nvme_tsp.o $(CS_API_PATH)/cs_api_nvme_tsp.d
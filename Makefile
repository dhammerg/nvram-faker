TRUNK ?=.

include $(TRUNK)/arch.mk

CC ?= cc

CFLAGS += -ggdb
CFLAGS += -DINI_MAX_LINE=2000
CFLAGS += -DINI_USE_STACK=0

INI_PATH = $(TRUNK)/contrib/inih
INCLUDES = -I$(INI_PATH)

LIB = libnvram-faker.so

COMMON_OBJS = ini.o

COMPAT_OBJ = compat.o
NVRAM_DATA_OBJ = nvram_data.o

.PHONY: all test exe clean

all: $(LIB)

# --------------------------------------------------
# Common INI object
# --------------------------------------------------

ini.o:
	$(MAKE) -C $(INI_PATH) ini.o
	cp $(INI_PATH)/ini.o .

# --------------------------------------------------
# Shared library
# --------------------------------------------------

nvram-faker.o: nvram-faker.c
	$(CC) -Wall $(INCLUDES) $(CFLAGS) -fPIC -c -o $@ $<

compat.o: compat.c
	$(CC) -Wall $(INCLUDES) $(CFLAGS) -fPIC -c -o $@ $<

nvram_data.o: nvram_data.c
	$(CC) -Wall $(INCLUDES) $(CFLAGS) -fPIC -c -o $@ $<

$(LIB): nvram-faker.o ini.o $(COMPAT_OBJ) $(NVRAM_DATA_OBJ)
	$(CC) -shared -o $@ $^ -Wl,-nostdlib

# --------------------------------------------------
# Standalone test
# --------------------------------------------------

test.o: test.c
	$(CC) -Wall $(INCLUDES) $(CFLAGS) \
		-DNVRAM_EXE -DDEBUG \
		-DINI_FILE_PATH=\"./nvram.ini\" \
		-c -o $@ $<

nvram-faker-test.o: nvram-faker.c
	$(CC) -Wall $(INCLUDES) $(CFLAGS) \
		-DNVRAM_EXE -DDEBUG \
		-DINI_FILE_PATH=\"./nvram.ini\" \
		-fPIC -c -o $@ $<

test: test.o nvram-faker-test.o ini.o
	$(CC) -Wall -o $@ $^

# --------------------------------------------------
# Standalone nvram faker executable
# --------------------------------------------------

nvram_faker_main.o: nvram_faker_main.c
	$(CC) -Wall $(INCLUDES) $(CFLAGS) \
		-DNVRAM_EXE -DDEBUG \
		-DINI_FILE_PATH=\"./nvram.ini\" \
		-c -o $@ $<

exe: nvram_faker_main.o nvram-faker.o ini.o
	$(CC) -Wall -o nvram_faker_exe $^

# --------------------------------------------------
# Clean
# --------------------------------------------------

clean:
	rm -f *.o *.so nvram_faker_exe test
	-$(MAKE) -C $(INI_PATH) clean
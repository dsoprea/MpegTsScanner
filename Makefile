# TODO: Create a common object file with tsscanner.o and pat.o .

PWD=$(shell pwd)

VERSION1=1
VERSION2=0
VERSION3=0

VERSION=$(VERSION1).$(VERSION2).$(VERSION3)
VERSION_TRANSITIONAL=$(VERSION1).$(VERSION2)
VERSION_ANCHOR=$(VERSION1)
INSTALL_PATH=/usr/local/lib

SRC_PATH=$(PWD)/tsscanner
EXAMPLE_PATH=$(PWD)/examples
OUTPUT_PATH=$(PWD)/build
DVBPSI_LD_PATHPARAM=#-L/home/dustin/build/libdvbpsi-0.2.2/src/.libs
TSSCANNER_INC_PATHPARAM=-I$(PWD)/

TSSCANNER_SO_FILENAME=libtsscanner.so
TSSCANNER_SO_NAME=$(TSSCANNER_SO_FILENAME).$(VERSION_ANCHOR)
TSFINDFIRSTPROGRAM_SO_FILENAME=libtsfindfirstprogram.so
TSFINDFIRSTPROGRAM_SO_NAME=$(TSFINDFIRSTPROGRAM_SO_FILENAME).$(VERSION_ANCHOR)

TSSCANNER_SO_INSTALLPATH=$(INSTALL_PATH)/$(TSSCANNER_SO_NAME)
TSFINDFIRSTPROGRAM_SO_INSTALLPATH=$(INSTALL_PATH)/$(TSFINDFIRSTPROGRAM_SO_NAME)

CC=gcc
CFLAGS=-g
#-Wall -Werror 

all: $(OUTPUT_PATH)/$(TSSCANNER_SO_NAME) $(OUTPUT_PATH)/$(TSFINDFIRSTPROGRAM_SO_NAME)

$(OUTPUT_PATH)/$(TSSCANNER_SO_NAME): $(OUTPUT_PATH)/tsscanner.o $(OUTPUT_PATH)/pat.o
# -Bdynamic makes sure to add the library to the list of required shared objects (in the output of ldd).
# -Wl,-soname,mpeg_scanner.so 
	$(CC) -shared $(CFLAGS) -Wno-deprecated-declarations -Wl,-soname,$(TSSCANNER_SO_NAME) -o $(OUTPUT_PATH)/$(TSSCANNER_SO_NAME) -Wl,-Bstatic $(OUTPUT_PATH)/tsscanner.o $(OUTPUT_PATH)/pat.o -Wl,-Bdynamic -lm $(DVBPSI_LD_PATHPARAM) -ldvbpsi

$(OUTPUT_PATH)/tsscanner.o: $(SRC_PATH)/tsscanner.c
	$(CC) -c -fpic -Wno-deprecated-declarations $(TSSCANNER_INC_PATHPARAM) $(CFLAGS) -o $(OUTPUT_PATH)/tsscanner.o $(SRC_PATH)/tsscanner.c

$(OUTPUT_PATH)/pat.o: $(SRC_PATH)/pat.c
	$(CC) -c -fpic -Wno-deprecated-declarations $(TSSCANNER_INC_PATHPARAM) $(CFLAGS) -o $(OUTPUT_PATH)/pat.o $(SRC_PATH)/pat.c

$(OUTPUT_PATH)/tsfindfirstprogram.o: $(SRC_PATH)/tsfindfirstprogram.c
	$(CC) -c -fpic -Wno-deprecated-declarations $(TSSCANNER_INC_PATHPARAM) $(CFLAGS) -o $(OUTPUT_PATH)/tsfindfirstprogram.o $(SRC_PATH)/tsfindfirstprogram.c

$(OUTPUT_PATH)/$(TSFINDFIRSTPROGRAM_SO_NAME): $(OUTPUT_PATH)/tsscanner.o $(OUTPUT_PATH)/tsfindfirstprogram.o $(OUTPUT_PATH)/pat.o
	$(CC) -shared -Wno-deprecated-declarations $(CFLAGS) -Wl,-soname,$(TSFINDFIRSTPROGRAM_SO_NAME) -o $(OUTPUT_PATH)/$(TSFINDFIRSTPROGRAM_SO_NAME) -Wl,-Bstatic $(OUTPUT_PATH)/tsscanner.o $(OUTPUT_PATH)/tsfindfirstprogram.o $(OUTPUT_PATH)/pat.o -Wl,-Bdynamic $(DVBPSI_LD_PATHPARAM) -ldvbpsi -lm

examples: $(EXAMPLE_PATH)/tsfindfirstprogram_example

$(EXAMPLE_PATH)/tsfindfirstprogram_example.o: $(EXAMPLE_PATH)/tsfindfirstprogram_example.c
	$(CC) -c $(TSSCANNER_INC_PATHPARAM) $(CFLAGS) -o $(EXAMPLE_PATH)/tsfindfirstprogram_example.o $(EXAMPLE_PATH)/tsfindfirstprogram_example.c

$(EXAMPLE_PATH)/tsfindfirstprogram_example: $(EXAMPLE_PATH)/tsfindfirstprogram_example.o
	$(CC) -Wno-deprecated-declarations $(TSSCANNER_INC_PATHPARAM) $(CFLAGS) -Wl,-Bstatic -o $(EXAMPLE_PATH)/findfirstprogram_example $(EXAMPLE_PATH)/tsfindfirstprogram_example.o -Wl,-Bdynamic -lm $(DVBPSI_LD_PATHPARAM) -ldvbpsi -ltsscanner -ltsfindfirstprogram

install:
	cp $(OUTPUT_PATH)/$(TSSCANNER_SO_NAME) $(INSTALL_PATH)
	ln -s $(INSTALL_PATH)/$(TSSCANNER_SO_NAME) $(INSTALL_PATH)/$(TSSCANNER_SO_FILENAME)
	ln -s $(INSTALL_PATH)/$(TSSCANNER_SO_NAME) $(INSTALL_PATH)/$(TSSCANNER_SO_FILENAME).$(VERSION)
	cp $(OUTPUT_PATH)/$(TSFINDFIRSTPROGRAM_SO_NAME) $(INSTALL_PATH)
	ln -s $(INSTALL_PATH)/$(TSFINDFIRSTPROGRAM_SO_NAME) $(INSTALL_PATH)/$(TSFINDFIRSTPROGRAM_SO_FILENAME)
	ln -s $(INSTALL_PATH)/$(TSFINDFIRSTPROGRAM_SO_NAME) $(INSTALL_PATH)/$(TSFINDFIRSTPROGRAM_SO_FILENAME).$(VERSION)

clean:
	rm -fr examples/*_example examples/*_example.o build/* $(INSTALL_PATH)/$(TSSCANNER_SO_FILENAME)* $(INSTALL_PATH)/$(TSFINDFIRSTPROGRAM_SO_FILENAME)*


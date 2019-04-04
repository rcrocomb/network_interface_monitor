# Author: Robert Crocombe
#

CC = gcc
CXX = g++
AR = ar

# -O2 optimize a bit: usually -O3 is not so great for gcc
# -Wall turn on all warnings
# -MMD autobuild dependencies: leave out system headers
# -MF the name of the dependency file to use.
COMMON_FLAGS = -DDEBUG_ON=$(DEBUG_ON) -g3 -Wall
CCFLAGS = $(COMMON_FLAGS)
CXXFLAGS = $(COMMON_FLAGS)
LIBRARIES =

DEBUG_ON=1

SOURCE_DIR = .
INCLUDE_DIR = .

INCLUDES = -I$(INCLUDE_DIR)

# What we're generating


MAIN_SOURCE = $(SOURCE_DIR)/main.cpp \
	      $(SOURCE_DIR)/network_stats.cpp

CXX_SOURCE = $(MAIN_SOURCE)
C_SOURCE =

# here's what we want to make
MAINFILE = main

# here's how we make it
.SUFFIXES: .cpp .c .o

.c.o:
	$(CC) $(CCFLAGS) $(INCLUDES) -c $*.c -o $*.o
	$(CC) $(CCFLAGS) $(INCLUDES) -MM $*.c > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -rf $*.d.tmp
.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $*.cpp -o $*.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MM $*.cpp > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -rf $*.d.tmp

# the targets

CXX_OBJECTS = $(CXX_SOURCE:.cpp=.o)
C_OBJECTS = $(C_SOURCE:.c=.o)

OBJECTS = $(CXX_OBJECTS) $(C_OBJECTS)
DEPS = $(OBJECTS:.o=.d)

$(MAINFILE):	$(OBJECTS)
		$(CXX) $(OBJECTS) $(LIBRARIES) -o $@

-include $(OBJECTS:.o=.d)

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(DEPS)

.PHONY: mrproper
mrproper:
	rm -f $(OBJECTS) $(MAINFILE) $(DEPS)

# ========================================================================================
# Makefile for Client test		
# ========================================================================================


# ========================================================================================
# Set include directories.
INCLUDEDIR = /usr/local/include

# ========================================================================================
# Compile flags
CPPFLAGS = 
CC=gcc
#compile
# ========================================================================================
# TARGET/Source definitions
SRCDIR = .
TARGET = epoll_client
OBJS = $(TARGET).o
SRC = $(wildcard $(SRCDIR)/*.c)

# ========================================================================================
# Make all option.
LDLIBS = 
LIBS = -l pthread

all: $(TARGET)
$(TARGET): $(SRC:.cpp=.o)
		$(CC) $(CPPFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS) $(LIBS)

# ========================================================================================
# Clean all option

clean:
	rm -f $(SRCDIR)*.o $(SRCDIR)*~ $(SRCDIR)fcs $(SRCDIR).depend
	rm -f *.o *~ $(TARGET) .depend $(TARGET)




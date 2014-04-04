SDLCFLAGS = -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT
SDLLDFLAGS = -lSDL -lpthread

CC = gcc
CFLAGS = -Wall -O2 $(SDLCFLAGS)
#CFLAGS = -Wall -O2 $(SDLCFLAGS) -DUSE_SEGFAULT_UNGRAB
#CFLAGS = -Wall -g $(SDLCFLAGS)
LDFLAGS = -lm $(SDLLDFLAGS)
OBJDIR = obj
TARGET = $(OBJDIR)/fenton

OBJS =	$(OBJDIR)/bswap.o \
	$(OBJDIR)/vec.o \
	$(OBJDIR)/pak.o \
	$(OBJDIR)/appio.o \
	$(OBJDIR)/fenton.o \
	$(OBJDIR)/render.o

all: $(TARGET)

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

########################################################################

$(OBJDIR)/bswap.o: bswap.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/vec.o: vec.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/pak.o: pak.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/appio.o: appio.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/fenton.o: fenton.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/render.o: render.c
	$(CC) -c $(CFLAGS) $? -o $@

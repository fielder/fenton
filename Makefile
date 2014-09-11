SDLCFLAGS = -I/usr/include/SDL2 -D_REENTRANT
SDLLDFLAGS = -L/usr/lib/x86_64-linux-gnu -lSDL2

CC = gcc
CFLAGS = -Wall -O2 $(SDLCFLAGS)
LDFLAGS = -lm $(SDLLDFLAGS)
OBJDIR = obj
TARGET = $(OBJDIR)/fenton

OBJS =	$(OBJDIR)/vec.o \
	$(OBJDIR)/pak.o \
	$(OBJDIR)/appio.o \
	$(OBJDIR)/fenton.o

all: $(TARGET)

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

########################################################################

#$(OBJDIR)/bswap.o: bswap.c
#	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/vec.o: vec.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/pak.o: pak.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/appio.o: appio.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/fenton.o: fenton.c
	$(CC) -c $(CFLAGS) $? -o $@
#$(OBJDIR)/render.o: render.c
#	$(CC) -c $(CFLAGS) $? -o $@
#$(OBJDIR)/rdata.o: rdata.c
#	$(CC) -c $(CFLAGS) $? -o $@

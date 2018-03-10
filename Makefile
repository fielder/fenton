SDLCFLAGS = -I/usr/include/SDL2 -D_REENTRANT
SDLLDFLAGS = -L/usr/lib/x86_64-linux-gnu -lSDL2

CC = gcc
CFLAGS = -Wall -O2 $(SDLCFLAGS)
#CFLAGS = -g -Wall -O2 $(SDLCFLAGS)
LDFLAGS = -lm $(SDLLDFLAGS)
OBJDIR = obj
TARGET = $(OBJDIR)/fenton

OBJS =	$(OBJDIR)/bdat.o \
	$(OBJDIR)/vec.o \
	$(OBJDIR)/pak.o \
	$(OBJDIR)/appio.o \
	$(OBJDIR)/fdata.o \
	$(OBJDIR)/render.o \
	$(OBJDIR)/r_misc.o \
	$(OBJDIR)/fenton.o \
	$(OBJDIR)/r_span.o \
	$(OBJDIR)/r_bsp.o \
	$(OBJDIR)/map.o \
	$(OBJDIR)/r_poly.o \
	$(OBJDIR)/clip.o \

all: $(TARGET)

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

########################################################################

$(OBJDIR)/bdat.o: bdat.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/vec.o: vec.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/pak.o: pak.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/appio.o: appio.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/fdata.o: fdata.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/render.o: render.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/r_misc.o: r_misc.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/fenton.o: fenton.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/r_span.o: r_span.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/r_bsp.o: r_bsp.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/map.o: map.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/r_poly.o: r_poly.c
	$(CC) -c $(CFLAGS) $? -o $@
$(OBJDIR)/clip.o: clip.c
	$(CC) -c $(CFLAGS) $? -o $@

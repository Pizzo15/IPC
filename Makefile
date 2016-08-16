# Variable
CC := gcc
LD := gcc
CFLAGS := -c -Wall

# Objects
elab2_OBJS := elab2.o functions.o
routine_OBJS := routine.o functions.o

# Libraries
LIBS := functions.h

# Target
all: elab2 routine

elab2: $(elab2_OBJS)
	$(LD) $(elab2_OBJS) -o elab2

routine: $(routine_OBJS)
	$(LD) $(routine_OBJS) -o routine

# Compiling
elab2.o: elab2.c $(LIBS)
	$(CC) $(CFLAGS) elab2.c

routine.o: routine.c $(LIBS)
	$(CC) $(CFLAGS) routine.c

functions.o: functions.c $(LIBS)
	$(CC) $(CFLAGS) functions.c

clean:
	rm -f $(elab2_OBJS) $(routine_OBJS) elab2 routine res.txt
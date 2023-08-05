# Makefile for libPowerEagle

CC = gcc
CFLAGS = -fPIC -Wall -Wextra -O2 -g -DSB_LINUX_BUILD -I. -I./../../
CPPFLAGS = -fPIC -Wall -Wextra -O2 -g -DSB_LINUX_BUILD -std=c++17 -I. -I./../../
LDFLAGS = -shared -lstdc++ -lcurl
RM = rm -f
STRIP = strip
TARGET_LIB = libPowerEagle.so

SRCS = main.cpp PowerEagle.cpp x2powercontrol.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: all
all: ${TARGET_LIB}

$(TARGET_LIB): $(OBJS)
	$(CC) ${LDFLAGS} -o $@ $^
	patchelf --add-needed libcurl.so.1 libPowerEagle.so
	$(STRIP) $@ >/dev/null 2>&1  || true

$(SRCS:.cpp=.d):%.d:%.cpp
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $< >$@

.PHONY: clean
clean:
	${RM} ${TARGET_LIB} ${OBJS} *.d

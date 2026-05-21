# This makefile is for all platforms.

CC      = cc
CXX     = c++

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
CFLAGS  = -g -O3 -Wall -std=c++17 -pthread -MMD -MD -I/usr/local/include -I/opt/homebrew/include -I/opt/homebrew/opt/mosquitto/include -I/opt/homebrew/opt/nlohmann-json/include
LIBS    = -lpthread -lmosquitto
LDFLAGS = -g -L/usr/local/lib -L/opt/homebrew/lib -L/opt/homebrew/opt/mosquitto/lib
else
CFLAGS  = -g -O3 -Wall -std=c++17 -Wno-psabi -pthread -MMD -MD -I/usr/local/include
LIBS    = -lpthread -lutil -lmosquitto
LDFLAGS = -g -L/usr/local/lib
endif

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

all:	MMDVMHost

MMDVMHost:	GitVersion.h $(OBJS) 
		$(CXX) $(OBJS) $(LDFLAGS) $(LIBS) -o MMDVMHost

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<
-include $(DEPS)

.PHONY install:
install: all
		install -m 755 MMDVMHost /usr/local/bin/

.PHONY install-service:
install-service: install /etc/MMDVMHost.ini
		@useradd --user-group -M --system mmdvm --shell /bin/false || true
		@usermod --groups dialout --append mmdvm || true
		@mkdir /var/log/mmdvm || true
		@chown mmdvm:mmdvm /var/log/mmdvm
		@cp ./linux/systemd/mmdvmhost.service /lib/systemd/system/
		@systemctl enable mmdvmhost.service

/etc/MMDVMHost.ini:
		@cp -n MMDVMHost.ini /etc/MMDVMHost.ini
		@sed -i 's/FilePath=./FilePath=\/var\/log\/mmdvm\//' /etc/MMDVMHost.ini
		@sed -i 's/Daemon=0/Daemon=1/' /etc/MMDVMHost.ini
		@chown mmdvm:mmdvm /etc/MMDVMHost.ini

.PHONY uninstall-service:
uninstall-service:
		@systemctl stop mmdvmhost.service || true
		@systemctl disable mmdvmhost.service || true
		@rm -f /usr/local/bin/MMDVMHost || true
		@rm -f /lib/systemd/system/mmdvmhost.service || true

clean:
		$(RM) MMDVMHost *.o *.d *.bak *~ GitVersion.h

# Export the current git version if the index file exists, else 000...
GitVersion.h:
ifneq ("$(wildcard .git/index)","")
	echo "const char *gitversion = \"$(shell git rev-parse HEAD)\";" > $@
else
	echo "const char *gitversion = \"0000000000000000000000000000000000000000\";" > $@
endif

ifeq ($(shell uname),Linux)
	LIBWEBRTC=libwebrtc/linux
else
	LIBWEBRTC=libwebrtc/mac
	CXXFLAGS= -O0 -I $(LIBWEBRTC)/include -std=c++14 -I $(LIBWEBRTC)/include/third_party/abseil-cpp -D WEBRTC_MAC=1 -D WEBRTC_POSIX=1 -fvisibility=hidden -I mrs3/mac
	LDFLAGS := $(LD_FLAGS) -framework Foundation $(LIBWEBRTC)/lib/libwebrtc.a mrs3/mac/libmrs3.a
	OS := macos
endif




all: test sv

test: test.cpp
	g++ $(CXXFLAGS) test.cpp $(LDFLAGS) -o test

sv: sv.cpp
	g++ $(CXXFLAGS) sv.cpp -o sv $(LDFLAGS) 

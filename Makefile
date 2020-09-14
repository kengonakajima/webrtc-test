ifeq ($(shell uname),Linux)
	LIBWEBRTC=libwebrtc/linux
else
	LIBWEBRTC=libwebrtc/mac
	CXXFLAGS=-I $(LIBWEBRTC)/include -std=c++14 -I $(LIBWEBRTC)/include/third_party/abseil-cpp -D WEBRTC_MAC=1 -D WEBRTC_POSIX=1
	LDFLAGS := $(LD_FLAGS) -framework Foundation $(LIBWEBRTC)/lib/libwebrtc.a
	OS := macos
endif




all: test sv

test: test.cpp
	g++ $(CXXFLAGS) test.cpp $(LDFLAGS) -o test

sv: sv.cpp
	g++ $(CXXFLAGS) sv.cpp -o sv

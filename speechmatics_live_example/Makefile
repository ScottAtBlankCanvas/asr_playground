CPP=clang
CXX=clang
CC=clang

LIB_HOME = ~/code/oss

BOOST_HOME = $(LIB_HOME)/boost_1_89_0
WEBSOCKETPP_HOME = $(LIB_HOME)/websocketpp-0.8.2/websocketpp

# additional flags
CPPFLAGS += -I /usr/local/include -std=c++14 -stdlib=libc++ -I $(WEBSOCKETPP_HOME) -I $(BOOST_HOME)

BOOST_LIBS = $(BOOST_HOME)/stage/lib/libboost_log.a \
	     $(BOOST_HOME)/stage/lib/libboost_thread.a \
	     $(BOOST_HOME)/stage/lib/libboost_program_options.a 

CPPFLAGS += -Wall -g
CFLAGS += -Wall -g
LDLIBS := $(BOOST_LIBS) -lc++ -lm -lssl -lcrypto

EXAMPLES=speechmatics_client

OBJS=speechmatics_client.o

.phony: all clean

all: $(OBJS) $(EXAMPLES)

clean:
	$(RM) $(EXAMPLES) $(OBJS)

env:
	echo "BOOST_HOME :" $(BOOST_HOME)
	echo "BOOST_LIBS :" $(BOOST_LIBS)
	

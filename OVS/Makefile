all: user_reader

USER_HEADR = tuple.h ringbuffer.h util.h hash.h chainsketch.hpp
USER_SRC = ringbuffer_user.cpp hash.c chainsketch.cpp
USER_LIB = -lpthread -lrt
INS_SET = -mbmi -msse -msse3 -msse4.1 -msse4.2 -mavx -march=native

user_reader: user_reader.cpp $(USER_HEADER) $(USER_SRC)
	g++ -O3 -g -Wall -std=c++11 -o $@ $< $(USER_SRC) $(USER_LIB) $(INS_SET)

clean:
	rm -rf user_reader

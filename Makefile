EXEC += main_hitter main_changer
all: $(EXEC)

CFLAGS = -Wall -std=c++11 -O3
HEADER += hash.h datatypes.hpp util.h adaptor.hpp 
SRC += hash.c adaptor.cpp
SKETCHHEADER += chainsketch.hpp
SKETCHSRC += chainsketch.cpp
LIBS= -lpcap 

main_changer: main_changer.cpp $(SRC) $(HEADER) $(SKETCHHEADER)
	g++ $(CFLAGS) $(INCLUDES) -o $@ $< $(SRC) $(SKETCHSRC) $(LIBS)

main_hitter: main_hitter.cpp $(SRC) $(HEADER) $(SKETCHHEADER) 
	g++ $(CFLAGS) $(INCLUDES) -o $@ $< $(SRC) $(SKETCHSRC) $(LIBS) 


clean:
	rm -rf $(EXEC)
	rm -rf *log*
	rm -rf *out*

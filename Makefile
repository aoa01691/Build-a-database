CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -pedantic

all: kvstore

kvstore: main.cpp
	$(CXX) $(CXXFLAGS) -o kvstore main.cpp

clean:
	rm -f kvstore

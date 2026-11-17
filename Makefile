CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic

agenda: src/main.cpp
	$(CXX) $(CXXFLAGS) -o $@ src/main.cpp

clean:
	rm -f agenda agenda_teste

.PHONY: clean

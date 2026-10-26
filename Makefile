CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

library: main.cpp utils.h models.h database.h library.h printer.h member_ui.h admin_ui.h
	$(CXX) $(CXXFLAGS) -o library main.cpp

run: library
	./library

clean:
	rm -f library

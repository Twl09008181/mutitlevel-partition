

CXX = g++
CXXFALG = -std=c++11 -g 
CPPFLAGS = -D DEBUG


obj = src/FM.o
DEP = include/FM.hpp

Lab2 : main.cpp $(obj) $(DEP)
	$(CXX) $(CXXFALG) $(CPPFLAGS) -o $@ $^ 

clean:
	rm -f Lab2.exe ${obj}
.phony : clean

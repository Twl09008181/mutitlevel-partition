

CXX = g++
CXXFALG = -std=c++11 -g -DDEBUG



obj = src/FM.o
DEP = include/FM.hpp

Lab2 : main.cpp $(obj) $(DEP)
	$(CXX) $(CXXFALG) -o $@ $^ 

clean:
	rm -f *.exe *.o

INFO = -Minfo=all
LIBS = -lboost_program_options -cudalib=cublas
GPU = -acc=gpu
CXX = pgc++

all:gpu

gpu2: main.cpp
$(CXX) $(GPU) $(INFO) $(LIBS) -o $@ $<

clean:all
rm gpu

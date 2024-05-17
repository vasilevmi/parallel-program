INFO = -Minfo=all
LIBS = -lboost_program_options
GPU = -acc=gpu
CXX = pgc++

all:gpu
	
gpu:
	$(CXX) $(GPU) $(INFO) $(LIBS) -o $@ reliz_2.cpp

clean:all
	rm gpu
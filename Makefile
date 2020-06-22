CXX=llvm-g++
INCS=-c -std=c++17 \
-I/usr/local/Cellar/glew/2.1.0_1/include \
-I/usr/local/Cellar/glfw/3.3.2/include \
-I/usr/local/Cellar/freeimage/3.18.0/include \
-I/usr/local/Cellar/glm/0.9.9.8/include \
-I/Users/YJ-work/cpp/myGL_glfw/fftWater/header

LIBS=-L/usr/local/Cellar/glfw/3.3.2/lib -lGLEW \
-L/usr/local/Cellar/glew/2.1.0_1/lib -lglfw \
-L/usr/local/Cellar/freeimage/3.18.0/lib -lfreeimage \
-framework GLUT -framework OpenGL -framework Cocoa

SRC_DIR=/Users/YJ-work/cpp/myGL_glfw/fftWater/src

all: main test

main: main.o fft.o timer.o common.o ocean.o skybox.o screenQuad.o
	$(CXX) $(LIBS) $^ -o $@

test: test.o
	$(CXX) $(LIBS) $^ -o $@

test.o: $(SRC_DIR)/test.cpp
	$(CXX) -c $(INCS) $^ -o $@

main.o: $(SRC_DIR)/main.cpp
	$(CXX) -c $(INCS) $^ -o $@

common.o: $(SRC_DIR)/common.cpp
	$(CXX) -c $(INCS) $^ -o $@

fft.o:$(SRC_DIR)/fft.cpp
	$(CXX) -c $(INCS) $^ -o $@

timer.o: $(SRC_DIR)/timer.cpp
	$(CXX) -c $(INCS) $^ -o $@

ocean.o: $(SRC_DIR)/ocean.cpp
	$(CXX) -c $(INCS) $^ -o $@

skybox.o: $(SRC_DIR)/skybox.cpp
	$(CXX) -c $(INCS) $^ -o $@

screenQuad.o: $(SRC_DIR)/screenQuad.cpp
	$(CXX) -c $(INCS) $^ -o $@

.PHONY: cleanObj cleanImg video

cleanImg:
	rm -v ./result/*

cleanObj:
	rm -f *.o

video:
	ffmpeg -r 30 -start_number 0 -i ./result/output%04d.bmp -vcodec mpeg4 -b:v 30M -s 400x300 ./result.mp4

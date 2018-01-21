OBJS = main.o imgui_impl_sdl_gl2.o imgui/imgui.o imgui/imgui_demo.o imgui/imgui_draw.o
LIBS = -lGL -lX11 -lXi -lGLEW `sdl2-config --libs`
CXXFLAGS = -I imgui -O3 -Wall -Wformat `sdl2-config --cflags`


all: $(OBJS)
	g++ -o main $(OBJS) $(CXXFLAGS) $(LIBS)

.cpp.o:
	g++ $(CXXFLAGS) -c -o $@ $<

clean:
	rm main $(OBJS)

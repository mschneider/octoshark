DEFINE_ANGLES  = -D SHEARING_ANGLES=-20,-10,0,10,20
DEFINE_TEXT    = -D TEXT_OUTPUT=\"\"
DEFINE_DRAW    =#-D SHOW_PICTURES # DRAW_LETTER_CONNECTIONS DRAW_LETTER_GROUPS 
DEFINE_IMAGE   =#-D IMAGE_OUTPUT=\"../Extraction/\"
DEFINE_CONTOUR =#-D NO_CONTOURS
DEFINE_LINE    = -D HORIZONTAL_LINES_WITH_FRUSTUM # HORIZONTAL_LINES_WITH_CONE=3 HORIZONTAL_LINES_WITH_FRUSTUM
DEFINES  = $(DEFINE_ANGLES) $(DEFINE_TEXT) $(DEFINE_IMAGE) $(DEFINE_CONTOUR) $(DEFINE_LINE) $(DEFINE_DRAW)
OPTFLAGS = -O3 -mtune=native -march=core2 -msse4

CPPFLAGS  = -Wextra $(OPTFLAGS) `pkg-config --cflags opencv`
LINKFLAGS = $(OPTFLAGS) `pkg-config --libs opencv`
MODULES = build/config.o build/pictures.o build/ray.o build/component.o build/contour.o build/candidate.o

octoshark: build/main.o $(MODULES)
	g++ -o octoshark $(LINKFLAGS) $^

build/%.o : src/%.cpp src/%.hpp
	mkdir -p build
	g++ -c -o $@ $(CPPFLAGS) $(DEFINES) $<
		
src/main.hpp:

xy: build/xy/main.o build/xy/config.o build/xy/pictures.o build/xy/ray.o build/xy/component.o build/xy/contour.o build/xy/candidate.o
	g++ -o octoshark $(LINKFLAGS) $^

build/xy/%.o : src/%.cpp src/%.hpp
	mkdir -p build
	mkdir -p build/xy	
	g++ -c -o $@ $(CPPFLAGS) $(DEFINE_TEXT) $<

frustum: build/frustum/main.o build/frustum/config.o build/frustum/pictures.o build/frustum/ray.o build/frustum/component.o build/frustum/contour.o build/frustum/candidate.o
	g++ -o octoshark $(LINKFLAGS) $^

build/frustum/%.o : src/%.cpp src/%.hpp
	mkdir -p build
	mkdir -p build/frustum	
	g++ -c -o $@ $(CPPFLAGS) $(DEFINE_TEXT) -D HORIZONTAL_LINES_WITH_FRUSTUM $<

cone30: build/cone30/main.o build/cone30/config.o build/cone30/pictures.o build/cone30/ray.o build/cone30/component.o build/cone30/contour.o build/cone30/candidate.o
	g++ -o octoshark $(LINKFLAGS) $^

build/cone30/%.o : src/%.cpp src/%.hpp
	mkdir -p build
	mkdir -p build/cone30	
	g++ -c -o $@ $(CPPFLAGS) $(DEFINE_TEXT) -D HORIZONTAL_LINES_WITH_CONE=3 $<

cone22: build/cone22/main.o build/cone22/config.o build/cone22/pictures.o build/cone22/ray.o build/cone22/component.o build/cone22/contour.o build/cone22/candidate.o
	g++ -o octoshark $(LINKFLAGS) $^

build/cone22/%.o : src/%.cpp src/%.hpp
	mkdir -p build
	mkdir -p build/cone22	
	g++ -c -o $@ $(CPPFLAGS) $(DEFINE_TEXT) -D HORIZONTAL_LINES_WITH_CONE=4 $<

clean:
	rm -f octoshark
	rm -rf build
	
rebuild: clean
	make -j
	
tests: build/tests/rays 
	build/tests/rays

build/tests/%: src/tests/%.cpp $(MODULES)
	mkdir -p build
	mkdir -p build/tests
	g++ -o $@ $(LINKFLAGS) $(CPPFLAGS) -g -O0 $< $(MODULES)

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -Wno-format
LINKFLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image -lcpr -lfmt
RESOURCEFLAGS = "./res/Resource.res"

TARGET = Overlay

BUILDDIR = .
BUILDEXE = $(BUILDDIR)/$(TARGET).exe

RELEASEFLAGS = -O2 -DNDEBUG -mwindows
DEBUGFLAGS = -g -O0 -DDEBUG

.PHONY: release

release:
	$(CXX) $(TARGET).cpp -o $(BUILDEXE) $(RELEASEFLAGS) $(RESOURCEFLAGS) $(CXXFLAGS) $(LINKFLAGS)

debug:
	$(CXX) $(TARGET).cpp -o $(BUILDEXE) $(DEBUGFLAGS) $(RESOURCEFLAGS) $(CXXFLAGS) $(LINKFLAGS)

# Other rules
clean:
	del $(TARGET).exe

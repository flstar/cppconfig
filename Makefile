.PHONY: all clean

all: ConfigTest

TARGET = ConfigTest
OBJS = ConfigTest.o Config.o
LIBS = -lyaml-cpp -lgtest -lpthread

CXX = g++
CXXFLAGS = -g

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

.c.o:
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -rf $(TARGET) $(OBJS)

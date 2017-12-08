.PHONY: default clean


TARGET = ConfigTest
SRCS = Config.cc ConfigTest.cc
LIBS = -lyaml-cpp -lgtest -lpthread

OBJS = $(SRCS:.cc=.o)
DEPS = $(OBJS:.o=.d)

CXX = g++
CXXFLAGS = -g -MMD

-include $(DEPS)

default: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LIBS) $^ -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET) $(OBJS) $(DEPS)

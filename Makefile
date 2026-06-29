CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS  = /usr/lib/x86_64-linux-gnu/libsqlite3.so.0
# If libsqlite3-dev is installed, replace above with: LDFLAGS = -lsqlite3

TARGET   = n003bsitsystem

.PHONY: all clean run install-deps

all:
	$(CXX) $(CXXFLAGS) -o $(TARGET) main.cpp $(LDFLAGS)
	@echo ""
	@echo "  Build successful! Run with: ./$(TARGET)"
	@echo ""

run: all
	./$(TARGET)

install-deps:
	sudo apt-get install -y libsqlite3-dev

clean:
	rm -f $(TARGET) n003bsit.db

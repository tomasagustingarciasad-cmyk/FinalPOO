# Makefile for Servidor Package
# Author: Generated for POO TP2 Req4

# Compiler and flags
CXXFLAGS = -std=c++17 -Wall -Wextra -g -O2 -pthread
INCLUDES = -I./inc -I./lib $(shell pkg-config --cflags libpqxx libpq)
LDFLAGS = -pthread $(shell pkg-config --libs libpqxx libpq)

# Server sources
SERVER_SOURCES = $(wildcard src/server/*.cpp)

# XML-RPC library source files
XMLRPC_SOURCES = lib/XmlRpcClient.cpp \
				 lib/XmlRpcDispatch.cpp \
				 lib/XmlRpcServer.cpp \
				 lib/XmlRpcServerConnection.cpp \
				 lib/XmlRpcServerMethod.cpp \
				 lib/XmlRpcSocket.cpp \
				 lib/XmlRpcSource.cpp \
				 lib/XmlRpcUtil.cpp \
				 lib/XmlRpcValue.cpp \
			 inc/Robot.cpp \
			 lib/SerialPort.cpp \
			 lib/db_pool.cpp \
			 lib/CSVLogger.cpp

# Object files for XML-RPC library
XMLRPC_OBJECTS = $(XMLRPC_SOURCES:.cpp=.o)
SERVER_OBJECTS = $(SERVER_SOURCES:.cpp=.o)

# Target executable
TARGET = servidor_rpc

# All targets
all: $(TARGET)

# Test sources and target
TEST_SOURCES = tests/test_server_methods.cpp
TEST_TARGET = tests/test_server_methods

# Build and/or run C++ unit test binary
test-cpp: $(TEST_TARGET)
	@echo "Built C++ test binary: $(TEST_TARGET)"

$(TEST_TARGET): $(TEST_SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I"../unit tests/cpp" $^ -o $@

# Integration test (Python) - default test target runs integration tests
test: integration-test

integration-test:
	@echo "Running integration tests (Python)..."
	@python3 tests/integration_test.py

# Server target
$(TARGET): $(XMLRPC_OBJECTS) $(SERVER_OBJECTS) main_servidor.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Generic rule for compiling .cpp files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean compiled files
clean:
	rm -f *.o $(TARGET)
	rm -f lib/*.o
	rm -f src/server/*.o

# Help target
help:
	@echo "Targets disponibles:"
	@echo "  all     - Compilar el servidor"
	@echo "  clean   - Limpiar archivos compilados"
	@echo "  help    - Mostrar esta ayuda"
	@echo ""
	@echo "Uso típico:"
	@echo "  make all"
	@echo "  ./$(TARGET) 8080"

# Declare phony targets
.PHONY: all clean help

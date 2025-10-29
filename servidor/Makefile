# Makefile for Servidor Package
# Author: Generated for POO TP2 Req4

# Compiler and flags
-CXXFLAGS = -std=c++14 -Wall -Wextra -g -O2
+CXXFLAGS = -std=c++14 -Wall -Wextra -g -O2 -pthread
 INCLUDES = -I./inc -I./lib
-LDFLAGS = 
+LDFLAGS = -pthread

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
				 lib/Robot.cpp \
				 lib/SerialPort.cpp

# Object files for XML-RPC library
XMLRPC_OBJECTS = $(XMLRPC_SOURCES:.cpp=.o)

# Target executable
TARGET = servidor_rpc

# All targets
all: $(TARGET)

# Server target
$(TARGET): $(XMLRPC_OBJECTS) main_servidor.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Generic rule for compiling .cpp files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean compiled files
clean:
	rm -f *.o $(TARGET)
	rm -f lib/*.o

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

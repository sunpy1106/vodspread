#Makefile
CC=g++ -Wall -g 

DFLAGS= -lpthread -I/usr/local/include -L/usr/local/lib   -lgslcblas -lm

LIB=error.o sock.o util.o config.o vodlog.o

CLIENT=client.o  clientEntrance.o 
ResouceManager= resourceManager.o 
SERVER=server.o serverEntrance.o  

target=  server client
all:$(target) 

client: $(LIB) $(CLIENT) 
	$(CC) $^ -o $@ $(DFLAGS)
server: $(LIB) $(SERVER) $(ResouceManager)
	$(CC) $^ -o $@ $(DFLAGS)

%.o:lib/%.cpp
	$(CC) -c $^
%.o:src/%.cpp
	$(CC) -c $^
%.o:test/%.cpp
	$(CC) -c $^

clean:
	rm -rf *.o
	rm -rf $(target)

#end



# Default target
all: clean sender receiver # tests

debug: clean sender receiver 

# We use an implicit rule to build an executable named 'sender'
sender: 
	cd src && $(MAKE) 
	mv src/sender sender

# We use an implicit rule to build an executable named 'receiver'
receiver: 
	mv src/receiver receiver

test : 
	cd tests && $(MAKE)
	cd tests && ./test 

.PHONY: clean

clean:
	rm -f receiver sender 
	cd src && $(MAKE) clean	

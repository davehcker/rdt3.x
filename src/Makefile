#Author: Nabil Rahiman
#Date:18/Jan/2017


#Reference
#http://makepp.sourceforge.net/1.19/makepp_tutorial.html

CC = gcc -c

# compiling flags here
CFLAGS = -Wall -I.

LINKER = gcc -o
# linking flags here
LFLAGS   = -Wall

OBJDIR = ../obj

CLIENT_OBJECTS := $(OBJDIR)/rdt_sender.o $(OBJDIR)/common.o $(OBJDIR)/packet.o
SERVER_OBJECTS := $(OBJDIR)/rdt_receiver.o $(OBJDIR)/common.o $(OBJDIR)/packet.o

#Program name
CLIENT := $(OBJDIR)/rdt_sender
SERVER := $(OBJDIR)/rdt_receiver

rm       = rm -f
rmdir       = rmdir 

TARGET:	$(OBJDIR) $(CLIENT)	$(SERVER)


$(CLIENT):	$(CLIENT_OBJECTS)
	$(LINKER)  $@  $(CLIENT_OBJECTS)
	@echo "Link complete!"

$(SERVER): $(SERVER_OBJECTS)
	$(LINKER)  $@  $(SERVER_OBJECTS)
	@echo "Link complete!"

$(OBJDIR)/%.o:	%.c common.h packet.h
	$(CC) $(CFLAGS)  $< -o $@
	@echo "Compilation complete!"

clean:
	@$(rm) $(CLIENT) $(SERVER) $(CLIENT_OBJECTS) $(SERVER_OBJECTS)
	@$(rmdir)  $(OBJDIR)
	@echo "Cleanup complete!"

$(OBJDIR):
	mkdir $(OBJDIR)

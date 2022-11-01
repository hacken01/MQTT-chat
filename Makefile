TARGET   = chat

CC       = gcc
CFLAGS   = -std=gnu99 -Wall -Wextra -g -DLOG_USE_COLOR

LINKER   = gcc
LFLAGS   =

SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

LIBRARIES= -lncurses -lpaho-mqtt3c -lpaho-mqtt3a -lpaho-mqtt3as

$(BINDIR)/$(TARGET): $(SOURCES) $(INCLUDES)
	$(LINKER) -o $@ $(SOURCES) $(CFLAGS) $(LIBRARIES)

clean:
	$(RM) $(OBJECTS)
	$(RM) $(BINDIR)/$(TARGET)

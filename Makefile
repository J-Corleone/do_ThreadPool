DIR := .

INC = $(wildcard $(DIR)/*.hh)
SRCS = $(wildcard $(DIR)/*.cc)

CC := g++
CFLAGS :=

OBJDIR := obj
OBJS := $(patsubst $(DIR)/%.cc, $(OBJDIR)/%.o, $(SRCS))

TARGET := main


all: cmp

cmp: $(OBJS)
# need '-c' option when compile only
$(OBJDIR)/%.o: $(DIR)/%.cc
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<


clean:
	@rm -rf $(OBJDIR) 
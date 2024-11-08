DIR := .

INC = $(wildcard $(DIR)/*.hh)
SRCS = $(wildcard $(DIR)/*.cc)

CXX := g++ -std=c++17
CXXFLAGS :=

OBJDIR := obj
OBJS := $(patsubst $(DIR)/%.cc, $(OBJDIR)/%.o, $(SRCS))

TARGET := main

all: clean cmp run

run: $(TARGET)
	@echo
	@echo '### RUNNING ###'
	@./$<

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^


cmp: $(OBJS)
# need '-c' option when compile only
$(OBJDIR)/%.o: $(DIR)/%.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<


clean:
	@rm -rf $(OBJDIR) $(TARGET)

.PHONY := clean cmp
CONF=$(OBJDIR)/.conf
include $(CONF)
CPPFLAGS+=-I. -I$(CORE)
ifneq ($(LIBRARY_DIRS),)
	CPPFLAGS+=$(patsubst %,-I%,$(LIBRARY_DIRS) $(wildcard $(patsubst %, %/utility,$(LIBRARY_DIRS))))
endif
CFLAGS+=-std=gnu99
CXXFLAGS+=-fno-exceptions
ALL_C_SRCS=$(wildcard $(patsubst %,%/*.c,$(LIBRARY_DIRS)) $(patsubst %,%/utility/*.c,$(LIBRARY_DIRS)) *.c $(OBJDIR)/*.c $(CORE)/*.c)
ALL_CC_SRCS=$(wildcard $(patsubst %,%/*.cc,$(LIBRARY_DIRS)) $(patsubst %,%/utility/*.cc,$(LIBRARY_DIRS)) *.cc $(OBJDIR)/*.cc $(CORE)/*.cc)
ALL_CPP_SRCS=$(wildcard $(patsubst %,%/*.cpp,$(LIBRARY_DIRS)) $(patsubst %,%/utility/*.cpp,$(LIBRARY_DIRS)) *.cpp $(OBJDIR)/*.cpp $(CORE)/*.cpp)
OBJS=$(ALL_C_SRCS:.c=.o) $(ALL_CC_SRCS:.cc=.o) $(ALL_CPP_SRCS:.cpp=.o)
DEPS=$(OBJS:.o=.d)
DEP_FILE=$(OBJDIR)/depends.mk
$(DEP_FILE): $(DEPS) $(CONF); cat $(DEPS) > $(DEP_FILE)
$(OBJDIR)/$(TARGET).elf: $(OBJS) $(CONF) $(DEP_FILE); $(CC) -o $@ $(OBJS) $(LDFLAGS)
$(OBJDIR)/%.o: %.c $(CONF); $(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@
$(OBJDIR)/%.o: %.cc $(CONF); $(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@
$(OBJDIR)/%.o: %.cpp $(CONF); $(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@
$(OBJDIR)/%.o: %.S $(CONF); $(CC) -c $(CPPFLAGS) $(ASFLAGS) $< -o $@
$(OBJDIR)/%.o: %.s $(CONF); $(CC) -c $(CPPFLAGS) $(ASFLAGS) $< -o $@
$(OBJDIR)/%.d: %.c $(CONF); $(CC) -MM $(CPPFLAGS) $(CFLAGS) $< -MF $@ -MT $(@:.d=.o)
$(OBJDIR)/%.d: %.cc $(CONF); $(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< -MF $@ -MT $(@:.d=.o)
$(OBJDIR)/%.d: %.cpp $(CONF); $(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< -MF $@ -MT $(@:.d=.o)
$(OBJDIR)/%.d: %.S $(CONF); $(CC) -MM $(CPPFLAGS) $(ASFLAGS) $< -MF $@ -MT $(@:.d=.o)
$(OBJDIR)/%.d: %.s $(CONF); $(CC) -MM $(CPPFLAGS) $(ASFLAGS) $< -MF $@ -MT $(@:.d=.o)
$(OBJDIR)/%.o: $(OBJDIR)/%.cpp $(CONF); $(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@
$(OBJDIR)/%.d: $(OBJDIR)/%.cpp $(CONF); $(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< -MF $@ -MT $(@:.d=.o)
$(CORE)/%.o: %.c $(CONF); $(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@
$(CORE)/%.o: %.cpp $(CONF); $(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@
$(CORE)/%.o: %.cc $(CONF); $(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@
$(CORE)/%.d: $(CORE)/%.c $(CONF); $(CC) -MM $(CPPFLAGS) $(CFLAGS) $< -MF $@ -MT $(@:.d=.o)
$(CORE)/%.d: $(CORE)/%.cc $(CONF); $(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< -MF $@ -MT $(@:.d=.o)
$(CORE)/%.d: $(CORE)/%.cpp $(CONF); $(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< -MF $@ -MT $(@:.d=.o)
$(CORE)/%.d: $(CORE)/%.S $(CONF); $(CC) -MM $(CPPFLAGS) $(ASFLAGS) $< -MF $@ -MT $(@:.d=.o)
$(CORE)/%.d: $(CORE)/%.s $(CONF); $(CC) -MM $(CPPFLAGS) $(ASFLAGS) $< -MF $@ -MT $(@:.d=.o)
$(LIBRARIES_DIR)/%.d: $(LIBRARIES_DIR)/%.c $(CONF); $(CC) -MM $(CPPFLAGS) $(CFLAGS) $< -MF $@ -MT $(@:.d=.o)
$(LIBRARIES_DIR)/%.d: $(LIBRARIES_DIR)/%.cc $(CONF); $(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< -MF $@ -MT $(@:.d=.o)
$(LIBRARIES_DIR)/%.d: $(LIBRARIES_DIR)/%.cpp $(CONF); $(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< -MF $@ -MT $(@:.d=.o)
$(LIBRARIES_DIR)/%.d: $(LIBRARIES_DIR)/%.S $(CONF); $(CC) -MM $(CPPFLAGS) $(ASFLAGS) $< -MF $@ -MT $(@:.d=.o)
$(LIBRARIES_DIR)/%.d: $(LIBRARIES_DIR)/%.s $(CONF); $(CC) -MM $(CPPFLAGS) $(ASFLAGS) $< -MF $@ -MT $(@:.d=.o)
$(OBJDIR)/%.d: $(OBJDIR)/%.c $(CONF); $(CC) -MM $(CPPFLAGS) $(CFLAGS) $< -MF $@ -MT $(@:.d=.o)
$(OBJDIR)/%.d: $(OBJDIR)/%.cc $(CONF); $(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< -MF $@ -MT $(@:.d=.o)
$(OBJDIR)/%.d: $(OBJDIR)/%.cpp $(CONF); $(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< -MF $@ -MT $(@:.d=.o)
$(OBJDIR)/%.d: $(OBJDIR)/%.S $(CONF); $(CC) -MM $(CPPFLAGS) $(ASFLAGS) $< -MF $@ -MT $(@:.d=.o)
$(OBJDIR)/%.d: $(OBJDIR)/%.s $(CONF); $(CC) -MM $(CPPFLAGS) $(ASFLAGS) $< -MF $@ -MT $(@:.d=.o)
$(OBJDIR)/%.hex: $(OBJDIR)/%.elf $(CONF); $(OBJCOPY) -O ihex -R .eeprom $< $@
$(OBJDIR)/%.eep: $(OBJDIR)/%.elf $(CONF); $(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 -O ihex $< $@
$(OBJDIR)/%.lss: $(OBJDIR)/%.elf $(CONF); $(OBJDUMP) -h -S $< > $@
$(OBJDIR)/%.sym: $(OBJDIR)/%.elf $(CONF); $(NM) -n $< > $@
ifneq ($(wildcard $(DEP_FILE)),)
include $(DEP_FILE)
endif
# Copyright 2013 Ben Hayden, based on work that is
# Copyright (C) 2010 Martin Oldfield <m@mjo.tc>, based on work that is
# Copyright Nicholas Zambetti, David A. Mellis & Hernando Barragan
# This file is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 2.1 of the
# License, or (at your option) any later version.

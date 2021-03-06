# GNU Make project makefile autogenerated by Premake

ifndef config
  config=debug_x86
endif

ifndef verbose
  SILENT = @
endif

.PHONY: clean prebuild prelink

ifeq ($(config),debug_x86)
  RESCOMP = windres
  TARGETDIR = ../../../build/bin/Debug-x86/samples/Client-Server/Server
  TARGET = $(TARGETDIR)/Server
  OBJDIR = ../../../build/obj/Debug-x86/samples/Client-Server/Server
  DEFINES +=
  INCLUDES += -I../../../Sockets/include -Isrc
  FORCE_INCLUDE +=
  ALL_CPPFLAGS += $(CPPFLAGS) -MMD -MP $(DEFINES) $(INCLUDES)
  ALL_CFLAGS += $(CFLAGS) $(ALL_CPPFLAGS) -m32 -g -std=c11
  ALL_CXXFLAGS += $(CXXFLAGS) $(ALL_CPPFLAGS) -m32 -g
  ALL_RESFLAGS += $(RESFLAGS) $(DEFINES) $(INCLUDES)
  LIBS += ../../../build/bin/Debug-x86/Sockets/libSHMSockets.so
  LDDEPS += ../../../build/bin/Debug-x86/Sockets/libSHMSockets.so
  ALL_LDFLAGS += $(LDFLAGS) -L/usr/lib32 -Wl,-rpath,'$$ORIGIN/../../../Sockets' -m32
  LINKCMD = $(CC) -o "$@" $(OBJECTS) $(RESOURCES) $(ALL_LDFLAGS) $(LIBS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
all: prebuild prelink $(TARGET)
	@:

endif

ifeq ($(config),debug_x86_64)
  RESCOMP = windres
  TARGETDIR = ../../../build/bin/Debug-x86_64/samples/Client-Server/Server
  TARGET = $(TARGETDIR)/Server
  OBJDIR = ../../../build/obj/Debug-x86_64/samples/Client-Server/Server
  DEFINES +=
  INCLUDES += -I../../../Sockets/include -Isrc
  FORCE_INCLUDE +=
  ALL_CPPFLAGS += $(CPPFLAGS) -MMD -MP $(DEFINES) $(INCLUDES)
  ALL_CFLAGS += $(CFLAGS) $(ALL_CPPFLAGS) -m64 -g -std=c11
  ALL_CXXFLAGS += $(CXXFLAGS) $(ALL_CPPFLAGS) -m64 -g
  ALL_RESFLAGS += $(RESFLAGS) $(DEFINES) $(INCLUDES)
  LIBS += ../../../build/bin/Debug-x86_64/Sockets/libSHMSockets.so
  LDDEPS += ../../../build/bin/Debug-x86_64/Sockets/libSHMSockets.so
  ALL_LDFLAGS += $(LDFLAGS) -L/usr/lib64 -Wl,-rpath,'$$ORIGIN/../../../Sockets' -m64
  LINKCMD = $(CC) -o "$@" $(OBJECTS) $(RESOURCES) $(ALL_LDFLAGS) $(LIBS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
all: prebuild prelink $(TARGET)
	@:

endif

ifeq ($(config),release_x86)
  RESCOMP = windres
  TARGETDIR = ../../../build/bin/Release-x86/samples/Client-Server/Server
  TARGET = $(TARGETDIR)/Server
  OBJDIR = ../../../build/obj/Release-x86/samples/Client-Server/Server
  DEFINES +=
  INCLUDES += -I../../../Sockets/include -Isrc
  FORCE_INCLUDE +=
  ALL_CPPFLAGS += $(CPPFLAGS) -MMD -MP $(DEFINES) $(INCLUDES)
  ALL_CFLAGS += $(CFLAGS) $(ALL_CPPFLAGS) -m32 -O2 -std=c11
  ALL_CXXFLAGS += $(CXXFLAGS) $(ALL_CPPFLAGS) -m32 -O2
  ALL_RESFLAGS += $(RESFLAGS) $(DEFINES) $(INCLUDES)
  LIBS += ../../../build/bin/Release-x86/Sockets/libSHMSockets.so
  LDDEPS += ../../../build/bin/Release-x86/Sockets/libSHMSockets.so
  ALL_LDFLAGS += $(LDFLAGS) -L/usr/lib32 -Wl,-rpath,'$$ORIGIN/../../../Sockets' -m32 -s
  LINKCMD = $(CC) -o "$@" $(OBJECTS) $(RESOURCES) $(ALL_LDFLAGS) $(LIBS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
all: prebuild prelink $(TARGET)
	@:

endif

ifeq ($(config),release_x86_64)
  RESCOMP = windres
  TARGETDIR = ../../../build/bin/Release-x86_64/samples/Client-Server/Server
  TARGET = $(TARGETDIR)/Server
  OBJDIR = ../../../build/obj/Release-x86_64/samples/Client-Server/Server
  DEFINES +=
  INCLUDES += -I../../../Sockets/include -Isrc
  FORCE_INCLUDE +=
  ALL_CPPFLAGS += $(CPPFLAGS) -MMD -MP $(DEFINES) $(INCLUDES)
  ALL_CFLAGS += $(CFLAGS) $(ALL_CPPFLAGS) -m64 -O2 -std=c11
  ALL_CXXFLAGS += $(CXXFLAGS) $(ALL_CPPFLAGS) -m64 -O2
  ALL_RESFLAGS += $(RESFLAGS) $(DEFINES) $(INCLUDES)
  LIBS += ../../../build/bin/Release-x86_64/Sockets/libSHMSockets.so
  LDDEPS += ../../../build/bin/Release-x86_64/Sockets/libSHMSockets.so
  ALL_LDFLAGS += $(LDFLAGS) -L/usr/lib64 -Wl,-rpath,'$$ORIGIN/../../../Sockets' -m64 -s
  LINKCMD = $(CC) -o "$@" $(OBJECTS) $(RESOURCES) $(ALL_LDFLAGS) $(LIBS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
all: prebuild prelink $(TARGET)
	@:

endif

OBJECTS := \
	$(OBJDIR)/main.o \

RESOURCES := \

CUSTOMFILES := \

SHELLTYPE := posix
ifeq (.exe,$(findstring .exe,$(ComSpec)))
	SHELLTYPE := msdos
endif

$(TARGET): $(GCH) ${CUSTOMFILES} $(OBJECTS) $(LDDEPS) $(RESOURCES) | $(TARGETDIR)
	@echo Linking Server
	$(SILENT) $(LINKCMD)
	$(POSTBUILDCMDS)

$(CUSTOMFILES): | $(OBJDIR)

$(TARGETDIR):
	@echo Creating $(TARGETDIR)
ifeq (posix,$(SHELLTYPE))
	$(SILENT) mkdir -p $(TARGETDIR)
else
	$(SILENT) mkdir $(subst /,\\,$(TARGETDIR))
endif

$(OBJDIR):
	@echo Creating $(OBJDIR)
ifeq (posix,$(SHELLTYPE))
	$(SILENT) mkdir -p $(OBJDIR)
else
	$(SILENT) mkdir $(subst /,\\,$(OBJDIR))
endif

clean:
	@echo Cleaning Server
ifeq (posix,$(SHELLTYPE))
	$(SILENT) rm -f  $(TARGET)
	$(SILENT) rm -rf $(OBJDIR)
else
	$(SILENT) if exist $(subst /,\\,$(TARGET)) del $(subst /,\\,$(TARGET))
	$(SILENT) if exist $(subst /,\\,$(OBJDIR)) rmdir /s /q $(subst /,\\,$(OBJDIR))
endif

prebuild:
	$(PREBUILDCMDS)

prelink:
	$(PRELINKCMDS)

ifneq (,$(PCH))
$(OBJECTS): $(GCH) $(PCH) | $(OBJDIR)
$(GCH): $(PCH) | $(OBJDIR)
	@echo $(notdir $<)
	$(SILENT) $(CC) -x c-header $(ALL_CFLAGS) -o "$@" -MF "$(@:%.gch=%.d)" -c "$<"
else
$(OBJECTS): | $(OBJDIR)
endif

$(OBJDIR)/main.o: src/main.c
	@echo $(notdir $<)
	$(SILENT) $(CC) $(ALL_CFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"

-include $(OBJECTS:%.o=%.d)
ifneq (,$(PCH))
  -include $(OBJDIR)/$(notdir $(PCH)).d
endif
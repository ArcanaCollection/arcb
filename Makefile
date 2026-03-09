# =========================
# Toolchain / flags
# =========================
CXX      := g++
CXXFLAGS := -std=c++17 -g3 -O0
CXXFLAGS += -Wall -Wextra
CXXFLAGS += -Wunused-variable -Wunused-function -Wunused-parameter -Wunreachable-code
CXXFLAGS += -Wunused-but-set-variable -Wunused-result
CXXFLAGS += -DDEBUG -DARC_PROFILE
CXXFLAGS += -Iinclude/common -Iinclude/core -Iinclude/parser -Iinclude/parser/util -Iinclude/core/util

LDFLAGS  ?=

# =========================
# Paths / target
# =========================
BUILDDIR := build
BINDIR   := $(BUILDDIR)/bin
TARGET   := $(BINDIR)/arcb

# =========================
# Sources
# =========================
SRCS :=
SRCS += $(wildcard src/*.cpp)
SRCS += $(wildcard src/core/*.cpp)
SRCS += $(wildcard src/core/util/*.cpp)
SRCS += $(wildcard src/parser/*.cpp)
SRCS += $(wildcard src/parser/util/*.cpp)
SRCS += $(wildcard src/common/*.cpp)

OBJS := $(patsubst src/%.cpp,$(BUILDDIR)/%.o,$(SRCS))

# =========================
# OS / shell layer
# =========================
ifeq ($(OS),Windows_NT)

    ifneq ($(MSYSTEM),)
        # MSYS2 / Git-Bash (POSIX shell)
        EXEEXT := .exe

        define MKDIR_P
            mkdir -p "$(1)"
        endef

        define RM_RF
            rm -rf "$(1)"
        endef

        define CP
            cp -f "$(1)" "$(2)"
        endef

        PREFIX ?= /usr/local
        BINDST := $(PREFIX)/bin
        SEP    := /
    else
        # Native Windows (cmd)
        EXEEXT := .exe
        winpath = $(subst /,\,$(1))

        define MKDIR_P
            if not exist "$(call winpath,$(1))" mkdir "$(call winpath,$(1))"
        endef

        define RM_RF
            if exist "$(call winpath,$(1))" rmdir /S /Q "$(call winpath,$(1))"
        endef

        define CP
            copy /Y "$(call winpath,$(1))" "$(call winpath,$(2))" >NUL
        endef

        PREFIX ?= C:\Program Files\arcb
        BINDST := $(PREFIX)\bin
        SEP    := \
		
    endif

else
    # Linux
    EXEEXT :=

    define MKDIR_P
        mkdir -p "$(1)"
    endef

    define RM_RF
        rm -rf "$(1)"
    endef

    define CP
        cp -f "$(1)" "$(2)"
    endef

    PREFIX ?= /usr/local
    BINDST := $(PREFIX)/bin
    SEP    := /
endif

TARGET := $(TARGET)$(EXEEXT)

# =========================
# MinGW runtime bundling (Windows only)
# =========================
ifeq ($(OS),Windows_NT)
    # Resolve absolute compiler path (works in MSYS2/Git-Bash; in cmd branch you typically set CXX to full path)
    CXXPATH := $(shell command -v $(CXX) 2>/dev/null)
    ifneq ($(strip $(CXXPATH)),)
        CXXDIR := $(dir $(CXXPATH))
    else
        CXXDIR := $(dir $(CXX))
    endif

    RUNTIME_DLLS := libstdc++-6.dll libgcc_s_seh-1.dll libgcc_s_dw2-1.dll libwinpthread-1.dll
endif

# =========================
# Targets
# =========================
.PHONY: all clean install bundle-dlls

all: $(TARGET) bundle-dlls

# Link
$(TARGET): $(OBJS)
	@$(call MKDIR_P,$(dir $@))
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Compile
$(BUILDDIR)/%.o: src/%.cpp
	@$(call MKDIR_P,$(dir $@))
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Copy MinGW runtime DLLs next to the exe (so it runs without PATH hacks)
bundle-dlls:
ifeq ($(OS),Windows_NT)
	@$(call MKDIR_P,$(dir $(TARGET)))
	@echo Bundling runtime DLLs from: $(CXXDIR)
	@for d in $(RUNTIME_DLLS); do \
		if [ -f "$(CXXDIR)$$d" ]; then \
			cp -f "$(CXXDIR)$$d" "$(dir $(TARGET))"; \
		else \
			echo "WARN: missing $(CXXDIR)$$d"; \
		fi; \
	done
else
	@:
endif

install: all
ifeq ($(OS),Windows_NT)
	@$(call MKDIR_P,$(BINDST))
	@$(call CP,$(TARGET),$(BINDST)$(SEP)arcb$(EXEEXT))
else
	@install -d "$(BINDST)"
	@install -m 0755 "$(TARGET)" "$(BINDST)/arcb"
endif

clean:
	@$(call RM_RF,$(BUILDDIR))

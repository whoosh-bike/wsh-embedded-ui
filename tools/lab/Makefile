# weui lab: the library and a target's C built into wasm for the browser UI.
#
#   make                         every variant of the target, into build/wasm/
#   make TARGET_DIR=<dir>        another target - a directory with a target.mk
#   make deps                    the files outside the target it is built from (TARGET_DEPS)
#
# The library is archived once per variant; a target's own files relink in a second.

WEUI       := $(abspath ../..)
EMBLIB     ?= $(abspath $(WEUI)/../wsh-embedded-libs)
TARGET_DIR ?= targets/demos
EMCC       ?= emcc

include $(TARGET_DIR)/target.mk

VARIANTS := COLOR-RGB GRAY-GRAY
OUT      := build/wasm
OBJ      := build/obj/$(TARGET_NAME)

LIB_SRC := $(wildcard $(WEUI)/common/*.c $(WEUI)/gfx/*/*.c $(WEUI)/external/fonts/*.c) \
	$(WEUI)/fs/weui_fs.c $(WEUI)/host/sim/display_sim.c \
	$(EMBLIB)/math/mathlib_common.c $(EMBLIB)/def_types.c
LAB_SRC := c/lab.c $(TARGET_SRC)

INC := -Ic -I$(WEUI)/host/sim -I$(EMBLIB) -I$(EMBLIB)/math -I$(EMBLIB)/string -I$(EMBLIB)/term \
	$(addprefix -I$(WEUI)/,common external/fonts fs gfx/anima gfx/core gfx/demo gfx/draw \
	gfx/effects gfx/image gfx/scenemgr gfx/sim gfx/text gfx/widgets) $(TARGET_INC)

CFLAGS := -std=gnu2x -O2 -Wall -Wno-unused-parameter -Wno-unused-function -Wno-switch \
	-Wno-format -Wno-comment \
	-DWEUI_DISPLAY_WIDTH=$(TARGET_W) -DWEUI_DISPLAY_HEIGHT=$(TARGET_H) $(TARGET_CFLAGS)

# --profiling-funcs keeps the functions' names in the module: the UI names a traced
# transition or action by its function pointer, a wasmTable index. The target links at
# -O1, which skips binaryen: at -O2 it merges functions with the same body - two
# transitions playing the same effect - and one would go by the other's name
LINK_OPT := -O1
LDFLAGS := -sMODULARIZE -sEXPORT_ES6 -sENVIRONMENT=worker -sALLOW_MEMORY_GROWTH --profiling-funcs \
	-sEXPORTED_RUNTIME_METHODS=HEAPU32,UTF8ToString,ccall,wasmTable \
	-sEXPORTED_FUNCTIONS=_Lab_Start,_Lab_ActionsNum,_Lab_ActionName,_Lab_RunAction,_Lab_IsLive,_Lab_InputsNum,_Lab_InputName,_Lab_InputGroup,_Lab_Input,_Lab_ScenesNum,_Lab_SceneName,_Lab_Advance,_Lab_SetBright,_Lab_TimeMs,_Lab_MaxWidth,_Lab_MaxHeight,_Lab_IsGray

variant_flags = -DWEUI_TEST_DISPLAY_GRAY=$(if $(findstring GRAY-,$(1)),1,0) \
	-DWEUI_TEST_LIB_GRAY=$(if $(findstring -GRAY,$(1)),1,0)

.PHONY: all clean deps
all: $(foreach v,$(VARIANTS),$(OUT)/$(TARGET_NAME)-$(v).mjs)

define VARIANT_RULES
$(OBJ)/$(1)/lib/%.o: $(WEUI)/%.c
	@mkdir -p $$(dir $$@)
	$(EMCC) $(CFLAGS) $(call variant_flags,$(1)) $(INC) -MMD -MP -c $$< -o $$@

$(OBJ)/$(1)/emblib/%.o: $(EMBLIB)/%.c
	@mkdir -p $$(dir $$@)
	$(EMCC) $(CFLAGS) $(call variant_flags,$(1)) $(INC) -MMD -MP -c $$< -o $$@

$(OBJ)/$(1)/libweui.a: $(patsubst $(WEUI)/%.c,$(OBJ)/$(1)/lib/%.o,$(filter $(WEUI)/%,$(LIB_SRC))) \
		$(patsubst $(EMBLIB)/%.c,$(OBJ)/$(1)/emblib/%.o,$(filter $(EMBLIB)/%,$(LIB_SRC)))
	emar rcs $$@ $$^

$(OUT)/$(TARGET_NAME)-$(1).mjs: $(LAB_SRC) $(OBJ)/$(1)/libweui.a $(wildcard c/*.h $(TARGET_DIR)/*.h) \
		$(TARGET_DEPS)
	@mkdir -p $(OUT)
	$(EMCC) $(filter-out -O2,$(CFLAGS)) $(LINK_OPT) $(call variant_flags,$(1)) $(INC) $(LAB_SRC) \
		$(OBJ)/$(1)/libweui.a $(LDFLAGS) -o $$@
endef

$(foreach v,$(VARIANTS),$(eval $(call VARIANT_RULES,$(v))))

-include $(shell find build/obj -name '*.d' 2>/dev/null)

deps:
	@echo $(TARGET_DEPS)

clean:
	rm -rf build

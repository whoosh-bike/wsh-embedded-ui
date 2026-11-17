# WeUi library build fragment.
# Include this file from the parent project Makefile:
#
#   WEUI_DIR := thirdparty/wsh-embedded-ui
#   include $(WEUI_DIR)/weui.mk
#   SRC       += $(WEUI_SRC)
#   CFLAGS    += $(WEUI_INC)
#
# The indication subsystem (ind/, LED strip effects) binds to a host led_strip_m
# BSP module and an ind_cfg.h. A host that only drives a display leaves it out:
#
#   WEUI_USE_IND := 0
#   WEUI_USE_DEMO := 0   # leave the showpieces out of a production build

ifndef WEUI_DIR
$(error WEUI_DIR is not set. Set it to the path of the weui library root before including weui.mk)
endif

WEUI_USE_IND ?= 1
WEUI_USE_DEMO ?= 1

# Only the library directories: host/, tests/ and tools/ run on the PC and never reach firmware
WEUI_SRC := $(foreach d,common external fs gfx ind,$(call rwildcard,$(WEUI_DIR)/$(d),*.c))

WEUI_INC := \
	-I$(WEUI_DIR)/common \
	-I$(WEUI_DIR)/external \
	-I$(WEUI_DIR)/external/fonts \
	-I$(WEUI_DIR)/fs \
	-I$(WEUI_DIR)/gfx/anima \
	-I$(WEUI_DIR)/gfx/core \
	-I$(WEUI_DIR)/gfx/demo \
	-I$(WEUI_DIR)/gfx/draw \
	-I$(WEUI_DIR)/gfx/effects \
	-I$(WEUI_DIR)/gfx/image \
	-I$(WEUI_DIR)/gfx/scenemgr \
	-I$(WEUI_DIR)/gfx/sim \
	-I$(WEUI_DIR)/gfx/text \
	-I$(WEUI_DIR)/gfx/widgets

ifneq ($(WEUI_USE_DEMO),1)
	WEUI_SRC := $(filter-out $(WEUI_DIR)/gfx/demo/%,$(WEUI_SRC))
	WEUI_INC := $(filter-out -I$(WEUI_DIR)/gfx/demo,$(WEUI_INC))
endif

ifeq ($(WEUI_USE_IND),1)
	WEUI_INC += \
		-I$(WEUI_DIR)/ind \
		-I$(WEUI_DIR)/ind/core \
		-I$(WEUI_DIR)/ind/effects \
		-I$(WEUI_DIR)/ind/misc
else
	WEUI_SRC := $(filter-out $(WEUI_DIR)/ind/%,$(WEUI_SRC))
endif

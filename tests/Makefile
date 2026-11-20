# Host test suite: weui drawn into a simulated panel, under ASan and UBSan.
#
#   make                  build and run every test, PNG frames land in out/
#   make T=push_lands     one test
#   make W=32 H=8         another panel size
#   make DISPLAY=GRAY     a gray panel; LIB=GRAY for the gray colour scheme
#   make BANDS=1          scenes go out in bands, as to a driver with no frame
#   make FRAME_MEM=1      effects work on the frames as memory; the frames match the same reference
#   make BANDS=1 FRAME_MEM=1   transitions composed in bands too, held to the in-place ones
#   make VECTOR=0         without the vector component: its tests and demos are left out, so
#                         the golden runs differ - record their reference with `make golden VECTOR=0`
#   make golden           record this host's frame hashes as the reference
#   make check            run, then diff the frame hashes against the reference
#
# Needs a C compiler with sanitizers, zlib and wsh-embedded-libs (EMBLIB, a sibling
# checkout by default). The QR test needs qrcodegen (QRGEN) and is left out without it.

WEUI    := ..
EMBLIB  ?= ../../wsh-embedded-libs
QRGEN   ?=
W       ?= 90
H       ?= 25
# DISPLAY is also set by XQuartz on macOS; only our two values count
DISPLAY := $(if $(filter COLOR GRAY,$(DISPLAY)),$(DISPLAY),COLOR)
LIB     := $(if $(filter RGB GRAY,$(LIB)),$(LIB),RGB)

BANDS   ?= 0
FRAME_MEM ?= 0
VECTOR  ?= 1

VARIANT := $(W)x$(H)-$(DISPLAY)-$(LIB)$(if $(QRGEN),-QR)$(if $(filter 1,$(BANDS)),-BANDS)$(if $(filter 0,$(VECTOR)),-NOVEC)
BUILD   := build/$(VARIANT)$(if $(filter 1,$(FRAME_MEM)),-MEM)
OUT     := out/$(VARIANT)
# libm differs in the last bits between hosts, and so do the hashes of anything drawn
# through sinf/powf: a reference only holds for the host it was recorded on
GOLDEN  := golden/$(shell uname -s)-$(shell uname -m)-$(VARIANT).txt

CFLAGS := -std=gnu2x -g -O1 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function \
	-Wno-switch -Wno-format -Wno-comment \
	-fsanitize=address,undefined,float-cast-overflow,float-divide-by-zero \
	-fno-sanitize-recover=all \
	-DWEUI_DISPLAY_WIDTH=$(W) -DWEUI_DISPLAY_HEIGHT=$(H) \
	-DWEUI_TEST_DISPLAY_GRAY=$(if $(filter GRAY,$(DISPLAY)),1,0) \
	-DWEUI_TEST_LIB_GRAY=$(if $(filter GRAY,$(LIB)),1,0) \
	-DWEUI_DISPLAY_BANDS=$(BANDS) -DWEUI_DISPLAY_FRAME_MEM=$(FRAME_MEM) \
	-DWEUI_USE_COMPONENT__VECTOR=$(VECTOR)

INC := -I. -I$(WEUI)/host/sim -I$(EMBLIB) -I$(EMBLIB)/math -I$(EMBLIB)/string -I$(EMBLIB)/term \
	$(addprefix -I$(WEUI)/,common external/fonts fs gfx/anima gfx/core gfx/demo gfx/draw \
	gfx/effects gfx/image gfx/scenemgr gfx/sim gfx/text gfx/vector gfx/widgets)

SRC := $(wildcard $(WEUI)/common/*.c $(WEUI)/gfx/*/*.c $(WEUI)/external/fonts/*.c) \
	$(WEUI)/fs/weui_fs.c $(WEUI)/host/sim/display_sim.c test_png.c weui_tests.c fill_reference.c \
	$(EMBLIB)/math/mathlib_common.c $(EMBLIB)/def_types.c

ifneq ($(QRGEN),)
	CFLAGS += -DWEUI_USE_COMPONENT__WIDGET_QR=1
	INC    += -I$(QRGEN)
	SRC    += $(QRGEN)/qrcodegen.c
endif

RUN := ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1

.PHONY: run golden check clean

run: $(BUILD)/weui_tests
	@mkdir -p $(OUT)
	$(RUN) $< $(OUT) $(T)

$(BUILD)/weui_tests: $(SRC) $(wildcard *.h $(WEUI)/host/sim/*.h $(WEUI)/*/*.h $(WEUI)/gfx/*/*.h)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(INC) $(SRC) -lz -lm -o $@

golden: $(BUILD)/weui_tests
	@mkdir -p $(OUT) golden
	$(RUN) $< $(OUT) > $(BUILD)/run.log || { cat $(BUILD)/run.log; exit 1; }
	grep '^hash ' $(BUILD)/run.log > $(GOLDEN)
	@echo "recorded $(GOLDEN)"

check: $(BUILD)/weui_tests
	@test -f $(GOLDEN) || { echo "no $(GOLDEN) - record one with 'make golden'"; exit 1; }
	@mkdir -p $(OUT)
	$(RUN) $< $(OUT) > $(BUILD)/run.log || { cat $(BUILD)/run.log; exit 1; }
	grep '^hash ' $(BUILD)/run.log | diff -u $(GOLDEN) -
	@echo "frames match $(GOLDEN)"

clean:
	rm -rf build out

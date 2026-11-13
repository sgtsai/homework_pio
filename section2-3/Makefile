# ============================================================================
#  考前練習材料 -- 兩個在自己電腦上就能跑的小程式
#
#      make                      編譯
#      make run                  依序執行兩個 demo
#      make run-critical-section 只跑 critical section
#      make run-transfer-engine  只跑 transfer engine
#      make mistake              transfer engine 的「最常見錯誤」示範
#      make clean                清掉編譯產物
#      make help                 顯示這份說明
# ============================================================================
CC     ?= cc
CFLAGS ?= -O2 -Wall -Wextra -pthread
BIN    := bin

CS := $(BIN)/critical-section
TE := $(BIN)/transfer-engine

.PHONY: all run run-critical-section run-transfer-engine mistake clean help

all: $(CS) $(TE)
	@echo
	@echo "編譯完成。接著試試：  make run"

$(BIN):
	@mkdir -p $(BIN)

$(CS): critical-section/main.c | $(BIN)
	$(CC) $(CFLAGS) -o $@ $<

$(TE): transfer-engine/main.c transfer-engine/engine.c transfer-engine/engine.h | $(BIN)
	$(CC) $(CFLAGS) -o $@ transfer-engine/main.c transfer-engine/engine.c

run: run-critical-section run-transfer-engine

run-critical-section: $(CS)
	@echo "=============== critical section ==============="
	@./$(CS)
	@echo

run-transfer-engine: $(TE)
	@echo "=============== transfer engine ================"
	@./$(TE)
	@echo

mistake: $(TE)
	@echo "=============== 最常見的錯誤 ==================="
	@./$(TE) --mistake
	@echo

clean:
	rm -rf $(BIN)

help:
	@sed -n '2,10p' Makefile | sed 's/^# \?//'

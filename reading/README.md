# 第二大題 考前 demo

這兩支程式讓你在考前先對第二大題會用到的背景有個概念，不是考題，也不需要開發板。

| 檔案 | 內容 | 怎麼用 |
|---|---|---|
| `demo_asm.c` | C 語言的基本 bitwise 操作（AND / OR / XOR / NOT / 移位，以及設定、清除、取出位元） | 用任何 C 編譯器編譯後執行，對照註解裡的數值 |
| `demo_pio.pio` | RP2350 的 PIO 是什麼，以及一支最小的 PIO 程式 | 閱讀即可，不需要執行 |

```sh
cc demo_asm.c -o demo_asm && ./demo_asm
```

## 想多準備的話

- RISC-V 指令集手冊（Unprivileged ISA）：<https://riscv.org/specifications/ratified/>
- RP2350 datasheet 的 PIO 章節：<https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf>

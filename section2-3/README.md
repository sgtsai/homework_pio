# 考前練習材料

## 延伸作業：安全的非同步搬運佇列

完成 demo 後，可以挑戰 [Section 2.3 homework](homework/homework.txt)：使用 C 與 pthread 實作容量為 8 的共享佇列、阻塞等待、安全關閉，以及有邊界檢查的 copy/fill 工作。

作業附有未完成的 [starter](homework/submission.c) 與 [自動評分程式](homework/grade.py)，滿分 100 分。從 `homework/` 執行 `python grade.py`。詳見 [作業說明](homework/README.md)。

## 原始 demo

這裡有兩個小程式，用來在考試前先接觸兩個概念。**在你自己的電腦上就能編譯執行，不需要任何開發板。**

這些程式跟考題本身沒有直接關係，裡面不會出現任何考題內容。它們只是讓你先摸過兩件事，省下考場上從零開始理解的時間。

## 編譯與執行

需要 `gcc`（或 `clang`）與 `make`。Linux 直接跑；Windows 建議用 WSL；macOS 也可以。

```bash
make            # 編譯
make run        # 依序執行兩個 demo
```

也可以分開跑：

| 指令 | 作用 |
|---|---|
| `make run-critical-section` | 只跑 critical section |
| `make run-transfer-engine` | 只跑 transfer engine |
| `make mistake` | transfer engine 的「最常見錯誤」示範 |
| `make clean` | 清掉編譯產物 |
| `make help` | 列出所有指令 |

原始碼在 `critical-section/` 與 `transfer-engine/` 兩個資料夾裡，編譯出來的執行檔放在 `bin/`。

## Critical section：兩個執行緒、一個共享變數

```bash
make run-critical-section
```

兩個工作者各對同一個變數加 1 共 20000 次，結果「理所當然」應該是 40000。

實際輸出大概像這樣：

```
expected counter = 40000   (12 CPUs available)

1. no lock            counter = 39734    LOST UPDATES     70.3 ms
2. lock, short hold   counter = 40000    ok               67.2 ms
3. lock, long hold    counter = 40000    ok              155.6 ms
```

三件事值得注意：

1. **第 1 行每次跑都不一樣，而且總是少一些。** 因為 `counter++` 不是一個步驟，而是「讀出來、加一、寫回去」三步。另一個工作者可能在你讀完還沒寫回去的空檔插進來，於是其中一次更新就這樣消失了。程式裡把這三步拆開寫出來，可以直接看到那個空檔在哪。

2. **第 2 行用鎖保護那三步，結果就精準了。** 被鎖保護的那一段程式碼叫做**臨界區（critical section）**。

3. **第 3 行也正確，但慢了一倍以上。** 它跟第 2 行的差別只有一個：把耗時的計算也放進鎖裡面。結果兩個工作者輪流等對方，等於退回單執行緒。**鎖要抓得越短越好**——只鎖真正在存取共享資料的那幾行。

> 如果你的環境只有一顆 CPU（某些虛擬機或容器），第 1 行可能會顯示 `ok`。程式會在開頭提醒你。那不代表沒有鎖是安全的，只是兩個工作者沒有真的同時在跑而已。

## Transfer engine：把工作「描述」出去，而不是自己寫迴圈

```bash
make run-transfer-engine
make mistake
```

這裡有一個假想的「搬運引擎」，是一個獨立於 CPU 之外的單位。你不寫複製迴圈，而是**描述**這件工作：從哪讀、寫到哪、幾筆、每筆多大、哪一邊的位址要往前走。描述完就啟動，然後去做別的事，之後再回來收結果。

使用的節奏固定是四拍：

```
描述 (configure)  →  啟動 (start)  →  等待 (wait / busy)  →  取用結果
```

輸出大概像這樣：

```
claimed channel 0 (there are 4)

1. copy 4096 words          dst matches src
   while it ran, this program went round its own loop 98705538 times

2. fill 4096 words          every word is 0xA5A5A5A5
   same engine, same count -- only the increment flags changed
```

四個值得記住的點：

1. **`count` 的單位是「幾筆傳輸」，不是「幾個位元組」。** 這是最常見的錯誤。跑 `make mistake` 會示範把位元組數填進去會發生什麼事——引擎不會抱怨，它只是忠實地照你說的做了四倍的量。
2. **位址要不要遞增，是描述的一部分。** 程式裡的「複製」和「填值」用的是同一個引擎、同樣的筆數，差別只在來源位址走不走。
3. **每筆的大小只能是 1、2 或 4 個位元組**，而且位址要對齊。填錯引擎會直接停下來告訴你哪條規則被違反了。
4. **通道是有限資源**，用之前要取得、用完要歸還。這個引擎只有四個。

### 這個程式沒辦法示範的事

那個引擎在這裡是用一條執行緒做的，也就是說**它還是佔用了你電腦的一顆核心**。

真正的硬體不是這樣——它消耗的是記憶體匯流排的頻寬，不是 CPU 的指令。搬運期間 CPU 是真的空出來的，可以去做完全不相干的事情。

所以輸出裡那個「主程式跑了幾圈」的數字，只能說明「啟動之後你的程式還能繼續往下跑」，**不能**說明「這件事沒有成本」。這一層差別沒有硬體是做不出來的，你先知道有這回事就好。

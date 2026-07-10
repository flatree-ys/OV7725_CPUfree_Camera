# OV7725_Camera_Library

**Zero-CPU-Overhead OV7725 Camera Streaming Library for RP2350**

**RP2350向けゼロCPUオーバーヘッド OV7725 カメラストリーミングライブラリ**

CPU を一切使わずに OV7725 カメラから連続画像取得を行う Arduino ライブラリです。DMA と PIO を駆使した完全自動化により、RP2350 の 2 つの CPU コアをカメラキャプチャに邪魔されず、AI/ML やリアルタイム処理に専念させることができます。

*This Arduino library captures images from OV7725 camera without using CPU at all. By leveraging DMA and PIO automation, both CPU cores of RP2350 can be dedicated to tasks like AI/ML and real-time processing without being interrupted by camera capture.*

## Key Features / 主な特徴

- **CPU-Free Camera Capture** / **CPU非介入キャプチャ**: DMA と PIO による自動化で、CPU は一切の画像取得処理に関与しません / Automation using DMA and PIO ensures CPU has zero involvement in image acquisition
  
- **Dual-Buffer Architecture** / **ダブルバッファ構造**: A面/B面のダブルバッファメモリで、キャプチャと処理を完全に並列化 / Dual-buffer (A/B surface) enables complete parallelization of capture and processing
  
- **Fixed 30 FPS Streaming** / **30fps 固定配信**: 安定した 30fps で連続キャプチャ / Stable 30fps continuous capture
  
- **Color & Monochrome Modes** / **カラー・モノクロ選択**: カラー or グレースケールを選択可能 / Choose between color or grayscale output
  
- **Multiple Resolutions** / **複数解像度対応**:
  - QVGA: 320×240
  - QQVGA: 160×120
  - QQQVGA: 80×60
  
- **Lightweight** / **軽量**: CPU オーバーヘッド ≈ 0% → TinyML、物体検出、リアルタイム画像処理を実行可能 / CPU overhead ≈ 0% enables heavy processing like TinyML, object detection, real-time image processing

## Hardware Requirements / ハードウェア要件

- **MCU**: Raspberry Pi Pico 2 (RP2350) / **Camera**: OmniVision OV7725 (320×240 QVGA)
- **I2C**: カメラ設定用 / For camera configuration
- **Reset Pin**: カメラリセット用 / For camera reset
- **PIO**: 2 State Machines + 2 DMA Channels

**技術仕様 / Technical Specifications**:
- PIO 動作周波数: 144MHz 固定（システムクロックから自動分周） / PIO operates at 144MHz fixed (auto-divided from system clock)
- RP2350 標準 150MHz で正常動作確認済み / Verified working at RP2350 standard 150MHz
- RP2040 での動作確認はしていません / RP2040 operation not verified
- メモリバッファ: QVGA で最大 307KB（カラー・ダブルバッファ） / Memory buffer: max 307KB at QVGA (color, dual-buffer)
- メモリ不足の場合は QQVGA またはモノクロモードを使用してください / Use QQVGA or monochrome mode if memory is insufficient

### Pinout Example / ピン配置例 (RP2350)

```
Data Pins (連続かつ D2 が最小値であること) / (Must be consecutive with D2 as minimum):
D2-D9 (8-bit parallel): GP2-GP9（または任意、ただし連続かつ D2 が最小値）
                        (or any GPIO, but must be consecutive with D2 as minimum)

Sync Pins (任意のGPIO) / (Any GPIO):
PCLK (Pixel Clock):     GP[任意] / GP[any]
HREF (Line Sync):       GP[任意] / GP[any]
VSYNC (Frame Sync):     GP[任意] / GP[any]
XCLK (Camera Clock):    GP[任意] / GP[any] (PWM対応ピン / PWM-capable pin)
RST (Reset):            GP[任意] / GP[any]

I2C Pins (RP2350 の仕様に従う) / (Follow RP2350 I2C spec):
SDA:  GP4 or GP[any]
SCL:  GP5 or GP[any]

具体例 / Example:
D2-D9: GP2-GP9
PCLK: GP10, HREF: GP11, VSYNC: GP12
XCLK: GP13, RST: GP14
SDA: GP20, SCL: GP21
```

## Installation / インストール

### Method 1: Arduino Library Manager / 方法1: Arduino ライブラリマネージャー

1. Arduino IDE を起動 / Open Arduino IDE
2. **Sketch → Include Library → Manage Libraries**
3. `OV7725_Camera_Library` を検索 / Search for `OV7725_Camera_Library`
4. インストール / Click Install

### Method 2: ZIP ファイルからインストール / From ZIP File

1. **Sketch → Include Library → Add .ZIP Library**
2. ダウンロード済みの ZIP ファイルを選択 / Select the downloaded ZIP file

## Quick Start / クイックスタート

### 基本的な流れ / Basic Flow

```cpp
// メモリ定義（必ず最初） / Memory definition (must be first)
#define CAM_MAX_WIDTH 320
#define CAM_MAX_HEIGHT 240
#define CAM_COLOR 2 //Color:2, Mono:1
static uint32_t DMA_bufC[(CAM_MAX_WIDTH * CAM_MAX_HEIGHT * CAM_COLOR / 4) + 16] __attribute__((aligned(32)));
static uint32_t DMA_bufD[(CAM_MAX_WIDTH * CAM_MAX_HEIGHT * CAM_COLOR / 4) + 16] __attribute__((aligned(32)));

#include "OV7725_PDS.h"

// ピン定義 / Pin definitions
#define CAM_D2    2
#define CAM_D3    3
#define CAM_D4    4
#define CAM_D5    5
#define CAM_D6    6
#define CAM_D7    7
#define CAM_D8    8
#define CAM_D9    9
#define CAM_PCLK  10
#define CAM_HREF  11
#define CAM_VSYNC 12
#define CAM_XCLK  13
#define CAM_RST   14
#define CAM_SDA   20
#define CAM_SCL   21

// カメラオブジェクト生成 / Create camera object
OV7725_PDS camera(
    CAM_D2, CAM_D3, CAM_D4, CAM_D5, CAM_D6, CAM_D7, CAM_D8, CAM_D9,
    CAM_PCLK, CAM_HREF, CAM_VSYNC, CAM_XCLK,
    CAM_SDA, CAM_SCL, CAM_RST,
    DMA_bufC, DMA_bufD
);

void setup() {
    Serial.begin(115200);
    
    // RP2350 標準 150MHz で動作 / Works at RP2350 standard 150MHz
    // オーバークロック不要 / Overclocking not required
    // オーバークロックの場合は 6MHz の倍数であること / If overclocking, must be multiple of 6MHz
    delay(100);
    
    // 初期化（解像度モード、色モード を指定） / Initialize with resolution and color mode
    camera.initstart(OV7725_PDS::RES_QQVGA_color);
    // サポートモード / Supported modes:
    // RES_QVGA_mono   : 320x240 Monochrome
    // RES_QVGA_color  : 320x240 Color (YUV)
    // RES_QQVGA_mono  : 160x120 Monochrome
    // RES_QQVGA_color : 160x120 Color (YUV)
    // RES_QQQVGA_mono : 80x60 Monochrome
    // RES_QQQVGA_color: 80x60 Color (YUV)
    
    delay(100);
    
    // カメラ開始 / Start camera
    camera.begin();
}

void loop() {
    uint8_t* framePtr = camera.getLatestFrame();
    
    if (framePtr != nullptr) {
        // 新しいフレームが到着 / New frame arrived
        // ここで画像処理実行 / Run image processing here
        Serial.write(framePtr, camera.getBufferSize());
    }
}
```

## Data Format & Dual-Buffer Architecture / データフォーマット・ダブルバッファ構造

### フレームデータ形式 / Frame Data Format

#### カラー形式 / Color Format (YUV 4:2:2)

OV7725 のカラー出力は YUV 4:2:2 フォーマット。2ピクセルごとに 4 バイト 
OV7725 color output is YUV 4:2:2 format. 4 bytes per 2 pixels:

```
Y0 U Y1 V の順序 / Order: Y0 U Y1 V
Y0: ピクセル1の輝度 / Y0: Pixel 1 luminance
U:  ピクセル1-2 共有の U 成分 / U: Shared U component for pixels 1-2
Y1: ピクセル2の輝度 / Y1: Pixel 2 luminance
V:  ピクセル1-2 共有の V 成分 / V: Shared V component for pixels 1-2
```

1行分のデータ例 / Example one line (QQVGA Color 160×120):
```
配列インデックス / Array Index:
[0]   [1]   [2]   [3]   [4]   [5]  ...  [319]  ← y=0行 (2ピクセルで4バイト / 2 pixels = 4 bytes)
Y0_0  U_0   Y1_0  V_0   Y0_1  U_1  ...  V_79

[320] [321] [322] [323] ...                      ← y=1行 / y=1 row
...
[38080]...[38399]                                ← y=119行 / y=119 row
```

ピクセルアクセス（カラー） / Pixel access (color):
```cpp
uint8_t* frame = camera.getLatestFrame();
int width = camera.getCamWidth();

// ピクセル (x, y) にアクセス / Access pixel at (x, y)
// x 座標の偶偶に注意 / Note: parity of x coordinate matters
uint8_t y_value, u_value, v_value;

if (x % 2 == 0) {
    // 偶数：Y0 の位置 / Even: Y0 position
    y_value = frame[y * width * 2 + x * 2];
    u_value = frame[y * width * 2 + x * 2 + 1];
} else {
    // 奇数：Y1 の位置 / Odd: Y1 position
    y_value = frame[y * width * 2 + x * 2];
    v_value = frame[y * width * 2 + x * 2 + 1];
}
```

#### モノクロ形式 / Monochrome Format

- **1 ピクセル = 1 バイト** / 1 Pixel = 1 Byte (8-bit grayscale, 0-255)
- **値 0** = 黒 / Black
- **値 255** = 白 / White

1行分のデータ例 / Example one line (QQVGA Mono 160×120):
```
配列インデックス / Array Index:
[0]   [1]   [2]  ...  [159]    ← y=0行 / y=0 row
[160] [161] [162]...  [319]    ← y=1行 / y=1 row
...
[19040]...[19199]              ← y=119行 / y=119 row
```

ピクセルアクセス（モノクロ） / Pixel access (monochrome):
```cpp
uint8_t* frame = camera.getLatestFrame();
int width = camera.getCamWidth();

// ピクセル (x, y) の輝度値 / Brightness value at (x, y)
uint8_t brightness = frame[y * width + x];
```

### ダブルバッファ動作 / Dual-Buffer Operation

30fps 固定で A面(bufC) / B面(bufD) を交互使用 / Alternates between bufC and bufD at fixed 30fps:

```
Frame 1: [DMA → bufC] ... [完成 / Complete] → bufC = Ready (ID=0)
Frame 2: [DMA → bufD] ... [完成 / Complete] → bufD = Ready (ID=1)
Frame 3: [DMA → bufC] ... [完成 / Complete] → bufC = Ready (ID=0)
...

Frame 読取と次フレーム取得が重ならない / Frame reading and next capture don't overlap
```

## API Reference

### OV7725_PDS Class

#### 初期化・開始停止 / Initialization & Control

**`bool initstart(ResolutionMode mode)`**
- PIO、DMA、カメラを一括初期化 / Initialize PIO, DMA, and camera
- `mode`: `RES_QVGA_mono`, `RES_QVGA_color`, `RES_QQVGA_mono`, `RES_QQVGA_color`, `RES_QQQVGA_mono`, `RES_QQQVGA_color`
- Returns: 成功時 `true` / `true` on success

**`bool begin()`**
- カメラと PIO を起動 / Start camera and PIO

**`bool stop()`**
- カメラと PIO を停止 / Stop camera and PIO

**`bool restart()`**
- カメラを再起動 / Restart camera

#### フレーム取得 / Frame Acquisition

**`uint8_t* getLatestFrame()`**
- ダンプバッファにコピーされた最新フレーム / Latest frame copied to dump buffer
- 新規フレーム非検出時は `nullptr` / Returns `nullptr` if no new frame

**`int getReadyBufferID()`**
- DMA 完了バッファ ID / DMA completed buffer ID (0 or 1, -1 if initializing)

**`uint32_t* getBufferC()` / `getBufferD()`**
- 生バッファアドレス / Raw buffer address

**`size_t getBufferSize()`**
- フレームサイズ（バイト数） / Frame size in bytes

**`int getCamWidth()` / `getCamHeight()` / `getCamColor()`**
- 解像度と色深度 / Resolution and color depth

#### 設定変更 / Configuration

**`void setting(uint8_t reg, uint8_t val)`**
- カメラレジスタを直接操作 / Directly control camera register

## Performance / パフォーマンス

| Mode | Resolution | Color | Buffer Size | Memory (dual) | FPS |
|------|------------|-------|-------------|---------------|-----|
| QVGA | 320×240 | Color | 153.6 KB | 307.2 KB | 30 |
| QVGA | 320×240 | Mono | 76.8 KB | 153.6 KB | 30 |
| QQVGA | 160×120 | Color | 38.4 KB | 76.8 KB | 30 |
| QQVGA | 160×120 | Mono | 19.2 KB | 38.4 KB | 30 |
| QQQVGA | 80×60 | Color | 9.6 KB | 19.2 KB | 30 |
| QQQVGA | 80×60 | Mono | 4.8 KB | 9.6 KB | 30 |

## Example Applications / 応用例

- **Color Object Detection** / **カラー物体検出**: カラー画像での物体検出 / Color image-based object detection
- **Line Tracing Robot** / **ライントレーサー**: ライントレース制御 / Line tracing control
- **AI Face Recognition** / **顔認識**: 顔認識 AI / Face recognition AI
- **Live Color Streaming** / **カラー映像配信**: カラー映像のリアルタイム配信 / Real-time color video streaming

## Important Notes / 重要な注意事項

### カラー vs モノクロの選択 / Color vs Monochrome

- **カラーモード** / **Color Mode**: より多くのメモリ使用（2倍）、視覚情報が豊富 / Uses 2x more memory, richer visual information
- **モノクロモード** / **Monochrome Mode**: メモリ節約、処理速度向上 / Saves memory, faster processing

### メモリ不足時の対策 / When Running Out of Memory

メモリが足りない場合は以下を検討してください / If memory is insufficient, consider:
1. QVGA → QQVGA に解像度を下げる / Reduce resolution from QVGA to QQVGA
2. Color → Monochrome に変更 / Switch from Color to Monochrome
3. ダブルバッファを 1 つ減らす / Reduce to single buffer (requires additional implementation)

### ピン配置 / Pinout

- **D2～D9**: 連続かつ D2 が最小値 / Must be consecutive with D2 as minimum
- **XCLK**: PWM 対応ピン / PWM-capable pin
- **RST**: 任意の GPIO / Any GPIO
- **SDA/SCL**: RP2350 の I2C 仕様に従う / Follow RP2350 I2C specification

### メモリ確保 / Memory Allocation

```cpp
static uint32_t DMA_bufC[...] __attribute__((aligned(32)));
static uint32_t DMA_bufD[...] __attribute__((aligned(32)));
```

必ずグローバル static で確保 / Must allocate as global static

## Troubleshooting / トラブルシューティング

| 症状 / Symptom | 原因 / Cause | 解決策 / Solution |
|------|------|--------|
| フレームが来ない / No frames | PIO 起動失敗、リセット失敗 / PIO startup or reset failed | リセットピン確認、電源確認 / Check reset pin, power |
| ノイズ画像 / Noisy image | タイミングズレ / Timing error | ケーブル短縮、接続確認 / Shorten cables, check connection |
| I2C エラー / I2C error | アドレス競合 / Address conflict | 他の I2C デバイス確認 / Check other I2C devices |
| メモリ不足 / Out of memory | バッファサイズ過剰 / Buffer too large | 解像度またはカラーモード変更 / Change resolution or color mode |

## License / ライセンス

MIT License - 自由に使用・改変・配布可能 / Free to use, modify, and distribute

## Contributing / 貢献

バグ報告・機能リクエストは GitHub Issues へ / Bug reports and feature requests to GitHub Issues

---

Made with RP2350 & Embedded Vision enthusiasts in mind

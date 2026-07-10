// ======================================================================
// 使用メモリ確保設定(静的確保):ダブルバッファ1つあたりのサイズ
// Memory Allocation Settings (Static Allocation): Size per single double-buffer
// 使用する最大の解像度に合わせて入力。（実使用はこの範囲で変更）
// Input according to the maximum resolution to be used. (Actual usage can be changed within this range)
// 【設定値】 / 【Setting Values】
// RES_QVGA_Color   (320*240 YUV)   :1
// RES_QQVGA_Color  (160*120 YUV)   :2
// RES_QQQVGA_Color (80*60 YUV)     :3
// RES_QVGA_nomo    (320*240)       :4
// RES_QQVGA_nomo   (160*120)       :5
// RES_QQQVGA_mono  (80* 60)        :6
// ======================================================================
#define USE_CAM_MODE  1
#define CAM_MAX_BUFFER_SIZE ( \
    (USE_CAM_MODE) == 2 ? (160 * 120 * 2) : \
    (USE_CAM_MODE) == 3 ? (80  * 60  * 2) : \
    (USE_CAM_MODE) == 4 ? (320 * 240 * 1) : \
    (USE_CAM_MODE) == 5 ? (160 * 120 * 1) : \
    (USE_CAM_MODE) == 6 ? (80  * 60  * 1) : (320 * 240 * 2) )
static uint32_t DMA_bufC[(CAM_MAX_BUFFER_SIZE / 4)] __attribute__((aligned(32)));
static uint32_t DMA_bufD[(CAM_MAX_BUFFER_SIZE / 4)] __attribute__((aligned(32)));
#include "OV7725_PDS.h" //サイズ定義後に実施のこと/To be executed after size definitions

// 8bitシリアルモード用 新ピン定義 (GP配置)
// New pin definitions for 8-bit serial mode (GPIO layout)
// 配置を変えても良いが、[D2]-[D9]は連続すること。
// The layout can be changed, but [D2] to [D9] must be contiguous.
// D2がGPIOで最も若く順序が一致していること(データ順)
// D2 must have the lowest GPIO pin number, and the sequence must match the data order.
// D0-D1はYUVモードでは使わない
// D0-D1 are not used in YUV mode
#define CAM_D2     2  // CAM_D2 (D2) -> GP2
#define CAM_D3     3  // CAM_D3 (D3) -> GP3
#define CAM_D4     4  // CAM_D4 (D4) -> GP4
#define CAM_D5     5  // CAM_D5 (D5) -> GP5
#define CAM_D6     6  // CAM_D6 (D6) -> GP6
#define CAM_D7     7  // CAM_D7 (D7) -> GP7
#define CAM_D8     8  // CAM_D6 (D8) -> GP8
#define CAM_D9     9  // CAM_D7 (D9) -> GP9
//ここからは任意でOK
//From here onwards is optional
#define CAM_PCLK  10  // CAM_PCLK    -> GP10
#define CAM_XCLK  11  // CAM_XCLK    -> GP11
#define CAM_HREF  12   // CAM_HREF    -> GP12
#define CAM_VSYNC 13   // CAM_VSYNC   -> GP13
#define CAM_RST   14  // CAM_RST -> GP14
//RP2350のSDA, SCLに合わせる(Wire1かWireかは自動判別)
//Match with the SDA and SCL of RP2350 (Wire1 or Wire will be automatically detected)
#define CAM_SDA   0  // RPI_SIOD (SDA) -> GP0
#define CAM_SCL   1  // RPI_SIOC (SCL) -> GP1


//カメラ設定ピン設定
//Camera configuration pin settings
OV7725_PDS camera(
    CAM_D2, CAM_D3, CAM_D4, CAM_D5, CAM_D6, CAM_D7, CAM_D8, CAM_D9, 
    CAM_PCLK, CAM_HREF, CAM_VSYNC, CAM_XCLK, 
    CAM_SDA, CAM_SCL, CAM_RST,
    DMA_bufC, DMA_bufD
);

void setup() {
    // 初期設定クロック解除
    //Release initial configuration clock
    //144MHz以上で6MHzの倍数なら動く
    //Works at 144MHz or higher if it is a multiple of 6MHz
    set_sys_clock_khz(150000, true);
    delay(100); 

    // シリアル設定
    // Serial settings
    Serial.begin(115200);
    delay(1000); 

    // カメラ/PIO/DMA一括設定
    //Bulk configuration for Camera/PIO/DMA
    camera.initstart(OV7725_PDS::RES_QVGA_Color);
    /* Frame rate:30fps(fixed)
        RES_QVGA_color  : 320*240 YUV - Data:[Y0-8bit][U-8bit][Y1-8bit][V-8bit]
        RES_QQVGA_color : 160*120 YUV - Data:[Y0-8bit][U-8bit][Y1-8bit][V-8bit]
        RES_QQQVGA_color:  80* 60 YUV - Data:[Y0-8bit][U-8bit][Y1-8bit][V-8bit]
        RES_QVGA_nomo   : 320*240  - Data:[Gray ScaleY-8bit]
        RES_QQVGA_nomo  : 160*120  - Data:[Gray ScaleY-8bit]
        RES_QQQVGA_mono :  80* 60  - Data:[Gray ScaleY-8bit]
    */
    delay(100); //安定化待ち/Waiting for stabilization

    /// カメラ設定/Camera Settings ///
    /*変更がある場合はここで下記の通りカメラのレジスタを叩く*/
    /*If there are changes, modify the camera registers here as shown below*/
    // //camera.setting(0x00, 0xFF); // レジスタ 0x00 に 0xFF を書き込み   
    // //camera.setting(0x00, 0xFF); // Write 0xFF to register 0x00   
    // camera.setting(0x13, 0x00);   // COM8: 0x00AGC/AEC/AWB全て無効
    // camera.setting(0x13, 0x00);   // COM8: 0x00 AGC/AEC/AWB all disabled
    // camera.setting(0x00, 0x10);      //明るさ：ゲインコントロール 
    // camera.setting(0x00, 0x10);      //Brightness: Gain Control 
    // camera.setting(0x01, 0x80);      //色調整：青
    // camera.setting(0x01, 0x80);      //Color Adjustment: Blue
    // camera.setting(0x02, 0x40);      //色調整：赤
    // camera.setting(0x02, 0x40);      //Color Adjustment: Red
    // camera.setting(0x03, 0x80);      //色調整：緑
    // camera.setting(0x03, 0x80);      //Color Adjustment: Green
    delay(50); //安定化待ち
    /////////////////

    //PIO&カメラ起動
    //Start PIO & Camera
    camera.begin();
}

//テスト用（バッファ直撮り）->シリアル通信遅れでCPUが周回遅れになる
//For testing (Direct buffer capture) -> Serial communication delay causes the CPU to fall a lap behind
int lastProcessedBufferID = -1;
void loop() {
    // 現在、DMAが書き込みを完了した「生のバッファID（0または1）」を直接取得
    // Directly obtain the "raw buffer ID (0 or 1)" that the DMA has currently finished writing to int currentReadyID = camera.getReadyBufferID();
    if(currentReadyID != lastProcessedBufferID) {
        uint32_t* rawBufferPtr = nullptr;
        if (currentReadyID == 0) {
            rawBufferPtr = camera.getBufferC(); // A面の生アドレスを直接ロックオン/Directly lock on to the raw address of Side A
        } else if (currentReadyID == 1) {
            rawBufferPtr = camera.getBufferD(); // B面の生アドレスを直接ロックオン/Directly lock on to the raw address of Side B
        } else {
            return; // 起動直後の初期化中(-1)などはスルー/Skip during initialization (-1) etc. immediately after startup
        }
        lastProcessedBufferID = currentReadyID;
        uint8_t marker[8] = { 0xFF, 0xAA, 0x55, 0xAA, 0xFF, 0xAA, 0x55, 0xAA };
        Serial.write(marker, 8);
        Serial.write((uint8_t*)rawBufferPtr, camera.getBufferSize());
        Serial.flush();
    }
}

//2系統CPUを使う場合は以下
//Below is for when using dual-core CPUs
void setup1() {
}
void loop1() {
}

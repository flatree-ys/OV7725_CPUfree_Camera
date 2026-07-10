#ifndef OV7725_H
#define OV7725_H

#include <Arduino.h>
#include <Wire.h>
#include "hardware/pio.h"

class OV7725 {
public:
OV7725(int d2, int d3, int d4, int d5, int d6, int d7, int d8, int d9, int pclk, int href, int vsync, int xclk, TwoWire* i2c = &Wire);
    bool init();
    bool stop();
    bool restart();
    void setExposure(uint16_t exposure);
    void setAnalogGain(uint8_t gain);
    bool startPIO();
    bool startXCLK();

    int getCamWidth() const { return _camWidth; }
    int getCamHeight() const { return _camHeight; }
    int getCamColor() const { return _camColor; }
    PIO getPIO() const { return _pio; }
    int getSM() const { return _sm; }
    int getVsyncSM() { return _vsyncSM; }

    enum ResolutionMode {
        RES_QVGA_mono,
        RES_QVGA_color,
        RES_QQVGA_mono,
        RES_QQVGA_color,
        RES_QQQVGA_mono,
        RES_QQQVGA_color
    };

    bool init(ResolutionMode mode);
    void writeRegister(uint8_t reg, uint8_t val);

private:
    int _pinD2, _pinD3, _pinD4, _pinD5, _pinD6, _pinD7, _pinD8, _pinD9;
    int _pinPCLK, _pinHREF, _pinVSYNC, _pinXCLK;
    int _camWidth, _camHeight, _camColor;
    int _vsyncSM;
    PIO _pio; int _sm; uint _pioOffset;
    pio_program_t* _compiledProgram;

    void initPIO();
    ResolutionMode _currentMode;
    TwoWire* _i2c;
};

#endif
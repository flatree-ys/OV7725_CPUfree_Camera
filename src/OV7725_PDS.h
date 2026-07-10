#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "OV7725.h"
#include "CameraDMAStreamer.h"

class OV7725_PDS {
public:
    using ResolutionMode = OV7725::ResolutionMode;
    static const ResolutionMode RES_QVGA_color   = OV7725::RES_QVGA_color;
    static const ResolutionMode RES_QQVGA_color  = OV7725::RES_QQVGA_color;
    static const ResolutionMode RES_QQQVGA_color = OV7725::RES_QQQVGA_color;
    static const ResolutionMode RES_QVGA_mono    = OV7725::RES_QVGA_mono;
    static const ResolutionMode RES_QQVGA_mono   = OV7725::RES_QQVGA_mono;
    static const ResolutionMode RES_QQQVGA_mono  = OV7725::RES_QQQVGA_mono;

    OV7725_PDS(int d2, int d3, int d4, int d5, int d6, int d7, int d8, int d9,
               int pclk, int href, int vsync, int xclk,
               int sda, int scl, int rst,
               uint32_t* bufC, uint32_t* bufD);
    ~OV7725_PDS();

    bool initstart(ResolutionMode mode);
    bool begin();
    bool stop();
    bool restart();
    
    uint8_t* getLatestFrame();
    size_t getBufferSize() const { return _bufferSize; }

    int getReadyBufferID() const { return _streamer ? _streamer->getReadyBufferID() : -1; }
    uint32_t* getBufferC() const { return _streamer ? _streamer->getBufferC() : nullptr; }
    uint32_t* getBufferD() const { return _streamer ? _streamer->getBufferD() : nullptr; }
    int getCamWidth() const { return _camera ? _camera->getCamWidth() : 0; }
    int getCamHeight() const { return _camera ? _camera->getCamHeight() : 0; }

    void setting(uint8_t reg, uint8_t val) {
        if (_camera) _camera->writeRegister(reg, val);
    }

private:
    OV7725* _camera;
    CameraDMAStreamer* _streamer;

    int _pinSDA;
    int _pinSCL;
    int _pinRST;
    TwoWire* _i2c; 
    size_t _bufferSize;
    uint32_t* _dumpBuffer; 
    int _lastProcessedBufferID;
    uint32_t* _bufC;
    uint32_t* _bufD;
};
#include "OV7725_PDS.h"
#include <malloc.h>

OV7725_PDS::OV7725_PDS(int d2, int d3, int d4, int d5, int d6, int d7, int d8, int d9,
                       int pclk, int href, int vsync, int xclk, 
                       int sda, int scl, int rst,
                       uint32_t* bufC, uint32_t* bufD)
    : _camera(nullptr), _streamer(nullptr), _pinSDA(sda), _pinSCL(scl), _pinRST(rst),
      _bufferSize(0), _dumpBuffer(nullptr), _lastProcessedBufferID(-1),
      _bufC(bufC), _bufD(bufD) {
    
    if (((sda / 2) % 2) == 0) {
        _i2c = &Wire;
    } else {
        _i2c = &Wire1;
    }

    _camera = new OV7725(d2, d3, d4, d5, d6, d7, d8, d9, pclk, href, vsync, xclk, _i2c);
}

OV7725_PDS::~OV7725_PDS() {
    if (_dumpBuffer) free(_dumpBuffer);
    if (_streamer) { delete _streamer; _streamer = nullptr; }
    if (_camera) { delete _camera; _camera = nullptr; }
}

bool OV7725_PDS::initstart(ResolutionMode mode) {
    _camera->startXCLK(); 
    delay(10);
    gpio_init(_pinRST);
    gpio_set_dir(_pinRST, GPIO_OUT);    
    gpio_put(_pinRST, 0);
    delay(10);
    gpio_put(_pinRST, 1);
    delay(10);

    _i2c->setSDA(_pinSDA);
    _i2c->setSCL(_pinSCL);
    _i2c->begin();

    delay(20); 
    if (!_camera->init(mode)) return false;
    delay(10); 

    _bufferSize = (size_t)(_camera->getCamWidth() * _camera->getCamHeight() * _camera->getCamColor());
    _dumpBuffer = nullptr; 

    _streamer = new CameraDMAStreamer(
        _camera->getPIO(), 
        _camera->getSM(), 
        _camera->getVsyncSM(), 
        _camera->getCamWidth(), 
        _camera->getCamHeight(), 
        _camera->getCamColor(),
        _bufC, _bufD
    );
    delay(10); 
    _streamer->begin();
    delay(10); 

    return true;
}

bool OV7725_PDS::begin() {
    if (_camera) {
        return _camera->startPIO();
    }
    return false;
}

bool OV7725_PDS::stop() {
    if (_camera) {
        return _camera->stop();
    }
    return false;
}

bool OV7725_PDS::restart(){
    if (_camera) {
        return _camera->restart();
    }
    return false;
}

uint8_t* OV7725_PDS::getLatestFrame() {
    if (!_streamer) return nullptr;

    int currentReadyID = _streamer->getReadyBufferID();
    
    if (currentReadyID != _lastProcessedBufferID) {
        void* activeBufferPtr = nullptr;
        
        if (currentReadyID == 0) {
            activeBufferPtr = (void*)_streamer->getBufferC();
        } else if (currentReadyID == 1) {
            activeBufferPtr = (void*)_streamer->getBufferD();
        } else {
            return nullptr;
        }

        _lastProcessedBufferID = currentReadyID;

        if (_dumpBuffer == nullptr) {
            _dumpBuffer = (uint32_t*)memalign(8, _bufferSize);
            if (_dumpBuffer == nullptr) return nullptr; 
        }

        memcpy(_dumpBuffer, activeBufferPtr, _bufferSize);
        return (uint8_t*)_dumpBuffer;
    }
    return nullptr;
}
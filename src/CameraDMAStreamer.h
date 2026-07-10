#ifndef CAMERA_DMA_STREAMER_H
#define CAMERA_DMA_STREAMER_H

#include <Arduino.h>
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "OV7725.h"

struct DMAChunk {
    uint32_t ctrl_value;     
    void* read_addr;         
    void* write_addr;        
    uint32_t transfer_count; 
};

class CameraDMAStreamer {
public:
    CameraDMAStreamer(
      PIO pio, int sm, int vsyncSM, 
      int width, int height, int color, 
      uint32_t* bufC, uint32_t* bufD
    );
    void begin();
    int getReadyBufferID();      
    uint32_t* getBufferC() { return _bufC; }
    uint32_t* getBufferD() { return _bufD; }
private:
    PIO _pio;
    int _sm;
    int _chanCtrl; 
    int _chanData; 
    int _vsyncSM;
    int _camWidth;
    int _camHeight;
    int _camColor;

    uint32_t* _bufC;
    uint32_t* _bufD;
    
    static struct DMAChunk camera_chunks[9] __attribute__((aligned(64))); 
    
    static const uint32_t VAL_ZERO;
    static const uint32_t VAL_ONE;
    static volatile uint32_t DMA_SIGNAL_X; 
    static volatile uint32_t _dummyRoom;
};

#endif
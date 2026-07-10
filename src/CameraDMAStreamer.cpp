#include "CameraDMAStreamer.h"
#include "hardware/pwm.h"
#include "pico/multicore.h"
#include <malloc.h>

struct DMAChunk CameraDMAStreamer::camera_chunks[9] __attribute__((aligned(64)));

const uint32_t CameraDMAStreamer::VAL_ZERO = 0;
const uint32_t CameraDMAStreamer::VAL_ONE  = 1;
volatile uint32_t CameraDMAStreamer::DMA_SIGNAL_X = -1; 
volatile uint32_t CameraDMAStreamer::_dummyRoom = 0;

CameraDMAStreamer::CameraDMAStreamer(PIO pio, int sm, int vsyncSM, int width, int height, int color, uint32_t* bufC, uint32_t* bufD) 
    : _pio(pio), _sm(sm), _vsyncSM(vsyncSM), _camWidth(width), _camHeight(height), _camColor(color), 
      _chanCtrl(-1), _chanData(-1), _bufC(bufC), _bufD(bufD) { 
}
int CameraDMAStreamer::getReadyBufferID() {
    return DMA_SIGNAL_X; 
}

void CameraDMAStreamer::begin() {
    uint32_t active_transfer_bytes = _camWidth * _camHeight * _camColor;

    _chanCtrl = dma_claim_unused_channel(true); 
    _chanData = dma_claim_unused_channel(true); 
    
    pio_sm_clear_fifos(_pio, _sm);
    void* pio_fifo_addr = (void*)&_pio->rxf[_sm]; 

    dma_channel_config cCtrl = dma_channel_get_default_config(_chanCtrl);
    channel_config_set_transfer_data_size(&cCtrl, DMA_SIZE_32);
    channel_config_set_read_increment(&cCtrl, true);  
    channel_config_set_write_increment(&cCtrl, true); 
    channel_config_set_ring(&cCtrl, true, 4); 
    dma_channel_configure(
        _chanCtrl,
        &cCtrl, 
        &dma_hw->ch[_chanData].al1_ctrl, 
        &camera_chunks[3],                     
        4,                               
        false                                  
    );

    dma_channel_config cDataRaw = dma_channel_get_default_config(_chanData);
    channel_config_set_transfer_data_size(&cDataRaw, DMA_SIZE_32); 
    channel_config_set_read_increment(&cDataRaw, false);  
    channel_config_set_dreq(&cDataRaw, pio_get_dreq(_pio, _sm, false));
    channel_config_set_chain_to(&cDataRaw, _chanCtrl);        
    dma_channel_configure(
        _chanData, 
        &cDataRaw, 
        _bufC,
        pio_fifo_addr,
        0,
        false
    );

    uint vsync_dreq = pio_get_dreq(_pio, _vsyncSM, false);

    dma_channel_config cWaitA = cDataRaw;
    channel_config_set_write_increment(&cWaitA, false);
    channel_config_set_dreq(&cWaitA, vsync_dreq); 
    camera_chunks[0].ctrl_value     = cWaitA.ctrl;
    camera_chunks[0].read_addr      = pio_fifo_addr;       
    camera_chunks[0].write_addr     = (void*)&_dummyRoom; 
    camera_chunks[0].transfer_count = 4;                  

    dma_channel_config cDataA = cDataRaw;
    channel_config_set_write_increment(&cDataA, true); 
    camera_chunks[1].ctrl_value     = cDataA.ctrl;
    camera_chunks[1].read_addr      = pio_fifo_addr;
    camera_chunks[1].write_addr     = _bufC;
    camera_chunks[1].transfer_count = active_transfer_bytes /4; 

    dma_channel_config cFlagA = cDataRaw;
    channel_config_set_write_increment(&cFlagA, false);
    channel_config_set_dreq(&cFlagA, 0x3f); 
    camera_chunks[2].ctrl_value     = cFlagA.ctrl;
    camera_chunks[2].read_addr      = (void*)&VAL_ZERO;
    camera_chunks[2].write_addr     = (void*)&DMA_SIGNAL_X; 
    camera_chunks[2].transfer_count = 1;

    static uint32_t pio_irq_clear_val = 0x10;
    dma_channel_config cVSYNCreset = cDataRaw;
    channel_config_set_write_increment(&cVSYNCreset, false);
    channel_config_set_dreq(&cVSYNCreset, 0x3f); 
    camera_chunks[3].ctrl_value     = cVSYNCreset.ctrl;
    camera_chunks[3].read_addr      = (void*)&_pio->rxf[_vsyncSM];
    camera_chunks[3].write_addr     = (void*)&_dummyRoom;
    camera_chunks[3].transfer_count = 4;

    camera_chunks[4] = camera_chunks[0];          

    camera_chunks[5] = camera_chunks[1]; 
    camera_chunks[5].write_addr     = _bufD;               

    camera_chunks[6] = camera_chunks[2]; 
    camera_chunks[6].read_addr      = (void*)&VAL_ONE;     

    camera_chunks[7] = camera_chunks[3]; 

    static const void* RESTART_TARGET_ADDR = &camera_chunks[0]; 
    dma_channel_config cLoop = cDataRaw;
    channel_config_set_write_increment(&cLoop, false);
    channel_config_set_dreq(&cLoop, 0x3f); 
    camera_chunks[8].ctrl_value     = cLoop.ctrl;
    camera_chunks[8].read_addr      = (void*)&RESTART_TARGET_ADDR; 
    camera_chunks[8].write_addr     = (void*)&dma_hw->ch[_chanCtrl].al3_read_addr_trig; 
    camera_chunks[8].transfer_count = 1;

    DMA_SIGNAL_X = 0xFFFFFFFF; 

    dma_start_channel_mask(1u << _chanCtrl);
}
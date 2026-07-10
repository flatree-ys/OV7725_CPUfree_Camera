#include "OV7725.h"
#include <Wire.h>
#include "hardware/pwm.h"

OV7725::OV7725(int d2, int d3, int d4, int d5, int d6, int d7, int d8, int d9, int pclk, int href, int vsync, int xclk, TwoWire* i2c) {
    _pinD2 = d2; 
    _pinD3 = d3; 
    _pinD4 = d4; 
    _pinD5 = d5; 
    _pinD6 = d6; 
    _pinD7 = d7; 
    _pinD8 = d8; 
    _pinD9 = d9; 
    _pinPCLK = pclk; 
    _pinHREF = href; 
    _pinVSYNC = vsync; 
    _pinXCLK = xclk;
    _i2c = i2c;
    _pio = pio0; 
    _compiledProgram = nullptr;
}

bool OV7725::startPIO() {
    writeRegister(0x09, 0x01);
    delay(100);
    pio_sm_set_enabled(_pio, _vsyncSM, true); 
    pio_sm_set_enabled(_pio, _sm, true);
    return true;
}

bool OV7725::startXCLK() {
    gpio_set_function(_pinXCLK, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(_pinXCLK);
    uint32_t sys_clk_hz = clock_get_hz(clk_sys);
    uint32_t total_count = sys_clk_hz / 24000000;
    uint32_t wrap_val = total_count - 1;
    pwm_set_wrap(slice_num, wrap_val);
    pwm_set_gpio_level(_pinXCLK, total_count / 2);
    pwm_set_clkdiv(slice_num, 1.0f);
    pwm_set_enabled(slice_num, true);
    delay(100);
    return true;
}

void OV7725::initPIO() {
    _sm = pio_claim_unused_sm(_pio, true);
    gpio_set_input_hysteresis_enabled(_pinD2, true);
    gpio_set_input_hysteresis_enabled(_pinD3, true);
    gpio_set_input_hysteresis_enabled(_pinD4, true);
    gpio_set_input_hysteresis_enabled(_pinD5, true);
    gpio_set_input_hysteresis_enabled(_pinD6, true);
    gpio_set_input_hysteresis_enabled(_pinD7, true);
    gpio_set_input_hysteresis_enabled(_pinD8, true);
    gpio_set_input_hysteresis_enabled(_pinD9, true); 
    gpio_set_input_hysteresis_enabled(_pinPCLK, true);
    gpio_set_input_hysteresis_enabled(_pinHREF, true);
    gpio_set_input_hysteresis_enabled(_pinVSYNC, true);
    gpio_pull_down(_pinD2);
    gpio_pull_down(_pinD3);
    gpio_pull_down(_pinD4);
    gpio_pull_down(_pinD5);
    gpio_pull_down(_pinD6);
    gpio_pull_down(_pinD7);
    gpio_pull_down(_pinD8);
    gpio_pull_down(_pinD9);
    gpio_pull_down(_pinPCLK);
    gpio_pull_down(_pinHREF);
    gpio_pull_down(_pinVSYNC);


    const uint16_t* pio_instructions = nullptr;
    size_t pio_instructions_length = 0;
    uint16_t wrap_max = 12;
    if (_currentMode == OV7725::RES_QVGA_color) {
        static const uint16_t pio_qvga[] = {
            (uint16_t)pio_encode_wait_gpio(true, _pinHREF),
            (uint16_t)pio_encode_wait_gpio(true, _pinPCLK),
            (uint16_t)pio_encode_in(pio_pins, 8),
            (uint16_t)pio_encode_wait_gpio(false, _pinPCLK),      
        };
        pio_instructions = pio_qvga;
        pio_instructions_length = sizeof(pio_qvga) / sizeof(uint16_t);
        wrap_max = 3;
    } else if (_currentMode == OV7725::RES_QQVGA_color)  {
        static const uint16_t pio_qqvga[] = {
            (uint16_t)pio_encode_set(pio_y, 1),
            (uint16_t)pio_encode_wait_gpio(true, _pinHREF),
            //
            (uint16_t)pio_encode_set(pio_x, 3),
            (uint16_t)pio_encode_wait_gpio(true, _pinPCLK),
            //
            (uint16_t)pio_encode_in(pio_pins, 8), 
            (uint16_t)pio_encode_wait_gpio(false, _pinPCLK),
            (uint16_t)pio_encode_jmp_x_dec(_pioOffset +3),
            //
            (uint16_t)pio_encode_set(pio_x, 3),
            (uint16_t)pio_encode_wait_gpio(true, _pinPCLK),	
            (uint16_t)pio_encode_wait_gpio(false, _pinPCLK),
            (uint16_t)pio_encode_jmp_x_dec(_pioOffset +8),
            //
            (uint16_t)pio_encode_jmp_pin(_pioOffset + 2),
            //
            (uint16_t)pio_encode_jmp_y_dec(_pioOffset +1),
            //
            (uint16_t)pio_encode_set(pio_y, 1), 
            (uint16_t)pio_encode_wait_gpio(true, _pinHREF),
            (uint16_t)pio_encode_wait_gpio(false, _pinHREF),
            (uint16_t)pio_encode_jmp_y_dec(_pioOffset +14),
        };
        pio_instructions = pio_qqvga;
        pio_instructions_length = sizeof(pio_qqvga) / sizeof(uint16_t);
        wrap_max = 16;
    } else if (_currentMode == OV7725::RES_QQQVGA_color)  {
        static const uint16_t pio_qqqvga[] = {
            (uint16_t)pio_encode_set(pio_y, 1),
            (uint16_t)pio_encode_wait_gpio(true, _pinHREF),
            //
            (uint16_t)pio_encode_set(pio_x, 3),
            (uint16_t)pio_encode_wait_gpio(true, _pinPCLK),
            (uint16_t)pio_encode_in(pio_pins, 8),
            (uint16_t)pio_encode_wait_gpio(false, _pinPCLK),
            (uint16_t)pio_encode_jmp_x_dec(_pioOffset +3),
            //
            (uint16_t)pio_encode_set(pio_x, 11),
            (uint16_t)pio_encode_wait_gpio(true, _pinPCLK),
            (uint16_t)pio_encode_wait_gpio(false, _pinPCLK),
            (uint16_t)pio_encode_jmp_x_dec(_pioOffset +8),
            //
            (uint16_t)pio_encode_jmp_pin(_pioOffset + 2),
            //
            (uint16_t)pio_encode_jmp_y_dec(_pioOffset +1),
            //
            (uint16_t)pio_encode_set(pio_y, 5), 
            (uint16_t)pio_encode_wait_gpio(true, _pinHREF),
            (uint16_t)pio_encode_wait_gpio(false, _pinHREF),
            (uint16_t)pio_encode_jmp_y_dec(_pioOffset +14),
        };
        pio_instructions = pio_qqqvga;
        pio_instructions_length = sizeof(pio_qqqvga) / sizeof(uint16_t);
        wrap_max = 16;
    } else if (_currentMode == OV7725::RES_QVGA_mono)  {
        static const uint16_t pio_qvga[] = {
            (uint16_t)pio_encode_wait_gpio(true, _pinHREF),
            (uint16_t)pio_encode_wait_gpio(true, _pinPCLK),	
            (uint16_t)pio_encode_in(pio_pins, 8),
            (uint16_t)pio_encode_wait_gpio(false, _pinPCLK),
            (uint16_t)pio_encode_wait_gpio(true, _pinPCLK),
            (uint16_t)pio_encode_wait_gpio(false, _pinPCLK),         
        };
        pio_instructions = pio_qvga;
        pio_instructions_length = sizeof(pio_qvga) / sizeof(uint16_t);
        wrap_max = 5;
    } else if (_currentMode == OV7725::RES_QQVGA_mono)  {
        static const uint16_t pio_qqvga[] = {
            (uint16_t)pio_encode_wait_gpio(true, _pinHREF),
            //
            (uint16_t)pio_encode_wait_gpio(true, _pinPCLK),
            (uint16_t)pio_encode_in(pio_pins, 8),
            (uint16_t)pio_encode_wait_gpio(false, _pinPCLK),
            //
            (uint16_t)pio_encode_set(pio_x, 2),
            (uint16_t)pio_encode_wait_gpio(true, _pinPCLK),
            (uint16_t)pio_encode_wait_gpio(false, _pinPCLK),
            (uint16_t)pio_encode_jmp_x_dec(_pioOffset +5),
            //
            (uint16_t)pio_encode_jmp_pin(_pioOffset + 1), 
            //
            (uint16_t)pio_encode_set(pio_y, 0),
            (uint16_t)pio_encode_wait_gpio(true, _pinHREF),
            (uint16_t)pio_encode_wait_gpio(false, _pinHREF),
            (uint16_t)pio_encode_jmp_y_dec(_pioOffset +10),
        };
        pio_instructions = pio_qqvga;
        pio_instructions_length = sizeof(pio_qqvga) / sizeof(uint16_t);
        wrap_max = 12;
    }else if (_currentMode == OV7725::RES_QQQVGA_mono)  {
        static const uint16_t pio_qqqvga[] = {
            (uint16_t)pio_encode_wait_gpio(true, _pinHREF),
            //
            (uint16_t)pio_encode_wait_gpio(true, _pinPCLK),
            (uint16_t)pio_encode_in(pio_pins, 8),
            (uint16_t)pio_encode_wait_gpio(false, _pinPCLK),
            //
            (uint16_t)pio_encode_set(pio_x, 6),
            (uint16_t)pio_encode_wait_gpio(true, _pinPCLK),
            (uint16_t)pio_encode_wait_gpio(false, _pinPCLK),
            (uint16_t)pio_encode_jmp_x_dec(_pioOffset +5),
            //
            (uint16_t)pio_encode_jmp_pin(_pioOffset + 1),
            //
            (uint16_t)pio_encode_set(pio_y, 2),
            (uint16_t)pio_encode_wait_gpio(true, _pinHREF),
            (uint16_t)pio_encode_wait_gpio(false, _pinHREF),
            (uint16_t)pio_encode_jmp_y_dec(_pioOffset +10),
        };
        pio_instructions = pio_qqqvga;
        pio_instructions_length = sizeof(pio_qqqvga) / sizeof(uint16_t);
        wrap_max = 12;
    }

    if (_compiledProgram) delete _compiledProgram;
    _compiledProgram = new pio_program_t();
    _compiledProgram->instructions = pio_instructions;
    _compiledProgram->length = (uint8_t)pio_instructions_length;
    _compiledProgram->origin = -1;

    _pioOffset = pio_add_program(_pio, _compiledProgram);
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_in_pins(&c, _pinD2); 

    sm_config_set_jmp_pin(&c, _pinHREF);

    sm_config_set_in_shift(&c, true, true, 32);
    float pio_div = (float)clock_get_hz(clk_sys) / 144000000.0f;
    sm_config_set_clkdiv(&c, pio_div);

    sm_config_set_wrap(&c, _pioOffset + 0, _pioOffset + wrap_max);
    pio_sm_init(_pio, _sm, _pioOffset, &c);

    for (int pin = _pinD2; pin <= _pinD9; pin++) {
        pio_gpio_init(_pio, pin);
        gpio_set_dir(pin, GPIO_IN);
    }
    pio_gpio_init(_pio, _pinPCLK);
    gpio_set_dir(_pinPCLK, GPIO_IN);
    pio_gpio_init(_pio, _pinHREF);
    gpio_set_dir(_pinHREF, GPIO_IN);
    pio_gpio_init(_pio, _pinVSYNC);
    gpio_set_dir(_pinVSYNC, GPIO_IN);

    
    static uint16_t vsync_instructions[] = {
        (uint16_t)pio_encode_wait_gpio(true, _pinVSYNC),
        (uint16_t)pio_encode_in(pio_x, 32),
        (uint16_t)pio_encode_push(false, false),
    };
    static const pio_program_t vsync_program = {
        .instructions = vsync_instructions,
        .length = sizeof(vsync_instructions) / sizeof(vsync_instructions[0]),
        .origin = -1,
    };
    _vsyncSM = pio_claim_unused_sm(_pio, true); 
    uint vsync_offset = pio_add_program(_pio, &vsync_program);
    pio_sm_config c_vsync = pio_get_default_sm_config();
    sm_config_set_clkdiv(&c_vsync, 1.0f);

    sm_config_set_wrap(&c_vsync, vsync_offset + 0, vsync_offset + 2);
    pio_sm_init(_pio, _vsyncSM, vsync_offset, &c_vsync);
}

bool OV7725::init(ResolutionMode mode) {
    _currentMode = mode;
    if (mode == RES_QVGA_color) {
        _camWidth = 320;
        _camHeight = 240;
        _camColor = 2;
    } else if (mode == RES_QQVGA_color) {
        _camWidth = 160;
        _camHeight = 120;
        _camColor = 2;
    } else if (mode == RES_QQQVGA_color) {
        _camWidth = 80;
        _camHeight = 60;
        _camColor = 2;
    } else if (mode == RES_QVGA_mono) {
        _camWidth = 320;
        _camHeight = 240;
        _camColor = 1;
    } else if (mode == RES_QQVGA_mono) {
        _camWidth = 160;
        _camHeight = 120;
        _camColor = 1;
    } else if (mode == RES_QQQVGA_mono) {
        _camWidth = 80;
        _camHeight = 60;
        _camColor = 1;
    }
    initPIO();
    delay(50);


    writeRegister(0x09, 0x11);
    writeRegister(0x12, 0x80);
    delay(50);
    writeRegister(0x32, 0x00);
    writeRegister(0x2A, 0x00);
    writeRegister(0x11, 0x02);
    writeRegister(0x12, 0x40);
    
    writeRegister(0x42, 0x7F); 
    writeRegister(0x4D, 0x00);
    writeRegister(0x63, 0xF0);
    writeRegister(0x64, 0xFF);
    writeRegister(0x65, 0x20);
    writeRegister(0x66, 0x00);
    writeRegister(0x67, 0x00);
    writeRegister(0x69, 0x5D);
    
    writeRegister(0x13, 0xFE);
    writeRegister(0x0D, 0x81);
    writeRegister(0x0F, 0xC5);
    writeRegister(0x14, 0x11);
    writeRegister(0x22, 0xFF);
    writeRegister(0x23, 0x01);
    writeRegister(0x24, 0x34);
    writeRegister(0x25, 0x3C);
    writeRegister(0x26, 0xA1);
    writeRegister(0x2B, 0x00);
    writeRegister(0x6B, 0xAA);
    
    writeRegister(0x90, 0x0A);
    writeRegister(0x91, 0x01);
    writeRegister(0x92, 0x01);
    writeRegister(0x93, 0x01);
    
    writeRegister(0x94, 0x5F);
    writeRegister(0x95, 0x53);
    writeRegister(0x96, 0x11);
    writeRegister(0x97, 0x1A);
    writeRegister(0x98, 0x3D);
    writeRegister(0x99, 0x5A);
    writeRegister(0x9A, 0x1E);
    
    writeRegister(0x9B, 0x00);
    writeRegister(0x9C, 0x25);
    writeRegister(0xA7, 0x65);
    writeRegister(0xA8, 0x65);
    writeRegister(0x0E, 0x41);
    writeRegister(0x37, 0x00);

    delay(30);
    return true;
}

bool OV7725::restart() {
    writeRegister(0x09, 0x01);
    delay(100);
    pio_sm_set_enabled(_pio, _vsyncSM, true); 
    pio_sm_set_enabled(_pio, _sm, true);
    return true;
}

bool OV7725::stop() {
    pio_sm_set_enabled(_pio, _vsyncSM, false); 
    pio_sm_set_enabled(_pio, _sm, false);
    delay(100); 
    writeRegister(0x09, 0x11);
    return true;
}


void OV7725::writeRegister(uint8_t reg, uint8_t val) {
    _i2c->beginTransmission(0x21);
    _i2c->write(reg);              
    _i2c->write(val);              
    _i2c->endTransmission();
}


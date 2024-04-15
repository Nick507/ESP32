// Driver for TFT SPI 4" display 480x320 with controller ST7796S and touch screen on XPT2046

// WARNING! this display has a bug with diode D1 - need to short it
// https://github.com/Bodmer/TFT_eSPI/issues/849
// http://www.lcdwiki.com/res/MSP4021/4.0inch_SPI_Schematic.pdf


#include <string.h>
#include <rom/ets_sys.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "boards/my_machine_map.h"
#include "lathePanel/display.h"
#include "freertos/task.h"
#include "soc/spi_reg.h"

// Pins configuration - moved to my_machine_map.h
// #define TFT_CS_PIN      GPIO_NUM_5
// #define TFT_RST_PIN     GPIO_NUM_21
// #define TFT_DC_PIN      GPIO_NUM_22
// #define TFT_MOSI_PIN    GPIO_NUM_23
// #define TFT_SCK_PIN     GPIO_NUM_18
// #define TFT_MISO_PIN    GPIO_NUM_19

// #define TOUCH_CS_PIN    GPIO_NUM_4
// #define TOUCH_CLK_PIN   TFT_SCK_PIN   // shared
// #define TOUCH_DIN_PIN   TFT_MOSI_PIN  // shared
// #define TOUCH_DOUT_PIN  TFT_MISO_PIN  // shared

// low level optimization - work directly with SPI via registers
#define TFT_SPI_PORT 3 // VSPI

volatile uint32_t* tft_spi_cmd_reg       = (volatile uint32_t*)(SPI_CMD_REG(TFT_SPI_PORT));
volatile uint32_t* tft_spi_user_reg      = (volatile uint32_t*)(SPI_USER_REG(TFT_SPI_PORT));
volatile uint32_t* tft_spi_mosi_dlen_reg = (volatile uint32_t*)(SPI_MOSI_DLEN_REG(TFT_SPI_PORT));
volatile uint32_t* tft_spi_w0_reg        = (volatile uint32_t*)(SPI_W0_REG(TFT_SPI_PORT));
volatile uint32_t* tft_spi_clock_reg     = (volatile uint32_t*)(SPI_CLOCK_REG(TFT_SPI_PORT));

#define TFT_SPI_WAIT while (*tft_spi_cmd_reg & SPI_USR)
#define TFT_SPI_START *tft_spi_cmd_reg = SPI_USR
#define TFT_SPI_LEN(L) *tft_spi_mosi_dlen_reg = L

#define TFT_WRITE_BITS(D, B) TFT_SPI_LEN(B - 1);  \
                             *tft_spi_w0_reg = D; \
                             TFT_SPI_START;       \
                             TFT_SPI_WAIT;

#define TFT_SPI_WRITE_COORD(C,D)  TFT_WRITE_BITS((uint16_t)((D)<<8 | (D)>>8)<<16 | (uint16_t)((C)<<8 | (C)>>8), 32)

#define TFT_SET_BUS_WRITE_MODE *tft_spi_user_reg = SPI_USR_MOSI // | SPI_WR_BYTE_ORDER
#define TFT_SET_BUS_READ_MODE  *tft_spi_user_reg = SPI_USR_MOSI | SPI_USR_MISO | SPI_DOUTDIN

#define TFT_CS_ENABLE    GPIO.out_w1tc = (1 << TFT_CS_PIN); 
#define TFT_CS_DISABLE   GPIO.out_w1ts = (1 << TFT_CS_PIN); 
#define TOUCH_CS_ENABLE  GPIO.out_w1tc = (1 << TOUCH_CS_PIN);
#define TOUCH_CS_DISABLE GPIO.out_w1ts = (1 << TOUCH_CS_PIN);
#define TFT_DC_CMD       GPIO.out_w1tc = (1 << TFT_DC_PIN);
#define TFT_DC_DATA      GPIO.out_w1ts = (1 << TFT_DC_PIN);

#define TFT_SPI_BEGIN    *tft_spi_clock_reg = 0x80003043; TFT_CS_ENABLE; TFT_SET_BUS_WRITE_MODE;
#define TFT_SPI_END      TFT_SPI_WAIT; TFT_CS_DISABLE; TFT_SET_BUS_READ_MODE;

#define TOUCH_SPI_BEGIN  *tft_spi_clock_reg = 0x000674E7; TOUCH_CS_ENABLE; TFT_SET_BUS_READ_MODE;
#define TOUCH_SPI_END    TFT_SPI_WAIT; TOUCH_CS_DISABLE;

// constants

// variables
//static spi_device_handle_t displayHandle;
//static spi_device_handle_t touchHandle;

void writecommand(uint8_t cmd)
{
    TFT_SPI_BEGIN;
    TFT_DC_CMD;
    TFT_WRITE_BITS(cmd, 8);
    TFT_SPI_END;
}

void writedata(uint8_t d)
{
    TFT_SPI_BEGIN;
    TFT_DC_DATA;
    TFT_WRITE_BITS(d, 8);
    TFT_SPI_END;
}

void hdDisplayInit()
{
    //Configuration for the SPI bus
    spi_bus_config_t buscfg =
    {
        .mosi_io_num = TFT_MOSI_PIN,
        .miso_io_num = TFT_MISO_PIN,
        .sclk_io_num = TFT_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * 2 + 8
    };

    esp_err_t ret = spi_bus_initialize(VSPI_HOST, &buscfg, SPI_DMA_DISABLED);
    assert(ret == ESP_OK);

    gpio_config_t gpioCfg =
    {
        .pin_bit_mask = (1 << TFT_RST_PIN) | (1 << TFT_DC_PIN) | (1 << TFT_CS_PIN) | (1 << TOUCH_CS_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&gpioCfg);

    // TODO: find a way to initialize, now we use default (after reset) settings

    // spi_device_interface_config_t displayDevCfg =
    // {
    //     .command_bits = 0,
    //     .address_bits = 0,
    //     .dummy_bits = 0,
    //     .clock_speed_hz = 80000000,
    //     .duty_cycle_pos = 128,        //50% duty cycle
    //     .mode = 0,
    //     .spics_io_num = TFT_CS_PIN,
    //     .cs_ena_posttrans = 0,
    //     .queue_size = 1,
    //     .flags = SPI_DEVICE_NO_DUMMY, //0,
    //     .pre_cb = 0,
    //     .post_cb = 0
    // };

    // ret = spi_bus_add_device(VSPI_HOST, &displayDevCfg, &displayHandle);
    // assert(ret == ESP_OK);

    // spi_device_interface_config_t touchDevCfg =
    // {
    //     .command_bits = 0,
    //     .address_bits = 0,
    //     .dummy_bits = 0,
    //     .clock_speed_hz = 1000000,
    //     .duty_cycle_pos = 128,        //50% duty cycle
    //     .mode = 0,
    //     .spics_io_num = TOUCH_CS_PIN,
    //     .cs_ena_posttrans = 3,        //Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
    //     .queue_size = 3,
    // };

    // ret = spi_bus_add_device(VSPI_HOST, &touchDevCfg, &touchHandle);
    // assert(ret == ESP_OK);

    TFT_CS_DISABLE;
    TOUCH_CS_DISABLE;

    // reset
    gpio_set_level(TFT_RST_PIN, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(TFT_RST_PIN, 1);
        
    vTaskDelay(120 / portTICK_PERIOD_MS);

	writecommand(0x01); //Software reset
	vTaskDelay(120 / portTICK_PERIOD_MS);

	writecommand(0x11); //Sleep exit                                            
	vTaskDelay(120 / portTICK_PERIOD_MS);

	writecommand(0xF0); //Command Set control                                 
	writedata(0xC3);    //Enable extension command 2 partI
	
	writecommand(0xF0); //Command Set control                                 
	writedata(0x96);    //Enable extension command 2 partII
	
	writecommand(0x36); //Memory Data Access Control MX, MY, RGB mode                                    
	writedata(0xE8);    //X-Mirror, Top-Left to right-Buttom, RGB  
	
    /*#
#define TFT_MAD_MY  0x80
#define TFT_MAD_MX  0x40
#define TFT_MAD_MV  0x20
#define TFT_MAD_ML  0x10
#define TFT_MAD_BGR 0x08
#define TFT_MAD_MH  0x04
#define TFT_MAD_RGB 0x00
*/

	writecommand(0x3A); //Interface Pixel Format                                    
	writedata(0x55);    //Control interface color format set to 16
	
	writecommand(0xB4); //Column inversion 
	writedata(0x01);    //1-dot inversion

	writecommand(0xB6); //Display Function Control
	writedata(0x80);    //Bypass
	writedata(0x02);    //Source Output Scan from S1 to S960, Gate Output scan from G1 to G480, scan cycle=2
	writedata(0x3B);    //LCD Drive Line=8*(59+1)

	writecommand(0xE8); //Display Output Ctrl Adjust
	writedata(0x40);
	writedata(0x8A);	
	writedata(0x00);
	writedata(0x00);
	writedata(0x29);    //Source eqaulizing period time= 22.5 us
	writedata(0x19);    //Timing for "Gate start"=25 (Tclk)
	writedata(0xA5);    //Timing for "Gate End"=37 (Tclk), Gate driver EQ function ON
	writedata(0x33);
	
	writecommand(0xC1); //Power control2                          
	writedata(0x06);    //VAP(GVDD)=3.85+( vcom+vcom offset), VAN(GVCL)=-3.85+( vcom+vcom offset)
	 
	writecommand(0xC2); //Power control 3                                      
	writedata(0xA7);    //Source driving current level=low, Gamma driving current level=High
	 
	writecommand(0xC5); //VCOM Control
	writedata(0x18);    //VCOM=0.9

	vTaskDelay(120 / portTICK_PERIOD_MS);
	
	//ST7796 Gamma Sequence
	writecommand(0xE0); //Gamma"+"                                             
	writedata(0xF0);
	writedata(0x09); 
	writedata(0x0b);
	writedata(0x06); 
	writedata(0x04);
	writedata(0x15); 
	writedata(0x2F);
	writedata(0x54); 
	writedata(0x42);
	writedata(0x3C); 
	writedata(0x17);
	writedata(0x14);
	writedata(0x18); 
	writedata(0x1B); 
	 
	writecommand(0xE1); //Gamma"-"                                             
	writedata(0xE0);
	writedata(0x09); 
	writedata(0x0B);
	writedata(0x06); 
	writedata(0x04);
	writedata(0x03); 
	writedata(0x2B);
	writedata(0x43); 
	writedata(0x42);
	writedata(0x3B); 
	writedata(0x16);
	writedata(0x14);
	writedata(0x17); 
	writedata(0x1B);

    vTaskDelay(120 / portTICK_PERIOD_MS);
	
	writecommand(0xF0); //Command Set control                                 
	writedata(0x3C);    //Disable extension command 2 partI

	writecommand(0xF0); //Command Set control                                 
	writedata(0x69);    //Disable extension command 2 partII

    vTaskDelay(120 / portTICK_PERIOD_MS);

	writecommand(0x29); //Display on                                          	

    displayClear(TFT_BLACK);

    displaySetTextColor(TFT_WHITE, TFT_BLACK);
}

void displaySetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    TFT_DC_CMD;
    TFT_WRITE_BITS(0x2A, 8);
    TFT_DC_DATA;
    TFT_SPI_WRITE_COORD(x1, x2);
    TFT_DC_CMD;
    TFT_WRITE_BITS(0x2B, 8);
    TFT_DC_DATA;
    TFT_SPI_WRITE_COORD(y1, y2);
    TFT_DC_CMD;
    TFT_WRITE_BITS(0x2C, 8);
    TFT_DC_DATA;
}

void displayRect(int16_t x, int16_t y, uint16_t sx, uint16_t sy, uint16_t color)
{
    TFT_SPI_BEGIN;
    displaySetWindow(x, y, x + sx - 1, y + sy - 1);

    volatile uint32_t* w0 = tft_spi_w0_reg;
    uint32_t color32 = color << 8 | color >> 8; // little endian to big
    color32 = color32 | (color32 << 16);
    uint32_t i = 0;
    uint32_t len = sx * sy;
    assert(len);
    uint32_t rem = len & 0x1F;
    len = len - rem;

    // Start with partial buffer pixels
    if (rem)
    {
        TFT_SPI_WAIT;
        for (i = 0; i < rem; i += 2) *w0++ = color32;
        TFT_SPI_LEN((rem << 4) - 1);
        TFT_SPI_START;
        if (!len)
        {
            TFT_SPI_END;
            return;
        }
        i = i >> 1; 
        while(i++ < 16) *w0++ = color32;
    }

    TFT_SPI_WAIT;
    if(!rem) while (i++ < 16) *w0++ = color32;
    TFT_SPI_LEN(511);

    // End with full buffer to maximise useful time for downstream code
    while(len)
    {
        TFT_SPI_WAIT;
        TFT_SPI_START;
        len -= 32;
    }

    TFT_SPI_END;
}

volatile uint32_t* window_w0;
static uint32_t windowLen = 0, windowCurPos = 0;

void hdDisplayStartWindow(int16_t x, int16_t y, uint16_t sx, uint16_t sy)
{
    TFT_SPI_BEGIN;
    displaySetWindow(x, y, x + sx - 1, y + sy - 1);
    window_w0 = tft_spi_w0_reg;
    windowLen = sx * sy;
    windowCurPos = 0;

    assert(windowLen);
}

void hdDisplayPushPixelToWindow(uint16_t color)
{
    assert(windowLen);

    uint32_t colorBE = (uint16_t)(color >> 8 | color << 8); // little endian to big endian

    TFT_SPI_WAIT;

    if(windowCurPos & 1) *window_w0++ |= (colorBE << 16); else *window_w0 = colorBE;

    if((++windowCurPos & 0x1F) == 0)
    {
        TFT_SPI_LEN(511); // 32 pixels
        TFT_SPI_START;
        window_w0 = tft_spi_w0_reg;
    }
    
    if(windowCurPos >= windowLen)
    {
        if(windowCurPos & 0x1F)
        {
            TFT_SPI_WAIT;
            TFT_SPI_LEN(((windowCurPos & 0x1F) << 4) - 1);
            TFT_SPI_START;
        }
        windowLen = 0;
        TFT_SPI_END;
    }
}

//=====================================================================================
//
//                             T O U C H  S C R E E N
//
//=====================================================================================

#define CMP_SWAP(a, b) if(a > b) {uint16_t tmp = a; a = b; b = tmp;}

uint16_t xpt2046ReadMedian(uint8_t cmd)
{
    uint16_t t[5];
   
    for(uint8_t i = 0; i < 5; ++i)
    {
        TFT_WRITE_BITS(cmd, 24);    
        t[i] = ((*tft_spi_w0_reg & 0x7F00) >> 3) | ((*tft_spi_w0_reg & 0xF80000) >> 19);
    }

    CMP_SWAP(t[0], t[1]);
    CMP_SWAP(t[2], t[3]);
    CMP_SWAP(t[0], t[2]);
    CMP_SWAP(t[1], t[4]);
    CMP_SWAP(t[0], t[1]);
    CMP_SWAP(t[2], t[3]);
    CMP_SWAP(t[1], t[2]);
    CMP_SWAP(t[3], t[4]);
    CMP_SWAP(t[2], t[3]);

    return t[2];
}

bool displayReadTouch(int16_t * resX, int16_t * resY)
{
    static int16_t pressFilter = 0, minX, maxX, minY, maxY, lastX, lastY;
    static bool state = false;

    TOUCH_SPI_BEGIN;
    uint16_t tz1 = xpt2046ReadMedian(0xB1);
    uint16_t tz2 = xpt2046ReadMedian(0xC1);
    TOUCH_SPI_END;

    if(tz2 - tz1 > 3900)
    {
        pressFilter = 0;
        state = false;
        return state;
    }
    
    TOUCH_SPI_BEGIN;
    uint16_t tx = xpt2046ReadMedian(0x91); 
    uint16_t ty = xpt2046ReadMedian(0xD1);
    TOUCH_SPI_END;

    int16_t x = (tx - 300) * DISPLAY_WIDTH / 3600;
    int16_t y = (ty - 200) * DISPLAY_HEIGHT / 3600;

    if(x < 0 || x > DISPLAY_WIDTH || y < 0 || y > DISPLAY_HEIGHT)
    {
        pressFilter = 0;
        state = false;
        return state;
    }

    if(pressFilter++ == 0)
    {
        minX = x;
        maxX = x;
        minY = y;
        maxY = y;
    }
    else
    {
        if(x < minX) minX = x;
        if(x > maxX) maxX = x;
        if(y < minY) minY = y;
        if(y > maxY) maxY = y;
    }

    if(pressFilter < 5)
    {
        if(state)
        {
            *resX = lastX;
            *resY = lastY;
        }
        return state;
    } 

    pressFilter = 0;

    if(maxX - minX > 10 || maxY - minY > 10)
    {
        state = false;
        return false;
    }

    lastX = minX + (maxX - minX) / 2;
    lastY = minY + (maxY - minY) / 2;
    *resX = lastX;
    *resY = lastY;
    state = true;

    return state;
}

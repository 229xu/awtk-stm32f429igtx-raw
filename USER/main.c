#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "string.h"
#include "sdram.h"
#include "malloc.h"
#include "w25qxx.h"
#include "ff.h"
#include "exfuns.h"
#include "string.h"
#include "sdio_sdcard.h"
#include "fontupd.h"
#include "text.h"
#include "ltdc.h"
#include "base/g2d.h"
#include "touch.h"

/************************************************
 ALIENTEK ������STM32F429������ʵ��42
 ������ʾʵ��-HAL�⺯����
 ����֧�֣�www.openedv.com
 �Ա����̣�http://eboard.taobao.com	
 ��ע΢�Ź���ƽ̨΢�źţ�"����ԭ��"����ѻ�ȡSTM32���ϡ�
 ������������ӿƼ����޹�˾	
 ���ߣ�����ԭ�� @ALIENTEK
************************************************/
		
#include "base/lcd.h"
#include "base/mem.h"
#include "lcd/lcd_mem_rgb565.h"

extern int gui_app_start(int lcd_w, int lcd_h);

void LTDC_IRQHandler(void)
{
  HAL_LTDC_IRQHandler(&LTDC_Handler);
}

static uint8_t* s_next_fb = NULL;
static uint8_t* s_online_fb = NULL;
static uint8_t* s_framebuffers[3];

ret_t lcd_stmf429_begin_frame(lcd_t* lcd, rect_t* dirty_rect) {
	uint32_t i = 0;
	lcd_mem_t* mem = (lcd_mem_t*)lcd;
	
	mem->online_fb = NULL;
	mem->next_fb = NULL;
	mem->offline_fb = NULL;
	for(i = 0; i < ARRAY_SIZE(s_framebuffers); i++) {
		uint8_t* iter = s_framebuffers[i];
		if(iter != s_online_fb && iter != s_next_fb) {
			mem->offline_fb = iter;
			break;
		}
	}
		
	return RET_OK;
}

ret_t lcd_stmf429_swap(lcd_t* lcd) {
  lcd_mem_t* mem = (lcd_mem_t*)lcd;
       
  s_next_fb = mem->offline_fb;

  return RET_OK;
}

void HAL_LTDC_LineEvenCallback(LTDC_HandleTypeDef *LTDC_Handler)
{
  if(s_next_fb != NULL) {
    HAL_LTDC_SetAddress(LTDC_Handler,(uint32_t)s_next_fb,0);
    __HAL_LTDC_RELOAD_CONFIG(LTDC_Handler);
		s_online_fb = s_next_fb;
    s_next_fb = NULL;
  }
	
  HAL_LTDC_ProgramLineEvent(LTDC_Handler,0);
}

#define FB_ADDR (uint8_t*)0XC0000000

lcd_t* stm32f429_create_lcd(wh_t w, wh_t h) {
	lcd_t* lcd = NULL;
	uint32_t size = w * h * lcdltdc.pixsize;
	s_framebuffers[0] = FB_ADDR;
	s_framebuffers[1] = FB_ADDR+size;
	s_framebuffers[2] = FB_ADDR+2*size;
	
  lcd = lcd_mem_rgb565_create_three_fb(w, h, s_framebuffers[0], s_framebuffers[1], s_framebuffers[2]);

  lcd->swap = lcd_stmf429_swap;
	lcd->begin_frame = lcd_stmf429_begin_frame;
	
	HAL_NVIC_SetPriority(LTDC_IRQn,1,1);    
  HAL_NVIC_EnableIRQ(LTDC_IRQn);
  HAL_LTDC_ProgramLineEvent(&LTDC_Handler,0);
	
  return lcd;
}

int main(void)
{	
	HAL_Init();										 //��ʼ��HAL��	 
	Stm32_Clock_Init(360,25,2,8);	 //����ʱ��,180Mhz
	delay_init(180);							 //��ʼ����ʱ����
	uart_init(115200);						 //��ʼ��USART
	LED_Init();										 //��ʼ��LED 
	KEY_Init();										 //��ʼ������
	SDRAM_Init();									 //SDRAM��ʼ��
	LCD_Init();										 //LCD��ʼ��
	W25QXX_Init();						     //��ʼ��W25Q256

  tp_dev.init();
		
	return gui_app_start(lcdltdc.pwidth, lcdltdc.pheight);
}

/*
 * main.c
 * 
 * STM32 DMX512
 * Copyright (C) 2011 Erik Van Hamme, all rights reserved
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* This is the application main file. */

/*
 * -------------------- System Includes ----------------------------------------
 */

#include <stdint.h>

/*
 * -------------------- Local Includes -----------------------------------------
 */

#include "protocol.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"

/*
 * -------------------- Defines ------------------------------------------------
 */

/**
 * @brief Toggles the state of the run led.
 * 
 * The run led indicates that the main loop of the application is still running.
 * 
 * This method is implemented as a register call because it is highly 
 * performance critical.
 * 
 * This method is implemented as a define, to ensure that the code is inlined by
 * the compiler. This is done for performance reasons.
 * 
 * GCC treats the inline keyword as an optimization hint. The compiler may still
 * ignore the keyword and not inline the function. This is avoided by using a
 * define.
 * 
 * @param None.
 * @return None.
 */
#define toggle_runled() GPIOC->ODR ^= (1 << 12)

/**
 * @brief Toggles the state of the DE and (not)RE lines for USART1.
 * 
 * This method is implemented as a register call because it is highly 
 * performance critical.
 * 
 * This method is implemented as a define, to ensure that the code is inlined by
 * the compiler. This is done for performance reasons.
 * 
 * GCC treats the inline keyword as an optimization hint. The compiler may still
 * ignore the keyword and not inline the function. This is avoided by using a
 * define.
 * 
 * @param None.
 * @return None.
 */
#define toggle_usart1_de_re() GPIOA->ODR ^= 0x0030

/**
 * @brief Brings the PA9 pin under the control of USART1.
 * 
 * The DMX protocol requires a degree of control over the line that cannot be
 * achieved by use of a UART alone. The protocol requires that the line is 
 * pulled low for a given amount of microseconds. Therefore, the PA9 pin is
 * switched between being controlled by the UART and as GPIO.
 * 
 * This method is implemented as a register call because it is highly 
 * performance critical.
 * 
 * This method is implemented as a define, to ensure that the code is inlined by
 * the compiler. This is done for performance reasons.
 * 
 * GCC treats the inline keyword as an optimization hint. The compiler may still
 * ignore the keyword and not inline the function. This is avoided by using a
 * define.
 * 
 * @param None.
 * @return None.
 */
#define set_pa9_uart() GPIOA->CRH = 0x000000B0

/**
 * @brief Brings the PA9 pin under the control of GPIO port A.
 * 
 * The DMX protocol requires a degree of control over the line that cannot be
 * achieved by use of a UART alone. The protocol requires that the line is 
 * pulled low for a given amount of microseconds. Therefore, the PA9 pin is
 * switched between being controlled by the UART and as GPIO.
 * 
 * This method is implemented as a register call because it is highly 
 * performance critical.
 * 
 * This method is implemented as a define, to ensure that the code is inlined by
 * the compiler. This is done for performance reasons.
 * 
 * GCC treats the inline keyword as an optimization hint. The compiler may still
 * ignore the keyword and not inline the function. This is avoided by using a
 * define.
 * 
 * @param None.
 * @return None.
 */
#define set_pa9_gpio() GPIOA->CRH = 0x00000030

/**
 * @brief Blocks for X microseconds by using TIM2.
 * 
 * The wait is achieved by setting up the timer TIM2, followed by enabling
 * the timer, and waiting until the timer period has lapsed. After the timer 
 * period has lapsed, the timer is disabled again.
 * 
 * This method is implemented as a register call because it is highly 
 * performance critical.
 * 
 * This method is implemented as a define, to ensure that the code is inlined by
 * the compiler. This is done for performance reasons.
 * 
 * GCC treats the inline keyword as an optimization hint. The compiler may still
 * ignore the keyword and not inline the function. This is avoided by using a
 * define.
 * 
 * @param X The amount of microseconds to block.
 * @return None.
 */
#define tim2_wait_usec(X) \
        TIM2->PSC = 71; \
        TIM2->ARR = X; \
        TIM2->CNT = 0; \
        TIM2->CR1 |= TIM_CR1_CEN; \
        while (TIM2->CNT != TIM2->ARR) { } \
        TIM2->CR1 &= ~TIM_CR1_CEN

/**
 * @brief Transmits the given byte over USART1 and waits until the transmit is 
 * complete.
 * 
 * This implements a blocking send for USART1.
 * 
 * This method is implemented as a register call because it is highly 
 * performance critical.
 * 
 * This method is implemented as a define, to ensure that the code is inlined by
 * the compiler. This is done for performance reasons.
 * 
 * GCC treats the inline keyword as an optimization hint. The compiler may still
 * ignore the keyword and not inline the function. This is avoided by using a
 * define.
 * 
 * @param X The byte to send out on USART1.
 * @return None.
 */
#define usart1_tx_and_wait(X) \
        USART1->DR = X; \
        while(!(USART1->SR & (1 << 6))) { }

/**
 * @brief Transmits the given byte over USART2 and waits until the transmit is 
 * complete.
 * 
 * This implements a blocking send for USART2.
 * 
 * This method is implemented as a register call because it is highly 
 * performance critical.
 * 
 * This method is implemented as a define, to ensure that the code is inlined by
 * the compiler. This is done for performance reasons.
 * 
 * GCC treats the inline keyword as an optimization hint. The compiler may still
 * ignore the keyword and not inline the function. This is avoided by using a
 * define.
 * 
 * @param X The byte to send out on USART2.
 * @return None.
 */
#define usart2_tx_and_wait(X) \
        USART2->DR = X; \
        while(!(USART2->SR & (1 << 6))) { }

/*
 * -------------------- Type definitions ---------------------------------------
 */

/*
 * -------------------- Global variables ---------------------------------------
 */

/**
 * @brief The DMX database.
 * 
 * This database holds the byte value of all the channels in the DMX512 
 * protocol.
 * 
 * Please note that there are 513 places in this buffer. The byte at position 0
 * is the start code, which is always 0.
 * 
 * See the DMX 512 spec for more information.
 */
uint8_t dmx_data[513];

/**
 * @brief Update flag. This flag indicates that the DMX devices in the
 * field must be updated.
 */
volatile uint8_t update;

/**
 * @brief Divider counter for the 25Hz signal.
 */
volatile uint8_t divider;

/*
 * -------------------- Prototypes ---------------------------------------------
 */

/*
 * -------------------- Method implementations ---------------------------------
 */

void init() {

    /*
     * Step 1: Enable the various hardware devices needed for the application.
     */

    // enable GPIOA
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // enable GPIOC
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    // enable USART1
    RCC_APB2PeriphClockCmd(RCC_APB2ENR_USART1EN, ENABLE);
    
    // enable USART2
    RCC_APB1PeriphClockCmd(RCC_APB1ENR_USART2EN, ENABLE);

    // enable TIM2
    RCC_APB1PeriphClockCmd(RCC_APB1ENR_TIM2EN, ENABLE);

    /*
     * Step 2: Set the pin configurations for all required pins.
     */

    // configure USART1 pins: TX in alternate function push pull
    GPIO_InitTypeDef pinCfg;
    pinCfg.GPIO_Mode = GPIO_Mode_AF_PP;
    pinCfg.GPIO_Pin = GPIO_Pin_9;
    pinCfg.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &pinCfg);

    // configure runled pin
    pinCfg.GPIO_Mode = GPIO_Mode_Out_PP;
    pinCfg.GPIO_Pin = GPIO_Pin_12;
    GPIO_Init(GPIOC, &pinCfg);

    // configure PA4 pin for output
    pinCfg.GPIO_Pin = GPIO_Pin_4;
    GPIO_Init(GPIOA, &pinCfg);

    // configure PA5 pin for output
    pinCfg.GPIO_Pin = GPIO_Pin_5;
    GPIO_Init(GPIOA, &pinCfg);

    // configure USART2 pins: PA0-WKUP/USART2_CTS
    pinCfg.GPIO_Mode = GPIO_Mode_AF_PP;
    pinCfg.GPIO_Pin = GPIO_Pin_0;
    GPIO_Init(GPIOA, &pinCfg);

    // configure USART2 pins: PA1/USART2_RTS
    pinCfg.GPIO_Pin = GPIO_Pin_1;
    GPIO_Init(GPIOA, &pinCfg);

    // configure USART2 pins: PA2/USART2_TX
    pinCfg.GPIO_Pin = GPIO_Pin_2;
    GPIO_Init(GPIOA, &pinCfg);

    // configure USART2 pins: PA3/USART2_RX
    pinCfg.GPIO_Pin = GPIO_Pin_1;
    GPIO_Init(GPIOA, &pinCfg);

    /*
     * Step 3: Configure USART2 for communication to the PC, and
     *         USART1 for communication on the DMX line.
     */

    // configure USART1
    USART_InitTypeDef usartCfg;
    usartCfg.USART_BaudRate = 250000;
    usartCfg.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usartCfg.USART_Mode = USART_Mode_Tx;
    usartCfg.USART_Parity = USART_Parity_No;
    usartCfg.USART_StopBits = USART_StopBits_2;
    usartCfg.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART1, &usartCfg);

    // enable USART1
    USART_Cmd(USART1, ENABLE);

    // configure USART2
    // Please note that this USART is configured to use RTS/CTS hardware
    // flow control.
    // The use of flow control implies that you must close the solder
    // bridges on the PCB of the STM32-P103 development board as seen in the
    // schematic.
    usartCfg.USART_BaudRate = 38400;
    usartCfg.USART_HardwareFlowControl = USART_HardwareFlowControl_RTS_CTS;
    usartCfg.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    usartCfg.USART_Parity = USART_Parity_No;
    usartCfg.USART_StopBits = USART_StopBits_1;
    usartCfg.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART2, &usartCfg);

    // enable USART2
    USART_Cmd(USART2, ENABLE);

    /*
     * Step 4: Interrupt configuration. 
     */

    // configure system tick timer for 1 millisecond system tick
    SysTick_Config(SystemCoreClock / 1000);
}

/**
 * @brief System tick interrupt handler.
 */
void SysTick_Handler() {
    divider++;
    if (divider == 40) {
        divider = 0;
        update = 1;
    }
    protocol_tick();
}

int main() {

    // initialize the hardware
    init();

    // application main loop
    while (1) {

        /*
         * Section 1: deal with the communications.
         */

        // check if there are bytes in the rx register
        if (USART2->SR & USART_FLAG_RXNE) {

            // read the byte, this resets the RXNE flag
            uint8_t byte = USART2->DR;

            // send the byte to the protocol
            protocol_receive(byte);
        }
        
        /*
         * Section 2: Update the DMX value of the connected devices if needed.
         * The update is triggered at 25 Hz by the @ref SysTick_Handler.
         */

        // if the periodic update of the DMX devices has been triggered,
        // run the update
        if (update) {
            
            // reset the update flag first
            update = 0;

            // toggle the run led to indicate that the main loop is alive
            toggle_runled();

            /*
             * BEGIN Time critical code
             */

            // bring the TX pin under GPIO control
            // this has the added effect that the pin is low
            set_pa9_gpio();

            // set the DE and (not)RE lines high
            toggle_usart1_de_re();

            // wait 88 microseconds for the DMX break
            tim2_wait_usec(88);

            // bring the TX pin under UART control
            // this has the added effect that the pin is high
            set_pa9_uart();

            // wait 8 microseconds for the MAB (Mark After Break)
            tim2_wait_usec(8);

            // transmit the DMX data
            for (uint16_t i = 0; i < 513; i++) {

                // transmit the byte with a blocking send
                usart1_tx_and_wait(dmx_data[i]);

            }

            // wait one additional character time before releasing the line
            tim2_wait_usec(44);

            // release the line
            toggle_usart1_de_re();

            // wait at least 250 microseconds in between the frames
            tim2_wait_usec(250);

            /*
             * END Time critical code
             */
        }
    }
    return 0;
}

void app_set_rgb(uint16_t addr, uint8_t r, uint8_t g, uint8_t b) {
    
    // save the colors in the dmx data
    dmx_data[addr] = r;
    dmx_data[addr + 1] = g;
    dmx_data[addr + 2] = b;
    
    // tell the protocol that we are done
    protocol_set_rgb_done(addr);
}

void app_get_rgb(uint16_t addr) {
    
    // get the dmx data
    uint8_t red = dmx_data[addr];
    uint8_t green = dmx_data[addr + 1];
    uint8_t blue = dmx_data[addr + 2];
    
    // send the data to the protocol
    protocol_get_rgb_done(addr, red, green, blue);
}

void app_set(uint16_t from, uint16_t to, uint8_t c) {
    
    // save the dmx data
    for (uint16_t i = from; i <= to; i++) {
        dmx_data[i] = c;
    }
    
    // tell the protocol that we are done
    protocol_set_done(from, to);
}

void app_get(uint16_t addr) {
    
    // get the dmx data
    uint8_t col = dmx_data[addr];
    
    // send the data to the protocol
    protocol_get_done(addr, col);
}

void app_send_byte(uint8_t byte) {
    // send the byte out over usart2 in a blocking way
    usart2_tx_and_wait(byte);
}

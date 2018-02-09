/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*****************************************************
 Getting Started Guide for Zybo


 This demo displays the status of the switches on the
 LEDs and prints a message to the serial communication
 when a button is pressed.

 Terminal Settings:
  -Baud: 115200
  -Data bits: 8
  -Parity: no
  -Stop bits: 1

 1/6/14: Created by MarshallW
 ****************************************************/

#include <stdio.h>
#include <stdbool.h>
#include "platform.h"
#include <xgpio.h>
#include <xtime_l.h>
#include "xparameters.h"
#include "sleep.h"
#include <stdlib.h>
#include <math.h>

#define BUTTONS_CHANNEL 1
#define SWITCHES_CHANNEL 2
#define LEDS_CHANNEL 1
#define APROX_CYCLES_PER_SECOND 325003200LLU //actually 2 times more
#define TIMER 100

int getCurrPressedButton(XGpio *input);
void blinkingLeds(XGpio *output, const int winning_led_data, const int microseconds);
int areSwitchesUp(XGpio *input, const unsigned channel);
int areSwitchesDown(XGpio *input, const unsigned channel);
void generateActions(int *button_actions, const int n);
int generateRandomBetween(const int min, const int max);
void changeLedsData(int pressed_button, int button_actions[], int *current_led_data);
int isButton(const int current_pressed_button);
int sameButtonPressed(int prev_pressed_button, int currentPressedButton);
int isBombDiffused(const int winning_led_data, const int current_led_data);
int generateCurrentLedData(const int winning_led_data, int button_actions[]);

int main() {
	XGpio input, output;
	XTime xTimeCurr;
	XTime game_ticks = TIMER * APROX_CYCLES_PER_SECOND;
	XTime remaining_ticks;
	XTime xTimePrev;
	int start_flag = 0;
	int data_initialised_flag = 0;
	int button_actions[4];
	int winning_led_data;
	int current_led_data;

	int curr_pressed_button; // = -1;
	int prev_pressed_button; // = -1;

	XGpio_Initialize(&input, XPAR_AXI_GPIO_0_DEVICE_ID); //initialize input XGpio variable
	XGpio_Initialize(&output, XPAR_AXI_GPIO_1_DEVICE_ID); //initialize output XGpio variable
	XGpio_SetDataDirection(&input, BUTTONS_CHANNEL, 0xF);   //set first channel tristate buffer to input
	XGpio_SetDataDirection(&input, SWITCHES_CHANNEL, 0xF);   //set second channel tristate buffer to input
	XGpio_SetDataDirection(&output, LEDS_CHANNEL, 0x0);  //set first channel tristate buffer to output
	init_platform();

	XTime_GetTime(&xTimeCurr);
	srand((int)xTimeCurr);

//	generateActions(button_actions);
//	current_led_data = generateCurrentLedData(winning_led_data, button_actions);

	while(1){

		if((areSwitchesDown(&input, SWITCHES_CHANNEL) && start_flag == 1) || data_initialised_flag == 0) {
			start_flag = 0;
			remaining_ticks = game_ticks;
			winning_led_data = generateRandomBetween(1, 15); // exemple  0b0101;
			//xil_printf("winning_led_data = %d\n", winning_led_data);
			curr_pressed_button = -1;
			prev_pressed_button = -1;
			generateActions(button_actions, sizeof(button_actions) / sizeof(button_actions[0]));
			current_led_data = generateCurrentLedData(winning_led_data, button_actions);
			data_initialised_flag = 1;
		}

		if(remaining_ticks > game_ticks) {
			xil_printf("BOOOOOOOM\n");
			blinkingLeds(&output, 0b1111, 100000);
			continue;
		}

		if(isBombDiffused(winning_led_data, current_led_data)){
			xil_printf("Succesfully Diffused\n");
			blinkingLeds(&output, winning_led_data, 500000);
			continue;
		}

		XGpio_DiscreteWrite(&output, LEDS_CHANNEL, current_led_data); //set leds value

		if(!areSwitchesUp(&input, SWITCHES_CHANNEL)) { //checks switches
			blinkingLeds(&output, winning_led_data, 500000);
		} else {
			if(start_flag == 0){
				XTime_GetTime(&xTimeCurr);
				xTimePrev = xTimeCurr;
				start_flag = 1;
			}

			prev_pressed_button = curr_pressed_button;
			curr_pressed_button = getCurrPressedButton(&input);

			//check if current pressed button is realy a button and if its not the same as previous
			if(isButton(curr_pressed_button) && !sameButtonPressed(prev_pressed_button, curr_pressed_button)) {
				changeLedsData(curr_pressed_button, button_actions, &current_led_data);
			}
		}

		if(start_flag == 1) {
			XTime_GetTime(&xTimeCurr);
			remaining_ticks -= xTimeCurr - xTimePrev;
			xTimePrev = xTimeCurr;
		}

	}
	cleanup_platform();
	return 0;
}

int generateCurrentLedData(const int winning_led_data, int button_actions[]) {
	const int changes_number = 5;
	int current_led_data = winning_led_data;
	while(current_led_data == winning_led_data)
		for(int i = 0; i < changes_number; ++i) {
			int button_action = rand() % 4;
			xil_printf("button_action%d = %d\n", i, button_action);
			changeLedsData(button_action, button_actions, &current_led_data);
		}
	//xil_printf("current_led_data = %d\n", current_led_data);
	return current_led_data;
}

void blinkingLeds(XGpio *output, const int winning_led_data, const int microseconds) {
	XGpio_DiscreteWrite(output, 1, winning_led_data); //write switch data to the LEDs
	usleep(microseconds);
	XGpio_DiscreteWrite(output, 1, 0b0000);     //switch leds off
	usleep(microseconds);
}

int sameButtonPressed(int prev_pressed_button, int currentPressedButton) {
	int result = (prev_pressed_button == currentPressedButton) ? 1 : 0;
	if(result == 1)
		xil_printf("same button pressed\n");
	return result;
}

int isBombDiffused(const int winning_led_data, const int current_led_data) {
	return winning_led_data == current_led_data ? 1 : 0;
}

int isButton(const int current_pressed_button) {
	return (current_pressed_button>=0 && current_pressed_button<=3) ? 1 : 0; //range of button values;
}

int areSwitchesUp(XGpio *input, const unsigned channel) {
	return (XGpio_DiscreteRead(input, channel) == 0b1111);
}

int areSwitchesDown(XGpio *input, const unsigned channel) {
	return (XGpio_DiscreteRead(input, channel) == 0b0000);
}

void generateActions(int *button_actions, const int n) {
	for(int i = 0; i < n; ++i) {
		button_actions[i] = generateRandomBetween(0b0001, 0b1110);
	}
}

int generateRandomBetween(const int min, const int max) {
	return rand() % (max - min + 1) + min;
}

int getCurrPressedButton(XGpio *input) {

	int curr_buttons_data = XGpio_DiscreteRead(input, BUTTONS_CHANNEL); //get button data

	if(curr_buttons_data == 0b0000) {
		return -2;//do nothing
	}else if(curr_buttons_data == 0b0001) {
		xil_printf("button 0 pressed\n");
		return 0;
	}else if(curr_buttons_data == 0b0010) {
		xil_printf("button 1 pressed\n");
		return 1;
	}else if(curr_buttons_data == 0b0100) {
		xil_printf("button 2 pressed\n");
		return 2;
	}else if(curr_buttons_data == 0b1000) {
		xil_printf("button 3 pressed\n");
		return 3;
	}else {
		xil_printf("multiple buttons pressed\n");
		return -3;
	}
}

void changeLedsData(int pressed_button, int button_actions[], int *current_led_data) {
	*current_led_data = *current_led_data ^ button_actions[pressed_button];
	usleep(100000);
}

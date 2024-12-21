/*
 * uart.c
 *
 *  Created on: Dec 13, 2024
 *      Author: thinkie
 */


#include "timer.h"
#include "uart.h"
#include "stdio.h"
#include "string.h"
#include "main.h"

#define MAX_BUFFER_SIZE 30
unsigned char temp = 0;
uint8_t buffer[MAX_BUFFER_SIZE] = "!ADC=";
uint8_t index_buffer = 0;
uint8_t flagSendData = 0;

typedef enum {
    INIT_RECEIVE = 0,
    WAIT_HEADER,
    RECEIVE_DATA,
    RECEIVE_DATA_RST_2,
    RECEIVE_DATA_RST_3,
    END_DATA_RST,
    RECEIVE_DATA_OK_2,
    END_DATA_OK
} StatusReceive;

StatusReceive statusReceive = WAIT_HEADER;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART2) {
		HAL_UART_Receive_IT(&huart2, &temp, 1);
		HAL_UART_Transmit(&huart2, &temp, 1, 200);
		command_parser_fsm();
	}
}

uint16_t ADC_value = 0;

char str[4] = "0000";
enum comState {
	WAIT_COMMAND, SEND_DATA, RESEND_DATA
};
enum comState statusOfCom = WAIT_COMMAND;

void uart_communiation_fsm(void) {
	switch (statusOfCom) {
	case WAIT_COMMAND:
		if (flagSendData) {
			statusOfCom = SEND_DATA;
			//Reading ADC
			HAL_ADC_Start(&hadc1);
			HAL_ADC_PollForConversion(&hadc1, 100);
			ADC_value = HAL_ADC_GetValue(&hadc1);
			// Convert to string and print
			sprintf(str, "%hu", ADC_value);
		}
		break;
	case SEND_DATA:
		HAL_UART_Transmit(&huart2, buffer, 5, 100);
		HAL_UART_Transmit(&huart2, (uint8_t*) str, strlen(str), 100);
		HAL_UART_Transmit(&huart2, (uint8_t*) "#\r\n", 3, 100);
		if (flagSendData) {
			statusOfCom = RESEND_DATA;
			setTimer1(1000);
		}
		break;
	case RESEND_DATA:
		if (isTIM2_flag1()) {
			HAL_UART_Transmit(&huart2, buffer, 5, 100);
			HAL_UART_Transmit(&huart2, (uint8_t*) str, strlen(str), 100);
			HAL_UART_Transmit(&huart2, (uint8_t*) "#\r\n", 3, 100);
			setTimer1(1000);
		}
		if (!flagSendData) {
			statusOfCom = WAIT_COMMAND;
			setTimer1(100);
		}
		break;
	default:
		statusOfCom = WAIT_COMMAND;
	}
}

void command_parser_fsm(void) {
	switch (statusReceive) {
	case WAIT_HEADER:
		if (temp == '!') {
			statusReceive = RECEIVE_DATA;
		}
		break;
	case RECEIVE_DATA:
		if (temp == 'R')
			statusReceive = RECEIVE_DATA_RST_2;
		else if (temp == 'O')
			statusReceive = RECEIVE_DATA_OK_2;
		else
			statusReceive = WAIT_HEADER;
		break;
	case RECEIVE_DATA_RST_2:
		if (temp == 'S')
			statusReceive = RECEIVE_DATA_RST_3;
		else
			statusReceive = WAIT_HEADER;
		break;
	case RECEIVE_DATA_RST_3:
		if (temp == 'T')
			statusReceive = END_DATA_RST;
		else
			statusReceive = WAIT_HEADER;
		break;
	case RECEIVE_DATA_OK_2:
		if (temp == 'K')
			statusReceive = END_DATA_OK;
		else
			statusReceive = WAIT_HEADER;
		break;
	case END_DATA_RST:
		if (temp == '#') {
			statusReceive = WAIT_HEADER;
			flagSendData = 1;
		} else
			statusReceive = WAIT_HEADER;
		break;
	case END_DATA_OK:
		if (temp == '#') {
			statusReceive = WAIT_HEADER;
			flagSendData = 0;
		} else
			statusReceive = WAIT_HEADER;
		break;
	default:
		statusReceive = INIT_RECEIVE;
		break;
	}
}

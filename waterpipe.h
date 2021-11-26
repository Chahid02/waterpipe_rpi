/*!
**************************************************************
* Copyright (c) Lukasz Piatek. All rights reserved.
* @file    waterpipe.h
* @author  Lukasz Piatek
* @version V1.0
* @date    2021-09-28
* @brief   Default main function.
**************************************************************
*/

#ifndef WATERPIPE_H_
#define WATERPIPE_H_


/*=========================================================*/
/*== PRIVATE MACROS =======================================*/
/*=========================================================*/

/*!< SET LOG INFO */
#define LOG_ERROR(MSG)                                                   \
    {                                                                    \
        printf("ErrorLog: %s [%s:%d]\n\n", MSG, __FUNCTION__, __LINE__); \
    }

/*!<  DISABLE GoTo */
#pragma GCC poison goto

/*!< SET USER __NOP() MACRO */
#define __NOP() __asm("NOP");

/*!< SET DEBUG MODE ON/OFF */
#define DEBUG 1

/*=========================================================*/
/*== DEBUG DEFINITION =====================================*/
/*=========================================================*/
#if DEBUG == 1
#define debugModMsg printf("[X] DEBUGGING MODE IS ON [X]\r\n")
#define debugMsg(x) printf(x)
#define debugVal(x, y) printf(x, y)
#define debug2Val(x, y, z) printf(x, y, z)
#else
#define debugModMsg printf("[X] DEBUGGING MODE IS OFF[X]\r\n")
#define debugMsg(x)
#define debugVal(x, y)
#define debug2Val(x, y, z)
#endif

/*=========================================================*/
/*== PROTOTYPE DECLARATION ================================*/
/*=========================================================*/
void MOTOR_PINS(void);
void debugTerm(void);
void MOTOR_A_ON(float_t pwmDC);
void MOTOR_B_ON(int dutyCycle);
void sig_handler(int32_t signum);
void timer_Us(int64_t uSeconds);
void timer_handler (int32_t sigNr);
float filterChar(char *string, char *searchString, char *term);

#endif
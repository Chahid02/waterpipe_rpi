/*!
*****************************************************************
* @file    waterpipe.c
* @brief   RBP WATERPIPE
* @author  Lukasz Piatek
* @version V1.0
* @date    2021-09-28
* @brief   BME280 Driver
* @copyright Copyright (c) Lukasz Piatek. All rights reserved.
*****************************************************************
*/

/*=========================================================*/
/*== INCLUDES =============================================*/
/*=========================================================*/
#include <wiringPi.h>
#include <softPwm.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include "waterpipe.h"
/*=========================================================*/
/*== TERMINAL PRINTCLEAR ==================================*/
/*=========================================================*/
#define clrscr() printf("\e[1;1H\e[2J") // Clear screen.

/*=========================================================*/
/*== PWM MACROS ===========================================*/
/*=========================================================*/

#define SWPWM_PIN29 29
#define SWPWM_PIN28 28

#define PWM_PIN0 26
#define PWM_PIN1 23
#define L293D_IN1 0
#define L293D_IN2 2
#define L293D_EN1 3

#define L293D_IN3 1
#define L293D_IN4 4
#define L293D_EN2 5

/*=========================================================*/
/*== GLOBAL VARIABLES =====================================*/
/*=========================================================*/
uint16_t pwmRange = 1000;
int32_t pwmClockDefault = 54e6;
float_t pwmDC = 0.1;
uint8_t swTimerFact = 10;
uint8_t swPwmPeriod = 5; /*!< in ms */
uint8_t runMotA = 0;

int socketPi;
int status;
int bytesRead;
char data[1024] = {0};
int main(void)
{

    struct sockaddr_rc raspPicoServer = {0};
    const char *raspPico = "98:D3:71:FD:F4:3A";

    raspPicoServer.rc_family = AF_BLUETOOTH;
    raspPicoServer.rc_channel = (uint8_t)1;
    str2ba(raspPico, &raspPicoServer.rc_bdaddr);

    debugMsg("====================  RFCOM SERVER MODE STARTED  ===================== \r\n");

    /*!< Allocate socket */
    socketPi = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    raspPicoServer.rc_family = AF_BLUETOOTH;
    raspPicoServer.rc_channel = (uint8_t)1;
    str2ba(raspPico, &raspPicoServer.rc_bdaddr);

    status = connect(socketPi, (struct sockaddr *)&raspPicoServer, sizeof(raspPicoServer));

    if (status < 0)
    {
        perror("get no msg");
    }
    else
    {
        bytesRead = read(socketPi, data, sizeof(data));
        if (bytesRead > 0)
        {
            debugVal("%s\n", data);
        }
    }

    debugTerm();
    delay(1000);
    debugMsg("====================  wiringPi INIT STARTED  ========================= \r\n");
    delay(1000);
    if (wiringPiSetup() < 0)
    {
        debugMsg("[X] wiringPi Setup failed !!! [X]\r\n");
        return -1;
    }
    else
    {
        debugMsg("[X] wiringPi Setup successfull !!! [X]\r\n");
    }
    delay(1000);
    debugMsg("====================  HW PWM INIT STARTED  =========================== \r\n");
    delay(1000);
    uint8_t freqHz = 50;
    float_t pwmClock = pwmClockDefault / freqHz / pwmRange;
    float_t pwmFreq = pwmClockDefault / pwmClock / pwmRange;
    int dutyCycle = 1023 * pwmDC;

    debugVal("[X] HW PWM CLOCK : %d [X]\r\n", (int32_t)pwmClock);
    debugVal("[X] HW PWM FREQ : %f [X]\r\n", pwmFreq);
    debugVal("[X] HW PWM DutyCycle : %d [X]\r\n", (int32_t)dutyCycle);

    pinMode(PWM_PIN0, PWM_OUTPUT); //pwm output mode
    pinMode(PWM_PIN1, PWM_OUTPUT); //pwm output mode
    debugVal("[X] HW PWM_PIN0 on : GPIO %d in OUTPUT MODE [X]\r\n", PWM_PIN0);
    debugVal("[X] HW PWM_PIN1 on : GPIO %d in OUTPUT MODE [X]\r\n", PWM_PIN1);
    delay(1000);

    debugMsg("====================  SW PWM INIT STARTED  =========================== \r\n");
    pinMode(SWPWM_PIN28, OUTPUT);
    pinMode(SWPWM_PIN29, OUTPUT);
    debugVal("[X] SW PWM_PIN28 on : GPIO %d in OUTPUT MODE [X]\r\n", SWPWM_PIN28);
    debugVal("[X] SW PWM_PIN29 on : GPIO %d in OUTPUT MODE [X]\r\n", SWPWM_PIN29);
    delay(1000);

    debugMsg("====================  HW PWM SETUP STARTED  ========================== \r\n");
    pwmSetMode(PWM_MODE_MS);
    pwmSetClock((int)pwmClock);
    pwmSetRange(pwmRange);

    debugVal("[X] HW PWM MODE SETS TO %s [X]\r\n", "PWM_MODE_MS");
    debugVal("[X] HW PWM PERIOD SETS TO %f ms [X]\r\n", (1 / pwmFreq) * 1000);
    debugVal("[X] HW PWM T_ON SETS TO %f ms [X]\r\n", pwmDC * ((1 / pwmFreq) * 1000));
    debugVal("[X] HW PWM T_OFF SETS TO %f ms [X]\r\n", (1 / pwmFreq) * 1000 - pwmDC * ((1 / pwmFreq) * 1000));

    delay(1000);

    debugMsg("====================  SWW PWM SETUP STARTED  ========================== \r\n");
    softPwmCreate(SWPWM_PIN28, 0, swPwmPeriod * swTimerFact); /*!< range = value(ms)*10 ! Measured with osci !  */
    //softPwmWrite(SWPWM_PIN28,swPwmPeriod * swTimerFact * pwmDC);/*!< DC in percent */
    debugVal("[X] SW PWM PERIOD SETS TO %d ms [X]\r\n", swPwmPeriod);
    debugVal("[X] SW PWM T_ON SETS TO %f ms [X]\r\n", pwmDC * swPwmPeriod);
    debugVal("[X] SW PWM T_OFF SETS TO %f ms [X]\r\n", swPwmPeriod - (pwmDC * swPwmPeriod));

    MOTOR_PINS();
    debugMsg("====================  OPERATION MODE STARTED  ======================== \r\n");
    //delay(5000);

    signal(SIGALRM, sig_handler);
    signal(SIGINT, sig_handler);
    alarm(1);

    runMotA = 1;
    memset(data, 0, sizeof(data));
    while (1)
    {
        //pwmWrite(PWM_PIN0, dutyCycle);/*!< Needs to be in the while loop */
        MOTOR_A_ON(pwmDC);
        //MOTOR_B_ON(511);
    }
    return 0;
}

/*!
**************************************************************
 * @brief Attempt to read the chip-id number of BM*-280 device
 * 
 * @note asdasdas
 * 
 * @warning
 * 
 * @param[in]  deviceAddr Description
 * @param[out]
 *
 * @return Result of reading the ID-register for chip 
 * identification
 *
 * @retval = 0 -> Success
 * @retval > 0 -> Warning
 * @retval < 0 -> Fail
 *
**************************************************************
 */
void MOTOR_PINS(void)
{
    debugMsg("====================  L293D GPIO INIT STARTED  ======================= \r\n");
    pinMode(0, OUTPUT);
    pinMode(2, OUTPUT);
    pinMode(3, OUTPUT);
    pinMode(4, OUTPUT);
    digitalWrite(L293D_EN2, LOW);
    digitalWrite(L293D_IN4, LOW);
}
/*!
**************************************************************
 * @brief Attempt to read the chip-id number of BM*-280 device
 * 
 * @note asdasdas
 * 
 * @warning
 * 
 * @param[in]  deviceAddr Description
 * @param[out]
 *
 * @return Result of reading the ID-register for chip 
 * identification
 *
 * @retval = 0 -> Success
 * @retval > 0 -> Warning
 * @retval < 0 -> Fail
 *
**************************************************************
 */
void MOTOR_A_ON(float_t pwmDC)
{
    //debugMsg("====================  L293D MOTOR-A ROTATING  ======================== \r\n");;
    softPwmWrite(SWPWM_PIN28, swPwmPeriod * swTimerFact * pwmDC); /*!< DC in percent */
    if (runMotA == 1)
    {
        digitalWrite(L293D_EN1, HIGH);
        digitalWrite(L293D_IN2, LOW);
    }
    else
    {
        digitalWrite(L293D_EN1, LOW);
        digitalWrite(L293D_IN2, LOW);
    }

    //debugVal("[X] Motor A Freq with DC: %d\r\n", pwmDC*100);
}
/*!
**************************************************************
 * @brief Attempt to read the chip-id number of BM*-280 device
 * 
 * @note asdasdas
 * 
 * @warning
 * 
 * @param[in]  deviceAddr Description
 * @param[out]
 *
 * @return Result of reading the ID-register for chip 
 * identification
 *
 * @retval = 0 -> Success
 * @retval > 0 -> Warning
 * @retval < 0 -> Fail
 *
**************************************************************
 */
void MOTOR_B_ON(int dutyCycle)
{
    debugMsg("====================  L293D MOTOR-B ROTATING  ======================== \r\n");
    digitalWrite(L293D_EN2, HIGH);
    pwmWrite(PWM_PIN1, dutyCycle);
    digitalWrite(L293D_IN4, LOW);
    debugVal("[X] Motor B Freq with DC: %d\r\n", dutyCycle);
}

/*!
**************************************************************
* @brief
*
* @param[in]  :
*
* @return Result of API execution status
*
* @retval = 0 -> Success.
* @retval > 0 -> Warning.
* @retval < 0 -> Fail.
*
*
**************************************************************
*/
void debugTerm(void)
{
    debugMsg("\n\n======================================================================\r\n");
    debugMsg("======================== DEBUG TERMINAL===============================\r\n");
    debugMsg("======================================================================\r\n\n");
}

void sig_handler(int32_t sigNr)
{
    if (sigNr == SIGALRM)
    { //signal handler for SIGALRM

        bytesRead = read(socketPi, data, sizeof(data));
        if (bytesRead < 0)
        {
            debugVal("%s\r\n", data);
            memset(data, 0, sizeof(data));
        }
        else
        {
            debugMsg("[X] No Bluetooth Data received !!! [X]\r\n");
        }

        //printf("2 Seconds Signal-IRQ\r\n");
        if (pwmDC < 1)
        {
            pwmDC += 0.1;
            //printf("[X] POWER %f [X]\r\n",pwmDC);
        }
        else if (pwmDC = 1.0f)
        {
            //printf("[X] FULL POWER [X]\r\n");
        }

        else
        {

            __NOP();
        }
        alarm(1);
    }

    if (sigNr == SIGINT)
    { // signal handler for SIGINT
        printf("\n[X] Exit Programm [X]\n");
        runMotA = 0;
        digitalWrite(L293D_EN1, LOW);
        exit(0);
    }
    memset(data, 0, sizeof(data));
    clrscr();
}
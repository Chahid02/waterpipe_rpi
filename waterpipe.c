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
#include <string.h>
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
uint8_t runMotB = 0;

uint16_t recMsg = 0;
uint usTim = 1;
int socketPi;
int status;
int bytesRead;
char data[1024] = {0};

float temperature, pressure, humidity, waterLevel, waterTemperature;
char *endTermimn = "ÿ";

int main(void)
{

    struct sockaddr_rc raspPicoServer = {0};
    //const char *raspPico = "98:D3:71:FD:F4:3A";
    //const char *raspPico = "00:20:08:00:1F:2A";
    const char *raspPico = "00:20:08:00:24:D8";

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
        __NOP();
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

    debugMsg("====================  INIT SIGNAL HANDLER STARTED  =================== \r\n");
    signal(SIGALRM, sig_handler);
    signal(SIGINT, sig_handler);
    //signal(SIGVTALRM, sig_handler);
    alarm(1);

    runMotA = 1;
    memset(data, 0, sizeof(data));

    timer_Us(100000);

    while (1)
    {

        if (usTim)
        {
        }

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

        /*  bytesRead = read(socketPi, data, sizeof(data));
        if (bytesRead > 0)
        {
            //debugVal("%s\r\n", data);
            debugVal("Bytes recieved:%d",bytesRead);
                filterChar(data, "A:","ÿ");
                filterChar(data, "B:","ÿ");
                filterChar(data, "C:","ÿ");
                filterChar(data, "D:","ÿ");
                filterChar(data, "E:","ÿ");  
                memset(data, 0, sizeof(data)); 
        }  */

        //printf("2 Seconds Signal-IRQ\r\n");
        if (pwmDC < 1)
        {
            pwmDC += 0.1;
            // debugVal("[X] POWER %f [X]\r\n",pwmDC);
        }
        else if (pwmDC = 1.0f)
        {
            // debugMsg("[X] FULL POWER [X]\r\n");
        }

        else
        {

            __NOP();
        }
        alarm(1);
    }
    else
    {
        __NOP();
    }

    if (sigNr == SIGINT)
    { // signal handler for SIGINT
        debugMsg("\n[X] Exit Programm [X]\n");
        runMotA = 0;
        runMotB = 0;
        digitalWrite(L293D_EN1, LOW);
        debugMsg("\n[X] Close Bluetooth Socket [X]\n");
        close(socketPi);
        debugMsg("\n[X] Exit Programm [X]\n");
        exit(0);
    }
    else
    {
        __NOP();
    }

    //clrscr();
}

void timer_Us(int64_t uSeconds)
{
    struct sigaction sa;
    struct itimerval timer;

    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = &timer_handler;
    sigaction(SIGVTALRM, &sa, NULL);

    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = uSeconds;

    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = uSeconds;

    setitimer(ITIMER_VIRTUAL, &timer, NULL);
}

void timer_handler(int32_t sigNr)
{

    usTim = !usTim;
    bytesRead = read(socketPi, data, sizeof(data));
    if (bytesRead > 0)
    {
        debugMsg("======================== BLUETOOTH TERMINAL===========================\r\n");
        //debugVal("%s\r\n", data);
        filterChar(data, "A:", "ÿ","[X] BME TEMP: ");
        filterChar(data, "B:", "ÿ","[X] BME PRESS: ");
        filterChar(data, "C:", "ÿ","[X] BME HUM: ");
        filterChar(data, "D:", "ÿ","[X] DS18B20 TEMP: ");
        filterChar(data, "E:", "ÿ","[X] WATERLEVEL: ");
        memset(data, 0, sizeof(data));
        clrscr();
    }
}

float filterChar(char *string, char *searchString, char *term, char *output)
{
    int len;
    char buff[strlen(searchString)];
    char *ret = strstr(string, searchString);

    if (ret != 0)
    {
        for (int i = 0; i < strlen(searchString); ++i)
        {
            buff[i] = ret[i];
        }
    }

    if (strcmp(buff, searchString) == 0 && ret != 0)
    {

        len = strcspn(ret, term);
        char buff[len];

        for (int i = 0; i < len; ++i)
        {
            buff[i] = ret[i + strlen(searchString)];
        }

        buff[len - strlen(searchString)] = '\0' /*"ÿ"*/;
        //debug2Val("\r\n %s%s [X] \r\n", searchString, buff);
        debug2Val("\r\n %s%s [X] \r\n", output, buff);
        return strtod(buff, NULL); /*!< strtod gives better control of undefined range */
    }
    else
    {
        /* debugMsg("=============================\r\n");
		debugVal("%s not found\r\n",searchString);
		debugMsg("=============================\r\n"); */
        return -200;
    }
}
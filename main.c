#include "simplelink.h"
#include "sl_common.h"

//LCD-Sensor driver
#include "msp.h"
#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include "LcdDriver/HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.h"
#include "HAL/HAL_I2C.h"
#include "HAL/HAL_TMP006.h"

//code libraries
#include <stdio.h>
#include <net/bsd/netdb.h>
#include <net/bsd/netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <socket.h>
#include <sys/types.h>
#include <unistd.h> // read(), write(), close()
#include <driverlib.h>
#define MAX 80
#define PORT 8080
#define SA struct sockaddr

/*
 * Values for below macros shall be modified per the access-point's (AP) properties
 * SimpleLink device will connect to following AP when the application is executed
 */
#define SSID_NAME       "MspNet"       /* Access point name to connect to. */
#define SEC_TYPE        SL_SEC_TYPE_WPA_WPA2     /* Security type of the Access piont */
#define PASSKEY         "mspmatteo"   /* Password in case of secure AP */
#define PASSKEY_LEN     pal_Strlen(PASSKEY)      /* Password length in case of secure AP */

#define APPLICATION_VERSION "1.0.0"
///http start
#define SL_STOP_TIMEOUT        0xFF
#define SMALL_BUF           32
#define MAX_SEND_BUF_SIZE   512
#define MAX_SEND_RCV_SIZE   1024
///http end

//CommProtocol
#define HANDSHAKE_ACK  "HANDSHAKE_ACK"

//LCD Definitions
#define MENU_SIZE 3
#define INTERFACES_DIM_MAX 5
#define DATA_BUFFER_DIM 10
//END LCD Definitions

//LCD Variables
typedef struct
{
    int currentPos;
    float bufferData[DATA_BUFFER_DIM];
} Data_t;

Data_t tempData;
Data_t luxData;

int ADC14_isOff = 0;
int back = 0;
int dataPos = 0;
int pressed = 0;
int currentState = 3;
int selectedElement = 0;
int samplingStop;
int rerender;

/* Graphic library context */
Graphics_Context g_sContext;

/* Variable for storing temperature value returned from TMP006 */
float temp;

/* ADC results buffer */
static uint16_t resultsBuffer[2];

//definig a pointer to void function not receiving parameters as behaviour_t
typedef void (*behaviour_t)();

typedef struct
{
    // element selection
    int selectedElement;
    int (*selectionFunction)();

    // interfaces handling
    int interfacesDim;
    int selectedInterface;
    void (*interfaces[INTERFACES_DIM_MAX])();

    // behaviour
    behaviour_t behaviours[INTERFACES_DIM_MAX];

} State_t;

State_t states[4];  //the array of states the machine will move into
Graphics_Rectangle menuRectangles[MENU_SIZE]; //array of rectangles that contains a rectangle for each state the machine can fall into.
//END LCD Variables

bool isConfigurationMode = false;

//tcp var
int sockfd, connfd, len;
struct sockaddr_in servaddr, cli;
int firstStart;
bool isPaired;
/* Application specific status/error codes */
typedef enum
{
    DEVICE_NOT_IN_STATION_MODE = -0x7D0, /* Choosing this number to avoid overlap with host-driver's error codes */
    STATUS_CODE_MAX = -0xBB8
} e_AppStatusCodes;
/*
 * GLOBAL VARIABLES -- Start
 */
_u32 g_Status = 0;

struct
{
    _u8 Recvbuff[MAX_SEND_RCV_SIZE];
    _u8 SendBuff[MAX_SEND_BUF_SIZE];

    _u8 HostName[SMALL_BUF];

    _u32 DestinationIP;
    _i16 SockID;
} g_AppData;

/*
 * GLOBAL VARIABLES -- End
 */
//these will be displayed into the menu state
void menuRectanglesInit()
{

    int height = 30;
    int space = 10;
    int i = 0;

    int XMIN = 5;
    int XMAX = 128 - XMIN;
    int YMIN = 5;

    for (i; i < MENU_SIZE; i++)
    {
        menuRectangles[i].xMin = XMIN;
        menuRectangles[i].xMax = XMAX;
        menuRectangles[i].yMin = YMIN + i * space + i * height;
        menuRectangles[i].yMax = menuRectangles[i].yMin + height;
    }
}

void ADC14_Kill()
{
    ADC14_disableConversion();
    while (ADC14_isBusy())
        ;
    ADC14_disableModule();
    ADC14_isOff = 1;
}

void ADC14_Revive()
{
    ADC14_enableModule();
    ADC14_enableConversion();
    ADC14_toggleConversionTrigger();
    ADC14_isOff = 0;
}

void menuInterfaceRender()
{
    Graphics_clearDisplay(&g_sContext);

    int i = 0;

    for (i; i < MENU_SIZE; i++)
    {
        Graphics_drawRectangle(&g_sContext, &menuRectangles[i]);
    }

    Graphics_fillRectangle(&g_sContext, &menuRectangles[selectedElement]);

    Graphics_drawStringCentered(
            &g_sContext, (int8_t*) "PAIRING MODE", 18, 64,
            (menuRectangles[0].yMax + menuRectangles[0].yMin) / 2, OPAQUE_TEXT);
    Graphics_drawStringCentered(
            &g_sContext, (int8_t*) "NORMAL MODE", 18, 64,
            (menuRectangles[1].yMax + menuRectangles[1].yMin) / 2, OPAQUE_TEXT);
    Graphics_drawStringCentered(
            &g_sContext, (int8_t*) "COMMUNICATION MODE", 18, 64,
            (menuRectangles[2].yMax + menuRectangles[2].yMin) / 2, OPAQUE_TEXT);

}

void menuStateBehaviour()
{

    //menuInterfaceRender();

}

int menuSelectionFunction()
{

    int intermediateValue;

    if (samplingStop == 0)
    {
        if (resultsBuffer[1] > 16330)    // joystick turned up
        {
            selectedElement--;
            samplingStop = 1;
            rerender = 1;

        }
        else if (resultsBuffer[1] < 50)    // joystick turned down
        {
            selectedElement++;
            samplingStop = 1;
            rerender = 1;

        }

        if (selectedElement < 0)
        {
            selectedElement = MENU_SIZE + selectedElement;
        }

        intermediateValue = selectedElement % MENU_SIZE;
        selectedElement = intermediateValue % MENU_SIZE;

        int i = 0;
        for (i; i < 300250; i++)
            ; // busy wait just to let some time pass before the new sample is taken

        if (!(P4IN & GPIO_PIN1) && pressed == 0)
        {
            pressed = 1;
            rerender = 1;

            int i = 0;
            for (i; i < 300250; i++)
                ; // busy wait just to let some time pass before the new sample is taken

            pressed = 0;

            ADC14_isOff = 1;
            currentState = selectedElement;
            ADC14_Kill();

        }

    }

    samplingStop = 0;

    return 0;
}

//-------------------------------------NORMAL STATE----------------------------

void normalModeRender()
{
    Graphics_clearDisplay(&g_sContext);
    //Graphics_drawRectangle(&g_sContext,&menuRectangles[0]);
    /* Display temperature */
    char tempString[30];
    char luxString[30];

    sprintf(tempString, "TEMP: %f", tempData.bufferData[tempData.currentPos]);
    Graphics_drawStringCentered(&g_sContext, (int8_t*) tempString, 11, 55, 40,
    OPAQUE_TEXT);

    sprintf(luxString, "LUX: %f", luxData.bufferData[luxData.currentPos]);
    Graphics_drawStringCentered(&g_sContext, (int8_t*) luxString, 11, 55, 60,
    OPAQUE_TEXT);

}

void normalModeBehaviour()
{

    float currentLuxData; //= OPT3001_getLux();
    luxData.bufferData[luxData.currentPos] = currentLuxData;
    int luxPartialPos = luxData.currentPos + 1;
    luxData.currentPos = luxPartialPos % DATA_BUFFER_DIM;

    /* Obtain temperature value from TMP006 */
    float tempIntermediate = TMP006_getTemp();

    tempIntermediate = tempIntermediate - 32;
    tempData.bufferData[tempData.currentPos] = tempIntermediate * 5 / 9;

    int tempPartialPos = tempData.currentPos + 1; //tempData.currentPos++;
    tempData.currentPos = tempPartialPos % DATA_BUFFER_DIM;

    rerender = 1;

}

//-------------------------------------PAIRING STATE----------------------------

void pairingInterfaceRender()
{
    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext, (int8_t*) "Pairing", 7, 64, 30,
                                OPAQUE_TEXT);
    rerender = 0;
}

void pairedInterfaceRender()
{
    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext, (int8_t*) "Paired", 7, 64, 30,
                                OPAQUE_TEXT);
    rerender = 0;
}

void pairingBehaviour()
{
    //(*states[currentState].interfaces[states[currentState].selectedInterface])();

    // add pairing code HERE

    if(!isPaired){
        communicationRoutine();
        isPaired = true;
    }
}

void communicationInterfaceRender()
{
    //Graphics_flushBuffer(&g_sContext);
    Graphics_clearDisplay(&g_sContext);

    char title[20];
    sprintf(title, "%s", "COLLECTED DATA");
    Graphics_drawStringCentered(&g_sContext, (int8_t*) title, 14, 64, 5,
                                OPAQUE_TEXT);

    char dataString[10];
    int i = 0;
    //char space[] = " ";

    for (i; i < DATA_BUFFER_DIM; i++)
    {
//        char currentData[5];
//        itoa(tempData.bufferData[0], currentData);
//        strncat(dataString, currentData, 2);
//        strncat(dataString, space, 1);

        sprintf(dataString, "%f", tempData.bufferData[i]);
        Graphics_drawStringCentered(&g_sContext, (int8_t*) dataString, 10, 64,
                                    20 + i * 10, OPAQUE_TEXT);
    }

    rerender = 0;
}

void tempDataInterface()
{
    //Graphics_flushBuffer(&g_sContext);
    Graphics_clearDisplay(&g_sContext);

    char title[20];
    sprintf(title, "%s", "COLLECTED DATA-TEMP");
    Graphics_drawStringCentered(&g_sContext, (int8_t*) title, 19, 64, 5,
                                OPAQUE_TEXT);

    char dataString[10];
    int i = 0;
    //char space[] = " ";

    for (i; i < DATA_BUFFER_DIM; i++)
    {

        sprintf(dataString, "%f", tempData.bufferData[i]);
        Graphics_drawStringCentered(&g_sContext, (int8_t*) dataString, 10, 64,
                                    20 + i * 10, OPAQUE_TEXT);
    }

    rerender = 0;

}

void luxDataInterface()
{
    Graphics_clearDisplay(&g_sContext);

    char title[20];
    sprintf(title, "%s", "COLLECTED DATA-LUX");
    Graphics_drawStringCentered(&g_sContext, (int8_t*) title, 18, 64, 5,
                                OPAQUE_TEXT);

    char dataString[10];
    int i = 0;

    for (i; i < DATA_BUFFER_DIM; i++)
    {

        sprintf(dataString, "%f", luxData.bufferData[i]);
        Graphics_drawStringCentered(&g_sContext, (int8_t*) dataString, 10, 64,
                                    20 + i * 10, OPAQUE_TEXT);
    }

    rerender = 0;

}

void noBehaviour()
{

    //(*states[currentState].interfaces[states[currentState].selectedInterface])();

}

int backSelectionFunction()
{

    // CODICE
    rerender = 1;
    ADC14_Revive();
    currentState = 3;

    return 0;
}

int communicateSelectionFunction()
{
    //char string[30];

    //renderStop = 0;
    int intermediateValue;

    if (samplingStop == 0)
    {
        if (resultsBuffer[0] > 16330)    // joystick turned left
        {
            int intermediateInterface = states[currentState].selectedInterface
                    + 1;
            states[currentState].selectedInterface = intermediateInterface
                    % states[currentState].interfacesDim;

            samplingStop = 1;
            rerender = 1;

        }
        else if (resultsBuffer[0] < 50)    // joystick turned right
        {
            int intermediateInterface = states[currentState].selectedInterface
                    + 1;
            states[currentState].selectedInterface = intermediateInterface
                    % states[currentState].interfacesDim;

            samplingStop = 1;
            rerender = 1;
            //Graphics_drawStringCentered(&g_sContext,(int8_t *)string,18,64,50,OPAQUE_TEXT);

        }

        //sprintf(string, "SElem : %d",selectedElement);
        //Graphics_drawStringCentered(&g_sContext,(int8_t *)string,30,64,50,OPAQUE_TEXT);

        int i = 0;
        for (i; i < 300250; i++)
            ;
        samplingStop = 0;
        // busy wait per scandire il tempo di campionamento

        //Graphics_drawStringCentered(&g_sContext,(int8_t *) itoa(selectedElement,string) ,4,64,50,OPAQUE_TEXT);
        //       if (rerender==1)
        //       {
        //           menuInterfaceRender();
        //           rerender = 0;
        //       }

        if (!(P4IN & GPIO_PIN1))
        {
            // TEST CHECK
            //Graphics_clearDisplay(&g_sContext);
            //Graphics_drawStringCentered(&g_sContext, (int8_t*) "BACK FUNC", 9, 64,10, OPAQUE_TEXT);

            // CODICE
            rerender = 1;
            currentState = 3;

            //interfacePlaceholder(); //*(states[currentState].interfaces[states[currentState].selectedInterface])();
        }
    }
    return 0;
}

void render()
{
    if (rerender == 1)
    {
        //Graphics_flushBuffer(&g_sContext);
        (*states[currentState].interfaces[states[currentState].selectedInterface])();
        rerender = 0;
    }

}

void communicationBehaviour()
{

}
//-------------------------------------COMMUNICATION STATE----------------------------

void statesInitializer()
{
    // states are sorted thinking about the flow interactions should follow --- pairing, then collecting data, then communication to send it
    // menu is put at the last one and will be avoided scrolling using the joystick

    // states[0] initialization -- pairing state

    states[0].selectedElement = -1;
    states[0].selectionFunction = &backSelectionFunction;
    states[0].interfacesDim = 3; //pairing,paired,error
    states[0].selectedInterface = 0;
    states[0].interfaces[0] = &pairingInterfaceRender;           //ok
    states[0].behaviours[0] = &pairingBehaviour;

    // states[1] initialization -- normal state

    states[1].selectedElement = -1;
    states[1].selectionFunction = &backSelectionFunction;
    states[1].interfacesDim = 1;   // mostra i dati irt
    states[1].selectedInterface = 0;
    states[1].interfaces[0] = &normalModeRender;
    states[1].behaviours[0] = &normalModeBehaviour;

    // states[2] initialization -- communication state

    states[2].selectedElement = -1;
    states[2].selectionFunction = &backSelectionFunction;
    states[2].interfacesDim = 2; // sending, ok,error
    states[2].selectedInterface = 0;
    states[2].interfaces[0] = &tempDataInterface; //communicationInterfaceRender;            //ok
    states[2].interfaces[1] = &luxDataInterface;
    states[2].behaviours[0] = &communicationBehaviour;

    // states[3] initialization -- menu state

    states[3].selectedElement = 0;
    states[3].selectionFunction = &menuSelectionFunction;
    states[3].interfacesDim = 1; // 1 per ogni rettangolo selezionabile
    states[3].selectedInterface = 0;
    states[3].interfaces[0] = &menuInterfaceRender;         //ok
    states[3].behaviours[0] = &noBehaviour;
}

void _adcInit()
{
    /* Configures Pin 6.0 and 4.4 as ADC input */
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6, GPIO_PIN0,
                                               GPIO_TERTIARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN4,
                                               GPIO_TERTIARY_MODULE_FUNCTION);

    /* Initializing ADC (ADCOSC/64/8) */
    ADC14_enableModule();
    ADC14_initModule(ADC_CLOCKSOURCE_ADCOSC, ADC_PREDIVIDER_64, ADC_DIVIDER_8,
                     0);

    /* Configuring ADC Memory (ADC_MEM0 - ADC_MEM1 (A15, A9)  with repeat)
     * with internal 2.5v reference */
    ADC14_configureMultiSequenceMode(ADC_MEM0, ADC_MEM1, true);
    ADC14_configureConversionMemory(ADC_MEM0,
    ADC_VREFPOS_AVCC_VREFNEG_VSS,
                                    ADC_INPUT_A15, ADC_NONDIFFERENTIAL_INPUTS);

    ADC14_configureConversionMemory(ADC_MEM1,
    ADC_VREFPOS_AVCC_VREFNEG_VSS,
                                    ADC_INPUT_A9, ADC_NONDIFFERENTIAL_INPUTS);

    /* Enabling the interrupt when a conversion on channel 1 (end of sequence)
     *  is complete and enabling conversions */
    ADC14_enableInterrupt(ADC_INT1);

    /* Enabling Interrupts */
    Interrupt_enableInterrupt(INT_ADC14);
    Interrupt_enableMaster();

    /* Setting up the sample timer to automatically step through the sequence
     * convert.
     */
    ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);

    /* Triggering the start of the sample */
    ADC14_enableConversion();
    ADC14_toggleConversionTrigger();
}

void _graphicsInit()
{
    /* Initializes display */
    Crystalfontz128x128_Init();

    /* Set default screen orientation */
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

    /* Initializes graphics context */
    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128,
                         &g_sCrystalfontz128x128_funcs);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
    Graphics_clearDisplay(&g_sContext);

}

void _temperatureSensorInit()
{
    /* Temperature Sensor initialization */
    /* Initialize I2C communication */
    Init_I2C_GPIO();
    I2C_init();
    /* Initialize TMP006 temperature sensor */
    TMP006_init();
    __delay_cycles(100000);

}

void _hwInit()
{
    /* Halting WDT and disabling master interrupts */
    WDT_A_holdTimer();
    Interrupt_disableMaster();

    /* Set the core voltage level to VCORE1 */
    PCM_setCoreVoltageLevel(PCM_VCORE1);

    /* Set 2 flash wait states for Flash bank 0 and 1*/
    FlashCtl_setWaitState(FLASH_BANK0, 2);
    FlashCtl_setWaitState(FLASH_BANK1, 2);

    /* Initializes Clock System */
    CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);
    CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_HSMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);

    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1, GPIO_PIN1);
    GPIO_enableInterrupt(GPIO_PORT_P1, GPIO_PIN1);
    Interrupt_enableInterrupt(INT_PORT1);

    Interrupt_setPriority(INT_PORT1, 0);
    Interrupt_setPriority(INT_ADC14, (uint8_t) 100);

    rerender = 1;

    _graphicsInit();
    _temperatureSensorInit();

    menuRectanglesInit();
    statesInitializer();

    _adcInit();
}

///*
// * Main function
// */
//int main(void)
//{
//
//    _hwInit();
//
//    while (1)
//    {
//        if(ADC14_isOff == 1)
//        {
//            render();
//            (*states[currentState].behaviours[0])();
//
//        }
//    }
//}

/* This interrupt is fired whenever a conversion is completed and placed in
 * ADC_MEM1. This signals the end of conversion and the results array is
 * grabbed and placed in resultsBuffer */
void ADC14_IRQHandler(void)
{
    uint64_t status;

    status = ADC14_getEnabledInterruptStatus();
    ADC14_clearInterruptFlag(status);

    /* ADC_MEM1 conversion completed */
    if (status & ADC_INT1)
    {

        /* Store ADC14 conversion results */
        resultsBuffer[0] = ADC14_getResult(ADC_MEM0);
        resultsBuffer[1] = ADC14_getResult(ADC_MEM1);

        render();

        if (currentState == 3)
            (*states[currentState].selectionFunction)();

    }
}

void PORT1_IRQHandler()
{
    /* Check which pins generated the interrupts */
    uint_fast16_t status = GPIO_getEnabledInterruptStatus(GPIO_PORT_P1);
    /* clear interrupt flag (to clear pending interrupt indicator */
    GPIO_clearInterruptFlag(GPIO_PORT_P1, status);
    if(status & GPIO_PIN1)
    {
        GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
        if(currentState == 0 || currentState == 1 || currentState == 2)
        {
            (*states[currentState].selectionFunction)();
        }
    }

}
/*
 * STATIC FUNCTION DEFINITIONS -- Start
 */
static _i32 establishConnectionWithAP();
static _i32 disconnectFromAP();
static _i32 configureSimpleLinkToDefaultState();
static _i32 initializeAppVariables();
static void displayBanner();
static _i32 getHostIP();
static _i32 createConnection();


void interruptSetup()
{
    //SETUP Button 1.1
    /* P1.1 as input for button */
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1, GPIO_PIN1);
    GPIO_enableInterrupt(GPIO_PORT_P1, GPIO_PIN1);
    GPIO_registerInterrupt(GPIO_PORT_P1, PORT1_IRQHandler);
    /* Enable interrupts on Port 1 (to catch P1.1 and P1.4 interrupts) */
    Interrupt_enableInterrupt(INT_PORT1);

    /* Enabling MASTER interrupts */
    Interrupt_enableMaster();
}

// Function designed for chat between client and server.
void configurationMode()
{

    // Accept the data packet from client and verification
    connfd = SL_EAGAIN;
    while (connfd == SL_EAGAIN)
    {
        connfd = accept(sockfd, (SA*) &cli, &len);
        if (!isConfigurationMode)
            break;
    }

    if (connfd < 0)
    {
        CLI_Write("server accept failed...\n");
        exit(0);
    }
    else
        CLI_Write("server accept the client...\n");

    char buff[MAX];
    CLI_Write("Entering function...\n");
    uint32_t rcvd_bytes = 0;

    CLI_Write("Starting new read\n\r");
    int status = 0;
    bzero(buff, sizeof(buff));

    while (status != sizeof(buff))
    {
        do
        {
            status = sl_Recv(connfd, buff, sizeof(buff), 0);
        }
        while (status == SL_EAGAIN);
        char bf[100];
        sprintf(bf, "\nReceived packets (%d bytes) successfully\n\r", status);
        CLI_Write(bf);
        if (status < 0)
        {
            CLI_Write("TCP Error occured!\n");
            sl_Close(connfd);
            return (-1);
        }
        else if (status == sizeof(buff))
        {
            sl_Send(connfd, HANDSHAKE_ACK, sizeof(HANDSHAKE_ACK), 0);
        }
    }
    char tmp[100] = "";
    sprintf(tmp, "\nReceived %u packets (%u bytes) successfully\n\r",
            (status / 1), status);
    CLI_Write(tmp);
    sprintf(tmp, "\nMessage is: %s\n", buff);
    CLI_Write(tmp);
    status = 0;

    //sprintf(buff, "Closing Connection");
    // and send that buffer to client
    //sl_Send(connfd, buff, sizeof(buff), 0);
    // After chatting close the socket
    close(sockfd);

}

int communicationRoutine()
{
    int retVal;
    displayBanner();

    /*
     * Following function configures the device to default state by cleaning
     * the persistent settings stored in NVMEM (viz. connection profiles &
     * policies, power policy etc)
     *
     * Applications may choose to skip this step if the developer is sure
     * that the device is in its default state at start of application
     *
     * Note that all profiles and persistent settings that were done on the
     * device will be lost
     */

    if (firstStart)
    {
        retVal = configureSimpleLinkToDefaultState();
        if (retVal < 0)
        {
            if (DEVICE_NOT_IN_STATION_MODE == retVal)
                CLI_Write(
                        " Failed to configure the device in its default state \n\r");
            LOOP_FOREVER();
        }

        CLI_Write(" Device is configured in default state \n\r");

        /*
         * Assumption is that the device is configured in station mode already
         * and it is in its default state
         */
        retVal = sl_Start(0, 0, 0);
        if ((retVal < 0) || (ROLE_STA != retVal))
        {
            CLI_Write(" Failed to start the device \n\r");
            LOOP_FOREVER();
        }
        CLI_Write(" Device started as STATION \n\r");
        firstStart = false;
    }
    else
        close(sockfd);

    /* Connecting to WLAN AP */
    retVal = establishConnectionWithAP();
    if (retVal < 0)
    {
        CLI_Write(" Failed to establish connection w/ an AP \n\r");
        LOOP_FOREVER();
    }

    CLI_Write(" Connection established w/ AP and IP is acquired \n\r");

    //close socket if not first start
//            if (!firstStart)
//                close(sockfd);

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
    {
        CLI_Write("socket creation failed...\n");
        exit(0);
    }
    else
        CLI_Write("Socket successfully created..\n");

    SlSockNonblocking_t enableOption;
    enableOption.NonblockingEnabled = 1;
    sl_SetSockOpt(sockfd, SL_SOL_SOCKET, SL_SO_NONBLOCKING,
                  (_u8*) &enableOption, sizeof(enableOption)); // Enable/disable nonblocking mode

    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*) &servaddr, sizeof(servaddr))) != 0)
    {
        CLI_Write("socket bind failed...\n");
        exit(0);
    }
    else
        CLI_Write("Socket successfully binded..\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0)
    {
        CLI_Write("Listen failed...\n");
        exit(0);
    }
    else
        CLI_Write("Server listening..\n");
    len = sizeof(cli);
    configurationMode();

    //code end
    retVal = disconnectFromAP();
    if (retVal < 0)
    {
        CLI_Write(" Failed to disconnect from the AP \n\r");
        LOOP_FOREVER();
    }
}

/*
 * Application's entry point
 */
int main(int argc, char **argv)
{

    _i32 retVal = -1;
    firstStart = true;
    retVal = initializeAppVariables();
    ASSERT_ON_ERROR(retVal);
    /* Stop WDT and initialize the system-clock of the MCU */
    stopWDT();
    interruptSetup();
    initClk();
    /* Configure command line interface */
    CLI_Configure();

    //communicationRoutine();

    _hwInit();

    while (1)
    {
        if (ADC14_isOff == 1)
        {
            render();
            (*states[currentState].behaviours[0])();
        }
    }

//    while (1)
//    {
//        if (isConfigurationMode)
//        {
//            communicationRoutine();
//        }
//        else
//        {
//            CLI_Write("Normal Mode\n");
//        }
//    }

    return 0;
}

/*
 * STATIC FUNCTION DEFINITIONS -- End
 */

/*
 * ASYNCHRONOUS EVENT HANDLERS -- Start
 */

/*!
 \brief This function handles WLAN events

 \param[in]      pWlanEvent is the event passed to the handler

 \return         None

 \note

 \warning
 */
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
    if (pWlanEvent == NULL)
        CLI_Write(" [WLAN EVENT] NULL Pointer Error \n\r");

    switch (pWlanEvent->Event)
    {
    case SL_WLAN_CONNECT_EVENT:
    {
        SET_STATUS_BIT(g_Status, STATUS_BIT_CONNECTION);

        /*
         * Information about the connected AP (like name, MAC etc) will be
         * available in 'slWlanConnectAsyncResponse_t' - Applications
         * can use it if required
         *
         * slWlanConnectAsyncResponse_t *pEventData = NULL;
         * pEventData = &pWlanEvent->EventData.STAandP2PModeWlanConnected;
         *
         */
    }
        break;

    case SL_WLAN_DISCONNECT_EVENT:
    {
        slWlanConnectAsyncResponse_t *pEventData = NULL;

        CLR_STATUS_BIT(g_Status, STATUS_BIT_CONNECTION);
        CLR_STATUS_BIT(g_Status, STATUS_BIT_IP_ACQUIRED);

        pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

        /* If the user has initiated 'Disconnect' request, 'reason_code' is SL_USER_INITIATED_DISCONNECTION */
        if (SL_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
        {
            CLI_Write(
                    " Device disconnected from the AP on application's request \n\r");
        }
        else
        {
            CLI_Write(" Device disconnected from the AP on an ERROR..!! \n\r");
        }
    }
        break;

    default:
    {
        CLI_Write(" [WLAN EVENT] Unexpected event \n\r");
    }
        break;
    }
}

/*!
 \brief This function handles events for IP address acquisition via DHCP
 indication

 \param[in]      pNetAppEvent is the event passed to the handler

 \return         None

 \note

 \warning
 */
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    if (pNetAppEvent == NULL)
        CLI_Write(" [NETAPP EVENT] NULL Pointer Error \n\r");

    switch (pNetAppEvent->Event)
    {
    case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
    {
        SET_STATUS_BIT(g_Status, STATUS_BIT_IP_ACQUIRED);

        /*
         * Information about the connected AP's IP, gateway, DNS etc
         * will be available in 'SlIpV4AcquiredAsync_t' - Applications
         * can use it if required
         *
         * SlIpV4AcquiredAsync_t *pEventData = NULL;
         * pEventData = &pNetAppEvent->EventData.ipAcquiredV4;
         * <gateway_ip> = pEventData->gateway;
         *
         */
    }
        break;

    default:
    {
        CLI_Write(" [NETAPP EVENT] Unexpected event \n\r");
    }
        break;
    }
}

/*!
 \brief This function handles callback for the HTTP server events

 \param[in]      pHttpEvent - Contains the relevant event information
 \param[in]      pHttpResponse - Should be filled by the user with the
 relevant response information

 \return         None

 \note

 \warning
 */
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent,
                                  SlHttpServerResponse_t *pHttpResponse)
{
    /*
     * This application doesn't work with HTTP server - Hence these
     * events are not handled here
     */
    CLI_Write(" [HTTP EVENT] Unexpected event \n\r");
}

/*!
 \brief This function handles general error events indication

 \param[in]      pDevEvent is the event passed to the handler

 \return         None
 */
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    /*
     * Most of the general errors are not FATAL are are to be handled
     * appropriately by the application
     */
    CLI_Write(" [GENERAL EVENT] \n\r");
}

/*!
 \brief This function handles socket events indication

 \param[in]      pSock is the event passed to the handler

 \return         None
 */
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    if (pSock == NULL)
        CLI_Write(" [SOCK EVENT] NULL Pointer Error \n\r");

    switch (pSock->Event)
    {
    case SL_SOCKET_TX_FAILED_EVENT:
    {
        /*
         * TX Failed
         *
         * Information about the socket descriptor and status will be
         * available in 'SlSockEventData_t' - Applications can use it if
         * required
         *
         * SlSockEventData_t *pEventData = NULL;
         * pEventData = & pSock->EventData;
         */
        switch (pSock->EventData.status)
        {
        case SL_ECLOSE:
            CLI_Write(
                    " [SOCK EVENT] Close socket operation failed to transmit all queued packets\n\r");
            break;

        default:
            CLI_Write(" [SOCK EVENT] Unexpected event \n\r");
            break;
        }
    }
        break;

    default:
        CLI_Write(" [SOCK EVENT] Unexpected event \n\r");
        break;
    }
}
/*
 * ASYNCHRONOUS EVENT HANDLERS -- End
 */

/*!
 \brief Gets the Server IP address

 \param[in]      none

 \return         zero for success and -1 for error

 \warning
 */
static _i32 getHostIP()
{
    _i32 status = 0;

    status = sl_NetAppDnsGetHostByName((_i8*) g_AppData.HostName,
                                       pal_Strlen(g_AppData.HostName),
                                       &g_AppData.DestinationIP, SL_AF_INET);
    ASSERT_ON_ERROR(status);

    return SUCCESS;
}

/*!
 \brief Create UDP socket to communicate with server.

 \param[in]      none

 \return         Socket descriptor for success otherwise negative

 \warning
 */
static _i32 createConnection()
{
    SlSockAddrIn_t Addr;

    _i16 sd = 0;
    _i16 AddrSize = 0;
    _i32 ret_val = 0;

    Addr.sin_family = SL_AF_INET;
    Addr.sin_port = sl_Htons(80);

    /* Change the DestinationIP endianity, to big endian */
    Addr.sin_addr.s_addr = sl_Htonl(g_AppData.DestinationIP);

    AddrSize = sizeof(SlSockAddrIn_t);

    sd = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, 0);
    if (sd < 0)
    {
        CLI_Write((_u8*) " Error creating socket\n\r\n\r");
        ASSERT_ON_ERROR(sd);
    }

    ret_val = sl_Connect(sd, (SlSockAddr_t*) &Addr, AddrSize);
    if (ret_val < 0)
    {
        /* error */
        CLI_Write((_u8*) " Error connecting to server\n\r\n\r");
        ASSERT_ON_ERROR(ret_val);
    }

    return sd;
}

/*!
 \brief This function configure the SimpleLink device in its default state. It:
 - Sets the mode to STATION
 - Configures connection policy to Auto and AutoSmartConfig
 - Deletes all the stored profiles
 - Enables DHCP
 - Disables Scan policy
 - Sets Tx power to maximum
 - Sets power policy to normal
 - Unregisters mDNS services
 - Remove all filters

 \param[in]      none

 \return         On success, zero is returned. On error, negative is returned
 */

static _i32 configureSimpleLinkToDefaultState()
{
    SlVersionFull ver = { 0 };
    _WlanRxFilterOperationCommandBuff_t RxFilterIdMask = { 0 };

    _u8 val = 1;
    _u8 configOpt = 0;
    _u8 configLen = 0;
    _u8 power = 0;

    _i32 retVal = -1;
    _i32 mode = -1;

    mode = sl_Start(0, 0, 0);
    ASSERT_ON_ERROR(mode);

    /* If the device is not in station-mode, try configuring it in station-mode */
    if (ROLE_STA != mode)
    {
        if (ROLE_AP == mode)
        {
            /* If the device is in AP mode, we need to wait for this event before doing anything */
            while (!IS_IP_ACQUIRED(g_Status))
            {
                _SlNonOsMainLoopTask();
            }
        }

        /* Switch to STA role and restart */
        retVal = sl_WlanSetMode(ROLE_STA);
        ASSERT_ON_ERROR(retVal);

        retVal = sl_Stop(SL_STOP_TIMEOUT);
        ASSERT_ON_ERROR(retVal);

        retVal = sl_Start(0, 0, 0);
        ASSERT_ON_ERROR(retVal);

        /* Check if the device is in station again */
        if (ROLE_STA != retVal)
        {
            /* We don't want to proceed if the device is not coming up in station-mode */
            ASSERT_ON_ERROR(DEVICE_NOT_IN_STATION_MODE);
        }
    }

    /* Get the device's version-information */
    configOpt = SL_DEVICE_GENERAL_VERSION;
    configLen = sizeof(ver);
    retVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &configOpt, &configLen,
                       (_u8*) (&ver));
    ASSERT_ON_ERROR(retVal);

    /* Set connection policy to Auto + SmartConfig (Device's default connection policy) */
    retVal = sl_WlanPolicySet(SL_POLICY_CONNECTION,
                              SL_CONNECTION_POLICY(1, 0, 0, 0, 1), NULL, 0);
    ASSERT_ON_ERROR(retVal);

    /* Remove all profiles */
    retVal = sl_WlanProfileDel(0xFF);
    ASSERT_ON_ERROR(retVal);

    /*
     * Device in station-mode. Disconnect previous connection if any
     * The function returns 0 if 'Disconnected done', negative number if already disconnected
     * Wait for 'disconnection' event if 0 is returned, Ignore other return-codes
     */
    retVal = sl_WlanDisconnect();
    if (0 == retVal)
    {
        /* Wait */
        while (IS_CONNECTED(g_Status))
        {
            _SlNonOsMainLoopTask();
        }
    }

    /* Enable DHCP client*/
    retVal = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE, 1, 1, &val);
    ASSERT_ON_ERROR(retVal);

    /* Disable scan */
    configOpt = SL_SCAN_POLICY(0);
    retVal = sl_WlanPolicySet(SL_POLICY_SCAN, configOpt, NULL, 0);
    ASSERT_ON_ERROR(retVal);

    /* Set Tx power level for station mode
     Number between 0-15, as dB offset from max power - 0 will set maximum power */
    power = 0;
    retVal = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID,
    WLAN_GENERAL_PARAM_OPT_STA_TX_POWER,
                        1, (_u8*) &power);
    ASSERT_ON_ERROR(retVal);

    /* Set PM policy to normal */
    retVal = sl_WlanPolicySet(SL_POLICY_PM, SL_NORMAL_POLICY, NULL, 0);
    ASSERT_ON_ERROR(retVal);

    /* Unregister mDNS services */
    retVal = sl_NetAppMDNSUnRegisterService(0, 0);
    ASSERT_ON_ERROR(retVal);

    /* Remove  all 64 filters (8*8) */
    pal_Memset(RxFilterIdMask.FilterIdMask, 0xFF, 8);
    retVal = sl_WlanRxFilterSet(SL_REMOVE_RX_FILTER, (_u8*) &RxFilterIdMask,
                                sizeof(_WlanRxFilterOperationCommandBuff_t));
    ASSERT_ON_ERROR(retVal);

    retVal = sl_Stop(SL_STOP_TIMEOUT);
    ASSERT_ON_ERROR(retVal);

    retVal = initializeAppVariables();
    ASSERT_ON_ERROR(retVal);

    return retVal; /* Success */
}

/*!
 \brief Connecting to a WLAN Access point

 This function connects to the required AP (SSID_NAME).
 The function will return once we are connected and have acquired IP address

 \param[in]  None

 \return     0 on success, negative error-code on error

 \note

 \warning    If the WLAN connection fails or we don't acquire an IP address,
 We will be stuck in this function forever.
 */
static _i32 establishConnectionWithAP()
{
    SlSecParams_t secParams = { 0 };
    _i32 retVal = 0;

    secParams.Key = PASSKEY;
    secParams.KeyLen = PASSKEY_LEN;
    secParams.Type = SEC_TYPE;

    retVal = sl_WlanConnect(SSID_NAME, pal_Strlen(SSID_NAME), 0, &secParams, 0);
    ASSERT_ON_ERROR(retVal);

    /* Wait */
    while ((!IS_CONNECTED(g_Status)) || (!IS_IP_ACQUIRED(g_Status)))
    {
        _SlNonOsMainLoopTask();
    }

    return SUCCESS;
}

/*!
 \brief Disconnecting from a WLAN Access point

 This function disconnects from the connected AP

 \param[in]      None

 \return         none

 \note

 \warning        If the WLAN disconnection fails, we will be stuck in this function forever.
 */
static _i32 disconnectFromAP()
{
    _i32 retVal = -1;

    /*
     * The function returns 0 if 'Disconnected done', negative number if already disconnected
     * Wait for 'disconnection' event if 0 is returned, Ignore other return-codes
     */
    retVal = sl_WlanDisconnect();
    if (0 == retVal)
    {
        /* Wait */
        while (IS_CONNECTED(g_Status))
        {
            _SlNonOsMainLoopTask();
        }
    }

    return SUCCESS;
}

/*!
 \brief This function initializes the application variables

 \param[in]  None

 \return     0 on success, negative error-code on error
 */
static _i32 initializeAppVariables()
{
    g_Status = 0;
    pal_Memset(&g_AppData, 0, sizeof(g_AppData));

    return SUCCESS;
}

/*!
 \brief This function displays the application's banner

 \param      None

 \return     None
 */
static void displayBanner()
{
    CLI_Write("\n\r\n\r");
    CLI_Write(" TCP TEST ");
    CLI_Write(APPLICATION_VERSION);
    CLI_Write(
            "\n\r*******************************************************************************\n\r");
}

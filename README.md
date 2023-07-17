# MSP432 SENSING STATION and WIFI ENDPOINT
# DESCRIPTION
This project realizes an IoT endpoint out of the [MSP432P401R LaunchPad Development Kit](https://www.ti.com/lit/ds/slas826e/slas826e.pdf) making it a device able of sensing the surrounding world using the sensors included in the [BOOSTXL-EDUMKII](https://www.ti.com/tool/BOOSTXL-EDUMKII) and making it also capable of searching for devices into the network thanks to the [CC3100BOOST](https://www.ti.com/tool/CC3100BOOST?keyMatch=&tisearch=search-everything&usecase=partmatches) and pairing with them in order to send the data it collects to make further processing or keeping track of it .
The device is under control of the user in a very quick and easy way thanks to a clean menu and 3 modes which are chosen from the user using the joystick present in the [BOOSTXL-EDUMKII](https://www.ti.com/tool/BOOSTXL-EDUMKII) and a button present on the board that acts as a 'back button' to come back to the starting menu.


# REQUIREMENTS
  - ## Hardware Requirements
    * [MSP432P401R LaunchPad Development Kit](https://www.ti.com/lit/ds/slas826e/slas826e.pdf)
    * [CC3100BOOST](https://www.ti.com/tool/CC3100BOOST?keyMatch=&tisearch=search-everything&usecase=partmatches)
    * [BOOSTXL-EDUMKII](https://www.ti.com/tool/BOOSTXL-EDUMKII)
    * USB Cable
    * PC
  - ## Software Requirements
    * [CCS v.12.0](https://www.ti.com/tool/download/CCSTUDIO/12.0.0) 
# PROJECT LAYOUT
msp432_endpoint/\
├── .launches\
├── .settings\
├── Debug\
├── HAL/\
│   ├── HAL_I2C.c\
│   ├── HAL_I2C.h\
│   ├── HAL_OPT3001.c\
│   ├── HAL_OPT3001.h\
│   ├── HAL_TMP006.c\
│   └── HAL_TMP006.h\
├── JSON/\
│   ├── cJSON.c\
│   └── cJSON.h\
├── LcdDriver/\
│   ├── Crystalfontz128x128_ST7735.c\
│   ├── Crystalfontz128x128_ST7735.h\
│   ├── HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.c\
│   └── HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.h
├── RemoteSystemsTempFiles\
├── board\
├── cli_uart/\
│   ├── cli_uart.c\
│   └── cli_uart.h\
├── driverlib/MSP432P4xx/\
│   ├── adc14.c\
│   ├── adc14.h\
│   ├── driverlib.h\
│   ├── driverlib.c\
│   ├── gpio.h\
│   ├── gpio.c\
│   ├── i2c.h\
│   ├── i2c.c\
│   ├── interrupt.c\
│   ├── interrupt.h\
│   ├── wdt_a.c\
│   └── wdt_a.h\
├── net/\
├── simplelink/\
│   └── include/\
│       ├── device.h\
│       ├── fs.h\
│       ├── netapp.h\
│       ├── netcfg.h\
│       ├── simplelink.h\
│       ├── socket.h\
│       ├── trace.h\
│       ├── user.h\
│       ├── wlan.h\
│       └── wlan_rx_filters.h\
├── spi_cc3100/\
│   ├── spi_cc3100.c\
│   └── spi_cc3100.h\
├── targetConfigs\
├── uart_cc3100/\
│   ├── uart_cc3100.c\
│   └── uart_cc3100.h\
├── .ccsproject\
├── .gitignore\
├── .project\
├── README.md\
├── README.txt\
├── main.c\
├── msp432p401r.cmd\
├── sl_common.h\
├── startup_msp432p401r_ccs.c\
└── system_msp432p401r.c\

# LIBRARIES
This project uses several libraries, some are given from Texas Instrument and are built to provide a light abstraction level for their products in order to avoid setting up each register of the system to implement the given behaviour.
These libraries are :

- "msp.h" : defines constant that map the pins and peripherals of the [MSP432P401R LaunchPad Development Kit](https://www.ti.com/lit/ds/slas826e/slas826e.pdf)
- "driverlib" : defines a thin HAL that provides easy-to-use functions to set and use [MSP432P401R LaunchPad Development Kit](https://www.ti.com/lit/ds/slas826e/slas826e.pdf) peripherals
- "grlib" : provides functions to manipulate pixels of the screen present on the [BOOSTXL-EDUMKII](https://www.ti.com/tool/BOOSTXL-EDUMKII) to draw many basic shapes 
- "LcdDriver/Crystalfontz128x128_ST7735" : defines basic functions and constants for the screen present on the [BOOSTXL-EDUMKII](https://www.ti.com/tool/BOOSTXL-EDUMKII)
- "HAL_I2C" : defines basic hardware abstraction layer for I2C between [MSP432P401R LaunchPad Development Kit](https://www.ti.com/lit/ds/slas826e/slas826e.pdf) and OPT300
- "HAL_TMP006" : defines basic hardware abstraction layer for interfacing with TMP006
- "HAL_OPT3001" : defines basic hardware abstraction layer for interfacing with OPT3001

others are standard libraries:

others are external :

- "[cJSON.h](https://github.com/DaveGamble/cJSON)" : a library that provides useful function to create, modify and read json objects.



# USAGE
* ## Setting Up The System
  In order to make the msp432 a sensing endpoint you need to :
  - Build the hardware system as shown in this picture :
  ![Image of how the hardware system should be put together](https://github.com/matteogastaldello/msp432-endpoint/blob/main/msg1257885643-11770.jpg)
  - Connect the hardware system from the usb slot on the [MSP432P401R LaunchPad Development Kit](https://www.ti.com/lit/ds/slas826e/slas826e.pdf)
    with the usb cable to the PC
  - Open  [CCS v.12.0](https://www.ti.com/tool/download/CCSTUDIO/12.0.0)  and import this github repo , then press the build button and then the flash button to flash the code inside the system.
* ## Running The System
  Now the system is showing you a menu with three possible choices.\
  The first mode to be chosen must be **PAIRING MODE**, which scans for the network using a socket searching for the platform to bind itself to.
  Then **NORMAL MODE** makes the hardware system sense the world surrounding it. In this case it uses both the light sensors and the temperature sensor provided by the
  [BOOSTXL-EDUMKI](https://www.ti.com/tool/BOOSTXL-EDUMKII).\
  Choosing **COMMUNICATION MODE**  finally makes the hardware system send the data collected in NORMAL MODE to the system it paired with at PAIRING MODE time.
  


# HOW DOES IT WORK ? 

This system works as a FSM (Finite State Machine) where each state is declared as follows :

```C
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

```

and all the states are put together in

```C
State_t states[4];  //the array of states the machine will move into
```

Each state is characterized by :
- behaviour : functions that wrap the logic of the state they belong to.
- interfaces : functions that render the screen of the display of the [BOOSTXL-EDUMKII](https://www.ti.com/tool/BOOSTXL-EDUMKII) according to the state the FSM founds itself in.


When the system starts, it shows a menu on the display where using the joystick provided by [BOOSTXL-EDUMKII](https://www.ti.com/tool/BOOSTXL-EDUMKII) the user can
scroll the choices and by pressing it selecting the one selected.

When in **PAIRING MODE** the system initiates itself as a station then tries to connect to the Acces Point (AP) specified in the code.  If this is successful it opens a  and sets it as non-blocking, binds it to the AP and then puts itself in listening then receving mode. When it receives a certain size of data in a buffer then sends an ack so the pairing is successfully done.

When in **NORMAL MODE** the system senses the world around it using both the temeprature sensor and the lux sensor provided by the [BOOSTXL-EDUMKII](https://www.ti.com/tool/BOOSTXL-EDUMKII) to fill 
with the collected data two buffers of Data_t type.

Data_t is defined as follows :

```C
// defining the number of samples each sensor collects and saves and that will be then sent in the next Json
#define DATA_BUFFER_DIM 5 
typedef struct
{
    int currentPos;
    float bufferData[DATA_BUFFER_DIM];
} Data_t;
```
When in **COMMUNICATION MODE** the system creates a json file holding the data collected in **NORMAL MODE** 
then sends it to the device it paired with at **PAIRING MODE** time.

### States Behaviours and Interfaces

- Pairing Mode

| Function | Description |
|   ---    |     ---     |
| `pairingBehaviour` | A wrapper function that sets the wifi module as a station and then scans the network to find a device to handshake with|
| `pairingInterfaceRender` | A wrapper function that clears the display of the [BOOSTXL-EDUMKII](https://www.ti.com/tool/BOOSTXL-EDUMKII) and then prints "pairing" in it|

- Normal Mode

| Function | Description |
|   ---  |     ---     |
| `normalModeBehaviour` |A wrapper function that retrieves data from both the lux sensor and the temperature sensor that a re part of the [BOOSTXL-EDUMKII](https://www.ti.com/tool/BOOSTXL-EDUMKII) and puts them in their respective dataBuffer at the first available position|
| `normalModeRender` | A wrapper function that clears the display of the [BOOSTXL-EDUMKII](https://www.ti.com/tool/BOOSTXL-EDUMKII) and prints the latest retrieved data from both the lux sensor and the temperature sensor |

- Communication Mode

| Function | Description |
|   ---    |     ---     |
|`communicationBehaviour`| A wrapper function that creates a cJSON object and fill it with two arrays filled each one with a data structure coming from one of the sensors used in normal mode to collect data |
| `tempDataInterface` | A wrapper function that clears the display of the [BOOSTXL-EDUMKII](https://www.ti.com/tool/BOOSTXL-EDUMKII) and prints in it the latest DATA_BUFFER_DIM data sensed by the temperature sensor presents in the [BOOSTXL-EDUMKII](https://www.ti.com/tool/BOOSTXL-EDUMKII) |
|`luxDataInterface`| A wrapper function that clears the display of the [BOOSTXL-EDUMKII](https://www.ti.com/tool/BOOSTXL-EDUMKII) and prints in it the latest DATA_BUFFER_DIM data sensed by the light sensor presents in the [BOOSTXL-EDUMKII](https://www.ti.com/tool/BOOSTXL-EDUMKII) |




# Problems Found During Development

- ## how is the state machine updating rendering and behaviour of the current state
Because the menu uses ADC14, the Analog to Digital Converter unit present on the [MSP432P401R LaunchPad Development Kit](https://www.ti.com/lit/ds/slas826e/slas826e.pdf) which uses interrupts to send the data it collects, then the system takes the user's choices based upon what happens in it.
Because it continues sampling from the moment it's up to the moment it's shut down ( using the function we built called ADC14_Kill ) then both the call of render()
and the firing of the behaviour is done from it.

When the ADC14 is killed then the control comes back to the infinite while cycle present in the main function that does the same thing as the ADC14, so it calls render() and makes the system behave according to the behaviour function the current state the system is in.

- ## killing and reviving of the ADC14
  Because of the shift we discussed before we found the need of shutting down the ADC14 when requested (killing action) and restartr it when needed (reviving action).

  This was at first difficult to deal with because the ADC14_disableModule at first kept giving false as result when called.
  This is how we handled the problem.

```C
  void ADC14_Kill()
{
    ADC14_disableConversion();
    while (ADC14_isBusy());
    ADC14_disableModule();
    ADC14_isOff = 1;
}
``` 

The problem was that ADC14_disableModule neede the ADC14 to be out conversion operations, so to do this we first needed to stop the process
of conversion by first calling ADC14_disableConversion() but after this the ADC14 still needed a while to stop converesion processes because at call time it already started one. So the code becomes aware of the end of this last process by busy waiting  over the condition of ADC14_isBusy().
Finally calling ADC14_disableModule() is able to take down the ADC14 unit.



```C
void ADC14_Revive()
{
    ADC14_enableModule();
    ADC14_enableConversion();
    ADC14_toggleConversionTrigger();
    ADC14_isOff = 0;
}
 ```  

ADC14_Revive does the same actions of ADC14_Kill but in a refversed order.
First of all it enables the module back calling ADC14_EnableModule(), then enalbes conversion calling ADC14_enableConversion and finally it calls 
ADC14_toggleConversionTrigger() in order to reactivate the conversion process. 
This last command is required as we read in adc14.h
- ## Setting Interrupts Priorities inside the NVIC
Because we needed to implement the function of going back to the menu from a choosen mode we decided to implement it using a button, but the interrupt of it kept being discarded as the ADC14 when active was keeping calling it's interrupt quicker than the one of the button.

We solved this porblem by acting on the NVIC (Nested Vector Interrupt Cntroller) unit in the msp432 and setting the priority of the button higher than the one of the adc14.
This made possible preemption of the button interrupt over the adc14 interrupts.

``` C
Interrupt_setPriority(INT_PORT1, 0);
Interrupt_setPriority(INT_ADC14, (uint8_t) 100);
```

- ## Setting a buffer dimension compatible according to what the memory map allowed us to do

Initially we choose DATA_BUFFER_DIM as 10 but when creating Json objects we found out that creating such objects as large as we wanted to was causing an overflow involving a reserved area of the memory (we understood this when comparing addresses of object the system was creating in debug mode and the memory map of the msp432), so we reduced the dimension of the buffer in order to fill the available space as much as we could but not enough to end up in the reserved area.
# Links to video and powerpoint


# SUPPORT
None
# AUTHORS and ACKNOWLEDGMENTS

Gastaldello Matteo\
Natali Federico\
Sabella Matteo 
## Acknowledgments

<a href="https://www.unitn.it/">
  <img src="https://github.com/matteogastaldello/msp432-endpoint/assets/95225168/032831a5-5a29-4f50-9f4e-f5689dd6f54a" width="300px">
</a>

# LICENSE
Take this code and make whatever you want, improve it, use it or break it but **DON'T** sell it

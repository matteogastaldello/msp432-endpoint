#include "simplelink.h"
#include "sl_common.h"

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

bool isConfigurationMode = false;

//tcp var
int sockfd, connfd, len;
struct sockaddr_in servaddr, cli;

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

/* Port1 ISR */
void PORT1_IRQHandler(void)
{
    /* Check which pins generated the interrupts */
    uint_fast16_t status = GPIO_getEnabledInterruptStatus(GPIO_PORT_P1);
    /* clear interrupt flag (to clear pending interrupt indicator */
    GPIO_clearInterruptFlag(GPIO_PORT_P1, status);
    /* check if we received P1.1 interrupt */
    if (status & GPIO_PIN1)
    {
        CLI_Write("Button 1.1 pressed\n");
        if (isConfigurationMode)
        {
            isConfigurationMode = false;
            CLI_Write(
                    "Returning to default state, configuration mode deactivated\n");
        }
        else
        {
            CLI_Write("Changing to Configuration Mode\n");
            isConfigurationMode = true;
        }
    }
}

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

    if (!isConfigurationMode)
        return;

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
        do{
            status = sl_Recv(connfd, buff, sizeof(buff), 0);
        }while(status == SL_EAGAIN);
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
            sl_Send(connfd, "MSP432_ACK", 11, 0);
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

/*
 * Application's entry point
 */
int main(int argc, char **argv)
{

    _i32 retVal = -1;

    retVal = initializeAppVariables();
    ASSERT_ON_ERROR(retVal);
    int firstStart = true;
    /* Stop WDT and initialize the system-clock of the MCU */
    stopWDT();
    interruptSetup();
    initClk();
    /* Configure command line interface */
    CLI_Configure();

    while (1)
    {
        if (isConfigurationMode)
        {
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

            /* Connecting to WLAN AP */
            retVal = establishConnectionWithAP();
            if (retVal < 0)
            {
                CLI_Write(" Failed to establish connection w/ an AP \n\r");
                LOOP_FOREVER();
            }

            CLI_Write(" Connection established w/ AP and IP is acquired \n\r");

            //close socket if not first start
            if (!firstStart)
                close(sockfd);

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
        else
        {
            CLI_Write("Normal Mode\n");
        }
    }

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

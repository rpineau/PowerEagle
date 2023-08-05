//
//
//  Created by Rodolphe Pineau on 3/11/2020.


#ifndef __PowerEagle_C__
#define __PowerEagle_C__

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>

#ifdef SB_MAC_BUILD
#include <unistd.h>
#endif

#ifdef SB_WIN_BUILD
#include <time.h>
#endif


#ifndef SB_WIN_BUILD
#include <curl/curl.h>
#else
#include "win_includes/curl.h"
#endif

#include <math.h>
#include <string.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <thread>
#include <ctime>
#include <cmath>
#include <future>
#include <mutex>


#include "../../licensedinterfaces/sberrorx.h"

#include "json.hpp"
using json = nlohmann::json;

#define PLUGIN_VERSION      1.0

// #define PLUGIN_DEBUG 3

#define NB_PORTS 11 // 4 pwr, 4 usb,  3 regulated 

// error codes
enum WeatherEagleErrors {PLUGIN_OK=0, NOT_CONNECTED, CANT_CONNECT, BAD_CMD_RESPONSE, COMMAND_FAILED, COMMAND_TIMEOUT, PARSE_FAILED};


class CPowerEagle
{
public:
    CPowerEagle();
    ~CPowerEagle();

    int     Connect();
    void    Disconnect(void);
    bool    IsConnected(void) { return m_bIsConnected; };
    void    getFirmware(std::string &sFirmware);

    int     getData();

    int     getSupply(double &dSupplyVolts);

    int     getPwrOut(int nIndex, double &dVolts, double &dCurrent, double &dPower, std::string &sLabel);
    int     setPwrOut(int nIndex, bool bOn);
    int     setPwrOutLabel(int nIndex, std::string sLabel);

    int     getPwrHub(int nIndex, bool &bOn, std::string &sLabel);
    int     setPwrHub(int nIndex, bool bOn);
    int     setPwrHubLabel(int nIndex, std::string sLabel);

    int     getRegOut(int nIndex, double &dVolts, double &dCurrent, double &dPower, std::string &sLabel, bool &bOn);
    void    setRegOutVal(int nIndex, double dVolts);
    void    getRegOutVal(int nIndex, double &dVolts);
    int     setRegOutOn(int nIndex,bool bOn);
    int     setRegOutLabel(int nIndex, std::string sLabel);

    int     getDarMode(bool &bOn);
    int     setDarkModeOn(bool bOn);
    
    static size_t writeFunction(void* ptr, size_t size, size_t nmemb, void* data);

    void getIpAddress(std::string &IpAddress);
    void setIpAddress(std::string IpAddress);

    void getTcpPort(int &nTcpPort);
    void setTcpPort(int nTcpPort);

#ifdef PLUGIN_DEBUG
    void  log(const std::string sLogLine);
#endif

protected:
    bool            m_bIsConnected;
    std::string     m_sFirmware;
    std::string     m_sModel;
    double          m_dFirmwareVersion;

    std::string     m_sSerialNumber;
    CURL            *m_Curl;
    std::string     m_sBaseUrl;

    std::string     m_sIpAddress;
    int             m_nTcpPort;

    double          m_dRegOut5_Volts;
    double          m_dRegOut6_Volts;
    double          m_dRegOut7_Volts;

    int             eagleEccoConnect();

    int             doGET(std::string sCmd, std::string &sResp);
    std::string     cleanupResponse(const std::string InString, char cSeparator);
    int             getModelName();
    int             getFirmwareVersion();

    std::string&    trim(std::string &str, const std::string &filter );
    std::string&    ltrim(std::string &str, const std::string &filter);
    std::string&    rtrim(std::string &str, const std::string &filter);
    std::string     findField(std::vector<std::string> &svFields, const std::string& token);


#ifdef PLUGIN_DEBUG
    // timestamp for logs
    const std::string getTimeStamp();
    std::ofstream m_sLogFile;
    std::string m_sLogfilePath;
    std::string m_sPlatform;
#endif


};

#endif //__PowerEagle_C__

//
//  Primaluce Eagle power X2 plugin
//
//  Created by Rodolphe Pineau on 3/11/2020.


#include "PowerEagle.h"


CPowerEagle::CPowerEagle()
{
    // set some sane values
    m_bIsConnected = false;
    m_sIpAddress.clear();
    m_nTcpPort = 0;
#ifdef PLUGIN_DEBUG
#if defined(SB_WIN_BUILD)
    m_sLogfilePath = getenv("HOMEDRIVE");
    m_sLogfilePath += getenv("HOMEPATH");
    m_sLogfilePath += "\\X2_PowerEagle.txt";
    m_sPlatform = "Windows";
#elif defined(SB_LINUX_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/X2_PowerEagle.txt";
    m_sPlatform = "Linux";
#elif defined(SB_MAC_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/X2_PowerEagle.txt";
    m_sPlatform = "macOS";
#endif
    m_sLogFile.open(m_sLogfilePath, std::ios::out |std::ios::trunc);
#endif

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [CPowerEagle] Version " << std::fixed << std::setprecision(2) << PLUGIN_VERSION << " build " << __DATE__ << " " << __TIME__ << " on "<< m_sPlatform << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [CPowerEagle] Constructor Called." << std::endl;
    m_sLogFile.flush();
#endif

    curl_global_init(CURL_GLOBAL_ALL);
    m_Curl = nullptr;

}

CPowerEagle::~CPowerEagle()
{
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [~CPowerEagle] Called." << std::endl;
    m_sLogFile.flush();
#endif

    if(m_bIsConnected) {
        Disconnect();
    }

    curl_global_cleanup();

#ifdef    PLUGIN_DEBUG
    // Close LogFile
    if(m_sLogFile.is_open())
        m_sLogFile.close();
#endif
}

int CPowerEagle::Connect()
{
    int nErr = SB_OK;
    std::string sDummy;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Called." << std::endl;
    m_sLogFile.flush();
#endif

    if(m_sIpAddress.empty())
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Base url = " << m_sBaseUrl << std::endl;
    m_sLogFile.flush();
#endif

    m_Curl = curl_easy_init();

    if(!m_Curl) {
        m_Curl = nullptr;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] CURL init failed" << std::endl;
        m_sLogFile.flush();
#endif
        return ERR_CMDFAILED;
    }

    m_bIsConnected = true;
    nErr = getData();
    if (nErr) {
        curl_easy_cleanup(m_Curl);
        m_Curl = nullptr;
        m_bIsConnected = false;
        return nErr;
    }
    return nErr;
}


void CPowerEagle::Disconnect()
{

    if(m_bIsConnected) {
        curl_easy_cleanup(m_Curl);
        m_Curl = nullptr;
        m_bIsConnected = false;

#ifdef PLUGIN_DEBUG
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Disconnect] Disconnected." << std::endl;
        m_sLogFile.flush();
#endif
    }
}

void CPowerEagle::getFirmware(std::string &sFirmware)
{
    sFirmware.assign(m_sFirmware);
}


int CPowerEagle::doGET(std::string sCmd, std::string &sResp)
{
    int nErr = PLUGIN_OK;
    CURLcode res;
    std::string response_string;
    std::string header_string;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [doGET] Called." << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [doGET] Doing get on " << sCmd << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [doGET] Full get url " << (m_sBaseUrl+sCmd) << std::endl;
    m_sLogFile.flush();
#endif

    res = curl_easy_setopt(m_Curl, CURLOPT_URL, (m_sBaseUrl+sCmd).c_str());
    if(res != CURLE_OK) { // if this fails no need to keep going
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [doGET] curl_easy_setopt Error = " << res << std::endl;
        m_sLogFile.flush();
#endif
        return ERR_CMDFAILED;
    }

    curl_easy_setopt(m_Curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(m_Curl, CURLOPT_POST, 0L);
    curl_easy_setopt(m_Curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_Curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(m_Curl, CURLOPT_HEADERDATA, &header_string);
    curl_easy_setopt(m_Curl, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt(m_Curl, CURLOPT_CONNECTTIMEOUT, 3); // 3 seconds timeout on connect

    // Perform the request, res will get the return code
    res = curl_easy_perform(m_Curl);
    // Check for errors
    if(res != CURLE_OK) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [doGET] curl_easy_perform Error = " << res << std::endl;
        m_sLogFile.flush();
#endif
        if(res == CURLE_COULDNT_CONNECT)
            return ERR_COMMNOLINK;
        return ERR_CMDFAILED;
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [doGET] response = " << response_string << std::endl;
    m_sLogFile.flush();
#endif

    sResp.assign(cleanupResponse(response_string,'\n'));

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [doGET] sResp = " << sResp << std::endl;
    m_sLogFile.flush();
#endif
    return nErr;
}

size_t CPowerEagle::writeFunction(void* ptr, size_t size, size_t nmemb, void* data)
{
    ((std::string*)data)->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

std::string CPowerEagle::cleanupResponse(const std::string InString, char cSeparator)
{
    std::string sSegment;
    std::vector<std::string> svFields;

    if(!InString.size()) {
#ifdef PLUGIN_DEBUG
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [cleanupResponse] InString is empty." << std::endl;
        m_sLogFile.flush();
#endif
        return InString;
    }


    std::stringstream ssTmp(InString);

    svFields.clear();
    // split the string into vector elements
    while(std::getline(ssTmp, sSegment, cSeparator))
    {
        if(sSegment.find("<!-") == -1)
            svFields.push_back(sSegment);
    }

    if(svFields.size()==0) {
        return std::string("");
    }

    sSegment.clear();
    for( std::string s : svFields)
        sSegment.append(trim(s,"\n\r "));
    return sSegment;
}

#pragma mark - Getter / Setter

int CPowerEagle::getData()
{
    int nErr = PLUGIN_OK;
    json jResp;
    std::string response_string;
    std::string PowerEagleError;

    if(!m_bIsConnected || !m_Curl)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] Called." << std::endl;
    m_sLogFile.flush();
#endif
    if(m_sFirmware.empty()) {
        // do http GET request to local server to get firmware info
        nErr = doGET("/getinfo", response_string);
        if(!nErr) {
            // process response_string
            try {
                jResp = json::parse(response_string);
                if(jResp.at("result").get<std::string>() == "OK") {
                    m_sFirmware = jResp.at("firmwareversion").get<std::string>();
                    m_sSerialNumber = jResp.at("firmwaserialnumberreversion").get<std::string>();
                }
                else {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] getinfo error : " << jResp << std::endl;
                    m_sLogFile.flush();
#endif
                }
            }
            catch (json::exception& e) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] json exception : " << e.what() << " - " << e.id << std::endl;
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] json exception response : " << response_string << std::endl;
                m_sLogFile.flush();
#endif
            }
        }
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_sFirmware        : " << m_sFirmware << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getData] m_sSerialNumber    : " << m_sSerialNumber << std::endl;
    m_sLogFile.flush();
#endif

    return nErr;
}

int CPowerEagle::getSupply(double &dSupplyVolts)
{
    int nErr = PLUGIN_OK;
    json jResp;
    std::string response_string;
    std::string PowerEagleError;

    if(!m_bIsConnected || !m_Curl)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getSupply] Called." << std::endl;
    m_sLogFile.flush();
#endif

    nErr = doGET("/getsupply", response_string);
    if(!nErr) {
        // process response_string
        try {
            jResp = json::parse(response_string);
            if(jResp.at("result").get<std::string>() == "OK") {
                dSupplyVolts = jResp.at("supply").get<std::double_t>();
            }
            else {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getSupply] getinfo error : " << jResp << std::endl;
                m_sLogFile.flush();
#endif
            }
        }
        catch (json::exception& e) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getSupply] json exception : " << e.what() << " - " << e.id << std::endl;
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getSupply] json exception response : " << response_string << std::endl;
            m_sLogFile.flush();
#endif
        }
    }

    return nErr;
}

int CPowerEagle::getPwrOut(int nIndex, double &dVolts, double &dCurrent, double &dPower, std::string &sLabel)
{
    int nErr = PLUGIN_OK;
    json jResp;
    std::string response_string;
    std::string PowerEagleError;
    std::stringstream ssTmp;

    if(!m_bIsConnected || !m_Curl)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getPwrOut] Called." << std::endl;
    m_sLogFile.flush();
#endif
    // <portidx>=1,2,3,4
    if(nIndex<1 || nIndex>4)
        return ERR_INDEX_OUT_OF_RANGE;
    
    ssTmp << "/getpwrout?idx=" << nIndex;
    nErr = doGET(ssTmp.str(), response_string);
    if(!nErr) {
        // process response_string
        try {
            jResp = json::parse(response_string);
            if(jResp.at("result").get<std::string>() == "OK") {
                dVolts = jResp.at("voltage").get<std::double_t>();
                dCurrent = jResp.at("current").get<std::double_t>();
                dPower = jResp.at("power").get<std::double_t>();
                sLabel = jResp.at("label").get<std::string>();
            }
            else {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getPwrOut] getinfo error : " << jResp << std::endl;
                m_sLogFile.flush();
#endif
            }
        }
        catch (json::exception& e) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getPwrOut] json exception : " << e.what() << " - " << e.id << std::endl;
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getPwrOut] json exception response : " << response_string << std::endl;
            m_sLogFile.flush();
#endif
        }
    }
    return nErr;
}

int CPowerEagle::setPwrOut(int nIndex, bool bOn)
{
    int nErr = PLUGIN_OK;
    json jResp;
    std::string response_string;
    std::string PowerEagleError;
    std::stringstream ssTmp;

    if(!m_bIsConnected || !m_Curl)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setPwrOut] Called." << std::endl;
    m_sLogFile.flush();
#endif
    // <portidx>=1,2,3,4
    if(nIndex<1 || nIndex>4)
        return ERR_INDEX_OUT_OF_RANGE;

    ssTmp << "/setpwrout?idx=" << nIndex << "&state=" << (bOn?1:0);
    nErr = doGET(ssTmp.str(), response_string);
    if(!nErr) {
        // process response_string
        try {
            jResp = json::parse(response_string);
            if(jResp.at("result").get<std::string>() != "OK") {
                return ERR_CMDFAILED;
            }
            else {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setPwrOut] getinfo error : " << jResp << std::endl;
                m_sLogFile.flush();
#endif
            }
        }
        catch (json::exception& e) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setPwrOut] json exception : " << e.what() << " - " << e.id << std::endl;
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setPwrOut] json exception response : " << response_string << std::endl;
            m_sLogFile.flush();
#endif
        }
    }

    return nErr;
}

int CPowerEagle::setPwrOutLabel(int nIndex, std::string sLabel)
{
    int nErr = PLUGIN_OK;
    json jResp;
    std::string response_string;
    std::string PowerEagleError;
    std::stringstream ssTmp;
    
    if(!m_bIsConnected || !m_Curl)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setPwrOutLabel] Called." << std::endl;
    m_sLogFile.flush();
#endif
    
    // <portidx>=1,2,3,4
    if(nIndex<1 || nIndex>4)
        return ERR_INDEX_OUT_OF_RANGE;

    ssTmp << "/setpwrout?idx=" << nIndex << "&label=" << sLabel;
    nErr = doGET(ssTmp.str(), response_string);
    if(!nErr) {
        // process response_string
        try {
            jResp = json::parse(response_string);
            if(jResp.at("result").get<std::string>() != "OK") {
                return ERR_CMDFAILED;
            }
            else {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setPwrOutLabel] getinfo error : " << jResp << std::endl;
                m_sLogFile.flush();
#endif
            }
        }
        catch (json::exception& e) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setPwrOutLabel] json exception : " << e.what() << " - " << e.id << std::endl;
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setPwrOutLabel] json exception response : " << response_string << std::endl;
            m_sLogFile.flush();
#endif
        }
    }

    return nErr;
}

int CPowerEagle::getPwrHub(int nIndex, bool &bOn, std::string &sLabel)
{
    int nErr = PLUGIN_OK;
    json jResp;
    std::string response_string;
    std::string PowerEagleError;
    std::stringstream ssTmp;
    
    if(!m_bIsConnected || !m_Curl)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getPwrHub] Called." << std::endl;
    m_sLogFile.flush();
#endif
    // <usb_portidx>=1,2,3,4
    if(nIndex<1 || nIndex>4)
        return ERR_INDEX_OUT_OF_RANGE;

    ssTmp << "/getpwrhub?idx=" << nIndex;
    nErr = doGET(ssTmp.str(), response_string);
    if(!nErr) {
        // process response_string
        try {
            jResp = json::parse(response_string);
            if(jResp.at("result").get<std::string>() == "OK") {
                bOn = (jResp.at("power").get<int>()==1);
                sLabel = jResp.at("label").get<std::string>();
            }
            else {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getPwrHub] getinfo error : " << jResp << std::endl;
                m_sLogFile.flush();
#endif
            }
        }
        catch (json::exception& e) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getPwrHub] json exception : " << e.what() << " - " << e.id << std::endl;
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getPwrHub] json exception response : " << response_string << std::endl;
            m_sLogFile.flush();
#endif
        }
    }
    
    return nErr;
}

int CPowerEagle::setPwrHub(int nIndex, bool bOn)
{
    int nErr = PLUGIN_OK;
    json jResp;
    std::string response_string;
    std::string PowerEagleError;
    std::stringstream ssTmp;
    
    if(!m_bIsConnected || !m_Curl)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setPwrHub] Called." << std::endl;
    m_sLogFile.flush();
#endif

    // <usb_portidx>=1,2,3,4
    if(nIndex<1 || nIndex>4)
        return ERR_INDEX_OUT_OF_RANGE;

    ssTmp << "/setpwrhub?idx=" << nIndex << "&state=" << (bOn?1:0);
    nErr = doGET(ssTmp.str(), response_string);
    if(!nErr) {
        // process response_string
        try {
            jResp = json::parse(response_string);
            if(jResp.at("result").get<std::string>() != "OK") {
                return ERR_CMDFAILED;
            }
            else {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setPwrHub] getinfo error : " << jResp << std::endl;
                m_sLogFile.flush();
#endif
            }
        }
        catch (json::exception& e) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setPwrHub] json exception : " << e.what() << " - " << e.id << std::endl;
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setPwrHub] json exception response : " << response_string << std::endl;
            m_sLogFile.flush();
#endif
        }
    }

    return nErr;
}

int CPowerEagle::setPwrHubLabel(int nIndex, std::string sLabel)
{
    int nErr = PLUGIN_OK;
    json jResp;
    std::string response_string;
    std::string PowerEagleError;
    std::stringstream ssTmp;
    
    if(!m_bIsConnected || !m_Curl)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setPwrHubLabel] Called." << std::endl;
    m_sLogFile.flush();
#endif

    // <usb_portidx>=1,2,3,4
    if(nIndex<1 || nIndex>4)
        return ERR_INDEX_OUT_OF_RANGE;

    ssTmp << "/setpwrhub?idx=" << nIndex << "&label=" << sLabel;
    nErr = doGET(ssTmp.str(), response_string);
    if(!nErr) {
        // process response_string
        try {
            jResp = json::parse(response_string);
            if(jResp.at("result").get<std::string>() != "OK") {
                return ERR_CMDFAILED;
            }
            else {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setPwrHubLabel] getinfo error : " << jResp << std::endl;
                m_sLogFile.flush();
#endif
            }
        }
        catch (json::exception& e) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setPwrHubLabel] json exception : " << e.what() << " - " << e.id << std::endl;
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setPwrHubLabel] json exception response : " << response_string << std::endl;
            m_sLogFile.flush();
#endif
        }
    }

    return nErr;
}

int CPowerEagle::getRegOut(int nIndex, double &dVolts, double &dCurrent, double &dPower, std::string &sLabel)
{
    int nErr = PLUGIN_OK;
    json jResp;
    std::string response_string;
    std::string PowerEagleError;
    std::stringstream ssTmp;
    
    if(!m_bIsConnected || !m_Curl)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getRegOut] Called." << std::endl;
    m_sLogFile.flush();
#endif
    // <rca_portidx>=5,6,7;
    if(nIndex<5 || nIndex>7)
        return ERR_INDEX_OUT_OF_RANGE;

    ssTmp << "/getregout?idx=" << nIndex;
    nErr = doGET(ssTmp.str(), response_string);
    if(!nErr) {
        // process response_string
        try {
            jResp = json::parse(response_string);
            if(jResp.at("result").get<std::string>() == "OK") {
                dVolts = jResp.at("voltage").get<std::double_t>();
                dCurrent = jResp.at("current").get<std::double_t>();
                dPower = jResp.at("power").get<std::double_t>();
                sLabel = jResp.at("label").get<std::string>();
            }
            else {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getRegOut] getinfo error : " << jResp << std::endl;
                m_sLogFile.flush();
#endif
            }
        }
        catch (json::exception& e) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getRegOut] json exception : " << e.what() << " - " << e.id << std::endl;
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getRegOut] json exception response : " << response_string << std::endl;
            m_sLogFile.flush();
#endif
        }
    }

    return nErr;
}

int CPowerEagle::setRegOut(int nIndex, double dVolts)
{
    int nErr = PLUGIN_OK;
    json jResp;
    std::string response_string;
    std::string PowerEagleError;
    std::stringstream ssTmp;
    
    if(!m_bIsConnected || !m_Curl)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setRegOut] Called." << std::endl;
    m_sLogFile.flush();
#endif
    // <rca_portidx>=5,6,7;
    if(nIndex<5 || nIndex>7)
        return ERR_INDEX_OUT_OF_RANGE;

    ssTmp << "/setregout?idx=" << nIndex << "&volt=" << std::fixed << std::setprecision(1) << dVolts;
    nErr = doGET(ssTmp.str(), response_string);
    if(!nErr) {
        // process response_string
        try {
            jResp = json::parse(response_string);
            if(jResp.at("result").get<std::string>() != "OK") {
                return ERR_CMDFAILED;
            }
            else {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setRegOut] getinfo error : " << jResp << std::endl;
                m_sLogFile.flush();
#endif
            }
        }
        catch (json::exception& e) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setRegOut] json exception : " << e.what() << " - " << e.id << std::endl;
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setRegOut] json exception response : " << response_string << std::endl;
            m_sLogFile.flush();
#endif
        }
    }

    return nErr;
}

int CPowerEagle::setRegOutLabel(int nIndex, std::string sLabel)
{
    int nErr = PLUGIN_OK;
    json jResp;
    std::string response_string;
    std::string PowerEagleError;
    std::stringstream ssTmp;

    if(!m_bIsConnected || !m_Curl)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setRegOutLabel] Called." << std::endl;
    m_sLogFile.flush();
#endif

    // <rca_portidx>=5,6,7;
    if(nIndex<5 || nIndex>7)
        return ERR_INDEX_OUT_OF_RANGE;

    ssTmp << "/setregout?idx=" << nIndex << "&label=" << sLabel;
    nErr = doGET(ssTmp.str(), response_string);
    if(!nErr) {
        // process response_string
        try {
            jResp = json::parse(response_string);
            if(jResp.at("result").get<std::string>() != "OK") {
                return ERR_CMDFAILED;
            }
            else {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setRegOutLabel] getinfo error : " << jResp << std::endl;
                m_sLogFile.flush();
#endif
            }
        }
        catch (json::exception& e) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setRegOutLabel] json exception : " << e.what() << " - " << e.id << std::endl;
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setRegOutLabel] json exception response : " << response_string << std::endl;
            m_sLogFile.flush();
#endif
        }
    }

    return nErr;
}


int CPowerEagle::getDarMode(bool &bOn)
{    int nErr = PLUGIN_OK;
    json jResp;
    std::string response_string;
    std::string PowerEagleError;
    bool bDarkModeState;
    
    if(!m_bIsConnected || !m_Curl)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [isDarModeOn] Called." << std::endl;
    m_sLogFile.flush();
#endif


    nErr = doGET("/getdarkmode", response_string);
    if(!nErr) {
        // process response_string
        try {
            jResp = json::parse(response_string);
            if(jResp.at("result").get<std::string>() != "OK") {
                bDarkModeState = jResp.at("darkModeActive").get<int>()==1?true:false;
            }
            else {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [isDarModeOn] getinfo error : " << jResp << std::endl;
                m_sLogFile.flush();
#endif
            }
        }
        catch (json::exception& e) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [isDarModeOn] json exception : " << e.what() << " - " << e.id << std::endl;
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [isDarModeOn] json exception response : " << response_string << std::endl;
            m_sLogFile.flush();
#endif
        }
    }

    return nErr;

}

int CPowerEagle::setDarkModeOn(bool bOn)
{
    int nErr = PLUGIN_OK;
    json jResp;
    std::string response_string;
    std::string PowerEagleError;
    std::stringstream ssTmp;

    if(!m_bIsConnected || !m_Curl)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setDarkModeOn] Called." << std::endl;
    m_sLogFile.flush();
#endif


    ssTmp << "/setdarkmode?active=" << (bOn?1:0);
    nErr = doGET(ssTmp.str(), response_string);
    if(!nErr) {
        // process response_string
        try {
            jResp = json::parse(response_string);
            if(jResp.at("result").get<std::string>() != "OK") {
                return ERR_CMDFAILED;
            }
            else {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setDarkModeOn] getinfo error : " << jResp << std::endl;
                m_sLogFile.flush();
#endif
            }
        }
        catch (json::exception& e) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setDarkModeOn] json exception : " << e.what() << " - " << e.id << std::endl;
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setDarkModeOn] json exception response : " << response_string << std::endl;
            m_sLogFile.flush();
#endif
        }
    }

    return nErr;
}


void CPowerEagle::getIpAddress(std::string &IpAddress)
{
    IpAddress = m_sIpAddress;
}

void CPowerEagle::setIpAddress(std::string IpAddress)
{
    m_sIpAddress = IpAddress;
    if(m_nTcpPort!=80 && m_nTcpPort!=443) {
        m_sBaseUrl = "http://"+m_sIpAddress+":"+std::to_string(m_nTcpPort);
    }
    else if (m_nTcpPort==443) {
        m_sBaseUrl = "https://"+m_sIpAddress;
    }
    else {
        m_sBaseUrl = "http://"+m_sIpAddress;
    }
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setIpAddress] New base url : " << m_sBaseUrl << std::endl;
    m_sLogFile.flush();
#endif

}

void CPowerEagle::getTcpPort(int &nTcpPort)
{
    nTcpPort = m_nTcpPort;
}

void CPowerEagle::setTcpPort(int nTcpPort)
{
    m_nTcpPort = nTcpPort;
    if(m_nTcpPort!=80) {
        m_sBaseUrl = "http://"+m_sIpAddress+":"+std::to_string(m_nTcpPort);
    }
    else {
        m_sBaseUrl = "http://"+m_sIpAddress;
    }
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setTcpPort] New base url : " << m_sBaseUrl << std::endl;
    m_sLogFile.flush();
#endif
}

std::string& CPowerEagle::trim(std::string &str, const std::string& filter )
{
    return ltrim(rtrim(str, filter), filter);
}

std::string& CPowerEagle::ltrim(std::string& str, const std::string& filter)
{
    str.erase(0, str.find_first_not_of(filter));
    return str;
}

std::string& CPowerEagle::rtrim(std::string& str, const std::string& filter)
{
    str.erase(str.find_last_not_of(filter) + 1);
    return str;
}

std::string CPowerEagle::findField(std::vector<std::string> &svFields, const std::string& token)
{
    for(int i=0; i<svFields.size(); i++){
        if(svFields[i].find(token)!= -1) {
            return svFields[i];
        }
    }
    return std::string();
}


#ifdef PLUGIN_DEBUG
void CPowerEagle::log(const std::string sLogLine)
{
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [log] " << sLogLine << std::endl;
    m_sLogFile.flush();

}

const std::string CPowerEagle::getTimeStamp()
{
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}
#endif


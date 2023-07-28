#include "x2powercontrol.h"

X2PowerControl::X2PowerControl(const char* pszDisplayName,
										const int& nInstanceIndex,
										SerXInterface						* pSerXIn,
										TheSkyXFacadeForDriversInterface	* pTheSkyXIn,
										SleeperInterface					* pSleeperIn,
										BasicIniUtilInterface				* pIniUtilIn,
										LoggerInterface						* pLoggerIn,
										MutexInterface						* pIOMutexIn,
										TickCountInterface					* pTickCountIn):m_bLinked(0)
{
    char szIpAddress[128];
	std::string sLabel;

	m_pTheSkyXForMounts = pTheSkyXIn;
	m_pSleeper = pSleeperIn;
	m_pIniUtil = pIniUtilIn;
	m_pIOMutex = pIOMutexIn;
	m_pTickCount = pTickCountIn;

	m_nISIndex = nInstanceIndex;

    if (m_pIniUtil) {
        m_pIniUtil->readString(PARENT_KEY, CHILD_KEY_IP, "127.0.0.1", szIpAddress, 128);
        m_PowerPorts.setIpAddress(std::string(szIpAddress));
        m_PowerPorts.setTcpPort(m_pIniUtil->readInt(PARENT_KEY, CHILD_KEY_PORT, 1380));
	}
}

X2PowerControl::~X2PowerControl()
{
	//Delete objects used through composition
	if (GetTheSkyXFacadeForDrivers())
		delete GetTheSkyXFacadeForDrivers();
	if (GetSleeper())
		delete GetSleeper();
	if (GetSimpleIniUtil())
		delete GetSimpleIniUtil();
	if (GetMutex())
		delete GetMutex();
}

int X2PowerControl::establishLink(void)
{
	int nErr = SB_OK;


    m_bLinked = true;

	return nErr;
}

int X2PowerControl::terminateLink(void)
{
	m_bLinked = false;

    return SB_OK;
}

bool X2PowerControl::isLinked() const
{
	return m_bLinked;
}


void X2PowerControl::driverInfoDetailedInfo(BasicStringInterface& str) const
{
    str = "Eagle Manager X";
}

double X2PowerControl::driverInfoVersion(void) const
{
    return PLUGIN_VERSION;
}

void X2PowerControl::deviceInfoNameShort(BasicStringInterface& str) const
{
    str = "Eagle Manager X";
}

void X2PowerControl::deviceInfoNameLong(BasicStringInterface& str) const
{
    deviceInfoNameShort(str);
}

void X2PowerControl::deviceInfoDetailedDescription(BasicStringInterface& str) const
{
    deviceInfoNameShort(str);
}

void X2PowerControl::deviceInfoFirmwareVersion(BasicStringInterface& str)
{
    if(m_bLinked) {
        str = "N/A";
        std::string sFirmware;
        X2MutexLocker ml(GetMutex());
        m_PowerPorts.getFirmware(sFirmware);
        str = sFirmware.c_str();
    }
    else
        str = "N/A";

}

void X2PowerControl::deviceInfoModel(BasicStringInterface& str)
{
    deviceInfoNameShort(str);
}

int X2PowerControl::queryAbstraction(const char* pszName, void** ppVal)
{
	*ppVal = NULL;

    if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
        *ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);
    else if (!strcmp(pszName, X2GUIEventInterface_Name))
        *ppVal = dynamic_cast<X2GUIEventInterface*>(this);
    else if (!strcmp(pszName, CircuitLabelsInterface_Name))
        *ppVal = dynamic_cast<CircuitLabelsInterface*>(this);
    else if (!strcmp(pszName, SetCircuitLabelsInterface_Name))
        *ppVal = dynamic_cast<SetCircuitLabelsInterface*>(this);

	return 0;
}

#pragma mark - UI binding

int X2PowerControl::execModalSettingsDialog()
{
    int nErr = SB_OK;
    X2ModalUIUtil uiutil(this, GetTheSkyXFacadeForDrivers());
    X2GUIInterface*                    ui = uiutil.X2UI();
    X2GUIExchangeInterface*            dx = NULL;//Comes after ui is loaded
    bool bPressedOK = false;
    double dPortVolts;
    double dPortAmps;
    double dPortPower;
    std::string sTmp;
    bool bDarkMode;
    std::stringstream ssTmp;
    
    if (NULL == ui)
        return ERR_POINTER;

    nErr = ui->loadUserInterface("PowerEagle.ui", deviceType(), m_nISIndex);
    if (nErr)
        return nErr;

    if (NULL == (dx = uiutil.X2DX()))
        return ERR_POINTER;

    if(m_bLinked) {
        dx->setEnabled("pushButton",true);
        dx->setEnabled("pushButton_2",true);

        nErr = m_PowerPorts.getDarMode(bDarkMode);
        if(bDarkMode)
            dx->setChecked("radioButton_3", 1);
        else
            dx->setChecked("radioButton_4", 1);


        nErr =  m_PowerPorts.getSupply(dPortVolts);
        ssTmp << std::fixed << std::setprecision(1) << dPortVolts << "V";
        dx->setText("mainVoltage", ssTmp.str().c_str());

        std::stringstream().swap(ssTmp);
        nErr = m_PowerPorts.getPwrOut(1, dPortVolts, dPortAmps, dPortPower, sTmp);
        ssTmp << std::fixed << std::setprecision(1) << dPortVolts << " V / " << dPortAmps << " A / " << dPortPower << " W";
        dx->setText("powerPort1", ssTmp.str().c_str());

        std::stringstream().swap(ssTmp);
        nErr = m_PowerPorts.getPwrOut(2, dPortVolts, dPortAmps, dPortPower, sTmp);
        ssTmp << std::fixed << std::setprecision(1) << dPortVolts << " V / " << dPortAmps << " A / " << dPortPower << " W";
        dx->setText("powerPort2", ssTmp.str().c_str());

        std::stringstream().swap(ssTmp);
        nErr = m_PowerPorts.getPwrOut(3, dPortVolts, dPortAmps, dPortPower, sTmp);
        ssTmp << std::fixed << std::setprecision(1) << dPortVolts << " V / " << dPortAmps << " A / " << dPortPower << " W";
        dx->setText("powerPort3", ssTmp.str().c_str());

        std::stringstream().swap(ssTmp);
        nErr = m_PowerPorts.getPwrOut(4, dPortVolts, dPortAmps, dPortPower, sTmp);
        ssTmp << std::fixed << std::setprecision(1) << dPortVolts << " V / " << dPortAmps << " A / " << dPortPower << " W";
        dx->setText("powerPort4", ssTmp.str().c_str());

        std::stringstream().swap(ssTmp);
        nErr = m_PowerPorts.getRegOut(5, dPortVolts, dPortAmps, dPortPower, sTmp);
        ssTmp << std::fixed << std::setprecision(1) << dPortVolts << " V / " << dPortAmps << " A / " << dPortPower << " W";
        dx->setText("regPort1", ssTmp.str().c_str());
        dx->setPropertyDouble("regPort1Volts", "value", dPortVolts);
        
        std::stringstream().swap(ssTmp);
        nErr = m_PowerPorts.getRegOut(6, dPortVolts, dPortAmps, dPortPower, sTmp);
        ssTmp << std::fixed << std::setprecision(1) << dPortVolts << " V / " << dPortAmps << " A / " << dPortPower << " W";
        dx->setText("regPort2", ssTmp.str().c_str());
        dx->setPropertyDouble("regPort2Volts", "value", dPortVolts);

        std::stringstream().swap(ssTmp);
        nErr = m_PowerPorts.getRegOut(7, dPortVolts, dPortAmps, dPortPower, sTmp);
        ssTmp << std::fixed << std::setprecision(1) << dPortVolts << " V / " << dPortAmps << " A / " << dPortPower << " W";
        dx->setText("regPort3", ssTmp.str().c_str());
        dx->setPropertyDouble("regPort3Volts", "value", dPortVolts);

    }
    else {
        dx->setEnabled("pushButton",false);
        dx->setEnabled("pushButton_2",false);
    }

    //Display the user interface
    if ((nErr = ui->exec(bPressedOK)))
        return nErr;

    //Retreive values from the user interface
    if (bPressedOK) {
    }
    return nErr;

}

void X2PowerControl::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
{
    int nErr;
    double dPortVolts;
    double dPortAmps;
    double dPortPower;
    std::string sTmp;
    std::stringstream ssTmp;

    if (!strcmp(pszEvent, "on_timer")) {
        nErr =  m_PowerPorts.getSupply(dPortVolts);
        ssTmp << std::fixed << std::setprecision(1) << dPortVolts << "V";
        uiex->setText("mainVoltage", ssTmp.str().c_str());

        std::stringstream().swap(ssTmp);
        nErr = m_PowerPorts.getPwrOut(1, dPortVolts, dPortAmps, dPortPower, sTmp);
        ssTmp << std::fixed << std::setprecision(1) << dPortVolts << " V / " << dPortAmps << " A / " << dPortPower << " W";
        uiex->setText("powerPort1", ssTmp.str().c_str());

        std::stringstream().swap(ssTmp);
        nErr = m_PowerPorts.getPwrOut(2, dPortVolts, dPortAmps, dPortPower, sTmp);
        ssTmp << std::fixed << std::setprecision(1) << dPortVolts << " V / " << dPortAmps << " A / " << dPortPower << " W";
        uiex->setText("powerPort2", ssTmp.str().c_str());

        std::stringstream().swap(ssTmp);
        nErr = m_PowerPorts.getPwrOut(3, dPortVolts, dPortAmps, dPortPower, sTmp);
        ssTmp << std::fixed << std::setprecision(1) << dPortVolts << " V / " << dPortAmps << " A / " << dPortPower << " W";
        uiex->setText("powerPort3", ssTmp.str().c_str());

        std::stringstream().swap(ssTmp);
        nErr = m_PowerPorts.getPwrOut(4, dPortVolts, dPortAmps, dPortPower, sTmp);
        ssTmp << std::fixed << std::setprecision(1) << dPortVolts << " V / " << dPortAmps << " A / " << dPortPower << " W";
        uiex->setText("powerPort4", ssTmp.str().c_str());

        std::stringstream().swap(ssTmp);
        nErr = m_PowerPorts.getRegOut(5, dPortVolts, dPortAmps, dPortPower, sTmp);
        ssTmp << std::fixed << std::setprecision(1) << dPortVolts << " V / " << dPortAmps << " A / " << dPortPower << " W";
        uiex->setText("regPort1", ssTmp.str().c_str());
        
        std::stringstream().swap(ssTmp);
        nErr = m_PowerPorts.getRegOut(6, dPortVolts, dPortAmps, dPortPower, sTmp);
        ssTmp << std::fixed << std::setprecision(1) << dPortVolts << " V / " << dPortAmps << " A / " << dPortPower << " W";
        uiex->setText("regPort2", ssTmp.str().c_str());

        std::stringstream().swap(ssTmp);
        nErr = m_PowerPorts.getRegOut(7, dPortVolts, dPortAmps, dPortPower, sTmp);
        ssTmp << std::fixed << std::setprecision(1) << dPortVolts << " V / " << dPortAmps << " A / " << dPortPower << " W";
        uiex->setText("regPort3", ssTmp.str().c_str());
    }


    else if (!strcmp(pszEvent, "on_pushButton_3_clicked")) {
        uiex->propertyDouble("regPort1Volts", "value", dPortVolts);
        nErr = m_PowerPorts.setRegOut(5, dPortVolts);
    }

    else if (!strcmp(pszEvent, "on_pushButton_4_clicked")) {
        uiex->propertyDouble("regPort2Volts", "value", dPortVolts);
        nErr = m_PowerPorts.setRegOut(6, dPortVolts);
    }
    else if (!strcmp(pszEvent, "on_pushButton_5_clicked")) {
        uiex->propertyDouble("regPort3Volts", "value", dPortVolts);
        nErr = m_PowerPorts.setRegOut(7, dPortVolts);
    }
    
    else if (!strcmp(pszEvent, "on_radioButton_3_clicked")) {
        nErr = m_PowerPorts.setDarkModeOn(true);
    }
    else if (!strcmp(pszEvent, "on_radioButton_4_clicked")) {
        nErr = m_PowerPorts.setDarkModeOn(false);
    }
    
}

int X2PowerControl::numberOfCircuits(int& nNumber)
{
	return NB_PORTS;
}

int X2PowerControl::circuitState(const int& nIndex, bool& bZeroForOffOneForOn)
{
	int nErr = SB_OK;
    int nNewIndex;
    double dVolts;
    double dCurrent;
    double dPower;
    bool bOn;
    std::string sLabel;

    if(!m_bLinked)
        return ERR_NOLINK;
    switch(nIndex) {
            // power port <portidx>=1,2,3,4
        case 0:
            nNewIndex = 1;
            nErr = m_PowerPorts.getPwrOut(nNewIndex, dVolts, dCurrent, dPower, sLabel);
            bZeroForOffOneForOn = (dVolts>0.0);
            break;
        case 1:
            nNewIndex = 2;
            nErr = m_PowerPorts.getPwrOut(nNewIndex, dVolts, dCurrent, dPower, sLabel);
            bZeroForOffOneForOn = (dVolts>0.0);
            break;
        case 2:
            nNewIndex = 3;
            nErr = m_PowerPorts.getPwrOut(nNewIndex, dVolts, dCurrent, dPower, sLabel);
            bZeroForOffOneForOn = (dVolts>0.0);
            break;
        case 3:
            nNewIndex = 4;
            nErr = m_PowerPorts.getPwrOut(nNewIndex, dVolts, dCurrent, dPower, sLabel);
            bZeroForOffOneForOn = (dVolts>0.0);
            break;
            /// usb port  <usb_portidx>=1,2,3,4
        case 4:
            nNewIndex = 1;
            nErr = m_PowerPorts.getPwrHub(nNewIndex, bOn, sLabel);
            bZeroForOffOneForOn = bOn;
            break;
        case 5:
            nNewIndex = 2;
            nErr = m_PowerPorts.getPwrHub(nNewIndex, bOn, sLabel);
            bZeroForOffOneForOn = bOn;
            break;
        case 6:
            nNewIndex = 3;
            nErr =  m_PowerPorts.getPwrHub(nNewIndex, bOn, sLabel);
            bZeroForOffOneForOn = bOn;
            break;
        case 7:
            nNewIndex = 4;
            nErr = m_PowerPorts.getPwrHub(nNewIndex, bOn, sLabel);
            bZeroForOffOneForOn = bOn;
            break;
        default :
            nErr = ERR_CMDFAILED;
    }
    
	return nErr;
}

int X2PowerControl::setCircuitState(const int& nIndex, const bool& bZeroForOffOneForOn)
{
	int nErr = SB_OK;
    int nNewIndex;

	if(!m_bLinked)
        return ERR_NOLINK;

    switch(nIndex) {
            // power port <portidx>=1,2,3,4
        case 0:
            nNewIndex = 1;
            nErr =  m_PowerPorts.setPwrOut(nNewIndex, bZeroForOffOneForOn);
            break;
        case 1:
            nNewIndex = 2;
            nErr =  m_PowerPorts.setPwrOut(nNewIndex, bZeroForOffOneForOn);
            break;
        case 2:
            nNewIndex = 3;
            nErr =  m_PowerPorts.setPwrOut(nNewIndex, bZeroForOffOneForOn);
            break;
        case 3:
            nNewIndex = 4;
            nErr =  m_PowerPorts.setPwrOut(nNewIndex, bZeroForOffOneForOn);
            break;
            /// usb port  <usb_portidx>=1,2,3,4
        case 4:
            nNewIndex = 1;
            nErr =  m_PowerPorts.setPwrHub(nNewIndex, bZeroForOffOneForOn);
            break;
        case 5:
            nNewIndex = 2;
            nErr =  m_PowerPorts.setPwrHub(nNewIndex, bZeroForOffOneForOn);
            break;
        case 6:
            nNewIndex = 3;
            nErr =  m_PowerPorts.setPwrHub(nNewIndex, bZeroForOffOneForOn);
            break;
        case 7:
            nNewIndex = 4;
            nErr =  m_PowerPorts.setPwrHub(nNewIndex, bZeroForOffOneForOn);
            break;
        default :
            nErr = ERR_CMDFAILED;
}

	return nErr;
}

int X2PowerControl::circuitLabel(const int &nZeroBasedIndex, BasicStringInterface &str)
{
    int nErr = SB_OK;
    int nNewIndex;
    double dVolts;
    double dCurrent;
    double dPower;
    bool bOn;
    std::string sLabel;

    if(m_bLinked) {
        switch(nZeroBasedIndex) {
                // power port <portidx>=1,2,3,4
            case 0:
                nNewIndex = 1;
                nErr = m_PowerPorts.getPwrOut(nNewIndex, dVolts, dCurrent, dPower, sLabel);
                str = sLabel.c_str();
                break;
            case 1:
                nNewIndex = 2;
                nErr = m_PowerPorts.getPwrOut(nNewIndex, dVolts, dCurrent, dPower, sLabel);
                str = sLabel.c_str();
                break;
            case 2:
                nNewIndex = 3;
                nErr = m_PowerPorts.getPwrOut(nNewIndex, dVolts, dCurrent, dPower, sLabel);
                str = sLabel.c_str();
                break;
            case 3:
                nNewIndex = 4;
                nErr = m_PowerPorts.getPwrOut(nNewIndex, dVolts, dCurrent, dPower, sLabel);
                str = sLabel.c_str();
                break;
                /// usb port  <usb_portidx>=1,2,3,4
            case 4:
                nNewIndex = 1;
                nErr = m_PowerPorts.getPwrHub(nNewIndex, bOn, sLabel);
                str = sLabel.c_str();
                break;
            case 5:
                nNewIndex = 2;
                nErr = m_PowerPorts.getPwrHub(nNewIndex, bOn, sLabel);
                str = sLabel.c_str();
                break;
            case 6:
                nNewIndex = 3;
                nErr = m_PowerPorts.getPwrHub(nNewIndex, bOn, sLabel);
                str = sLabel.c_str();
                break;
            case 7:
                nNewIndex = 4;
                nErr = m_PowerPorts.getPwrHub(nNewIndex, bOn, sLabel);
                str = sLabel.c_str();
                break;
            default :
                nErr = ERR_CMDFAILED;
           }
    }
    else {
        std::string sLabel = "Eagle port " + std::to_string(nZeroBasedIndex+1);
        str = sLabel.c_str();
    }

    return nErr;
}

int X2PowerControl::setCircuitLabel(const int &nZeroBasedIndex, const char *str)
{
    int nErr = SB_OK;
    int nNewIndex;

    if(m_bLinked) {
        switch(nZeroBasedIndex) {
                // power port <portidx>=1,2,3,4
            case 0:
                nNewIndex = 1;
                nErr = m_PowerPorts.setPwrOutLabel(nNewIndex, std::string(str));
                break;
            case 1:
                nNewIndex = 2;
                nErr = m_PowerPorts.setPwrOutLabel(nNewIndex, std::string(str));
                break;
            case 2:
                nNewIndex = 3;
                nErr = m_PowerPorts.setPwrOutLabel(nNewIndex, std::string(str));
                break;
            case 3:
                nNewIndex = 4;
                nErr = m_PowerPorts.setPwrOutLabel(nNewIndex, std::string(str));
                break;
                /// usb port  <usb_portidx>=1,2,3,4
            case 4:
                nNewIndex = 1;
                nErr = m_PowerPorts.setPwrHubLabel(nNewIndex, std::string(str));
                break;
            case 5:
                nNewIndex = 2;
                nErr = m_PowerPorts.setPwrHubLabel(nNewIndex, std::string(str));
                break;
            case 6:
                nNewIndex = 3;
                nErr = m_PowerPorts.setPwrHubLabel(nNewIndex, std::string(str));
                break;
            case 7:
                nNewIndex = 4;
                nErr = m_PowerPorts.setPwrHubLabel(nNewIndex, std::string(str));
                break;
            default :
                nErr = ERR_CMDFAILED;
        }
    }
    else {
        std::string sLabel = "Eagle port " + std::to_string(nZeroBasedIndex+1);
        str = sLabel.c_str();
    }

    return nErr;
}

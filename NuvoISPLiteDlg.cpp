#include "pch.h"
#include "framework.h"
#include "NuvoISPLite.h"
#include "Queue.h"
#include "NuvoISPLiteDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// This is main thread.
UINT MainThread(LPVOID param)
{
    CNuvoISPLiteDlg* pDlg = (CNuvoISPLiteDlg *)param;

    while (pDlg->m_bThreadFlag)
    {
        pDlg->UartHandler(); // It is processing for received data from UART

        if (pDlg->GetConnectFlag() == false)
        {
            // Current status is disconnect with target board.
            if (pDlg->GetStep() == 0)
                pDlg->SendConnect(); // Request connect to target board.
        }
        else
        {
            // Current status is connect with target board.
            if (pDlg->GetStep() == 3)
                pDlg->DoProgramAPROM(); // Request program to target board.
        }

        pDlg->MFCSleep(30);
    }

    return 0;
}

CNuvoISPLiteDlg::CNuvoISPLiteDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_NUVOISPLITE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    m_pThread = NULL;
    m_bThreadFlag = false;
    m_bConnectFlag = false;
    m_ComNum = L"";
    m_pQueue = NULL;
    m_u8Step = S_INIT;
    m_pFileBuffer = NULL;
    m_u32FileSize = 0;
    m_u16Checksum = 0;
    m_u32SentChecksum = 0;
    m_u32UpdateLength = 0;
    m_bUpdateFlag = false;
    m_u32StartAddress = 0;
    m_fPercent = 0.0f;
}

void CNuvoISPLiteDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PORT, m_SelComPort);
    DDX_Control(pDX, IDC_PROGRESS, m_ProgressBar);
}

BEGIN_MESSAGE_MAP(CNuvoISPLiteDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_CONNECT, &CNuvoISPLiteDlg::OnBnClickedConnect)
    ON_MESSAGE(WM_SERIAL_READ_MESSAGE, &CNuvoISPLiteDlg::OnSerialMessage)
    ON_MESSAGE(WM_CUSTOM_UPDATEDATA, &CNuvoISPLiteDlg::OnCustomUpdateData)
    ON_BN_CLICKED(IDC_APROM, &CNuvoISPLiteDlg::OnBnClickedAprom)
    ON_BN_CLICKED(IDC_START, &CNuvoISPLiteDlg::OnBnClickedStart)
END_MESSAGE_MAP()

BOOL CNuvoISPLiteDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

    m_pQueue = new CQueue;
    m_pQueue->Init();

    if (ScanPCCom())
        m_SelComPort.SetCurSel(0);

    CreateThread();

    m_ProgressBar.SetRange(0, 100);

    SetDlgItemText(IDC_STATUS, L"");
    SetDlgItemText(IDC_FILE_INFO, L"File not load.");
    SetDlgItemText(IDC_FILE_PATH_APROM, L"");

    return TRUE;
}

void CNuvoISPLiteDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CNuvoISPLiteDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

UINT CNuvoISPLiteDlg::ScanPCCom()
{
    m_SelComPort.ResetContent();
    //m_SelComPort.AddString(_T("Scan Port"));
    UINT nComNum = 0;
    HKEY hKEY;
    LONG hResult = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("HARDWARE\\DEVICEMAP\\SERIALCOMM"), 0, KEY_READ, &hKEY);

    if (hResult != ERROR_SUCCESS) {
        return  0;
    }

    TCHAR strInf[30];
    DWORD type_1 = REG_SZ;
    DWORD cbData_1 = 10;
    DWORD aa = 30, num = 0, a1, a2, a3, a4, a5, a6, a7;
    hResult = ::RegQueryInfoKey(hKEY, strInf, &a7, NULL, &a3, &a1, &a2, &num, &a4, &a5, &a6, NULL);

    if (hResult != ERROR_SUCCESS) {
        RegCloseKey(hKEY);
        return   0;
    }

    BYTE portName[30];
    CString csr;

    for (DWORD i = 0; i < num; i++) {
        aa = 30;
        cbData_1 = 30;
        hResult = ::RegEnumValue(hKEY, i, strInf, &aa, NULL, &type_1, portName, &cbData_1);

        if ((hResult != ERROR_SUCCESS) && (hResult != ERROR_MORE_DATA)) {
            continue;
        }

        csr.Format(_T("%s"), portName);
        m_SelComPort.AddString(csr);
        nComNum++;
    }

    RegCloseKey(hKEY);
    m_SelComPort.SetCurSel(0);

    return nComNum;
}

void CNuvoISPLiteDlg::CreateThread()
{
    if (m_pThread == NULL && m_bThreadFlag == false)
    {
        m_bThreadFlag = true;
        m_pThread = AfxBeginThread(MainThread, this);
    }
}

void CNuvoISPLiteDlg::ReleaseThread()
{
    if (m_pThread != NULL && m_bThreadFlag == true)
    {
        m_bThreadFlag = false;
        WaitForSingleObject(m_pThread->m_hThread, INFINITE);
    }
}

void CNuvoISPLiteDlg::MFCSleep(DWORD dwMillisecond)
{
    MSG msg;
    DWORD dwStart;
    dwStart = GetTickCount();

    while (GetTickCount() - dwStart < dwMillisecond)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

BOOL CNuvoISPLiteDlg::DestroyWindow()
{
    ReleaseThread();
    ReleaseFile();

    if (m_Serial.IsOpen() == TRUE)
    {
        m_bConnectFlag = false;
        m_Serial.Close();
    }

    if (m_pQueue != NULL)
        delete m_pQueue;

    return CDialogEx::DestroyWindow();
}

void CNuvoISPLiteDlg::OnBnClickedConnect()
{
    if (m_Serial.IsOpen() == TRUE)
    {
        // Try disconnect to target board.
        m_Serial.Close();

        m_bConnectFlag = false;
        m_u8Step = S_INIT;

        GetDlgItem(IDC_CONNECT)->SetWindowText(_T("Connect"));
        ((CComboBox*)GetDlgItem(IDC_PORT))->EnableWindow(TRUE);
        SetDlgItemText(IDC_STATUS, L"");
    }
    else
    {
        // Try connect to target board.
        m_SelComPort.GetWindowText(m_ComNum);

        if (ERROR_SUCCESS != m_Serial.Open(m_ComNum, this, WM_SERIAL_READ_MESSAGE))
        {
            AfxMessageBox(_T("Can not open serial port."));
            return;
        }
        m_Serial.Setup(CSerial::EBaud115200);
        m_Serial.SetupHandshaking(CSerial::EHandshakeOff);

        GetDlgItem(IDC_CONNECT)->SetWindowText(_T("Disconnect"));
        ((CComboBox*)GetDlgItem(IDC_PORT))->EnableWindow(FALSE);
        SetDlgItemText(IDC_STATUS, L"Please reset the target board.");
    }
}

LRESULT CNuvoISPLiteDlg::OnSerialMessage(WPARAM wParam, LPARAM lParam)
{
    CSerial::EEvent eEvent = CSerial::EEvent(LOWORD(wParam));
    CSerial::EError eError = CSerial::EError(HIWORD(wParam));

    unsigned char u8InChar;
    DWORD nReadCount;

    if (eEvent & CSerial::EEventRecv)
    {
        while (TRUE)
        {
            m_Serial.Read(&u8InChar, 1, &nReadCount);
            if (nReadCount == 0)
                break;

            if (m_pQueue != NULL)
                m_pQueue->Push(u8InChar);
        }
    }

    return 0;
}

void CNuvoISPLiteDlg::SendConnect()
{
    BYTE u8Buffer[64];
    memset(&u8Buffer[0], 0, sizeof(u8Buffer));

    if (m_Serial.IsOpen() == TRUE)
    {
        DWORD dwHeader = CMD_CONNECT;
        memcpy(&u8Buffer[0], &dwHeader, 4);
        m_Serial.Write(&u8Buffer[0], 64);
    }
}

void CNuvoISPLiteDlg::SendGetDeviceID()
{
    BYTE u8Buffer[64];
    memset(&u8Buffer[0], 0, sizeof(u8Buffer));

    if (m_Serial.IsOpen() == TRUE)
    {
        DWORD dwHeader = CMD_GET_DEVICEID;
        memcpy(&u8Buffer[0], &dwHeader, 4);
        m_Serial.Write(&u8Buffer[0], 64);
    }
}

void CNuvoISPLiteDlg::UartHandler()
{
    DWORD dwCommand;
    BYTE u8Byte[4];
    int i;
    CString wcsTest = L"";

    if (m_pQueue->GetUsedLength() >= 64)
    {
        for (i = 0; i < 4; i++)
             m_pQueue->Pull(&u8Byte[i]);
        dwCommand = ((u8Byte[3] & 0xFF) << 24 | (u8Byte[2] & 0xFF) << 16 | (u8Byte[1] & 0xFF) << 8 | (u8Byte[0] & 0xFF));

        switch (dwCommand)
        {
        case CMD_CONNECT:
            for (i = 0; i < 60; i++)
                m_pQueue->Pull(&u8Byte[0]); // Dummy

            SendGetDeviceID();
            m_u8Step = S_RECEIVED_ACK_FOR_CONNECT;
        break;
        case CMD_GET_DEVICEID:
        {
            for (i = 0; i < 4; i++)
                m_pQueue->Pull(&u8Byte[0]); // Dummy

            for (i = 0; i < 4; i++)
                m_pQueue->Pull(&u8Byte[i]);

            DWORD dwDeviceID = ((u8Byte[3] & 0xFF) << 24 | (u8Byte[2] & 0xFF) << 16 | (u8Byte[1] & 0xFF) << 8 | (u8Byte[0] & 0xFF));
            if (dwDeviceID == DEVICE_ID) // Compare
            {
                m_bConnectFlag = true;
                m_u8Step = S_CORRECT_DEVICE_ID;
                SetDlgItemText(IDC_STATUS, L"Connected");
            }

            for (i = 0; i < 52; i++)
                m_pQueue->Pull(&u8Byte[0]); // Dummy
        }
        break;
        default:
            if (GetStep() == S_REQUEST_TO_PROGRAM)
            {
                if (dwCommand == m_u32SentChecksum) {
                    m_bUpdateFlag = true;
                }

                for (i = 0; i < 60; i++)
                    m_pQueue->Pull(&u8Byte[0]); // Dummy
            }
            else
            {
                m_pQueue->Flush();
            }
        break;
        }
    }
}

LRESULT CNuvoISPLiteDlg::OnCustomUpdateData(WPARAM wParam, LPARAM lParam)
{
    CString wcsTemp = L"";

    m_ProgressBar.SetPos((int)m_fPercent);
    wcsTemp.Format(L"%.0f %%", m_fPercent);
    SetDlgItemText(IDC_PERCENT, wcsTemp);

    if (wParam == true)
    {
        ((CButton*)GetDlgItem(IDC_START))->EnableWindow(TRUE);
        AfxMessageBox(L"Done!");
    }

    return 0;
}

void CNuvoISPLiteDlg::OnBnClickedAprom()
{
    CString wcsFilter = L"All file(*.*)|*.*||";
    CFileDialog dlgOpen(TRUE, L"", L"", OFN_HIDEREADONLY, wcsFilter);

    if (IDOK == dlgOpen.DoModal())
    {
        CString pathName = dlgOpen.GetPathName();
        CFile file;
        ReleaseFile();

        if (file.Open(pathName, CFile::modeRead))
        {
            DWORD i;

            m_u32FileSize = file.GetLength();
            m_pFileBuffer = new BYTE[m_u32FileSize];
            file.Read(&m_pFileBuffer[0], m_u32FileSize);
            for (i = 0; i < m_u32FileSize; i++)
            {
                m_u16Checksum += m_pFileBuffer[i];
            }

            file.Close();

            CString wcsFileInfo = L"";
            wcsFileInfo.Format(L"File size: %.2f K Bytes, Checksum: %x", (double)(m_u32FileSize / 1024.0f), m_u16Checksum);
            SetDlgItemText(IDC_FILE_INFO, wcsFileInfo);

            SetDlgItemText(IDC_FILE_PATH_APROM, pathName);
        }
        else
        {
            SetDlgItemText(IDC_FILE_INFO, L"File not load.");
            SetDlgItemText(IDC_FILE_PATH_APROM, L"");

            AfxMessageBox(L"Can not open the file.");
        }
    }
}

void CNuvoISPLiteDlg::ReleaseFile()
{
    if (m_pFileBuffer != NULL)
        delete[]m_pFileBuffer;
    m_pFileBuffer = NULL;
    m_u32FileSize = 0;
    m_u16Checksum = 0;
}

void CNuvoISPLiteDlg::OnBnClickedStart()
{
    if (m_Serial.IsOpen() == TRUE && GetConnectFlag() == true)
    {
        if (m_u8Step == S_CORRECT_DEVICE_ID)
        {
            m_u32SentChecksum = 0;
            m_u32UpdateLength = 0;
            m_bUpdateFlag = true;
            m_u32StartAddress = 0;
            m_fPercent = 0.0f;

            ((CButton*)GetDlgItem(IDC_START))->EnableWindow(FALSE);
            SetDlgItemText(IDC_PERCENT, L"0 %");
            m_ProgressBar.SetPos(0);

            m_u8Step = S_REQUEST_TO_PROGRAM;
        }
    }
}

DWORD CNuvoISPLiteDlg::CalcCheckSum(BYTE* pBytes, WORD u16Size)
{
    DWORD u32Sum = 0;
    WORD i;

    for (i = 0; i < u16Size; i++)
        u32Sum += pBytes[i] & 0xFF;

    return u32Sum;
}

WORD CNuvoISPLiteDlg::UpdateAPROM(DWORD u32StartAddr, DWORD u32TotalLength, DWORD u32CurrAddr, BYTE* pBuffer)
{
    WORD u16UpdateLength = 0;
    WORD u16WriteLength = u32TotalLength - (u32CurrAddr - u32StartAddr);
    BYTE* pBytes = new BYTE[64];
    memset(&pBytes[0], 0, 64);

    if (u32StartAddr == u32CurrAddr)
    {
        if (u16WriteLength > 64 - 16)
            u16WriteLength = 64 - 16;

        DWORD u32Header = CMD_UPDATE_APROM;
        memcpy(&pBytes[0], &u32Header, 4);

        u32Header = 0; // command index
        memcpy(&pBytes[4], &u32Header, 4);

        u32Header = u32StartAddr;
        memcpy(&pBytes[8], &u32Header, 4);

        u32Header = u32TotalLength;
        memcpy(&pBytes[12], &u32Header, 4);

        memcpy(&pBytes[16], &pBuffer[u32CurrAddr], u16WriteLength);

        m_u32SentChecksum = CalcCheckSum(pBytes, 64);
    }
    else
    {
        if (u16WriteLength > 64 - 8)
            u16WriteLength = 64 - 8;

        DWORD u32Header = 0;
        //memcpy(&pBytes[0], &u32Header, 4);

        u32Header = 0; // command index
        memcpy(&pBytes[4], &u32Header, 4);

        memcpy(&pBytes[8], &pBuffer[u32CurrAddr], u16WriteLength);

        m_u32SentChecksum = CalcCheckSum(pBytes, 64);
    }

    u16UpdateLength = u16WriteLength;
    m_Serial.Write(&pBytes[0], 64);

    return u16UpdateLength;
}

void CNuvoISPLiteDlg::DoProgramAPROM()
{
    WORD u16UpdatedLength;
    bool bDoneFlag = false;

    if (m_pFileBuffer != NULL && m_bUpdateFlag == true)
    {
        if (m_u32UpdateLength < m_u32FileSize)
        {
            u16UpdatedLength = UpdateAPROM(m_u32StartAddress, m_u32FileSize, m_u32StartAddress + m_u32UpdateLength, &m_pFileBuffer[0]);
            m_u32UpdateLength += u16UpdatedLength;
            m_fPercent = ((double)m_u32UpdateLength / (double)m_u32FileSize) * 100.0f;
        }
        else
        {
            // done
            m_fPercent = 100.0f;
            bDoneFlag = true;
            m_u8Step = S_CORRECT_DEVICE_ID;
        }

        m_bUpdateFlag = false;

        SendMessage(WM_CUSTOM_UPDATEDATA, (WPARAM)bDoneFlag, NULL);
    }
}
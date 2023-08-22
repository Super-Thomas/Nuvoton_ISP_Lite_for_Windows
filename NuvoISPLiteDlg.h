#pragma once

#include "SerialMFC.h"

// Custom message
#define WM_SERIAL_READ_MESSAGE	(WM_USER + 1) // This message for recevied uart data.
#define WM_CUSTOM_UPDATEDATA	(WM_USER + 2) // This message for update items. You can not update items within thread.

// Command
#define CMD_UPDATE_APROM		0x000000A0 // Update data to the internal flash of target board.
#define CMD_CONNECT				0x000000AE // Try connect to target board.
#define CMD_GET_DEVICEID		0x000000B1 // Request device ID to target board.

// Device id
#define DEVICE_ID				0x00845120 // 0x00845120 is device id for M451RG6AE. Im used M451RG6AE so I will check only M451RG6AE.

// Status
typedef enum tagStep {
	S_INIT = 0,
	S_RECEIVED_ACK_FOR_CONNECT, // A connection response was received from target board.
	S_CORRECT_DEVICE_ID, // Device ID matches.
	S_REQUEST_TO_PROGRAM, // Current status is under programming.

} STEP;

class CQueue;

// CNuvoISPLiteDlg 대화 상자
class CNuvoISPLiteDlg : public CDialogEx
{
private:
	CSerialMFC m_Serial; // Serial module
	bool m_bConnectFlag; // Whether connect with target. false = disconnect, true = connected.
	CString m_ComNum; // Selected comport number

	CQueue* m_pQueue; // Queue for serial data
	
	BYTE m_u8Step; // Please refer "enum STEP".

	// Selected file
	BYTE* m_pFileBuffer; // File buffer
	DWORD m_u32FileSize; // File size
	WORD m_u16Checksum; // File checksum

	// Programming
	DWORD m_u32SentChecksum; // Checksum about sent data.
	DWORD m_u32UpdateLength; // Updated size
	bool m_bUpdateFlag; // True if the checksum sent from the target board matches the checksum of the sent data, otherwise false.
	DWORD m_u32StartAddress; // Programming starts from this address.
	double m_fPercent; // programming progress

public:
	CWinThread* m_pThread; // Thread object
	bool m_bThreadFlag; // Thread flag. true = run, false = exit.

public:
	void CreateThread();
	void ReleaseThread();

	void MFCSleep(DWORD dwMillisecond); // Sleep function with process messages.
	
	UINT ScanPCCom(); // Search the serial port of current PC.
	
	bool GetConnectFlag() { return m_bConnectFlag; }
	
	void SendConnect(); // Send UART data about connect.
	void SendGetDeviceID(); // Send UART data about device id.

	void UartHandler(); //Process received UART data.

	BYTE GetStep() { return m_u8Step; }

	void ReleaseFile();

	DWORD CalcCheckSum(BYTE* pBytes, WORD u16Size); // Calculate the checksum.
	WORD UpdateAPROM(DWORD u32StartAddr, DWORD u32TotalLength, DWORD u32CurrAddr, BYTE* pBuffer); // Update data to APROM
	void DoProgramAPROM(); // Update selected file to APROM.

public:
	CNuvoISPLiteDlg(CWnd* pParent = nullptr);

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_NUVOISPLITE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	HICON m_hIcon;

	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnSerialMessage(WPARAM wParam, LPARAM lParam); // Message function for custom message.
	afx_msg LRESULT OnCustomUpdateData(WPARAM wParam, LPARAM lParam); // Message function for custom message.
	DECLARE_MESSAGE_MAP()
public:
	CComboBox m_SelComPort;
	virtual BOOL DestroyWindow();
	afx_msg void OnBnClickedConnect();
	afx_msg void OnBnClickedAprom();
	afx_msg void OnBnClickedStart();
	CProgressCtrl m_ProgressBar;
};

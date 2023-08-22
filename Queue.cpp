#include "pch.h"
#include "framework.h"
#include "NuvoISPLite.h"
#include "Queue.h"

CQueue::CQueue()
{
	m_u8Front = 0;
	m_u8Rear = 0;
	m_pu8Buffer = NULL;
	m_u8UsedLength = 0;
}

CQueue::~CQueue()
{
	if (m_pu8Buffer != NULL)
	{
		delete[]m_pu8Buffer;
		m_pu8Buffer = NULL;
	}
}

void CQueue::Init()
{
	m_u8Front = 0;
	m_u8Rear = 0;
	if (m_pu8Buffer != NULL)
	{
		delete[]m_pu8Buffer;
		m_pu8Buffer = NULL;
	}
	m_pu8Buffer = new BYTE[RX_BUF_SIZE];
	memset(&m_pu8Buffer[0], 0, RX_BUF_SIZE);
	m_u8UsedLength = 0;
}

bool CQueue::IsEmpty()
{
	if (m_u8Front == m_u8Rear)
	{
		return true;
	}

	return false;
}

bool CQueue::IsFull()
{
	BYTE u8Temp = (m_u8Rear + 1) % RX_BUF_SIZE;
	if (m_u8Front == u8Temp)
	{
		return true;
	}

	return false;
}

void CQueue::Flush()
{
	m_u8Front = 0;
	m_u8Rear = 0;
	m_u8UsedLength = 0;
}

void CQueue::Push(BYTE u8Byte)
{
	if (m_pu8Buffer != NULL && IsFull() == false)
	{
		m_u8Rear = (m_u8Rear + 1) % RX_BUF_SIZE;
		m_pu8Buffer[m_u8Rear] = u8Byte;
		m_u8UsedLength++;
	}
}

void CQueue::Pull(BYTE* pu8Byte)
{
	if (m_pu8Buffer != NULL && IsEmpty() == false)
	{
		m_u8Front = (m_u8Front + 1) % RX_BUF_SIZE;
		*pu8Byte = m_pu8Buffer[m_u8Front];
		m_u8UsedLength--;
	}
}

#ifndef __QUEUE_H_
#define __QUEUE_H_

#pragma once

#define RX_BUF_SIZE	256

class CQueue
{
private:
	BYTE m_u8Front;
	BYTE m_u8Rear;
	BYTE* m_pu8Buffer;
	BYTE m_u8UsedLength;

public:
	void Init();
	bool IsEmpty();
	bool IsFull();
	void Flush();
	void Push(BYTE u8Byte);
	void Pull(BYTE* pu8Byte);
	BYTE GetUsedLength() { return m_u8UsedLength; }

public:
	CQueue::CQueue();
	CQueue::~CQueue();
};

#endif //__QUEUE_H_

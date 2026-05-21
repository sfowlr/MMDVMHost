/*
 *   Copyright (C) 2025 by Spencer Fowler Metzler
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if !defined(SCTPSocket_H)
#define SCTPSocket_H

#if defined(HAS_SCTP)

#include "INetSocket.h"

#include <string>

class CSCTPSocket : public INetSocket {
public:
	CSCTPSocket(const std::string& localAddress, unsigned short localPort);
	virtual ~CSCTPSocket();

	void setHeartbeat(unsigned int intervalMs);
	void setMaxRetransmit(unsigned int maxRetransmit);
	void setTTL(unsigned int ttlMs);

	bool open(const sockaddr_storage& address) override;

	int  read(unsigned char* buffer, unsigned int length, sockaddr_storage& address, unsigned int& addressLength) override;
	bool write(const unsigned char* buffer, unsigned int length, const sockaddr_storage& address, unsigned int addressLength) override;

	void close() override;

private:
	std::string    m_localAddress;
	unsigned short m_localPort;
	int            m_fd;
	sa_family_t    m_af;
	sockaddr_storage m_peerAddr;
	unsigned int   m_peerAddrLen;
	unsigned int   m_heartbeatMs;
	unsigned int   m_maxRetransmit;
	unsigned int   m_ttlMs;
};

#endif

#endif

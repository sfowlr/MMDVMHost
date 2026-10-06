/*
 *   Copyright (C) 2015,2016,2017,2018,2020,2021,2023,2026 by Jonathan Naylor G4KLX
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

#if !defined(DMRNetwork_H)
#define	DMRNetwork_H

#include "INetSocket.h"
#include "UDPSocket.h"
#include "Timer.h"
#include "RingBuffer.h"
#include "DMRData.h"
#include "Defines.h"

#if defined(USE_DMR)

#include <string>
#include <cstdint>
#include <random>

class CDMRNetwork
{
public:
	CDMRNetwork(const std::string& gatewayAddress, unsigned short gatewayPort, const std::string& localAddress, unsigned short localPort, unsigned int id, bool duplex, bool slot1, bool slot2, bool debug, const std::string& protocol = "UDP", unsigned int sctpHeartbeat = 0U, unsigned int sctpMaxRetransmit = 0U, unsigned int sctpTTL = 0U, unsigned short sctpLocalUDPPort = 9899U, unsigned short sctpRemoteUDPPort = 9899U);
	~CDMRNetwork();

	void setConfig(const std::string& callsign, const char* version, unsigned int rxFrequency, unsigned int txFrequency, unsigned int colorCode);

	bool open();

	void enable(bool enabled);

	bool read(CDMRData& data, TrunkingCommandParameters& command);

	bool write(const CDMRData& data);

	bool writeRadioPosition(unsigned int id, const unsigned char* data);

	bool writeTalkerAlias(unsigned int id, unsigned char type, const unsigned char* data);

	bool wantsBeacon();

	void clock(unsigned int ms);

	bool isConnected() const;

	void close();

private: 
	std::string      m_addressStr;
	sockaddr_storage m_addr;
	unsigned int     m_addrLen;
	unsigned short   m_port;
	uint8_t*         m_id;
	bool             m_duplex;
	bool             m_debug;
	INetSocket*      m_socket;
	bool             m_enabled;
	bool             m_slot1;
	bool             m_slot2;
	unsigned char*   m_buffer;
	uint32_t*        m_streamId;
	CRingBuffer<unsigned char> m_rxData;
	bool             m_beacon;
	std::mt19937     m_random;
	CTimer           m_pingTimer;
	std::string      m_callsign;
	std::string      m_version;
	unsigned int     m_rxFrequency;
	unsigned int     m_txFrequency;
	unsigned int     m_colorCode;

	bool writeConfig();

	bool write(const unsigned char* data, unsigned int length);
};

#endif

#endif

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

#include "SCTPSocket.h"

#if defined(HAS_SCTP)

#include "UDPSocket.h"
#include "Log.h"

#include <cassert>
#include <cerrno>
#include <cstring>

CSCTPSocket::CSCTPSocket(const std::string& localAddress, unsigned short localPort, unsigned short remoteUDPPort) :
m_localAddress(localAddress),
m_localPort(localPort),
m_remoteUDPPort(remoteUDPPort),
m_fd(SCTP_FD_INVALID),
m_af(AF_UNSPEC),
m_peerAddr(),
m_peerAddrLen(0U),
m_heartbeatMs(0U),
m_maxRetransmit(0U),
m_ttlMs(0U)
{
	::memset(&m_peerAddr, 0, sizeof(m_peerAddr));
}

CSCTPSocket::~CSCTPSocket()
{
	close();
}

void CSCTPSocket::setHeartbeat(unsigned int intervalMs)
{
	m_heartbeatMs = intervalMs;
}

void CSCTPSocket::setMaxRetransmit(unsigned int maxRetransmit)
{
	m_maxRetransmit = maxRetransmit;
}

void CSCTPSocket::setTTL(unsigned int ttlMs)
{
	m_ttlMs = ttlMs;
}

bool CSCTPSocket::initLib(unsigned short localUDPPort)
{
	return sctp_lib_init(localUDPPort);
}

void CSCTPSocket::cleanupLib()
{
	sctp_lib_cleanup();
}

bool CSCTPSocket::open(const sockaddr_storage& address)
{
	assert(!sctp_fd_valid(m_fd));

	m_af = address.ss_family;

	// Store peer address for read() to return
	::memcpy(&m_peerAddr, &address, sizeof(sockaddr_storage));

	// Resolve local bind address
	sockaddr_storage localAddr;
	unsigned int localAddrLen;
	struct addrinfo hints;
	::memset(&hints, 0, sizeof(hints));
	hints.ai_flags  = AI_PASSIVE;
	hints.ai_family = m_af;

	int err = CUDPSocket::lookup(m_localAddress, m_localPort, localAddr, localAddrLen, hints);
	if (err != 0) {
		LogError("SCTP: local address is invalid - %s", m_localAddress.c_str());
		return false;
	}

	// Create SCTP one-to-one socket
	m_fd = sctp_create_socket(m_af, m_remoteUDPPort);
	if (!sctp_fd_valid(m_fd)) {
		LogError("SCTP: cannot create socket, err: %d", errno);
		return false;
	}

	// Set SCTP_NODELAY to minimize latency
	int nodelay = 1;
	if (sctp_setopt(m_fd, IPPROTO_SCTP, SCTP_NODELAY, &nodelay, sizeof(nodelay)) < 0)
		LogWarning("SCTP: cannot set SCTP_NODELAY, err: %d", errno);

	// Configure single stream
	struct sctp_initmsg initmsg;
	::memset(&initmsg, 0, sizeof(initmsg));
	initmsg.sinit_num_ostreams  = 1;
	initmsg.sinit_max_instreams = 1;
	initmsg.sinit_max_attempts  = 3;
	if (sctp_setopt(m_fd, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg)) < 0)
		LogWarning("SCTP: cannot set SCTP_INITMSG, err: %d", errno);

	// Bind to local address if port is specified
	if (m_localPort > 0U) {
		int reuse = 1;
		if (sctp_setopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
			LogError("SCTP: cannot set SO_REUSEADDR, err: %d", errno);
			close();
			return false;
		}

		if (sctp_bind(m_fd, (sockaddr*)&localAddr, localAddrLen) < 0) {
			LogError("SCTP: cannot bind to local address, err: %d", errno);
			close();
			return false;
		}
	}

	// Determine peer address length
	if (m_af == AF_INET)
		m_peerAddrLen = sizeof(struct sockaddr_in);
	else if (m_af == AF_INET6)
		m_peerAddrLen = sizeof(struct sockaddr_in6);
	else
		m_peerAddrLen = sizeof(sockaddr_storage);

	// Connect to peer (establishes SCTP association)
	if (sctp_connect(m_fd, (sockaddr*)&m_peerAddr, m_peerAddrLen) < 0) {
		LogError("SCTP: cannot connect to peer, err: %d", errno);
		close();
		return false;
	}

	// Set non-blocking after connect (usrsctp needs blocking connect)
	sctp_set_nonblocking(m_fd);

	// Configure heartbeat interval
	if (m_heartbeatMs > 0U) {
		struct sctp_paddrparams paddrparams;
		::memset(&paddrparams, 0, sizeof(paddrparams));
		::memcpy(&paddrparams.spp_address, &m_peerAddr, m_peerAddrLen);
		paddrparams.spp_hbinterval = m_heartbeatMs;
		paddrparams.spp_flags      = SPP_HB_ENABLE;
		if (sctp_setopt(m_fd, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS, &paddrparams, sizeof(paddrparams)) < 0)
			LogWarning("SCTP: cannot set heartbeat interval, err: %d", errno);
		else
			LogInfo("SCTP heartbeat interval set to %ums", m_heartbeatMs);
	}

	// Configure max retransmissions before association failure
	if (m_maxRetransmit > 0U) {
		struct sctp_assocparams assocparams;
		::memset(&assocparams, 0, sizeof(assocparams));
		assocparams.sasoc_asocmaxrxt = m_maxRetransmit;
		if (sctp_setopt(m_fd, IPPROTO_SCTP, SCTP_ASSOCINFO, &assocparams, sizeof(assocparams)) < 0)
			LogWarning("SCTP: cannot set max retransmissions, err: %d", errno);
		else
			LogInfo("SCTP max retransmissions set to %u", m_maxRetransmit);
	}

	if (m_ttlMs > 0U)
		LogInfo("SCTP PR-SCTP TTL set to %ums", m_ttlMs);

	LogInfo("SCTP association established on local port %hu", m_localPort);

	return true;
}

int CSCTPSocket::read(unsigned char* buffer, unsigned int length, sockaddr_storage& address, unsigned int& addressLength)
{
	assert(buffer != nullptr);
	assert(length > 0U);

	if (!sctp_fd_valid(m_fd))
		return 0;

	int len = sctp_poll_recv(m_fd, buffer, length);
	if (len < 0) {
		LogError("SCTP: read error or association lost");
		return -1;
	}

	if (len == 0)
		return 0;

	// Return stored peer address for compatibility with CUDPSocket::match()
	::memcpy(&address, &m_peerAddr, sizeof(sockaddr_storage));
	addressLength = m_peerAddrLen;

	return len;
}

bool CSCTPSocket::write(const unsigned char* buffer, unsigned int length, const sockaddr_storage& address, unsigned int addressLength)
{
	assert(buffer != nullptr);
	assert(length > 0U);
	assert(sctp_fd_valid(m_fd));

	ssize_t ret;
	if (m_ttlMs > 0U)
		ret = sctp_send_ttl(m_fd, buffer, length, m_ttlMs);
	else
		ret = sctp_send_data(m_fd, buffer, length);

	if (ret < 0) {
		LogError("SCTP: error from send, err: %d", errno);
		return false;
	}

	return ret == ssize_t(length);
}

void CSCTPSocket::close()
{
	sctp_close_fd(m_fd);
	m_fd = SCTP_FD_INVALID;
}

#endif

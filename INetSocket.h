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

#if !defined(INetSocket_H)
#define INetSocket_H

#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/socket.h>
#else
#include <ws2tcpip.h>
#include <Winsock2.h>
#endif

class INetSocket {
public:
	virtual ~INetSocket() = default;

	virtual bool open(const sockaddr_storage& address) = 0;

	virtual int  read(unsigned char* buffer, unsigned int length, sockaddr_storage& address, unsigned int& addressLength) = 0;
	virtual bool write(const unsigned char* buffer, unsigned int length, const sockaddr_storage& address, unsigned int addressLength) = 0;

	virtual void close() = 0;
};

#endif

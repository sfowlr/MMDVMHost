/*
 *   Copyright (C) 2025 by Spencer Fowler
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

/*
 * SCTP compatibility layer.
 *
 * On Linux: thin pass-through to the kernel SCTP API (lksctp / netinet/sctp.h).
 * On macOS (or anywhere HAS_USRSCTP is defined): wraps the usrsctp userspace
 * library behind the same interface so that SCTPSocket.cpp needs minimal changes.
 *
 * The main abstraction is sctp_fd_t: an int file descriptor on kernel SCTP,
 * a struct socket* on usrsctp.
 */

#if !defined(SCTPCompat_H)
#define SCTPCompat_H

#if defined(HAS_SCTP)

#if defined(HAS_USRSCTP)

/* ── usrsctp (userspace) ─────────────────────────────────────────────── */

#include <usrsctp.h>
#include <cerrno>
#include <cstring>

typedef struct socket* sctp_fd_t;
#define SCTP_FD_INVALID nullptr

/* Global lifecycle — call once from main / DMRNetwork setup. */
inline bool sctp_lib_init(unsigned short localUDPPort = 9899)
{
	usrsctp_init(localUDPPort, nullptr, nullptr);
	return true;
}

inline void sctp_lib_cleanup()
{
	/* best-effort; may block briefly while associations drain */
	usrsctp_finish();
}

/* Socket creation — non-blocking is set later, after connect() */
inline sctp_fd_t sctp_create_socket(int af, unsigned short remoteUDPPort = 9899)
{
	struct socket* s = usrsctp_socket(af, SOCK_STREAM, IPPROTO_SCTP,
	                                  nullptr, nullptr, 0, nullptr);
	if (s != nullptr) {
		/* Tell usrsctp which UDP port the remote peer listens on */
		struct sctp_udpencaps encaps;
		::memset(&encaps, 0, sizeof(encaps));
		encaps.sue_port = htons(remoteUDPPort);
		usrsctp_setsockopt(s, IPPROTO_SCTP, SCTP_REMOTE_UDP_ENCAPS_PORT,
		                   &encaps, sizeof(encaps));
	}
	return s;
}

/* Set non-blocking mode (call after connect) */
inline void sctp_set_nonblocking(sctp_fd_t s)
{
	usrsctp_set_non_blocking(s, 1);
}

/* Socket options — translate SOL_SOCKET/SO_REUSEADDR to SCTP_REUSE_PORT */
inline int sctp_setopt(sctp_fd_t s, int level, int optname,
                       const void* optval, socklen_t optlen)
{
	if (level == SOL_SOCKET && optname == SO_REUSEADDR) {
		level   = IPPROTO_SCTP;
		optname = SCTP_REUSE_PORT;
	}
	return usrsctp_setsockopt(s, level, optname, optval, optlen);
}

/* Bind — usrsctp takes non-const sockaddr */
inline int sctp_bind(sctp_fd_t s, const struct sockaddr* addr, socklen_t addrlen)
{
	return usrsctp_bind(s, const_cast<struct sockaddr*>(addr), addrlen);
}

/* Connect — usrsctp takes non-const sockaddr */
inline int sctp_connect(sctp_fd_t s, const struct sockaddr* addr, socklen_t addrlen)
{
	return usrsctp_connect(s, const_cast<struct sockaddr*>(addr), addrlen);
}

/*
 * Non-blocking read.
 *
 * Kernel SCTP uses poll() + recv().  usrsctp sockets are not real file
 * descriptors so poll() cannot be used.  Instead the socket is set
 * non-blocking at creation time and we call usrsctp_recvv() directly.
 *
 * Returns:
 *   >0  bytes received
 *    0  no data available (EWOULDBLOCK / EAGAIN)
 *   -1  error or association lost
 */
inline int sctp_poll_recv(sctp_fd_t s, void* buf, size_t len)
{
	struct sockaddr_storage from;
	socklen_t fromlen = sizeof(from);
	struct sctp_rcvinfo rcvinfo;
	socklen_t infolen = sizeof(rcvinfo);
	unsigned int infotype = 0;
	int flags = 0;

	ssize_t n = usrsctp_recvv(s, buf, len,
	                           (struct sockaddr*)&from, &fromlen,
	                           &rcvinfo, &infolen, &infotype, &flags);
	if (n < 0) {
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			return 0;   /* no data ready */
		return -1;      /* real error */
	}

	if (n == 0)
		return -1;  /* peer closed association */

	return (int)n;
}

/* Send (plain) */
inline ssize_t sctp_send_data(sctp_fd_t s, const void* buf, size_t len)
{
	return usrsctp_sendv(s, buf, len, nullptr, 0,
	                     nullptr, 0, SCTP_SENDV_NOINFO, 0);
}

/*
 * Send with PR-SCTP timed reliability (TTL).
 *
 * usrsctp_sendv with SCTP_SENDV_SNDINFO lets us set snd_flags and
 * snd_ppid/snd_context/snd_assoc_id, plus the prinfo for PR-SCTP.
 */
inline ssize_t sctp_send_ttl(sctp_fd_t s, const void* buf, size_t len,
                             uint32_t ttlMs)
{
	struct sctp_sendv_spa spa;
	::memset(&spa, 0, sizeof(spa));

	spa.sendv_flags            = SCTP_SEND_SNDINFO_VALID | SCTP_SEND_PRINFO_VALID;
	spa.sendv_sndinfo.snd_sid  = 0;
	spa.sendv_prinfo.pr_policy = SCTP_PR_SCTP_TTL;
	spa.sendv_prinfo.pr_value  = ttlMs;

	return usrsctp_sendv(s, buf, len, nullptr, 0,
	                     &spa, sizeof(spa), SCTP_SENDV_SPA, 0);
}

/* Close */
inline void sctp_close_fd(sctp_fd_t s)
{
	if (s != SCTP_FD_INVALID)
		usrsctp_close(s);
}

/* Validity check */
inline bool sctp_fd_valid(sctp_fd_t s)
{
	return s != SCTP_FD_INVALID;
}

#else /* !HAS_USRSCTP — kernel SCTP (Linux) */

/* ── Kernel SCTP (lksctp) ────────────────────────────────────────────── */

/* netinet/in.h must precede netinet/sctp.h, which #defines IPPROTO_SCTP */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <poll.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

typedef int sctp_fd_t;
#define SCTP_FD_INVALID (-1)

inline bool sctp_lib_init(unsigned short = 0)    { return true; }
inline void sctp_lib_cleanup() { }

inline sctp_fd_t sctp_create_socket(int af, unsigned short = 0)
{
	return ::socket(af, SOCK_STREAM, IPPROTO_SCTP);
}

inline void sctp_set_nonblocking(sctp_fd_t s) { (void)s; /* kernel: poll-based */ }

inline int sctp_setopt(sctp_fd_t s, int level, int optname,
                       const void* optval, socklen_t optlen)
{
	return ::setsockopt(s, level, optname, optval, optlen);
}

inline int sctp_bind(sctp_fd_t s, const struct sockaddr* addr, socklen_t addrlen)
{
	return ::bind(s, addr, addrlen);
}

inline int sctp_connect(sctp_fd_t s, const struct sockaddr* addr, socklen_t addrlen)
{
	return ::connect(s, addr, addrlen);
}

/*
 * Non-blocking read via poll() + recv().
 *
 * Returns:
 *   >0  bytes received
 *    0  no data available
 *   -1  error or association lost
 */
inline int sctp_poll_recv(sctp_fd_t s, void* buf, size_t len)
{
	struct pollfd pfd;
	pfd.fd      = s;
	pfd.events  = POLLIN;
	pfd.revents = 0;

	int ret = ::poll(&pfd, 1, 0);
	if (ret < 0)
		return -1;

	if ((pfd.revents & POLLIN) == 0)
		return 0;

	if (pfd.revents & (POLLHUP | POLLERR))
		return -1;

	ssize_t n = ::recv(s, buf, len, 0);
	if (n <= 0)
		return -1;

	return (int)n;
}

inline ssize_t sctp_send_data(sctp_fd_t s, const void* buf, size_t len)
{
	return ::send(s, buf, len, 0);
}

inline ssize_t sctp_send_ttl(sctp_fd_t s, const void* buf, size_t len,
                             uint32_t ttlMs)
{
	return ::sctp_sendmsg(s, buf, len, nullptr, 0,
	                      0, 0, 0, ttlMs, 0);
}

inline void sctp_close_fd(sctp_fd_t s)
{
	if (s != SCTP_FD_INVALID)
		::close(s);
}

inline bool sctp_fd_valid(sctp_fd_t s)
{
	return s != SCTP_FD_INVALID;
}

#endif /* HAS_USRSCTP */

#endif /* HAS_SCTP */

#endif /* SCTPCompat_H */
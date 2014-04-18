// Copyright (C) 2011  Internet Systems Consortium, Inc. ("ISC")
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
// OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#ifndef ASIOLINK_DNS_SERVICE_H
#define ASIOLINK_DNS_SERVICE_H 1

#include <resolve/resolver_interface.h>

#include <asiolink/io_service.h>
#include <asiolink/simple_callback.h>

namespace isc {
namespace asiodns {

class DNSLookup;
class DNSAnswer;
class DNSServiceImpl;

/// \brief A base class for common \c DNSService interfaces.
///
/// This class is defined mainly for test code so it can use a faked/mock
/// version of a derived class and test scenarios that would involve
/// \c DNSService without actually instantiating the real service class.
///
/// It doesn't intend to be a customization for other purposes - we generally
/// expect non test code only use \c DNSService directly.
/// For this reason most of the detailed description are given in the
/// \c DNSService class.  See that for further details of specific methods
/// and class behaviors.
class DNSServiceBase {
protected:
    /// \brief Default constructor.
    ///
    /// This is protected so this class couldn't be accidentally instantiated
    /// directly, even if there were no pure virtual functions.
    DNSServiceBase() {}

public:
    /// \brief Flags for optional server properties.
    ///
    /// The values of this enumerable type are intended to be used to specify
    /// a particular property of the server created via the \c addServer
    /// variants.  As we see need for more such properties, a compound
    /// form of flags (i.e., a single value generated by bitwise OR'ed
    /// multiple flag values) will be allowed.
    ///
    /// Note: the description is given here because it's used in the method
    /// signature.  It essentially belongs to the derived \c DNSService
    /// class.
    enum ServerFlag {
        SERVER_DEFAULT = 0, ///< The default flag (no particular property)
        SERVER_SYNC_OK = 1 ///< The server can act in the "synchronous" mode.
                           ///< In this mode, the client ensures that the
                           ///< lookup provider always completes the query
                           ///< process and it immediately releases the
                           ///< ownership of the given buffer.  This allows
                           ///< the server implementation to introduce some
                           ///< optimization such as omitting unnecessary
                           ///< operation or reusing internal resources.
                           ///< Note that in functionality the non
                           ///< "synchronous" mode is compatible with the
                           ///< synchronous mode; it's up to the server
                           ///< implementation whether it exploits the
                           ///< information given by the client.
    };

public:
    /// \brief The destructor.
    virtual ~DNSServiceBase() {}

    virtual void addServerTCPFromFD(int fd, int af) = 0;
    virtual void addServerUDPFromFD(int fd, int af,
                                    ServerFlag options = SERVER_DEFAULT) = 0;
    virtual void clearServers() = 0;

    /// \brief Set the timeout for TCP DNS services
    ///
    /// The timeout is used for incoming TCP connections, so
    /// that the connection is dropped if not all query data
    /// is read.
    ///
    /// For existing DNSServer objects, where the timeout is
    /// relevant (i.e. TCPServer instances), the timeout value
    /// is updated.
    /// The given value is also kept to use for DNSServer instances
    /// which are created later
    ///
    /// \param timeout The timeout in milliseconds
    virtual void setTCPRecvTimeout(size_t timeout) = 0;

    virtual asiolink::IOService& getIOService() = 0;
};

/// \brief Handle DNS Queries
///
/// DNSService is the service that handles DNS queries and answers with
/// a given IOService. This class is mainly intended to hold all the
/// logic that is shared between the authoritative and the recursive
/// server implementations. As such, it handles asio and listening
/// sockets.
class DNSService : public DNSServiceBase {
    ///
    /// \name Constructors and Destructor
    ///
    /// Note: The copy constructor and the assignment operator are
    /// intentionally defined as private, making this class non-copyable.
    //@{
private:
    DNSService(const DNSService& source);
    DNSService& operator=(const DNSService& source);

private:
    // Bit or'ed all defined \c ServerFlag values.  Used internally for
    // compatibility check.  Note that this doesn't have to be used by
    // applications, and doesn't have to be defined in the "base" class.
    static const unsigned int SERVER_DEFINED_FLAGS = 1;

public:
    /// \brief The constructor without any servers.
    ///
    /// Use addServerTCPFromFD() or addServerUDPFromFD() to add some servers.
    ///
    /// \param io_service The IOService to work with
    /// \param lookup The lookup provider (see \c DNSLookup)
    /// \param answer The answer provider (see \c DNSAnswer)
    DNSService(asiolink::IOService& io_service,
               DNSLookup* lookup, DNSAnswer* answer);

    /// \brief The destructor.
    virtual ~DNSService();
    //@}

    /// \brief Add another TCP server/listener to the service from already
    /// opened file descriptor
    ///
    /// Adds a new TCP server using an already opened file descriptor (eg. it
    /// only wraps it so the file descriptor is usable within the event loop).
    /// The file descriptor must be associated with a TCP socket of the given
    /// address family that is bound to an appropriate port (and possibly a
    /// specific address) and is ready for listening to new connection
    /// requests but has not actually started listening.
    ///
    /// At the moment, TCP servers don't support any optional properties;
    /// so unlike the UDP version of the method it doesn't have an \c options
    /// argument.
    ///
    /// \param fd the file descriptor to be used.
    /// \param af the address family of the file descriptor. Must be either
    ///     AF_INET or AF_INET6.
    /// \throw isc::InvalidParameter if af is neither AF_INET nor AF_INET6.
    /// \throw isc::asiolink::IOError when a low-level error happens, like the
    ///     fd is not a valid descriptor or it can't be listened on.
    virtual void addServerTCPFromFD(int fd, int af);

    /// \brief Add another UDP server to the service from already opened
    ///    file descriptor
    ///
    /// Adds a new UDP server using an already opened file descriptor (eg. it
    /// only wraps it so the file descriptor is usable within the event loop).
    /// The file descriptor must be associated with a UDP socket of the given
    /// address family that is bound to an appropriate port (and possibly a
    /// specific address).
    ///
    /// \param fd the file descriptor to be used.
    /// \param af the address family of the file descriptor. Must be either
    ///     AF_INET or AF_INET6.
    /// \param options Optional properties of the server (see ServerFlag).
    ///
    /// \throw isc::InvalidParameter if af is neither AF_INET nor AF_INET6,
    ///     or the given \c options include an unsupported or invalid value.
    /// \throw isc::asiolink::IOError when a low-level error happens, like the
    ///     fd is not a valid descriptor or it can't be listened on.
    virtual void addServerUDPFromFD(int fd, int af,
                                    ServerFlag options = SERVER_DEFAULT);

    /// \brief Remove all servers from the service
    void clearServers();

    /// \brief Return the native \c io_service object used in this wrapper.
    ///
    /// This is a short term work around to support other BUNDY modules
    /// that share the same \c io_service with the authoritative server.
    /// It will eventually be removed once the wrapper interface is
    /// generalized.
    asio::io_service& get_io_service() { return io_service_.get_io_service(); }

    /// \brief Return the IO Service Object
    ///
    /// \return IOService object for this DNS service.
    virtual asiolink::IOService& getIOService() { return (io_service_);}

    virtual void setTCPRecvTimeout(size_t timeout);
private:
    DNSServiceImpl* impl_;
    asiolink::IOService& io_service_;
};

} // namespace asiodns
} // namespace isc
#endif // ASIOLINK_DNS_SERVICE_H

// Local Variables:
// mode: c++
// End:

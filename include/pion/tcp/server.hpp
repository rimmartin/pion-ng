// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#ifndef __PION_TCP_SERVER_HEADER__
#define __PION_TCP_SERVER_HEADER__

#include <vector>
#include <set>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <pion/config.hpp>
#include <pion/logger.hpp>
#include <pion/scheduler.hpp>
#include <pion/tcp/connection.hpp>


namespace pion {    // begin namespace pion
namespace tcp {     // begin namespace tcp


///
/// tcp::acceptors_base: base class implementing the switch
/// between only one acceptor (until C++03) or the ability for
/// multiple acceptors/endpoints (since C++11)
/// 
class PION_API acceptors_base
{
public:
    typedef boost::asio::ip::tcp::acceptor acceptor_t;
#ifdef BOOST_ASIO_HAS_MOVE
    using acceptors_t = std::vector<acceptor_t>;
#else
    typedef acceptor_t acceptors_t[1];
#endif


protected:
    acceptors_base(scheduler& sched, size_t n);
    acceptors_base(size_t n);
    ~acceptors_base();

#ifdef BOOST_ASIO_HAS_MOVE
    void resize_acceptors(size_t n);
#endif


private:
    /// the default scheduler object used to manage worker threads
    single_service_scheduler m_default_scheduler;

protected:
    /// reference to the active scheduler object used to manage worker threads
    scheduler& m_active_scheduler;


    /// manages async TCP connections per endpoint
#ifdef BOOST_ASIO_HAS_MOVE
    acceptors_t m_tcp_acceptors;
#else
    // prevent automatic construction
    union
    {
        acceptors_t m_tcp_acceptors;
    };
#endif
};

///
/// tcp::server: a multi-threaded, asynchronous TCP server
/// 
class PION_API server :
    private acceptors_base,
    private boost::noncopyable
{
public:
    typedef std::vector<boost::asio::ip::tcp::endpoint> endpoints_t;

public:

    /// default destructor
    virtual ~server() { if (m_is_listening) stop(false); }
    
    /// starts listening for new connections
    void start(void);

    /**
     * stops listening for new connections
     *
     * @param wait_until_finished if true, blocks until all pending connections have closed
     */
    void stop(bool wait_until_finished = false);
    
    /// the calling thread will sleep until the server has stopped listening for connections
    void join(void);
    
    /**
     * configures server for SSL using a PEM-encoded RSA private key file
     *
     * @param pem_key_file name of the file containing a PEM-encoded private key
     */
    void set_ssl_key_file(const std::string& pem_key_file);

    /// returns the number of active tcp connections
    std::size_t get_connections(void) const;

    /// returns first tcp port number that the server listens for connections on
    inline unsigned int get_port(void) const { return m_endpoints.at(0).port(); }
    
    /// sets first tcp port number that the server listens for connections on
    inline void set_port(unsigned int p) { m_endpoints.at(0).port(p); }
    
    /// returns first IP address that the server listens for connections on
    inline boost::asio::ip::address get_address(void) const { return m_endpoints.at(0).address(); }
    
    /// sets first IP address that the server listens for connections on
    inline void set_address(const boost::asio::ip::address& addr) { m_endpoints.at(0).address(addr); }
    
    /// returns first tcp endpoint that the server listens for connections on
    inline const boost::asio::ip::tcp::endpoint& get_endpoint(void) const { return m_endpoints.at(0); }

    /// sets first tcp endpoint that the server listens for connections on
    inline void set_endpoint(const boost::asio::ip::tcp::endpoint& ep) { m_endpoints.at(0) = ep; }

    /// returns tcp endpoints that the server listens for connections on
    inline const endpoints_t& get_endpoints(void) const { return m_endpoints; }

#ifdef BOOST_ASIO_HAS_MOVE
    /// sets tcp endpoints that the server listens for connections on
    inline void set_endpoints(endpoints_t endpoints);
#endif

    /// returns true if the server uses SSL to encrypt connections
    inline bool get_ssl_flag(void) const { return m_ssl_flag; }
    
    /// sets value of SSL flag (true if the server uses SSL to encrypt connections)
    inline void set_ssl_flag(bool b = true) { m_ssl_flag = b; }
    
    /// returns the SSL context for configuration
    inline connection::ssl_context_type& get_ssl_context_type(void) { return m_ssl_context; }
    
    /// returns true if the server is listening for connections
    inline bool is_listening(void) const { return m_is_listening; }
    
    /// sets the logger to be used
    inline void set_logger(logger log_ptr) { m_logger = log_ptr; }
    
    /// returns the logger currently in use
    inline logger get_logger(void) { return m_logger; }
    
    /// returns mutable reference to the first TCP connection acceptor
    inline boost::asio::ip::tcp::acceptor& get_acceptor(void) { return m_tcp_acceptors.at(0); }

    /// returns const reference to the first TCP connection acceptor
    inline const boost::asio::ip::tcp::acceptor& get_acceptor(void) const { return m_tcp_acceptors.at(0); }

    /// returns mutable reference to the TCP connection acceptors
    inline acceptors_t& get_acceptors(void) { return m_tcp_acceptors; }

    /// returns const reference to the TCP connection acceptors
    inline const acceptors_t& get_acceptors(void) const { return m_tcp_acceptors; }

    
protected:
        
    /**
     * protected constructor so that only derived objects may be created
     * 
     * @param tcp_port port number used to listen for new connections (IPv4)
     */
    explicit server(const unsigned int tcp_port);
    
    /**
     * protected constructor so that only derived objects may be created
     * 
     * @param endpoint TCP endpoint used to listen for new connections (see ASIO docs)
     */
    explicit server(const boost::asio::ip::tcp::endpoint& endpoint);

    /**
     * protected constructor so that only derived objects may be created
     * 
     * @param sched the scheduler that will be used to manage worker threads
     * @param tcp_port port number used to listen for new connections (IPv4)
     */
    explicit server(scheduler& sched, const unsigned int tcp_port = 0);
    
    /**
     * protected constructor so that only derived objects may be created
     * 
     * @param sched the scheduler that will be used to manage worker threads
     * @param endpoint TCP endpoint used to listen for new connections (see ASIO docs)
     */
    server(scheduler& sched, const boost::asio::ip::tcp::endpoint& endpoint);

    virtual void handle_ssl_handshake_error(const tcp::connection_ptr& tcp_conn,
                                            const boost::system::error_code& handshake_error)
    {
        tcp_conn->set_lifecycle(connection::LIFECYCLE_CLOSE); // make sure it will get closed
        tcp_conn->finish();
    }
#ifdef BOOST_ASIO_HAS_MOVE
    /**
     * protected constructor so that only derived objects may be created
     *
     * @param endpoints TCP endpoints used to listen for new connections (see ASIO docs)
     */
    explicit server(endpoints_t endpoints);

    /**
     * protected constructor so that only derived objects may be created
     *
     * @param sched the scheduler that will be used to manage worker threads
     * @param endpoints TCP endpoints used to listen for new connections (see ASIO docs)
     */
    server(scheduler& sched, endpoints_t endpoints);
#endif

    /**
     * handles a new TCP connection; derived classes SHOULD override this
     * since the default behavior does nothing
     * 
     * @param tcp_conn the new TCP connection to handle
     */
    virtual void handle_connection(const tcp::connection_ptr& tcp_conn) {
        tcp_conn->set_lifecycle(connection::LIFECYCLE_CLOSE); // make sure it will get closed
        tcp_conn->finish();
    }
    
    /// called before the TCP server starts listening for new connections
    virtual void before_starting(void) {}

    /// called after the TCP server has stopped listing for new connections
    virtual void after_stopping(void) {}
    
    /// returns an async I/O service used to schedule work
    inline boost::asio::io_context& get_executor(void) { return m_active_scheduler.get_executor(); }
    
    
    /// primary logging interface used by this class
    logger                  m_logger;
    
    
private:
        
    /// handles a request to stop the server
    void handle_stop_request(void);
    
    /// listens for a new connection
    void listen(void);

    /**
     * handles new connections (checks if there was an accept error)
     *
     * @param tcp_conn the new TCP connection (if no error occurred)
     * @param accept_error true if an error occurred while accepting connections
     */
    void handle_accept(const tcp::connection_ptr& tcp_conn,
                      const boost::system::error_code& accept_error);

    /**
     * handles new connections following an SSL handshake (checks for errors)
     *
     * @param tcp_conn the new TCP connection (if no error occurred)
     * @param handshake_error true if an error occurred during the SSL handshake
     */
    void handle_ssl_handshake(const tcp::connection_ptr& tcp_conn,
                            const boost::system::error_code& handshake_error);
    
    /// This will be called by connection::finish() after a server has
    /// finished handling a connection.  If the keep_alive flag is true,
    /// it will call handle_connection(); otherwise, it will close the
    /// connection and remove it from the server's management pool
    void finish_connection(const tcp::connection_ptr& tcp_conn);
    
    /// prunes orphaned connections that did not close cleanly
    /// and returns the remaining number of connections in the pool
    std::size_t prune_connections(void);
    
    
    /// data type for a pool of TCP connections
    typedef std::set<tcp::connection_ptr>   ConnectionPool;
    

    /// context used for SSL configuration
    connection::ssl_context_type            m_ssl_context;
        
    /// condition triggered when the server has stopped listening for connections
    boost::condition                        m_server_has_stopped;

    /// condition triggered when the connection pool is empty
    boost::condition                        m_no_more_connections;

    /// pool of active connections associated with this server 
    ConnectionPool                          m_conn_pool;

    /// tcp endpoints used to listen for new connections
    endpoints_t                             m_endpoints;

    /// true if the server uses SSL to encrypt connections
    bool                                    m_ssl_flag;

    /// set to true when the server is listening for new connections
    bool                                    m_is_listening;

    /// mutex to make class thread-safe
    mutable boost::mutex                    m_mutex;
};


/// data type for a server pointer
typedef boost::shared_ptr<server>    server_ptr;


}   // end namespace tcp
}   // end namespace pion

#endif

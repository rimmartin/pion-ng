// ---------------------------------------------------------------------
// pion:  a Boost C++ framework for building lightweight HTTP interfaces
// ---------------------------------------------------------------------
// Copyright (C) 2007-2014 Splunk Inc.  (https://github.com/splunk/pion)
//
// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt
//

#include <utility>
#include <boost/asio/version.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <pion/config.hpp>
#include <pion/admin_rights.hpp>
#include <pion/tcp/server.hpp>


namespace pion {    // begin namespace pion
namespace tcp {     // begin namespace tcp

#ifdef BOOST_ASIO_HAS_MOVE
acceptors_base::acceptors_base(scheduler& sched, size_t n) :
    m_active_scheduler{ sched }
{
    m_tcp_acceptors.reserve(n);
    for (size_t i = 0; i < n; ++i)
        m_tcp_acceptors.emplace_back(m_active_scheduler.get_executor());
}

acceptors_base::acceptors_base(size_t n) :
    m_default_scheduler{},
    m_active_scheduler{ m_default_scheduler }
{
    m_tcp_acceptors.reserve(n);
    for (size_t i = 0; i < n; ++i)
        m_tcp_acceptors.emplace_back(m_active_scheduler.get_executor());
}

acceptors_base::~acceptors_base()
{}

void acceptors_base::resize_acceptors(size_t n)
{
    // decrase number of acceptors_base
    if (n < m_tcp_acceptors.size()) {
        m_tcp_acceptors.erase(m_tcp_acceptors.cbegin() + n, m_tcp_acceptors.cend());
    }
    // incrase number of acceptors_base
    else if (n > m_tcp_acceptors.size()) {
        m_tcp_acceptors.reserve(n);
        for (size_t i = m_tcp_acceptors.size(); i < n; ++i)
            m_tcp_acceptors.emplace_back(m_active_scheduler.get_executor());
    }
}
#else
acceptors_base::acceptors_base(scheduler& sched, size_t) :
    m_active_scheduler(sched)
{
    new (&m_tcp_acceptors[0]) acceptor_t(m_default_scheduler.get_executor());
}

acceptors_base::acceptors_base(size_t) :
    m_default_scheduler(),
    m_active_scheduler(m_default_scheduler)
{
    new (&m_tcp_acceptors[0]) acceptor_t(m_default_scheduler.get_executor());
}

acceptors_base::~acceptors_base()
{
    m_tcp_acceptors[0].~basic_socket_acceptor();
}
#endif
    
// tcp::server member functions

server::server(scheduler& sched, const unsigned int tcp_port) :
    acceptors_base(sched, 1),
    m_logger(PION_GET_LOGGER("pion.tcp.server")),
#ifdef PION_HAVE_SSL
#  if BOOST_ASIO_VERSION >= 101009
    m_ssl_context(boost::asio::ssl::context::tls),
#  else
    m_ssl_context(boost::asio::ssl::context::sslv23),
#  endif
#else
    m_ssl_context(0),
#endif
    m_endpoints(1, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), tcp_port)),
    m_ssl_flag(false),
    m_is_listening(false)
{}

server::server(scheduler& sched, const boost::asio::ip::tcp::endpoint& endpoint) :
    acceptors_base(sched, 1),
    m_logger(PION_GET_LOGGER("pion.tcp.server")),
#ifdef PION_HAVE_SSL
#  if BOOST_ASIO_VERSION >= 101009
    m_ssl_context(boost::asio::ssl::context::tls),
#  else
    m_ssl_context(boost::asio::ssl::context::sslv23),
#  endif
#else
    m_ssl_context(0),
#endif
    m_endpoints(1, endpoint),
    m_ssl_flag(false),
    m_is_listening(false)
{}

server::server(const unsigned int tcp_port) :
    acceptors_base(1),
    m_logger(PION_GET_LOGGER("pion.tcp.server")),
#ifdef PION_HAVE_SSL
    m_ssl_context(boost::asio::ssl::context::tls),
#else
    m_ssl_context(0),
#endif
    m_endpoints(1, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), tcp_port)),
    m_ssl_flag(false),
    m_is_listening(false)
{}

server::server(const boost::asio::ip::tcp::endpoint& endpoint) :
    acceptors_base(1),
    m_logger(PION_GET_LOGGER("pion.tcp.server")),
#ifdef PION_HAVE_SSL
#  if BOOST_ASIO_VERSION >= 101009
    m_ssl_context(boost::asio::ssl::context::tls),
#  else
    m_ssl_context(boost::asio::ssl::context::sslv23),
#  endif
#else
    m_ssl_context(0),
#endif
    m_endpoints(1, endpoint),
    m_ssl_flag(false),
    m_is_listening(false)
{}

#ifdef BOOST_ASIO_HAS_MOVE
server::server(endpoints_t endpoints) :
    acceptors_base{ endpoints.size() },
    m_logger(PION_GET_LOGGER("pion.tcp.server")),
#ifdef PION_HAVE_SSL
    m_ssl_context{ boost::asio::ssl::context::tls },
#else
    m_ssl_context(0),
#endif
    m_endpoints{ move(endpoints) },
    m_ssl_flag{ false },
    m_is_listening{ false }
{}
   
server::server(scheduler& sched, endpoints_t endpoints) :
    acceptors_base{ sched, endpoints.size() },
    m_logger(PION_GET_LOGGER("pion.tcp.server")),
#ifdef PION_HAVE_SSL
#  if BOOST_ASIO_VERSION >= 101009
    m_ssl_context(boost::asio::ssl::context::tls),
#  else
    m_ssl_context(boost::asio::ssl::context::sslv23),
#  endif
#else
    m_ssl_context(0),
#endif
    m_endpoints{ move(endpoints) },
    m_ssl_flag{ false },
    m_is_listening{ false }
{}
#endif
    
void server::start(void)
{
    if (m_endpoints.empty())
        throw std::logic_error{ "List of endpoints can't be empty." };

    // lock mutex for thread safety
    boost::mutex::scoped_lock server_lock(m_mutex);

    if (! m_is_listening) {
        before_starting();

        // configure the acceptor service
        for (size_t i = 0, n = boost::size(m_tcp_acceptors); i < n; ++i) {
            PION_LOG_INFO(m_logger, "Starting server on endpoint " << m_endpoints[i]);

            try {
                // alias
                acceptor_t& acceptor = m_tcp_acceptors[i];
                boost::asio::ip::tcp::endpoint& endpoint = m_endpoints[i];

                // get admin permissions in case we're binding to a privileged port
                pion::admin_rights use_admin_rights(endpoint.port() > 0 && endpoint.port() < 1024);
                acceptor.open(endpoint.protocol());
                // allow the acceptor to reuse the address (i.e. SO_REUSEADDR)
                // ...except when running not on Windows - see http://msdn.microsoft.com/en-us/library/ms740621%28VS.85%29.aspx
#ifndef PION_WIN32
                acceptor_t::reuse_address(true);
#endif
                acceptor.bind(endpoint);
                if (endpoint.port() == 0) {
                    // update the endpoint to reflect the port chosen by bind
                    endpoint = acceptor.local_endpoint();
                }
                acceptor.listen();
        } catch (std::exception& e) {
            PION_LOG_ERROR(m_logger, "Unable to bind to endpoint " << m_endpoints[i] << ": " << e.what());
            throw;
        }
        }

        m_is_listening = true;

        // unlock the mutex since listen() requires its own lock
        server_lock.unlock();
        listen();
        
        // notify the thread scheduler that we need it now
        m_active_scheduler.add_active_user();
    }
}

void server::stop(bool wait_until_finished)
{
    // lock mutex for thread safety
    boost::mutex::scoped_lock server_lock(m_mutex);

    if (m_is_listening) {
        m_is_listening = false;

        struct acceptor_closer
        {
            typedef void result_type;
            void operator()(pion::logger& m_logger, acceptor_t& acceptor)
            {
                PION_LOG_INFO(m_logger, "Shutting down server on endpoint " << acceptor.local_endpoint());
                acceptor.close();
            }
        };
        // this terminates any connections waiting to be accepted
        std::for_each(boost::rbegin(m_tcp_acceptors), boost::rend(m_tcp_acceptors),
            boost::bind(acceptor_closer(), boost::ref(m_logger), _1));
        
        if (! wait_until_finished) {
            // this terminates any other open connections
            std::for_each(m_conn_pool.begin(), m_conn_pool.end(),
                          boost::bind(&connection::close, _1));
        }
    
        // wait for all pending connections to complete
        while (! m_conn_pool.empty()) {
            // try to prun connections that didn't finish cleanly
            if (prune_connections() == 0)
                break;  // if no more left, then we can stop waiting
            // sleep for up to a quarter second to give open connections a chance to finish
            PION_LOG_INFO(m_logger, "Waiting for open connections to finish");
            scheduler::sleep(m_no_more_connections, server_lock, 0, 250000000);
        }
        
        // notify the thread scheduler that we no longer need it
        m_active_scheduler.remove_active_user();
        
        // all done!
        after_stopping();
        m_server_has_stopped.notify_all();
    }
}

void server::join(void)
{
    boost::mutex::scoped_lock server_lock(m_mutex);
    while (m_is_listening) {
        // sleep until server_has_stopped condition is signaled
        m_server_has_stopped.wait(server_lock);
    }
}

void server::set_ssl_key_file(const std::string& pem_key_file)
{
    // configure server for SSL
    set_ssl_flag(true);
#ifdef PION_HAVE_SSL
    m_ssl_context.set_options(boost::asio::ssl::context::default_workarounds
                              | boost::asio::ssl::context::no_sslv2
                              | boost::asio::ssl::context::single_dh_use);
    m_ssl_context.use_certificate_file(pem_key_file, boost::asio::ssl::context::pem);
    m_ssl_context.use_private_key_file(pem_key_file, boost::asio::ssl::context::pem);
#endif
}

void server::listen(void)
{
    // lock mutex for thread safety
    boost::mutex::scoped_lock server_lock(m_mutex);
    
    if (m_is_listening) {
        // prune connections that finished uncleanly
        prune_connections();

        for (size_t i = 0, n = boost::size(m_tcp_acceptors); i < n; ++i)
        {
            // alias
            acceptor_t& acceptor = m_tcp_acceptors[i];

            // create a new TCP connection object
            tcp::connection_ptr new_connection(connection::create(get_executor(),
                m_ssl_context, m_ssl_flag,
                boost::bind(&server::finish_connection,
                    this, _1),
                acceptor.local_endpoint()));

            // keep track of the object in the server's connection pool
            m_conn_pool.insert(new_connection);

            // use the object to accept a new connection
            new_connection->async_accept(acceptor,
                boost::bind(&server::handle_accept,
                    this, new_connection,
                    boost::asio::placeholders::error));
        }
    }
}

void server::handle_accept(const tcp::connection_ptr& tcp_conn,
                             const boost::system::error_code& accept_error)
{
    if (accept_error) {
        // an error occured while trying to a accept a new connection
        // this happens when the server is being shut down
        if (m_is_listening) {
            listen();   // schedule acceptance of another connection
            PION_LOG_WARN(m_logger, "Accept error on endpoint " << tcp_conn->get_local_endpoint() << ": " << accept_error.message());
        }
        finish_connection(tcp_conn);
    } else {
        // got a new TCP connection
        PION_LOG_DEBUG(m_logger, "New" << (tcp_conn->get_ssl_flag() ? " SSL " : " ")
                       << "connection on endpoint " << tcp_conn->get_local_endpoint());

        // schedule the acceptance of another new connection
        // (this returns immediately since it schedules it as an event)
        if (m_is_listening) listen();
        
        // handle the new connection
#ifdef PION_HAVE_SSL
        if (tcp_conn->get_ssl_flag()) {
            tcp_conn->async_handshake_server(boost::bind(&server::handle_ssl_handshake,
                                                         this, tcp_conn,
                                                         boost::asio::placeholders::error));
        } else
#endif
            // not SSL -> call the handler immediately
            handle_connection(tcp_conn);
    }
}

void server::handle_ssl_handshake(const tcp::connection_ptr& tcp_conn,
                                   const boost::system::error_code& handshake_error)
{
    if (handshake_error) {
        // an error occured while trying to establish the SSL connection
        PION_LOG_WARN(m_logger, "SSL handshake failed on endpoint " << tcp_conn->get_local_endpoint()
                      << " (" << handshake_error.message() << ')');
        tcp_conn->reset_ssl_flag();
        handle_ssl_handshake_error(tcp_conn, handshake_error);
    } else {
        // handle the new connection
        PION_LOG_DEBUG(m_logger, "SSL handshake succeeded on endpoint " << tcp_conn->get_local_endpoint());
        handle_connection(tcp_conn);
    }
}

void server::finish_connection(const tcp::connection_ptr& tcp_conn)
{
    boost::mutex::scoped_lock server_lock(m_mutex);
    if (m_is_listening && tcp_conn->get_keep_alive()) {
        
        // keep the connection alive
        handle_connection(tcp_conn);

    } else {
        PION_LOG_DEBUG(m_logger, "Closing connection on endpoint " << tcp_conn->get_local_endpoint());
        
        // remove the connection from the server's management pool
        ConnectionPool::iterator conn_itr = m_conn_pool.find(tcp_conn);
        if (conn_itr != m_conn_pool.end())
            m_conn_pool.erase(conn_itr);

        // trigger the no more connections condition if we're waiting to stop
        if (!m_is_listening && m_conn_pool.empty())
            m_no_more_connections.notify_all();
    }
}

std::size_t server::prune_connections(void)
{
    // assumes that a server lock has already been acquired
    ConnectionPool::iterator conn_itr = m_conn_pool.begin();
    while (conn_itr != m_conn_pool.end()) {
        if (conn_itr->unique()) {
            PION_LOG_WARN(m_logger, "Closing orphaned connection on endpoint " << (*conn_itr)->get_local_endpoint());
            (*conn_itr)->close();
            conn_itr = m_conn_pool.erase(conn_itr);
        } else {
            ++conn_itr;
        }
    }

    // return the number of connections remaining
    return m_conn_pool.size();
}

std::size_t server::get_connections(void) const
{
    boost::mutex::scoped_lock server_lock(m_mutex);
    return (m_is_listening ? (m_conn_pool.size() - 1) : m_conn_pool.size());
}

#ifdef BOOST_ASIO_HAS_MOVE
void server::set_endpoints(endpoints_t endpoints)
{
    if (m_is_listening)
        throw std::logic_error{ "Endpoints can't be modified once the tcp server has started." };

    resize_acceptors(endpoints.size());
    m_endpoints = move(endpoints);
}
#endif

}   // end namespace tcp
}   // end namespace pion

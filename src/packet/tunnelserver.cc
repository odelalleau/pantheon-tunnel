/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <thread>
#include <chrono>

#include <sys/socket.h>
#include <net/route.h>

#include "tunnelserver.hh"
#include "netdevice.hh"
#include "nat.hh"
#include "util.hh"
#include "interfaces.hh"
#include "address.hh"
#include "dns_server.hh"
#include "timestamp.hh"
#include "exception.hh"
#include "bindworkaround.hh"
#include "config.h"

using namespace std;
using namespace PollerShortNames;

TunnelServer::TunnelServer( const std::string & device_prefix, char ** const user_environment,
        const std::string & ingress_logfile,
        const std::string & egress_logfile )
    : user_environment_( user_environment ),
      egress_ingress( two_unassigned_addresses( get_mahimahi_base() ) ),
      nameserver_( first_nameserver() ),
      egress_tun_( device_prefix + "-" + to_string( getpid() ) , egress_addr(), ingress_addr() ),
      dns_outside_( egress_addr(), nameserver_, nameserver_ ),
      nat_rule_( ingress_addr() ),
      listening_socket_(),
      event_loop_(),
      ingress_log_(),
      egress_log_()
{
    /* make sure environment has been cleared */
    if ( environ != nullptr ) {
        throw runtime_error( "TunnelServer: environment was not cleared" );
    }

    /* initialize base timestamp value before any forking */
    initial_timestamp();

    /* open logfiles if called for */
    if ( not ingress_logfile.empty() ) {
        ingress_log_.reset( new ofstream( ingress_logfile ) );
        if ( not ingress_log_->good() ) {
            throw runtime_error( ingress_logfile + ": error opening for writing" );
        }

        *ingress_log_ << "# mahimahi mm-tunnelserver ingress: " << initial_timestamp() << endl;
    }
    if ( not egress_logfile.empty() ) {
        egress_log_.reset( new ofstream( egress_logfile ) );
        if ( not egress_log_->good() ) {
            throw runtime_error( egress_logfile + ": error opening for writing" );
        }

        *egress_log_ << "# mahimahi mm-tunnelserver egress: " << initial_timestamp() << endl;
    }

    /* bind the listening socket to an available address/port, and print out what was bound */
    listening_socket_.bind( Address() );
    /*
    cout << "Listener bound to port " << listening_socket_.local_address().port() << endl;

    cout << "Client's private address should be: " << ingress_addr().ip() << endl;
    cout << "Servers's private address is: " << egress_addr().ip() << endl;
    */

    cout << "mm-tunnelclient localhost " << listening_socket_.local_address().port() << " " << ingress_addr().ip() << " " << egress_addr().ip() << endl;
}

//template <typename... Targs>
void TunnelServer::start_downlink( )//Targs&&... Fargs )
{
    event_loop_.add_child_process( "downlink", [&] () {
            drop_privileges();

            /* restore environment */
            environ = user_environment_;

            EventLoop outer_loop;

            dns_outside_.register_handlers( outer_loop );

            /* tun device gets datagram -> read it -> give to socket */
            outer_loop.add_simple_input_handler( egress_tun_,
                    [&] () {
                    const string packet = egress_tun_.read();

                    if ( egress_log_ ) {
                    *egress_log_ << timestamp() << " - " << hash<string>()(packet) << endl;
                    }

                    ((FileDescriptor &) listening_socket_).write( packet );
                    return ResultType::Continue;
                    } );

            /* we get datagram from listening_socket_ process -> write it to tun device */
            outer_loop.add_simple_input_handler( listening_socket_,
                    [&] () {
                    const string packet = ((FileDescriptor &) listening_socket_).read();

                    if ( ingress_log_ ) {
                    *ingress_log_ << timestamp() << " - " << hash<string>()(packet) << endl;
                    }

                    egress_tun_.write( packet );
                    return ResultType::Continue;
                    } );
            return outer_loop.loop();
        } );
}

int TunnelServer::wait_for_exit( void )
{
    return event_loop_.loop();
}

struct TemporaryEnvironment
{
    TemporaryEnvironment( char ** const env )
    {
        if ( environ != nullptr ) {
            throw runtime_error( "TemporaryEnvironment: cannot be entered recursively" );
        }
        environ = env;
    }

    ~TemporaryEnvironment()
    {
        environ = nullptr;
    }
};

Address TunnelServer::get_mahimahi_base( void ) const
{
    /* temporarily break our security rule of not looking
       at the user's environment before dropping privileges */
    TemporarilyUnprivileged tu;
    TemporaryEnvironment te { user_environment_ };

    const char * const mahimahi_base = getenv( "MAHIMAHI_BASE" );
    if ( not mahimahi_base ) {
        return Address();
    }

    return Address( mahimahi_base, 0 );
}

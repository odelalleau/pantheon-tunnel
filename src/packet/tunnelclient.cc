/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <thread>
#include <chrono>

#include <sys/socket.h>
#include <net/route.h>

#include "tunnelclient.hh"
#include "netdevice.hh"
#include "system_runner.hh"
#include "util.hh"
#include "interfaces.hh"
#include "address.hh"
#include "timestamp.hh"
#include "exception.hh"
#include "bindworkaround.hh"
#include "config.h"

using namespace std;
using namespace PollerShortNames;

TunnelClient::TunnelClient( char ** const user_environment,
                                            const Address & server_address,
                                            const Address & local_private_address,
                                            const Address & server_private_address )
    : user_environment_( user_environment ),
      egress_ingress( server_private_address, local_private_address ),
      nameserver_( first_nameserver() ),
      server_socket_(),
      event_loop_()
{
    /* make sure environment has been cleared */
    if ( environ != nullptr ) {
        throw runtime_error( "TunnelClient: environment was not cleared" );
    }

    /* initialize base timestamp value before any forking */
    initial_timestamp();

    /* connect the server_socket to the server_address */
    server_socket_.connect( server_address );
}

//template <typename... Targs>
void TunnelClient::start_uplink( const string & shell_prefix,
                                                const vector< string > & command)
                                                //Targs&&... Fargs )
{
    /* Fork */
    event_loop_.add_child_process( "packetshell", [&]() {
            TunDevice ingress_tun( "ingress", ingress_addr(), egress_addr() );

            /* bring up localhost */
            interface_ioctl( SIOCSIFFLAGS, "lo",
                             [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );

            /* create default route */
            rtentry route;
            zero( route );

            route.rt_gateway = egress_addr().to_sockaddr();
            route.rt_dst = route.rt_genmask = Address().to_sockaddr();
            route.rt_flags = RTF_UP | RTF_GATEWAY;

            SystemCall( "ioctl SIOCADDRT", ioctl( UDPSocket().fd_num(), SIOCADDRT, &route ) );

            EventLoop inner_loop;

            /* Fork again after dropping root privileges */
            drop_privileges();

            /* restore environment */
            environ = user_environment_;

            /* set MAHIMAHI_BASE if not set already to indicate outermost container */
            SystemCall( "setenv", setenv( "MAHIMAHI_BASE",
                                          egress_addr().ip().c_str(),
                                          false /* don't override */ ) );

            inner_loop.add_child_process( join( command ), [&]() {
                    /* tweak bash prompt */
                    prepend_shell_prefix( shell_prefix );

                    return ezexec( command, true );
                } );


            /* ingress_tun device gets datagram -> read it -> give to server socket */
            inner_loop.add_simple_input_handler( ingress_tun,
                    [&] () {
                    server_socket_.write( ingress_tun.read() );
                    return ResultType::Continue;
                    } );

            /* we get datagram from server_socket_ process -> write it to ingress_tun device */
            inner_loop.add_simple_input_handler( server_socket_,
                    [&] () {
                    ingress_tun.write( server_socket_.read() );
                    return ResultType::Continue;
                    } );

            /* exit if finished
            inner_loop.add_action( Poller::Action( server_socket_, Direction::Out,
                        [&] () {
                        return ResultType::Exit;
                        } ); */

            return inner_loop.loop();
        }, true );  /* new network namespace */
}

int TunnelClient::wait_for_exit( void )
{
    return event_loop_.loop();
}

/*
template <class FerryQueueType>
int TunnelClient<FerryQueueType>::Ferry::loop( FerryQueueType & uplink_queue,
                                              FileDescriptor & ingress_tun,
                                              FileDescriptor & server_socket_ )
{

    return internal_loop( [&] () { return uplink_queue.wait_time(); } );
}
*/

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

Address TunnelClient::get_mahimahi_base( void ) const
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

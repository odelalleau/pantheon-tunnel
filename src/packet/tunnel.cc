/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <thread>
#include <chrono>
#include <string>
#include <fstream>
#include <memory>

#include <sys/socket.h>
#include <net/route.h>

#include "netdevice.hh"
#include "system_runner.hh"
#include "util.hh"
#include "event_loop.hh"
#include "interfaces.hh"
#include "address.hh"
#include "timestamp.hh"
#include "exception.hh"
#include "bindworkaround.hh"
#include "config.h"
#include "socketpair.hh"
#include "socket.hh"

using namespace std;
using namespace PollerShortNames;

int run_tunnel( char ** const user_environment, UDPSocket & peer_socket,
                  const Address & local_private_address,
                  const Address & peer_private_address,
                  const std::string & ingress_logfile,
                  const std::string & egress_logfile,
 const string & shell_prefix, const vector< string > & command)
{
    uint64_t uid_ = 0;
    EventLoop outside_shell_loop;

    /* make sure environment has been cleared */
    if ( environ != nullptr ) {
        throw runtime_error( shell_prefix + ": environment was not cleared" );
    }

    /* initialize base timestamp value before any forking */
    initial_timestamp();

    /* open logfiles if called for */
    std::unique_ptr<std::ofstream> ingress_log;
    std::unique_ptr<std::ofstream> egress_log;

    if ( not ingress_logfile.empty() ) {
        ingress_log.reset( new ofstream( ingress_logfile ) );
        if ( not ingress_log->good() ) {
            throw runtime_error( ingress_logfile + ": error opening for writing" );
        }

        *ingress_log << "# mahimahi " + shell_prefix + " ingress: " << initial_timestamp() << endl;
    }
    if ( not egress_logfile.empty() ) {
        egress_log.reset( new ofstream( egress_logfile ) );
        if ( not egress_log->good() ) {
            throw runtime_error( egress_logfile + ": error opening for writing" );
        }

        *egress_log << "# mahimahi " + shell_prefix + " egress: " << initial_timestamp() << endl;
    }


    /* Fork */
    outside_shell_loop.add_child_process( "packetshell", [&]() { // XXX add special child process?
            TunDevice tun( "tunnel", local_private_address, peer_private_address );

            /* bring up localhost */
            interface_ioctl( SIOCSIFFLAGS, "lo",
                             [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );

            /* create default route */
            rtentry route;
            zero( route );

            route.rt_gateway = peer_private_address.to_sockaddr();
            route.rt_dst = route.rt_genmask = Address().to_sockaddr();
            route.rt_flags = RTF_UP | RTF_GATEWAY;

            SystemCall( "ioctl SIOCADDRT", ioctl( UDPSocket().fd_num(), SIOCADDRT, &route ) );

            EventLoop inner_loop;

            /* Fork again after dropping root privileges */
            drop_privileges();

            /* restore environment */
            environ = user_environment;

            /* set MAHIMAHI_BASE if not set already to indicate outermost container */
            SystemCall( "setenv", setenv( "MAHIMAHI_BASE",
                                          peer_private_address.ip().c_str(),
                                          false /* don't override */ ) );

            inner_loop.add_child_process( join( command ), [&]() {
                    /* tweak bash prompt */
                    prepend_shell_prefix( shell_prefix );

                    return ezexec( command, true );
                } );


            /* tun device gets datagram -> read it -> give to server socket */
            inner_loop.add_simple_input_handler( tun,
                    [&] () {
                    const string packet = tun.read();

                    const uint64_t uid_to_send = uid_++;

                    if ( egress_log ) {
                    *egress_log << timestamp() << " - " << uid_to_send << " - " << packet.length() << endl;
                    }

                    peer_socket.write( string( (char *) &uid_to_send, sizeof(uid_to_send) ) + packet );

                    return ResultType::Continue;
                    } );

            /* we get datagram from peer_socket process -> write it to tun device */
            inner_loop.add_simple_input_handler( peer_socket,
                    [&] () {
                    const string packet = peer_socket.read();

                    uint64_t uid_received = *( (uint64_t *) packet.data() );
                    string contents = packet.substr( sizeof(uid_received) );

                    if ( ingress_log ) {
                    *ingress_log << timestamp() << " - " << uid_received << " - " << packet.length() << endl;
                    }

                    tun.write( contents );
                    return ResultType::Continue;
                    } );

            /* exit if finished
            inner_loop.add_action( Poller::Action( peer_socket, Direction::Out,
                        [&] () {
                        return ResultType::Exit;
                        } ); */

            return inner_loop.loop();
        }, true );  /* new network namespace */
    return outside_shell_loop.loop();
}

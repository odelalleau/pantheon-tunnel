/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */ 
#ifndef TUNNELCLIENT_HH
#define TUNNELCLIENT_HH

#include <string>
#include <fstream>
#include <memory>

#include "netdevice.hh"
#include "util.hh"
#include "address.hh"
#include "event_loop.hh"
#include "socketpair.hh"
#include "socket.hh"

class TunnelClient
{
private:
    char ** const user_environment_;
    std::pair<Address, Address> egress_ingress;
    Address nameserver_;

    UDPSocket server_socket_;

    EventLoop event_loop_;

    std::unique_ptr<std::ofstream> ingress_log_;
    std::unique_ptr<std::ofstream> egress_log_;

    const Address & egress_addr( void ) { return egress_ingress.first; }
    const Address & ingress_addr( void ) { return egress_ingress.second; }

    Address get_mahimahi_base( void ) const;

    uint64_t uid_ = 0;

public:
    TunnelClient( char ** const user_environment, const Address & server_address,
                  const Address & local_private_address,
                  const Address & server_private_address,
                  const std::string & ingress_logfile,
                  const std::string & egress_logfile );

    void start_uplink( const std::string & shell_prefix,
                       const std::vector< std::string > & command);

    int wait_for_exit( void );

    TunnelClient( const TunnelClient & other ) = delete;
    TunnelClient & operator=( const TunnelClient & other ) = delete;
};

#endif /* TUNNELCLIENT_HH */

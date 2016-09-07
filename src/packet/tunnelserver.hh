/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef TUNNELSERVER_HH
#define TUNNELSERVER_HH

#include <string>
#include <fstream>
#include <memory>

#include "netdevice.hh"
#include "nat.hh"
#include "util.hh"
#include "address.hh"
#include "dns_proxy.hh"
#include "event_loop.hh"
#include "socketpair.hh"
#include "autosocket.hh"

class TunnelServer
{
private:
    char ** const user_environment_;
    std::pair<Address, Address> egress_ingress;
    Address nameserver_;
    TunDevice egress_tun_;
    DNSProxy dns_outside_;
    NAT nat_rule_;

    AutoSocket listening_socket_;

    EventLoop event_loop_;

    std::unique_ptr<std::ofstream> log_;

    const Address & egress_addr( void ) { return egress_ingress.first; }
    const Address & ingress_addr( void ) { return egress_ingress.second; }

    Address get_mahimahi_base( void ) const;

public:
    TunnelServer( const std::string & device_prefix, char ** const user_environment,
                  const std::string & logfile );

    //template <typename... Targs>
    void start_downlink();// Targs&&... Fargs );

    int wait_for_exit( void );

    TunnelServer( const TunnelServer & other ) = delete;
    TunnelServer & operator=( const TunnelServer & other ) = delete;
};

#endif /* TUNNELSERVER_HH */

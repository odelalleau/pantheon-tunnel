/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef AUTOCONNECT_SOCKET_HH
#define AUTOCONNECT_SOCKET_HH

#include <iostream>

#include "socket.hh"

class AutoconnectSocket : public UDPSocket 
{
    private:
        bool connected_ = false;
    public:
        using UDPSocket::UDPSocket;

        std::string read( const size_t = 1 ) override
        {
            if ( not connected_ ) {
                std::pair<Address, std::string> recpair = UDPSocket::recvfrom();
                std::cout << "got connection from " << recpair.first.ip() << std::endl;
                UDPSocket::connect( recpair.first );
                connected_ = true;
                return recpair.second;
            } 
            return UDPSocket::read( );
        };
};

#endif /* AUTOCONNECT_SOCKET_HH */

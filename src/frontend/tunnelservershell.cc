/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <vector>
#include <string>
#include <iostream>

#include "tunnelshell.cc"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        /* clear environment while running as root */
        char ** const user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc != 1 ) {
            throw runtime_error( "Usage: " + string( argv[ 0 ] ) );
        }

        Address local_private_address, client_private_address;
        tie(local_private_address, client_private_address) = two_unassigned_addresses();

        UDPSocket listening_socket;
        /* bind the listening socket to an available address/port, and print out what was bound */
        listening_socket.bind( Address() );
        cout << "mm-tunnelclient localhost " << listening_socket.local_address().port() << " " << client_private_address.ip() << " " << local_private_address.ip() << endl;

        std::pair<Address, std::string> recpair =  listening_socket.recvfrom();
        cout << "got connection from " << recpair.first.ip() << endl;
        listening_socket.connect( recpair.first );

        TunnelShell tunnelserver( "/tmp/tunnelserver.ingress.log", "/tmp/tunnelserver.egress.log" );

        tunnelserver.start_link( user_environment, listening_socket, local_private_address, client_private_address, "[tunnelserver " + listening_socket.peer_address().str() + "] ", { shell_path() } );
        return tunnelserver.wait_for_exit();
    } catch ( const exception & e ) {
        print_exception( e );
        return EXIT_FAILURE;
    }
}

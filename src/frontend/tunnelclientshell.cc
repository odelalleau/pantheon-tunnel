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

        if ( argc < 5 ) {
            throw runtime_error( "Usage: " + string( argv[ 0 ] ) + " IP PORT LOCAL-PRIVATE-IP SERVER-PRIVATE-IP [command...]" );
        }

        const Address server{ argv[ 1 ], argv[ 2 ] };
        const Address local_private_address { argv[ 3 ], "0" };
        const Address server_private_address { argv[ 4 ], "0" };

        vector< string > command;

        if ( argc == 5 ) {
            command.push_back( shell_path() );
        } else {
            for ( int i = 5; i < argc; i++ ) {
                command.push_back( argv[ i ] );
            }
        }

        UDPSocket server_socket;
        /* connect the server_socket to the server_address */
        server_socket.connect( server );
        cerr << "client listening for server on port " << server_socket.local_address().port() << endl;
        // XXX error better if this write fails because server is not accepting connections
        const struct wrapped_packet_header to_send = { (uint64_t) -1 };
        server_socket.write( string( (char *) &to_send, sizeof(to_send) ) );

        TunnelShell tunnelclient( "/tmp/tunnelclient.ingress.log", "/tmp/tunnelclient.egress.log" );
        tunnelclient.start_link( user_environment, server_socket, local_private_address, server_private_address,
                "[tunnelclient " + server.str() + "] ", command );
        return tunnelclient.wait_for_exit();
    } catch ( const exception & e ) {
        print_exception( e );
        return EXIT_FAILURE;
    }
}

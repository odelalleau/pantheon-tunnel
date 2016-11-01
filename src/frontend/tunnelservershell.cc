/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <vector>
#include <string>
#include <iostream>
#include <getopt.h>

#include "autoconnect_socket.hh"
#include "tunnelshell.cc"
#include "tunnelshell_common.hh"

using namespace std;

void usage_error( const string & program_name )
{
    cerr << "Usage: " << program_name << " [OPTION]... [COMMAND]" << endl;
    cerr << endl;
    cerr << "Options = --ingress-log=FILENAME --egress-log=FILENAME --interface=INTERFACE" << endl;

    throw runtime_error( "invalid arguments" );
}

int main( int argc, char *argv[] )
{
    try {
        /* clear environment while running as root */
        char ** const user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        if ( argc < 1 ) {
            usage_error( argv[ 0 ] );
        }

        const option command_line_options[] = {
            { "ingress-log", required_argument, nullptr, 'n' },
            { "egress-log",  required_argument, nullptr, 'e' },
            { "interface",   required_argument, nullptr, 'i' },
            { 0,                             0, nullptr,  0  }
        };

        string ingress_logfile, egress_logfile, if_name;

        while ( true ) {
            const int opt = getopt_long( argc, argv, "",
                                         command_line_options, nullptr );
            if ( opt == -1 ) { /* end of options */
                break;
            }

            switch ( opt ) {
            case 'n':
                ingress_logfile = optarg;
                break;
            case 'e':
                egress_logfile = optarg;
                break;
            case 'i':
                if_name = optarg;
                break;
            case '?':
                usage_error( argv[ 0 ] );
            default:
                throw runtime_error( "getopt_long: unexpected return value " +
                                     to_string( opt ) );
            }
        }

        if ( optind > argc ) {
            usage_error( argv[ 0 ] );
        }

        vector< string > command;

        if ( optind == argc ) {
            command.push_back( shell_path() );
        } else {
            for ( int i = optind; i < argc; i++ ) {
                command.push_back( argv[ i ] );
            }
        }

        Address local_private_address, client_private_address;
        tie(local_private_address, client_private_address) = two_unassigned_addresses();

        AutoconnectSocket listening_socket;

        if ( !if_name.empty() ) {
            /* bind the listening socket to a specified interface */
            check_interface_for_binding( string( argv[ 0 ] ), if_name );
            listening_socket.bind( if_name );
        }
        /* bind the listening socket to an available address/port, and print out what was bound */
        listening_socket.bind( Address() );

        cout << "mm-tunnelclient localhost " << listening_socket.local_address().port() << " ";
        cout << client_private_address.ip() << " " << local_private_address.ip();
        cout << endl;

        TunnelShell tunnelserver( ingress_logfile, egress_logfile );

        tunnelserver.start_link( user_environment, listening_socket, local_private_address, client_private_address, "[tunnelserver] ", command );
        return tunnelserver.wait_for_exit();
    } catch ( const exception & e ) {
        print_exception( e );
        return EXIT_FAILURE;
    }
}

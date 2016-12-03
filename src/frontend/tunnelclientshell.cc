/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <vector>
#include <string>
#include <iostream>
#include <getopt.h>

#include "exception.hh"
#include "tunnelshell.hh"
#include "tunnelshell_common.hh"

using namespace std;

void usage_error( const string & program_name )
{
    cerr << "Usage: " << program_name << " IP PORT LOCAL-PRIVATE-IP SERVER-PRIVATE-IP [OPTION]... [COMMAND]" << endl;
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

        if ( argc < 5 ) {
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

        if ( optind + 4 > argc ) {
            usage_error( argv[ 0 ] );
        }

        const Address server{ argv[ optind ], argv[ optind + 1 ] };
        const Address local_private_address { argv[ optind + 2 ], "0" };
        const Address server_private_address { argv[ optind + 3 ], "0" };

        vector< string > command;

        if ( optind + 4 == argc ) {
            command.push_back( shell_path() );
        } else {
            for ( int i = optind + 4; i < argc; i++ ) {
                command.push_back( argv[ i ] );
            }
        }

        UDPSocket server_socket;

        if ( !if_name.empty() ) {
            /* bind the server socket to a specified interface */
            check_interface_for_binding( string( argv[ 0 ] ), if_name );
            server_socket.bind( if_name );
        }

        /* connect the server_socket to the server_address */
        server_socket.connect( server );
        cerr << "client listening for server on port " << server_socket.local_address().port() << endl;
        // XXX error better if this write fails because server is not accepting connections
        const wrapped_packet_header to_send = { (uint64_t) -1 };
        server_socket.write( string( (char *) &to_send, sizeof(to_send) ) );

        TunnelShell tunnelclient;
        tunnelclient.start_link( user_environment, server_socket,
                                 local_private_address, server_private_address,
                                 ingress_logfile, egress_logfile,
                                 "[tunnelclient " + server.str() + "] ",
                                 command );
        return tunnelclient.wait_for_exit();
    } catch ( const exception & e ) {
        print_exception( e );
        return EXIT_FAILURE;
    }
}

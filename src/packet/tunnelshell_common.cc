#include <string>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>

#include "tunnelshell_common.hh"
#include "exception.hh"
#include "file_descriptor.hh"

void check_interface_for_binding( const std::string &prog_name, const std::string &if_name ) {
    /* verify interface exists and has rp filtering disabled before binding to an interface */
    const std::string interface_rpf_path = "/proc/sys/net/ipv4/conf/" + if_name + "/rp_filter";
    const std::string all_rpf_path = "/proc/sys/net/ipv4/conf/all/rp_filter";

    if ( access( interface_rpf_path.c_str(), R_OK ) == -1 ) {
        throw std::runtime_error( prog_name + ": can't read " + interface_rpf_path + ", interface \"" + if_name + "\" probably doesn't exist" );
    }
    FileDescriptor rpf_interface( SystemCall( "open " + interface_rpf_path, open( interface_rpf_path.c_str(), O_RDONLY ) ) );
    FileDescriptor rpf_all( SystemCall( "open " + all_rpf_path, open( all_rpf_path.c_str(), O_RDONLY ) ) );

    if ( rpf_interface.read() != "0\n" or rpf_all.read() != "0\n" ) {
        throw std::runtime_error( prog_name + ": Can only run using --interface if both \"cat " + all_rpf_path
                + "\"= 0 and \"cat " + interface_rpf_path  + "\"= 0" );
    }
}

void send_n_wrapper_only_datagrams(const uint16_t num_packets, FileDescriptor &connected_socket, const uint64_t uid ) {
    const wrapped_packet_header to_send = { uid };
    for (uint16_t i = 0; i < num_packets; i++) {
        connected_socket.write( std::string( (char *) &to_send, sizeof(to_send) ) );
    }
}

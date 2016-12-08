#ifndef TUNNELSHELL_COMMON_HH
#define TUNNELSHELL_COMMON_HH

#include <string>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>

#include "exception.hh"
#include "file_descriptor.hh"

struct wrapped_packet_header {
    uint64_t uid;
};

void check_interface_for_binding( const std::string &prog_name, const std::string &if_name );

void send_n_wrapper_only_datagrams(const uint16_t num_packets, FileDescriptor &connected_socket, const uint64_t uid );

#endif /* TUNNELSHELL_COMMON_HH */

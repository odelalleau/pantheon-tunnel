#include <string>
#include <fcntl.h>

void check_interface_for_binding( const std::string &prog_name, const std::string &if_name ) {
    /* verify interface exists and has rp filtering disabled before binding to an interface */
    std::string rp_filter_path = "/proc/sys/net/ipv4/conf/" + if_name + "/rp_filter";

    if ( access( rp_filter_path.c_str(), R_OK ) == -1 ) {
        throw runtime_error( prog_name + ": can't read " + rp_filter_path + ", interface \"" + if_name + "\" probably doesn't exist" );
    }
    FileDescriptor rpf( SystemCall( "open " + rp_filter_path, open( rp_filter_path.c_str(), O_RDONLY ) ) );
    if ( rpf.read() != "0\n" ) {
        throw runtime_error( prog_name + ": Can only run if \"cat " + rp_filter_path + "\" = 0" );
    }
}

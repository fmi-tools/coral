/**
\file
\brief  Internal module header for dsb::net::ip.
*/
#ifndef DSB_NET_IP_HPP
#define DSB_NET_IP_HPP

#ifdef _WIN32
#   include <winsock2.h>
#else
#   include <netinet/in.h>
#endif

#include <string>
#include <vector>


namespace dsb
{
namespace net
{
namespace ip
{


/// Information about a network interface.
struct NetworkInterfaceInfo
{
    /// Interface name
    std::string name;

    /// IP address
    in_addr address;

    /// Subnet mask
    in_addr netmask;

    /// Broadcast address
    in_addr broadcastAddress;
};

/**
\brief  Returns information about available network interfaces.

\note
    On Windows, the loopback interface (typically 127.0.0.1) does not have
    a name, so the NetworkInterfaceInfo::name field will be empty.

\throws std::runtime_error on failure.
*/
std::vector<NetworkInterfaceInfo> GetNetworkInterfaces();


/// Converts an IP address to a string in dotted-decimal format.
std::string IPAddressToString(in_addr address);


/// Converts an IP address in dotted-decimal string format to an in_addr.
in_addr StringToIPAddress(const std::string& address);


}}} // namespace
#endif // header guard

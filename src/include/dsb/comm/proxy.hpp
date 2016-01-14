/**
\file
\brief  Contains a simple ZMQ proxy.
*/
#ifndef DSB_COMM_PROXY_HPP
#define DSB_COMM_PROXY_HPP

#include <chrono>
#include <memory>
#include <thread>
#include "boost/noncopyable.hpp"
#include "dsb/config.h"
#include "zmq.hpp"


namespace dsb
{
namespace comm
{


/**
\brief  An object that can be used to terminate a proxy spawned by SpawnProxy().

Objects of this class are not copyable, but they are movable.  This ensures that
there exists at most one Proxy object per spawned proxy.  It also means that
once the object reaches the end of its lifetime, there is no way to stop the
proxy manually, and it will run independently until the program terminates.
*/
class Proxy : boost::noncopyable
{
public:
    /**
    \brief  Move constructor.
    \post `other` no longer refers to a proxy, and the constructed object refers
        to the proxy previously handled by `other` (if any).
    */
    Proxy(Proxy&& other) DSB_NOEXCEPT;

    ~Proxy();

    /**
    \brief  Move assignment operator.
    \post `other` no longer refers to a proxy, and the object assigned to refers
        to the proxy previously handled by `other` (if any).
    */
    Proxy& operator=(Proxy&& rhs) DSB_NOEXCEPT;

    /**
    \brief  Stops the proxy.

    This stops the proxy immediately, without transferring any further messages
    between the sockets.  However, if the proxy is in the process of
    transferring a message when this function is called, the transfer will be
    completed.  (This also holds for multipart messages; the proxy will never
    stop before all frames are transferred.)

    Calling Stop() on a proxy which has already terminated has no effect.

    \param [in] wait    Whether the function should block until the proxy
                        thread has actually terminated.
    */
    void Stop(bool wait = false);

    /**
    \brief  Returns a reference to the `std::thread` that manages the proxy
            thread.

    \warning
        This function is included mainly for debugging and testing purposes,
        and may be removed in the future.  Do not rely on its existence.
    */
    std::thread& Thread__();

private:
    // Only SpawnProxy() may construct this class.
    friend Proxy SpawnProxy(
        zmq::socket_t&& socket1,
        zmq::socket_t&& socket2,
        std::chrono::milliseconds silenceTimeout);
    Proxy(zmq::socket_t controlSocket, std::thread thread);

    zmq::socket_t m_controlSocket;
    std::thread m_thread;
};


/**
\brief  A special value for the `silenceTimeout` parameter of SpawnProxy() which
        means the proxy should never terminate due to silence.
*/
const auto NEVER_TIMEOUT = std::chrono::milliseconds(-1);


/**
\brief  Spawns a ZMQ proxy.

This function spawns a proxy that runs in a separate thread.  Given two ZMQ
sockets, the proxy receives all incoming messages on either socket and sends
them immediately on the other.  It will keep running until one of the following
occur:

  - The proxy is explicitly terminated with Proxy::Stop().
  - No messages have been received on either socket for a (user-specified)
    period of time (see `silenceTimeout` below).
  - The program terminates.

It is recommended that `socket1` and `socket2` be sockets created with the ZMQ
context returned by dsb::comm:GlobalContext().  Otherwise, care must be
taken to ensure that their context(s) are kept alive for as long as the proxy
is running.

\param [in] socket1
    One of the sockets that should be linked by the proxy.
\param [in] socket2
    One of the sockets that should be linked by the proxy.
\param [in] silenceTimeout
    A duration after which the proxy will terminate if there have been no
    incoming messages on either socket.  This may take the special value
    #NEVER_TIMEOUT, which disables this feature.

\returns An object that can be used to stop the proxy manually.
    This object can safely be discarded if this functionality is not required.

\throws zmq::error_t if ZMQ reports an error.

\pre `socket1` and `socket2` refer to valid, connected/bound sockets.
\post `socket1` and `socket2` no longer refer to sockets (as their contents have
    been moved to the proxy thread).
*/
Proxy SpawnProxy(
    zmq::socket_t&& socket1,
    zmq::socket_t&& socket2,
    std::chrono::milliseconds silenceTimeout = NEVER_TIMEOUT);


/**
\brief  Spawns a ZMQ proxy. (Convenience function.)

This function creates two ZMQ sockets and binds them to the specified endpoints.
It then forwards to SpawnProxy() to spawn the actual proxy.

\param [in] socketType1
    The type of the first socket.
\param [in] endpoint1
    The endpoint to which the first socket gets bound.
\param [in] socketType2
    The type of the second socket.
\param [in] endpoint2
    The endpoint to which the second socket gets bound.
\param [in] silenceTimeout
    A duration after which the proxy will terminate if there have been no
    incoming messages on either socket.  This may take the special value
    #NEVER_TIMEOUT, which disables this feature.

\returns The return value of SpawnProxy()

\throws zmq::error_t if ZMQ reports an error.

\pre `socketType1` and `socketType2` are valid ZMQ socket types,
     `endpoint1` and `endpoint2` are well-formed ZMQ endpoint specifications.
*/
Proxy SpawnProxy(
    int socketType1, const std::string& endpoint1,
    int socketType2, const std::string& endpoint2,
    std::chrono::milliseconds silenceTimeout = NEVER_TIMEOUT);


}}      // namespace
#endif  // header guard

/**
\file
\brief Functionality for starting and controlling a simulation domain.
*/
#ifndef DSB_DOMAIN_CONTROLLER_HPP
#define DSB_DOMAIN_CONTROLLER_HPP

#include <memory>
#include <string>
#include <vector>
#include "zmq.hpp"
#include "dsb/model.hpp"


namespace dsb
{
namespace domain
{


/**
\brief  Domain interface.
*/
// TODO: Decide what happens when the Controller object goes out of scope!
class Controller
{
public:
    /// Information about a slave type.
    struct SlaveType
    {
        std::string name;
        std::string uuid;
        std::string description;
        std::string author;
        std::string version;
        std::vector<dsb::model::Variable> variables;
        std::vector<std::string> providers;
    };

    /// Constructor.
    Controller(
        std::shared_ptr<zmq::context_t> context,
        const std::string& reportEndpoint,
        const std::string& infoEndpoint);

    /// Move constructor.
    Controller(Controller&& other);

    /// Move assignment operator.
    Controller& operator=(Controller&& other);

    /**
    \brief  Returns available slave types.
    */
    std::vector<SlaveType> GetSlaveTypes();

private:
    zmq::socket_t m_rpcSocket;
};


}}      //namespace
#endif  // header guard
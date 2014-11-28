#ifndef DSB_DOMAIN_LOCATOR
#define DSB_DOMAIN_LOCATOR

#include <string>


namespace dsb
{
namespace domain
{


class Locator
{
public:
    Locator() { }

    Locator(
        const std::string& reportMasterEndpoint,
        const std::string& reportSlavePEndpoint,
        const std::string& infoMasterEndpoint,
        const std::string& infoSlavePEndpoint,
        const std::string& execReqEndpoint);

    const std::string& ReportMasterEndpoint() const;
    const std::string& ReportSlavePEndpoint() const;
    const std::string& InfoMasterEndpoint() const;
    const std::string& InfoSlavePEndpoint() const;
    const std::string& ExecReqEndpoint() const;

private:
    std::string m_reportMasterEndpoint;
    std::string m_reportSlavePEndpoint;
    std::string m_infoMasterEndpoint;
    std::string m_infoSlavePEndpoint;
    std::string m_execReqEndpoint;
};


//TODO: This function is temporary and should be moved or removed.
Locator GetDomainEndpoints(const std::string& domainBrokerAddress);


}}      // namespace
#endif  // header guard

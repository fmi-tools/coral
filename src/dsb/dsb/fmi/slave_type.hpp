#ifndef DSB_FMI_SLAVE_TYPE_HPP
#define DSB_FMI_SLAVE_TYPE_HPP

#include <string>
#include "fmilib.h"
#include "fmilibcpp/fmi1/Fmu.hpp"
#include "dsb/domain/slave_provider.hpp"
#include "dsb/util.hpp"


namespace dsb
{
namespace fmi
{


class FmiSlaveType : public dsb::domain::ISlaveType
{
public:
    FmiSlaveType(
        const std::string& fmuPath,
        const std::string& slaveExePath);
    ~FmiSlaveType();

    std::string Name() const override;
    std::string Uuid() const override;
    std::string Description() const override;
    std::string Author() const override;
    std::string Version() const override;
    size_t VariableCount() const override;
    virtual dsb::model::Variable Variable(size_t index) const override;
    virtual bool InstantiateAndConnect(dsb::model::SlaveID slaveID /* TODO: Execution locator */) override;
    virtual std::string InstantiationFailureDescription() const override;

private:
    const std::string m_slaveExePath;
    dsb::util::TempDir m_unzipDir;
    std::shared_ptr<fmilib::fmi1::Fmu> m_fmu;
    fmi1_import_variable_list_t* m_varList;
};


}}      // namespace
#endif  // header guard

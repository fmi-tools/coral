#include "dsb/bus/slave_setup.hpp"

#include <cassert>
#include <limits>


namespace dsb
{
namespace bus
{


SlaveSetup::SlaveSetup()
    : startTime(std::numeric_limits<dsb::model::TimePoint>::signaling_NaN()),
      stopTime(std::numeric_limits<dsb::model::TimePoint>::signaling_NaN())
{
}


SlaveSetup::SlaveSetup(
    dsb::model::TimePoint startTime_,
    dsb::model::TimePoint stopTime_,
    const std::string& variablePubEndpoint_,
    const std::string& variableSubEndpoint_,
    const std::string& executionName_)
    : startTime(startTime_),
      stopTime(stopTime_),
      variablePubEndpoint(variablePubEndpoint_),
      variableSubEndpoint(variableSubEndpoint_),
      executionName(executionName_)
{
    assert(startTime <= stopTime);
    assert(!variablePubEndpoint.empty());
    assert(!variableSubEndpoint.empty());
}


}} // namespace

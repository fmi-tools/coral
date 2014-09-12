#include "dsb/bus/execution_state.hpp"

#include <iostream>
#include "boost/foreach.hpp"

#include "dsb/bus/execution_agent.hpp"
#include "dsb/bus/slave_tracker.hpp"
#include "dsb/comm.hpp"
#include "dsb/control.hpp"
#include "control.pb.h"


namespace dsb
{
namespace bus
{


namespace
{
    void SendOk(zmq::socket_t& socket)
    {
        auto m = dsb::comm::ToFrame("OK");
        socket.send(m);
    }

    void SendFailed(zmq::socket_t& socket, const std::string& reason)
    {
        auto m0 = dsb::comm::ToFrame("FAILED");
        auto m1 = dsb::comm::ToFrame(reason);
        socket.send(m0, ZMQ_MORE);
        socket.send(m1);
    }

    // This function handles an ADD_SLAVE call from the user.  It sends a (more or
    // less) immediate reply, which is either OK or FAILED.
    // The latter happens if the supplied slave ID already exists.
    void PerformAddSlaveRPC(
        ExecutionAgent& self,
        std::deque<zmq::message_t>& msg,
        zmq::socket_t& userSocket)
    {
        assert (self.rpcInProgress == ExecutionAgent::NO_RPC
                && "Cannot perform ADD_SLAVE when another RPC is in progress");
        assert (msg.size() == 2
                && "ADD_SLAVE message must be exactly two frames");
        assert (dsb::comm::ToString(msg.front()) == "ADD_SLAVE"
                && "PerformAddSlaveRPC() received non-ADD_SLAVE command");

        const auto slaveId = dsb::comm::DecodeRawDataFrame<uint16_t>(msg[1]);
        if (self.slaves.insert(std::make_pair(slaveId, dsb::bus::SlaveTracker())).second) {
            SendOk(userSocket);
        } else {
            SendFailed(userSocket, "Slave already added");
        }
    }

    // This function handles a SET_VARS call from the user.  It sends a (more or
    // less) immediate reply, which is either OK or FAILED.
    // The latter happens if the supplied slave ID is invalid.  Any errors
    // reported by the slave in question are reported asynchronously, and not
    // handled by this function at all.
    void PerformSetVarsRPC(
        ExecutionAgent& self,
        std::deque<zmq::message_t>& msg,
        zmq::socket_t& userSocket,
        zmq::socket_t& slaveSocket)
    {
        assert (self.rpcInProgress == ExecutionAgent::NO_RPC
                && "Cannot perform SET_VARS when another RPC is in progress");
        assert (msg.size() >= 2
                && "SET_VARS message must be at least two frames");
        assert (dsb::comm::ToString(msg.front()) == "SET_VARS"
                && "PerformSetVarsRPC() received non-SET_VARS command");

        const auto slaveId = dsb::comm::DecodeRawDataFrame<uint16_t>(msg[1]);
        auto it = self.slaves.find(slaveId);
        if (it == self.slaves.end()) {
            // TODO: Send some more info with the error.
            SendFailed(userSocket, "Invalid slave ID");
        } else {
            dsbproto::control::SetVarsData data;
            for (size_t i = 2; i < msg.size(); i+=2) {
                auto& newVar = *data.add_variable();
                newVar.set_id(dsb::comm::DecodeRawDataFrame<uint16_t>(msg[i]));
                newVar.mutable_value()->set_real_value(
                    dsb::comm::DecodeRawDataFrame<double>(msg[i+1]));
            }
            it->second.EnqueueSetVars(slaveSocket, data);
            SendOk(userSocket);
        }
    }
}


// =============================================================================
// Initializing
// =============================================================================

ExecutionInitializing::ExecutionInitializing() : m_waitingForReady(false) { }

void ExecutionInitializing::StateEntered(
    ExecutionAgent& self,
    zmq::socket_t& userSocket,
    zmq::socket_t& slaveSocket)
{
    // This assert may be removed in the future, if we add RPCs that may cross
    // into the "initialized" state.
    assert (self.rpcInProgress == ExecutionAgent::NO_RPC);
}

void ExecutionInitializing::UserMessage(
    ExecutionAgent& self,
    std::deque<zmq::message_t>& msg,
    zmq::socket_t& userSocket,
    zmq::socket_t& slaveSocket)
{
    assert (self.rpcInProgress == ExecutionAgent::NO_RPC);
    assert (!msg.empty());
    const auto msgType = dsb::comm::ToString(msg[0]);
    if (msgType == "SET_VARS") {
        PerformSetVarsRPC(self, msg, userSocket, slaveSocket);
    } else if (msgType == "WAIT_FOR_READY") {
        self.rpcInProgress = ExecutionAgent::WAIT_FOR_READY_RPC;
    } else if (msgType == "TERMINATE") {
        self.ChangeState<ExecutionTerminating>(userSocket, slaveSocket);
        SendOk(userSocket);
    } else if (msgType == "ADD_SLAVE") {
        PerformAddSlaveRPC(self, msg, userSocket);
    // } else if (message is CONNECT_VARS) {
    //      queue CONNECT_VARS on SlaveTracker
    } else {
        assert (false);
    }
}

void ExecutionInitializing::SlaveWaiting(
    ExecutionAgent& self,
    SlaveTracker& slaveHandler,
    zmq::socket_t& userSocket,
    zmq::socket_t& slaveSocket)
{
    // Check whether all slaves are Ready, and if so, switch to Ready state.
    bool allReady = true;
    BOOST_FOREACH (const auto& slave, self.slaves) {
        if (slave.second.State() != SLAVE_READY) allReady = false;
    }
    if (allReady) self.ChangeState<ExecutionReady>(userSocket, slaveSocket);
}

// =============================================================================
// Ready
// =============================================================================

void ExecutionReady::StateEntered(
    ExecutionAgent& self,
    zmq::socket_t& userSocket,
    zmq::socket_t& slaveSocket)
{
    // Any RPC in progress will by definition have succeeded when this state is
    // reached.
    if (self.rpcInProgress != ExecutionAgent::NO_RPC) {
        assert (self.rpcInProgress == ExecutionAgent::WAIT_FOR_READY_RPC
                || self.rpcInProgress == ExecutionAgent::STEP_RPC);
        SendOk(userSocket);
        self.rpcInProgress = ExecutionAgent::NO_RPC;
    }
}

void ExecutionReady::UserMessage(
    ExecutionAgent& self,
    std::deque<zmq::message_t>& msg,
    zmq::socket_t& userSocket,
    zmq::socket_t& slaveSocket)
{
    assert (self.rpcInProgress == ExecutionAgent::NO_RPC);
    assert (!msg.empty());
    const auto msgType = dsb::comm::ToString(msg[0]);
    if (msgType == "STEP") {
        assert (msg.size() == 3);
        const auto time     = dsb::comm::DecodeRawDataFrame<double>(msg[1]);
        const auto stepSize = dsb::comm::DecodeRawDataFrame<double>(msg[2]);

        // Create the STEP message body
        dsbproto::control::StepData stepData;
        stepData.set_timepoint(time);
        stepData.set_stepsize(stepSize);
        BOOST_FOREACH(auto& slave, self.slaves) {
            slave.second.SendStep(slaveSocket, stepData);
        }
        self.rpcInProgress = ExecutionAgent::STEP_RPC;
        self.ChangeState<ExecutionStepping>(userSocket, slaveSocket);
    } else if (msgType == "TERMINATE") {
        self.ChangeState<ExecutionTerminating>(userSocket, slaveSocket);
        SendOk(userSocket);
    } else if (msgType == "ADD_SLAVE") {
        PerformAddSlaveRPC(self, msg, userSocket);
    } else if (msgType == "SET_VARS") {
        PerformSetVarsRPC(self, msg, userSocket, slaveSocket);
        self.ChangeState<ExecutionInitializing>(userSocket, slaveSocket);
    } else if (msgType == "WAIT_FOR_READY") {
        SendOk(userSocket);
    } else {
    //  if message is CONNECT_VARS
    //      queue CONNECT_VARS on SlaveTracker
        assert (false);
    }
}

void ExecutionReady::SlaveWaiting(
    ExecutionAgent& self,
    SlaveTracker& slaveHandler,
    zmq::socket_t& userSocket,
    zmq::socket_t& slaveSocket)
{
}

// =============================================================================
// Stepping
// =============================================================================

void ExecutionStepping::StateEntered(
    ExecutionAgent& self,
    zmq::socket_t& userSocket,
    zmq::socket_t& slaveSocket)
{
    assert (self.rpcInProgress == ExecutionAgent::STEP_RPC);
}

void ExecutionStepping::UserMessage(
    ExecutionAgent& self,
    std::deque<zmq::message_t>& msg,
    zmq::socket_t& userSocket,
    zmq::socket_t& slaveSocket)
{
    assert (false);
}

void ExecutionStepping::SlaveWaiting(
    ExecutionAgent& self,
    SlaveTracker& slaveHandler,
    zmq::socket_t& userSocket,
    zmq::socket_t& slaveSocket)
{
    bool allPublished = true;
    BOOST_FOREACH (const auto& slave, self.slaves) {
        if (slave.second.IsSimulating() && slave.second.State() != SLAVE_PUBLISHED) {
            allPublished = false;
        }
    }
    if (allPublished) {
        self.ChangeState<ExecutionPublished>(userSocket, slaveSocket);
    }
}

// =============================================================================
// Published
// =============================================================================

void ExecutionPublished::StateEntered(
    ExecutionAgent& self,
    zmq::socket_t& userSocket,
    zmq::socket_t& slaveSocket)
{
    assert (self.rpcInProgress == ExecutionAgent::STEP_RPC);
    BOOST_FOREACH (auto& slave, self.slaves) {
        if (slave.second.IsSimulating()) {
            slave.second.SendRecvVars(slaveSocket);
        } else assert (false);
    }
}

void ExecutionPublished::UserMessage(
    ExecutionAgent& self,
    std::deque<zmq::message_t>& msg,
    zmq::socket_t& userSocket,
    zmq::socket_t& slaveSocket)
{
    assert (false);
}

void ExecutionPublished::SlaveWaiting(
    ExecutionAgent& self,
    SlaveTracker& slaveHandler,
    zmq::socket_t& userSocket,
    zmq::socket_t& slaveSocket)
{
    // Check whether all slaves are Ready, and if so, switch to Ready state.
    bool allReady = true;
    BOOST_FOREACH (const auto& slave, self.slaves) {
        if (slave.second.State() != SLAVE_READY) allReady = false;
    }
    if (allReady) self.ChangeState<ExecutionReady>(userSocket, slaveSocket);
}

// =============================================================================
// Terminating
// =============================================================================

// TODO: Shut down ExecutionAgent too!

ExecutionTerminating::ExecutionTerminating()
{
}

void ExecutionTerminating::StateEntered(
    ExecutionAgent& self,
    zmq::socket_t& userSocket,
    zmq::socket_t& slaveSocket)
{
    BOOST_FOREACH (auto& slave, self.slaves) {
        if (slave.second.State() & TERMINATABLE_STATES) {
            slave.second.SendTerminate(slaveSocket);
        }
    }
}

void ExecutionTerminating::UserMessage(
    ExecutionAgent& self,
    std::deque<zmq::message_t>& msg,
    zmq::socket_t& userSocket,
    zmq::socket_t& slaveSocket)
{
    assert (false);
}

void ExecutionTerminating::SlaveWaiting(
    ExecutionAgent& self,
    SlaveTracker& slaveHandler,
    zmq::socket_t& userSocket,
    zmq::socket_t& slaveSocket)
{
    assert (slaveHandler.State() & TERMINATABLE_STATES);
    slaveHandler.SendTerminate(slaveSocket);
}


}} // namespace

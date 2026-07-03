#pragma once

/*
    MCPCompat — extraction-compat re-export shim (spatcore namespace pass).

    The app-agnostic MCP core (registries, dispatcher, transport, change
    records, tier enforcement) moved to spatcore/control/mcp and now lives in
    namespace spatcore::control::mcp. App code historically names these types
    inside namespace WFSNetwork, so this header re-exports every public core
    symbol under WFSNetwork — existing call sites compile unchanged. App files
    include THIS header instead of the spatcore headers directly (same pattern
    as OSCProtocolTypes.h for the OSC-side core types).

    New code should prefer the qualified spatcore::control::mcp:: names.
*/

#include "../../../spatcore/control/mcp/MCPChangeRecords.h"
#include "../../../spatcore/control/mcp/MCPDispatcher.h"
#include "../../../spatcore/control/mcp/MCPLogSink.h"
#include "../../../spatcore/control/mcp/MCPPromptRegistry.h"
#include "../../../spatcore/control/mcp/MCPResourceRegistry.h"
#include "../../../spatcore/control/mcp/MCPTierEnforcement.h"
#include "../../../spatcore/control/mcp/MCPToolRegistry.h"
#include "../../../spatcore/control/mcp/MCPTransport.h"
#include "../../../spatcore/control/mcp/MCPUndoHooks.h"

namespace WFSNetwork
{

// MCPChangeRecords.h
using spatcore::control::mcp::AffectedGroup;
using spatcore::control::mcp::ChangeSubWrite;
using spatcore::control::mcp::ChangeRecord;
using spatcore::control::mcp::MCPChangeRecordBuffer;

// MCPDispatcher.h
using spatcore::control::mcp::ServerIdentity;
using spatcore::control::mcp::MCPDispatcher;

// MCPLogSink.h
using spatcore::control::mcp::MCPLogSink;

// MCPPromptRegistry.h
using spatcore::control::mcp::PromptArgument;
using spatcore::control::mcp::PromptEntry;
using spatcore::control::mcp::MCPPromptRegistry;

// MCPResourceRegistry.h
using spatcore::control::mcp::ResourceEntry;
using spatcore::control::mcp::MCPResourceRegistry;

// MCPTierEnforcement.h
using spatcore::control::mcp::MCPTierEnforcement;

// MCPToolRegistry.h
using spatcore::control::mcp::kTier2DescriptionSuffix;
using spatcore::control::mcp::kTier3DescriptionSuffix;
using spatcore::control::mcp::ToolResult;
using spatcore::control::mcp::ToolDescriptor;
using spatcore::control::mcp::MCPToolRegistry;

// MCPTransport.h
using spatcore::control::mcp::MCPTransport;

// MCPUndoHooks.h
using spatcore::control::mcp::MCPUndoHooks;

} // namespace WFSNetwork

#include <signal.h>

#include <numeric>
#include <vector>

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

static std::vector<int> eventBuffer(10);

static UA_StatusCode
readFromBuffer(UA_Server* server, const UA_NodeId* sessionId,
               void* sessionContext, const UA_NodeId* nodeId, void* nodeContext,
               UA_Boolean sourceTimeStamp, const UA_NumericRange* range,
               UA_DataValue* dataValue)
{
    UA_Int32 nextVal = 0;
    if (eventBuffer.size()) {
        nextVal = eventBuffer.back();
        eventBuffer.pop_back();
    } else {
        nextVal = -1;
    }

    UA_String nodeIdString = UA_STRING_NULL;
    UA_NodeId_print(sessionId, &nodeIdString);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "send value: %d, session Id: %s", nextVal, nodeIdString.data);
    UA_String_clear(&nodeIdString);

    UA_Variant_setScalarCopy(&dataValue->value, &nextVal,
                             &UA_TYPES[UA_TYPES_INT32]);
    dataValue->hasValue = true;

    return UA_STATUSCODE_GOOD;
}

static void addDataSourceVariable(UA_Server* server)
{
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName =
        UA_LOCALIZEDTEXT((char*)"en-US", (char*)"SimpleIntegerVariable");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;

    UA_NodeId thisNodeId = UA_NODEID_NUMERIC(1, 1);
    UA_QualifiedName currentName =
        UA_QUALIFIEDNAME(1, (char*)"simple-integer-variable-datasource");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableTypeNodeId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);

    UA_DataSource ds;
    ds.read = readFromBuffer;
    ds.write = NULL;
    UA_Server_addDataSourceVariableNode(
        server, thisNodeId, parentNodeId, parentReferenceNodeId, currentName,
        variableTypeNodeId, attr, ds, NULL, NULL);
}

static volatile UA_Boolean running = true;
static void stopHandler(int sign)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

static void closeSession_default(UA_Server* /*server*/,
                                 UA_AccessControl* /*ac*/,
                                 const UA_NodeId* sessionId,
                                 void* sessionContext)
{
    if (sessionContext)
        UA_ByteString_delete((UA_ByteString*)sessionContext);

    UA_String nodeIdString = UA_STRING_NULL;
    UA_NodeId_print(sessionId, &nodeIdString);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Close session %s",
                nodeIdString.data);
    UA_String_clear(&nodeIdString);
}

static UA_StatusCode
activateSession_default(UA_Server* server, UA_AccessControl* ac,
                        const UA_EndpointDescription* endpointDescription,
                        const UA_ByteString* secureChannelRemoteCertificate,
                        const UA_NodeId* sessionId,
                        const UA_ExtensionObject* userIdentityToken,
                        void** sessionContext)
{
    UA_String nodeIdString = UA_STRING_NULL;
    UA_NodeId_print(sessionId, &nodeIdString);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Activate session %s",
                nodeIdString.data);
    UA_String_clear(&nodeIdString);

    return UA_STATUSCODE_GOOD;
}

int main(void)
{
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    int i = 0;
    std::iota(std::begin(eventBuffer), std::end(eventBuffer), i++);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "event buffer size set to %lu", eventBuffer.size());

    UA_Server* server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    UA_ServerConfig* config = UA_Server_getConfig(server);
    config->accessControl.activateSession = activateSession_default;
    config->accessControl.closeSession = closeSession_default;

    addDataSourceVariable(server);

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
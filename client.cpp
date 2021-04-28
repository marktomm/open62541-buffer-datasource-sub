#include <signal.h>

#include <sstream>

#include <open62541/client_config_default.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>

static void deleteSubscriptionCallback(UA_Client* client,
                                       UA_UInt32 subscriptionId,
                                       void* subscriptionContext)
{
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                 "Subscription Id %d was deleted", subscriptionId);
}
static void handler_sub(UA_Client* client, UA_UInt32 subId, void* subContext,
                        UA_UInt32 monId, void* monContext, UA_DataValue* value)
{
    // is server sets hasValue to false, this will still be true
    if (value->hasValue) {
        UA_Int32 val = *(UA_Int32*)(value->value.data);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Got value: %d",
                    val);
    }
}

static volatile UA_Boolean running = true;
static void stopHandler(int sign)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

int main(void)
{
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Client* client = UA_Client_new();
    UA_ClientConfig* cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);

    std::stringstream ss;
    ss << "opc.tcp://localhost:" << 4840;
    UA_StatusCode status = UA_Client_connect(client, ss.str().c_str());
    if (status != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Unable to connect to server because: %s",
                     UA_StatusCode_name(status));
        return 1;
    }
    // Create a subscription
    UA_CreateSubscriptionRequest request =
        UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(
        client, request, NULL, NULL, deleteSubscriptionCallback);
    if (response.responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Create subscription succeeded, id %d",
                    response.subscriptionId);
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Unable to create subscription because: %s",
                     UA_StatusCode_name(response.responseHeader.serviceResult));
        UA_Client_delete(client);
        return 1;
    }

    // Add a MonitoredItem
    uint32_t vDiId = 1;
    // UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "vDiId: %d", vDiId);
    UA_NodeId vDiNode = UA_NODEID_NUMERIC(1, vDiId);
    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(vDiNode);
    monRequest.requestedParameters.queueSize = 10;
    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(
            client, response.subscriptionId, UA_TIMESTAMPSTORETURN_BOTH,
            monRequest, NULL, handler_sub, NULL);
    if (monResponse.statusCode == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Monitoring %d, %d",
                    vDiId, monResponse.monitoredItemId);
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Monitoring failed for %d, id %d because %s", vDiId,
                     monResponse.monitoredItemId,
                     UA_StatusCode_name(monResponse.statusCode));

        UA_Client_delete(client);
        return 1;
    }

#ifndef ABRUPT_DISCONNECT
    while (running) {
#endif
        UA_Client_run_iterate(client, 1000);
#ifndef ABRUPT_DISCONNECT
    }
#endif

#ifndef ABRUPT_DISCONNECT
    UA_Client_delete(client);
#endif
    return 0;
}
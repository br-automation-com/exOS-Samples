//KNOWN ISSUES
/*
NO checks on values are made. NodeJS har as a javascript language only "numbers" that will be created from SINT, INT etc.
This means that when writing from NodeJS to Automation Runtime, you should take care of that the value actually fits into 
the value assigned.

String arrays will most probably not work, as they are basically char[][]...

Strings are encoded as utf8 strings in NodeJS which means that special chars will reduce length of string. And generate funny 
charachters in Automation Runtime.

PLCs WSTRING is not supported.

Enums defined in typ file will parse to DINT (uint32_t). Enums are not supported in JavaScript.

Generally the generates code is not yet fully and understanably error handled. ex. if (napi_ok != .....

The code generated is NOT yet fully formatted to ones normal liking. There are missing indentations.
*/

#define NAPI_VERSION 6
#include <node_api.h>
#include <stdint.h>
#include <exos_api.h>
#include <exos_log.h>
#include "exos_watertank.h"
#include <uv.h>
#include <unistd.h>
#include <string.h>

#define SUCCESS(_format_, ...) exos_log_success(&logger, EXOS_LOG_TYPE_USER, _format_, ##__VA_ARGS__);
#define INFO(_format_, ...) exos_log_info(&logger, EXOS_LOG_TYPE_USER, _format_, ##__VA_ARGS__);
#define VERBOSE(_format_, ...) exos_log_debug(&logger, EXOS_LOG_TYPE_USER + EXOS_LOG_TYPE_VERBOSE, _format_, ##__VA_ARGS__);
#define ERROR(_format_, ...) exos_log_error(&logger, _format_, ##__VA_ARGS__);

#define BUR_NAPI_DEFAULT_BOOL_INIT false
#define BUR_NAPI_DEFAULT_NUM_INIT 0
#define BUR_NAPI_DEFAULT_STRING_INIT ""

static exos_log_handle_t logger;

typedef struct
{
    napi_ref ref;
    uint32_t ref_count;
    napi_threadsafe_function onchange_cb;
    napi_threadsafe_function connectiononchange_cb;
    napi_threadsafe_function onprocessed_cb; //used only for datamodel
    napi_value object_value; //volatile placeholder.
    napi_value value;        //volatile placeholder.
} obj_handles;

obj_handles watertank = {};
obj_handles FillValve = {};
obj_handles EnableHeater = {};
obj_handles HeaterConfig = {};
obj_handles Status = {};

napi_deferred deferred = NULL;
uv_idle_t cyclic_h;

WaterTank exos_data = {};
exos_datamodel_handle_t watertank_datamodel;
exos_dataset_handle_t FillValve_dataset;
exos_dataset_handle_t EnableHeater_dataset;
exos_dataset_handle_t HeaterConfig_dataset;
exos_dataset_handle_t Status_dataset;

// error handling (Node.js)
static void throw_fatal_exception_callbacks(napi_env env, const char *defaultCode, const char *defaultMessage)
{
    napi_value err;
    bool is_exception = false;

    napi_is_exception_pending(env, &is_exception);

    if (is_exception)
    {
        napi_get_and_clear_last_exception(env, &err);
        napi_fatal_exception(env, err);
    }
    else
    {
        napi_value code, msg;
        napi_create_string_utf8(env, defaultCode, NAPI_AUTO_LENGTH, &code);
        napi_create_string_utf8(env, defaultMessage, NAPI_AUTO_LENGTH, &msg);
        napi_create_error(env, code, msg, &err);
        napi_fatal_exception(env, err);
    }
}

// exOS callbacks
static void datasetEvent(exos_dataset_handle_t *dataset, EXOS_DATASET_EVENT_TYPE event_type, void *info)
{
    switch (event_type)
    {
    case EXOS_DATASET_EVENT_UPDATED:
        VERBOSE("dataset %s updated! latency (us):%i", dataset->name, (exos_datamodel_get_nettime(dataset->datamodel) - dataset->nettime));
        if(0 == strcmp(dataset->name,"FillValve"))
        {
            if (FillValve.onchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(FillValve.onchange_cb);
                napi_call_threadsafe_function(FillValve.onchange_cb, &dataset->nettime, napi_tsfn_blocking);
                napi_release_threadsafe_function(FillValve.onchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name,"EnableHeater"))
        {
            if (EnableHeater.onchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(EnableHeater.onchange_cb);
                napi_call_threadsafe_function(EnableHeater.onchange_cb, &dataset->nettime, napi_tsfn_blocking);
                napi_release_threadsafe_function(EnableHeater.onchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name,"HeaterConfig"))
        {
            if (HeaterConfig.onchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(HeaterConfig.onchange_cb);
                napi_call_threadsafe_function(HeaterConfig.onchange_cb, &dataset->nettime, napi_tsfn_blocking);
                napi_release_threadsafe_function(HeaterConfig.onchange_cb, napi_tsfn_release);
            }
        }
        break;

    case EXOS_DATASET_EVENT_PUBLISHED:
        VERBOSE("dataset %s published!", dataset->name);
        // fall through

    case EXOS_DATASET_EVENT_DELIVERED:
        if (event_type == EXOS_DATASET_EVENT_DELIVERED) { VERBOSE("dataset %s delivered!", dataset->name); }

        if(0 == strcmp(dataset->name, "HeaterConfig"))
        {
            //WaterTankHeaterConfig *heaterconfig = (WaterTankHeaterConfig *)dataset->data;
        }
        else if(0 == strcmp(dataset->name, "Status"))
        {
            //WaterTankStatus *status = (WaterTankStatus *)dataset->data;
        }
        break;

    case EXOS_DATASET_EVENT_CONNECTION_CHANGED:
        VERBOSE("dataset %s connecton changed to: %s", dataset->name, exos_get_state_string(dataset->connection_state));

        if(0 == strcmp(dataset->name, "FillValve"))
        {
            if (FillValve.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(FillValve.connectiononchange_cb);
                napi_call_threadsafe_function(FillValve.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(FillValve.connectiononchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name, "EnableHeater"))
        {
            if (EnableHeater.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(EnableHeater.connectiononchange_cb);
                napi_call_threadsafe_function(EnableHeater.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(EnableHeater.connectiononchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name, "HeaterConfig"))
        {
            if (HeaterConfig.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(HeaterConfig.connectiononchange_cb);
                napi_call_threadsafe_function(HeaterConfig.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(HeaterConfig.connectiononchange_cb, napi_tsfn_release);
            }
        }
        else if(0 == strcmp(dataset->name, "Status"))
        {
            if (Status.connectiononchange_cb != NULL)
            {
                napi_acquire_threadsafe_function(Status.connectiononchange_cb);
                napi_call_threadsafe_function(Status.connectiononchange_cb, exos_get_state_string(dataset->connection_state), napi_tsfn_blocking);
                napi_release_threadsafe_function(Status.connectiononchange_cb, napi_tsfn_release);
            }
        }

        switch (dataset->connection_state)
        {
        case EXOS_STATE_DISCONNECTED:
        case EXOS_STATE_CONNECTED:
        case EXOS_STATE_OPERATIONAL:
        case EXOS_STATE_ABORTED:
            break;
        }
        break;
    default:
        break;

    }
}

static void datamodelEvent(exos_datamodel_handle_t *datamodel, const EXOS_DATAMODEL_EVENT_TYPE event_type, void *info)
{
    switch (event_type)
    {
    case EXOS_DATAMODEL_EVENT_CONNECTION_CHANGED:
        INFO("application WaterTank changed state to %s", exos_get_state_string(datamodel->connection_state));

        if (watertank.connectiononchange_cb != NULL)
        {
            napi_acquire_threadsafe_function(watertank.connectiononchange_cb);
            napi_call_threadsafe_function(watertank.connectiononchange_cb, exos_get_state_string(datamodel->connection_state), napi_tsfn_blocking);
            napi_release_threadsafe_function(watertank.connectiononchange_cb, napi_tsfn_release);
        }

        switch (datamodel->connection_state)
        {
        case EXOS_STATE_DISCONNECTED:
        case EXOS_STATE_CONNECTED:
            break;
        case EXOS_STATE_OPERATIONAL:
            SUCCESS("WaterTank operational!");
            break;
        case EXOS_STATE_ABORTED:
            ERROR("WaterTank application error %d (%s) occured", datamodel->error, exos_get_error_string(datamodel->error));
            break;
        }
        break;
    case EXOS_DATAMODEL_EVENT_SYNC_STATE_CHANGED:
        break;

    default:
        break;

    }
}

// napi callback setup main function
static napi_value init_napi_onchange(napi_env env, napi_callback_info info, const char *identifier, napi_threadsafe_function_call_js call_js_cb, napi_threadsafe_function *result)
{
    size_t argc = 1;
    napi_value argv[1];

    if (napi_ok != napi_get_cb_info(env, info, &argc, argv, NULL, NULL))
    {
        char msg[100] = {};
        strcpy(msg, "init_napi_onchange() napi_get_cb_info failed - ");
        strcat(msg, identifier);
        napi_throw_error(env, "EINVAL", msg);
        return NULL;
    }

    if (argc < 1)
    {
        napi_throw_error(env, "EINVAL", "Too few arguments");
        return NULL;
    }

    napi_value work_name;
    if (napi_ok != napi_create_string_utf8(env, identifier, NAPI_AUTO_LENGTH, &work_name))
    {
        char msg[100] = {};
        strcpy(msg, "init_napi_onchange() napi_create_string_utf8 failed - ");
        strcat(msg, identifier);
        napi_throw_error(env, "EINVAL", msg);
        return NULL;
    }

    napi_valuetype cb_typ;
    if (napi_ok != napi_typeof(env, argv[0], &cb_typ))
    {
        char msg[100] = {};
        strcpy(msg, "init_napi_onchange() napi_typeof failed - ");
        strcat(msg, identifier);
        napi_throw_error(env, "EINVAL", msg);
        return NULL;
    }

    if (cb_typ == napi_function)
    {
        if (napi_ok != napi_create_threadsafe_function(env, argv[0], NULL, work_name, 0, 1, NULL, NULL, NULL, call_js_cb, result))
        {
            const napi_extended_error_info *info;
            napi_get_last_error_info(env, &info);
            napi_throw_error(env, NULL, info->error_message);
            return NULL;
        }
    }
    return NULL;
}

// js object callbacks
static void watertank_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value napi_true, napi_false, undefined;

    napi_get_undefined(env, &undefined);

    napi_get_boolean(env, true, &napi_true);
    napi_get_boolean(env, false, &napi_false);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &watertank.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - watertank.value");

    if (napi_ok != napi_get_reference_value(env, watertank.ref, &watertank.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - watertank ");

    switch (watertank_datamodel.connection_state)
    {
    case EXOS_STATE_DISCONNECTED:
        if (napi_ok != napi_set_named_property(env, watertank.object_value, "isConnected", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - watertank");

        if (napi_ok != napi_set_named_property(env, watertank.object_value, "isOperational", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - watertank");

        break;
    case EXOS_STATE_CONNECTED:
        if (napi_ok != napi_set_named_property(env, watertank.object_value, "isConnected", napi_true))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - watertank");

        if (napi_ok != napi_set_named_property(env, watertank.object_value, "isOperational", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - watertank");

        break;
    case EXOS_STATE_OPERATIONAL:
        if (napi_ok != napi_set_named_property(env, watertank.object_value, "isConnected", napi_true))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - watertank");

        if (napi_ok != napi_set_named_property(env, watertank.object_value, "isOperational", napi_true))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - watertank");

        break;
    case EXOS_STATE_ABORTED:
        if (napi_ok != napi_set_named_property(env, watertank.object_value, "isConnected", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - watertank");

        if (napi_ok != napi_set_named_property(env, watertank.object_value, "isOperational", napi_false))
            napi_throw_error(env, "EINVAL", "Can't set connectionState property - watertank");

        break;
    }

    if (napi_ok != napi_set_named_property(env, watertank.object_value, "connectionState", watertank.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - watertank");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - watertank");
}

static void watertank_onprocessed_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Error calling onProcessed - WaterTank");
}

static void FillValve_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &FillValve.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - FillValve.value");

    if (napi_ok != napi_get_reference_value(env, FillValve.ref, &FillValve.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - FillValve ");

    if (napi_ok != napi_set_named_property(env, FillValve.object_value, "connectionState", FillValve.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - FillValve");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - FillValve");
}

static void EnableHeater_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &EnableHeater.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - EnableHeater.value");

    if (napi_ok != napi_get_reference_value(env, EnableHeater.ref, &EnableHeater.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - EnableHeater ");

    if (napi_ok != napi_set_named_property(env, EnableHeater.object_value, "connectionState", EnableHeater.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - EnableHeater");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - EnableHeater");
}

static void HeaterConfig_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &HeaterConfig.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - HeaterConfig.value");

    if (napi_ok != napi_get_reference_value(env, HeaterConfig.ref, &HeaterConfig.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - HeaterConfig ");

    if (napi_ok != napi_set_named_property(env, HeaterConfig.object_value, "connectionState", HeaterConfig.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - HeaterConfig");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - HeaterConfig");
}

static void Status_connonchange_js_cb(napi_env env, napi_value js_cb, void *context, void *data)
{
    const char *string = data;
    napi_value undefined;

    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_create_string_utf8(env, string, strlen(string), &Status.value))
        napi_throw_error(env, "EINVAL", "Can't create utf8 string from char* - Status.value");

    if (napi_ok != napi_get_reference_value(env, Status.ref, &Status.object_value))
        napi_throw_error(env, "EINVAL", "Can't get reference - Status ");

    if (napi_ok != napi_set_named_property(env, Status.object_value, "connectionState", Status.value))
        napi_throw_error(env, "EINVAL", "Can't set connectionState property - Status");

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onConnectionChange callback - Status");
}

// js value callbacks
static void FillValve_onchange_js_cb(napi_env env, napi_value js_cb, void *context, void *netTime_exos)
{
    napi_value undefined, netTime, latency;
    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_get_reference_value(env, FillValve.ref, &FillValve.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
    }

    if (napi_ok != napi_create_int32(env, (int32_t)exos_data.FillValve, &FillValve.value))
    {
        napi_throw_error(env, "EINVAL", "Can convert C-variable to 32bit integer");
    }
        int32_t _latency = exos_datamodel_get_nettime(&watertank_datamodel) - *(int32_t *)netTime_exos;
        napi_create_int32(env, *(int32_t *)netTime_exos, &netTime);
        napi_create_int32(env, _latency, &latency);
        napi_set_named_property(env, FillValve.object_value, "nettime", netTime);
        napi_set_named_property(env, FillValve.object_value, "latency", latency);
    if (napi_ok != napi_set_named_property(env, FillValve.object_value, "value", FillValve.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
    }

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onChange callback");

    exos_dataset_publish(&FillValve_dataset);
}

static void EnableHeater_onchange_js_cb(napi_env env, napi_value js_cb, void *context, void *netTime_exos)
{
    napi_value undefined, netTime, latency;
    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_get_reference_value(env, EnableHeater.ref, &EnableHeater.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
    }

    if (napi_ok != napi_get_boolean(env, exos_data.EnableHeater, &EnableHeater.value))
    {
        napi_throw_error(env, "EINVAL", "Can't convert C-var to bool");
    }

        int32_t _latency = exos_datamodel_get_nettime(&watertank_datamodel) - *(int32_t *)netTime_exos;
        napi_create_int32(env, *(int32_t *)netTime_exos, &netTime);
        napi_create_int32(env, _latency, &latency);
        napi_set_named_property(env, EnableHeater.object_value, "nettime", netTime);
        napi_set_named_property(env, EnableHeater.object_value, "latency", latency);
    if (napi_ok != napi_set_named_property(env, EnableHeater.object_value, "value", EnableHeater.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
    }

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onChange callback");

    exos_dataset_publish(&EnableHeater_dataset);
}

static void HeaterConfig_onchange_js_cb(napi_env env, napi_value js_cb, void *context, void *netTime_exos)
{
    napi_value object0;
    napi_value property;
    napi_value undefined, netTime, latency;
    napi_get_undefined(env, &undefined);

    if (napi_ok != napi_get_reference_value(env, HeaterConfig.ref, &HeaterConfig.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
    }

    napi_create_object(env, &object0);
    if (napi_ok != napi_create_double(env, (double)exos_data.HeaterConfig.MaxTemperature, &property))
    {
        napi_throw_error(env, "EINVAL", "Can convert C-variable to double");
    }
    napi_set_named_property(env, object0, "MaxTemperature", property);
    if (napi_ok != napi_create_double(env, (double)exos_data.HeaterConfig.MaxPower, &property))
    {
        napi_throw_error(env, "EINVAL", "Can convert C-variable to double");
    }
    napi_set_named_property(env, object0, "MaxPower", property);
HeaterConfig.value = object0;
        int32_t _latency = exos_datamodel_get_nettime(&watertank_datamodel) - *(int32_t *)netTime_exos;
        napi_create_int32(env, *(int32_t *)netTime_exos, &netTime);
        napi_create_int32(env, _latency, &latency);
        napi_set_named_property(env, HeaterConfig.object_value, "nettime", netTime);
        napi_set_named_property(env, HeaterConfig.object_value, "latency", latency);
    if (napi_ok != napi_set_named_property(env, HeaterConfig.object_value, "value", HeaterConfig.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
    }

    if (napi_ok != napi_call_function(env, undefined, js_cb, 0, NULL, NULL))
        throw_fatal_exception_callbacks(env, "EINVAL", "Can't call onChange callback");

    exos_dataset_publish(&HeaterConfig_dataset);
}

// js callback inits
static napi_value watertank_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "WaterTank connection change", watertank_connonchange_js_cb, &watertank.connectiononchange_cb);
}

static napi_value watertank_onprocessed_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "WaterTank onProcessed", watertank_onprocessed_js_cb, &watertank.onprocessed_cb);
}

static napi_value FillValve_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "FillValve connection change", FillValve_connonchange_js_cb, &FillValve.connectiononchange_cb);
}

static napi_value EnableHeater_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "EnableHeater connection change", EnableHeater_connonchange_js_cb, &EnableHeater.connectiononchange_cb);
}

static napi_value HeaterConfig_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "HeaterConfig connection change", HeaterConfig_connonchange_js_cb, &HeaterConfig.connectiononchange_cb);
}

static napi_value Status_connonchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "Status connection change", Status_connonchange_js_cb, &Status.connectiononchange_cb);
}

static napi_value FillValve_onchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "FillValve dataset change", FillValve_onchange_js_cb, &FillValve.onchange_cb);
}

static napi_value EnableHeater_onchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "EnableHeater dataset change", EnableHeater_onchange_js_cb, &EnableHeater.onchange_cb);
}

static napi_value HeaterConfig_onchange_init(napi_env env, napi_callback_info info)
{
    return init_napi_onchange(env, info, "HeaterConfig dataset change", HeaterConfig_onchange_js_cb, &HeaterConfig.onchange_cb);
}

// publish methods
static napi_value HeaterConfig_publish_method(napi_env env, napi_callback_info info)
{
    napi_value object0, object1;
    double __value;

    if (napi_ok != napi_get_reference_value(env, HeaterConfig.ref, &HeaterConfig.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
        return NULL;
    }

    if (napi_ok != napi_get_named_property(env, HeaterConfig.object_value, "value", &HeaterConfig.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
        return NULL;
    }

    object0 = HeaterConfig.value;
    napi_get_named_property(env, object0, "MaxTemperature", &object1);
    if (napi_ok != napi_get_value_double(env, object1, &__value))
    {
        napi_throw_error(env, "EINVAL", "Expected number convertable to double float");
        return NULL;
    }
    exos_data.HeaterConfig.MaxTemperature = (float)__value;
    napi_get_named_property(env, object0, "MaxPower", &object1);
    if (napi_ok != napi_get_value_double(env, object1, &__value))
    {
        napi_throw_error(env, "EINVAL", "Expected number convertable to double float");
        return NULL;
    }
    exos_data.HeaterConfig.MaxPower = (float)__value;
    exos_dataset_publish(&HeaterConfig_dataset);
    return NULL;
}

static napi_value Status_publish_method(napi_env env, napi_callback_info info)
{
    napi_value object0, object1, object2;
    int32_t _value;
    double __value;

    if (napi_ok != napi_get_reference_value(env, Status.ref, &Status.object_value))
    {
        napi_throw_error(env, "EINVAL", "Can't get reference");
        return NULL;
    }

    if (napi_ok != napi_get_named_property(env, Status.object_value, "value", &Status.value))
    {
        napi_throw_error(env, "EINVAL", "Can't get property");
        return NULL;
    }

    object0 = Status.value;
    napi_get_named_property(env, object0, "LevelHigh", &object1);
    if (napi_ok != napi_get_value_bool(env, object1, &exos_data.Status.LevelHigh))
    {
        napi_throw_error(env, "EINVAL", "Expected bool");
        return NULL;
    }
    napi_get_named_property(env, object0, "LevelLow", &object1);
    if (napi_ok != napi_get_value_bool(env, object1, &exos_data.Status.LevelLow))
    {
        napi_throw_error(env, "EINVAL", "Expected bool");
        return NULL;
    }
    napi_get_named_property(env, object0, "FillValveDelay", &object1);
    if (napi_ok != napi_get_value_int32(env, object1, &_value))
    {
        napi_throw_error(env, "EINVAL", "Expected number convertable to 32bit integer");
        return NULL;
    }
    exos_data.Status.FillValveDelay = (int32_t)_value;
    napi_get_named_property(env, object0, "WaterLevel", &object1);
    if (napi_ok != napi_get_value_int32(env, object1, &_value))
    {
        napi_throw_error(env, "EINVAL", "Expected number convertable to 32bit integer");
        return NULL;
    }
    exos_data.Status.WaterLevel = (int32_t)_value;
    napi_get_named_property(env, object0, "Heater", &object1);
    napi_get_named_property(env, object1, "WaterTemperature", &object2);
    if (napi_ok != napi_get_value_double(env, object2, &__value))
    {
        napi_throw_error(env, "EINVAL", "Expected number convertable to double float");
        return NULL;
    }
    exos_data.Status.Heater.WaterTemperature = (float)__value;
    napi_get_named_property(env, object1, "HeatingActive", &object2);
    if (napi_ok != napi_get_value_bool(env, object2, &exos_data.Status.Heater.HeatingActive))
    {
        napi_throw_error(env, "EINVAL", "Expected bool");
        return NULL;
    }
    exos_dataset_publish(&Status_dataset);
    return NULL;
}

//logging functions
static napi_value log_error(napi_env env, napi_callback_info info)
{
    napi_value argv[1];
    size_t argc = 1;
    char log_entry[81] = {};
    size_t res;

    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (argc < 1)
    {
        napi_throw_error(env, "EINVAL", "Too few arguments for watertank.log.error()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for watertank.log.error()");
        return NULL;
    }

    exos_log_error(&logger, log_entry);
    return NULL;
}

static napi_value log_warning(napi_env env, napi_callback_info info)
{
    napi_value argv[1];
    size_t argc = 1;
    char log_entry[81] = {};
    size_t res;

    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (argc < 1)
    {
        napi_throw_error(env, "EINVAL", "Too few arguments for watertank.log.warning()");
        return  NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for watertank.log.warning()");
        return NULL;
    }

    exos_log_warning(&logger, EXOS_LOG_TYPE_USER, log_entry);
    return NULL;
}

static napi_value log_success(napi_env env, napi_callback_info info)
{
    napi_value argv[1];
    size_t argc = 1;
    char log_entry[81] = {};
    size_t res;

    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (argc < 1)
    {
        napi_throw_error(env, "EINVAL", "Too few arguments for watertank.log.success()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for watertank.log.success()");
        return NULL;
    }

    exos_log_success(&logger, EXOS_LOG_TYPE_USER, log_entry);
    return NULL;
}

static napi_value log_info(napi_env env, napi_callback_info info)
{
    napi_value argv[1];
    size_t argc = 1;
    char log_entry[81] = {};
    size_t res;

    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (argc < 1)
    {
        napi_throw_error(env, "EINVAL", "Too few arguments for watertank.log.info()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for watertank.log.info()");
        return NULL;
    }

    exos_log_info(&logger, EXOS_LOG_TYPE_USER, log_entry);
    return NULL;
}

static napi_value log_debug(napi_env env, napi_callback_info info)
{
    napi_value argv[1];
    size_t argc = 1;
    char log_entry[81] = {};
    size_t res;

    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (argc < 1)
    {
        napi_throw_error(env, "EINVAL", "Too few arguments for watertank.log.debug()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for watertank.log.debug()");
        return NULL;
    }

    exos_log_debug(&logger, EXOS_LOG_TYPE_USER, log_entry);
    return NULL;
}

static napi_value log_verbose(napi_env env, napi_callback_info info)
{
    napi_value argv[1];
    size_t argc = 1;
    char log_entry[81] = {};
    size_t res;

    napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

    if (argc < 1)
    {
        napi_throw_error(env, "EINVAL", "Too few arguments for watertank.log.verbose()");
        return NULL;
    }

    if (napi_ok != napi_get_value_string_utf8(env, argv[0], log_entry, sizeof(log_entry), &res))
    {
        napi_throw_error(env, "EINVAL", "Expected string as argument for watertank.log.verbose()");
        return NULL;
    }

    exos_log_warning(&logger, EXOS_LOG_TYPE_USER + EXOS_LOG_TYPE_VERBOSE, log_entry);
    return NULL;
}

// cleanup/cyclic
static void cleanup_watertank(void *env)
{
    uv_idle_stop(&cyclic_h);

    if (EXOS_ERROR_OK != exos_datamodel_delete(&watertank_datamodel))
    {
        napi_throw_error(env, "EINVAL", "Can't delete datamodel");
    }

    if (EXOS_ERROR_OK != exos_log_delete(&logger))
    {
        napi_throw_error(env, "EINVAL", "Can't delete logger");
    }
}

static void cyclic(uv_idle_t * handle) 
{
    int dummy = 0;
    exos_datamodel_process(&watertank_datamodel);
    napi_acquire_threadsafe_function(watertank.onprocessed_cb);
    napi_call_threadsafe_function(watertank.onprocessed_cb, &dummy, napi_tsfn_blocking);
    napi_release_threadsafe_function(watertank.onprocessed_cb, napi_tsfn_release);
    exos_log_process(&logger);
}

//read nettime for DataModel
static napi_value get_net_time(napi_env env, napi_callback_info info)
{
    napi_value netTime;

    if (napi_ok == napi_create_int32(env, exos_datamodel_get_nettime(&watertank_datamodel), &netTime))
    {
        return netTime;
    }
    else
    {
        return NULL;
    }
}

// init of module, called at "require"
static napi_value init_watertank(napi_env env, napi_value exports)
{
    napi_value watertank_conn_change, watertank_onprocessed, FillValve_conn_change, EnableHeater_conn_change, HeaterConfig_conn_change, Status_conn_change;
    napi_value FillValve_onchange, EnableHeater_onchange, HeaterConfig_onchange;
    napi_value HeaterConfig_publish, Status_publish;
    napi_value FillValve_value, EnableHeater_value, HeaterConfig_value, Status_value;

    napi_value dataModel, getNetTime, undefined, def_bool, def_number, def_string;
    napi_value log, logError, logWarning, logSuccess, logInfo, logDebug, logVerbose;
    napi_value object0, object1;

    napi_get_boolean(env, BUR_NAPI_DEFAULT_BOOL_INIT, &def_bool); 
    napi_create_int32(env, BUR_NAPI_DEFAULT_NUM_INIT, &def_number); 
    napi_create_string_utf8(env, BUR_NAPI_DEFAULT_STRING_INIT, strlen(BUR_NAPI_DEFAULT_STRING_INIT), &def_string);
    napi_get_undefined(env, &undefined); 

    // create base objects
    if (napi_ok != napi_create_object(env, &dataModel)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &log)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &watertank.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &FillValve.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &EnableHeater.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &HeaterConfig.value)) 
        return NULL; 

    if (napi_ok != napi_create_object(env, &Status.value)) 
        return NULL; 

    // build object structures
FillValve_value = def_number;
    napi_create_function(env, NULL, 0, FillValve_onchange_init, NULL, &FillValve_onchange);
    napi_set_named_property(env, FillValve.value, "onChange", FillValve_onchange);
    napi_set_named_property(env, FillValve.value, "nettime", undefined);
    napi_set_named_property(env, FillValve.value, "latency", undefined);
    napi_set_named_property(env, FillValve.value, "value", FillValve_value);
    napi_create_function(env, NULL, 0, FillValve_connonchange_init, NULL, &FillValve_conn_change);
    napi_set_named_property(env, FillValve.value, "onConnectionChange", FillValve_conn_change);
    napi_set_named_property(env, FillValve.value, "connectionState", def_string);

EnableHeater_value = def_bool;
    napi_create_function(env, NULL, 0, EnableHeater_onchange_init, NULL, &EnableHeater_onchange);
    napi_set_named_property(env, EnableHeater.value, "onChange", EnableHeater_onchange);
    napi_set_named_property(env, EnableHeater.value, "nettime", undefined);
    napi_set_named_property(env, EnableHeater.value, "latency", undefined);
    napi_set_named_property(env, EnableHeater.value, "value", EnableHeater_value);
    napi_create_function(env, NULL, 0, EnableHeater_connonchange_init, NULL, &EnableHeater_conn_change);
    napi_set_named_property(env, EnableHeater.value, "onConnectionChange", EnableHeater_conn_change);
    napi_set_named_property(env, EnableHeater.value, "connectionState", def_string);

    napi_create_object(env, &object0);
    napi_set_named_property(env, object0, "MaxTemperature", def_number);
    napi_set_named_property(env, object0, "MaxPower", def_number);
    HeaterConfig_value = object0;
    napi_create_function(env, NULL, 0, HeaterConfig_onchange_init, NULL, &HeaterConfig_onchange);
    napi_set_named_property(env, HeaterConfig.value, "onChange", HeaterConfig_onchange);
    napi_set_named_property(env, HeaterConfig.value, "nettime", undefined);
    napi_set_named_property(env, HeaterConfig.value, "latency", undefined);
    napi_create_function(env, NULL, 0, HeaterConfig_publish_method, NULL, &HeaterConfig_publish);
    napi_set_named_property(env, HeaterConfig.value, "publish", HeaterConfig_publish);
    napi_set_named_property(env, HeaterConfig.value, "value", HeaterConfig_value);
    napi_create_function(env, NULL, 0, HeaterConfig_connonchange_init, NULL, &HeaterConfig_conn_change);
    napi_set_named_property(env, HeaterConfig.value, "onConnectionChange", HeaterConfig_conn_change);
    napi_set_named_property(env, HeaterConfig.value, "connectionState", def_string);

    napi_create_object(env, &object0);
    napi_set_named_property(env, object0, "LevelHigh", def_bool);
    napi_set_named_property(env, object0, "LevelLow", def_bool);
    napi_set_named_property(env, object0, "FillValveDelay", def_number);
    napi_set_named_property(env, object0, "WaterLevel", def_number);
        napi_create_object(env, &object1);
    napi_set_named_property(env, object1, "WaterTemperature", def_number);
    napi_set_named_property(env, object1, "HeatingActive", def_bool);
    napi_set_named_property(env, object0, "Heater", object1);
    Status_value = object0;
    napi_create_function(env, NULL, 0, Status_publish_method, NULL, &Status_publish);
    napi_set_named_property(env, Status.value, "publish", Status_publish);
    napi_set_named_property(env, Status.value, "value", Status_value);
    napi_create_function(env, NULL, 0, Status_connonchange_init, NULL, &Status_conn_change);
    napi_set_named_property(env, Status.value, "onConnectionChange", Status_conn_change);
    napi_set_named_property(env, Status.value, "connectionState", def_string);

    //connect logging functions
    napi_create_function(env, NULL, 0, log_error, NULL, &logError);
    napi_set_named_property(env, log, "error", logError);
    napi_create_function(env, NULL, 0, log_warning, NULL, &logWarning);
    napi_set_named_property(env, log, "warning", logWarning);
    napi_create_function(env, NULL, 0, log_success, NULL, &logSuccess);
    napi_set_named_property(env, log, "success", logSuccess);
    napi_create_function(env, NULL, 0, log_info, NULL, &logInfo);
    napi_set_named_property(env, log, "info", logInfo);
    napi_create_function(env, NULL, 0, log_debug, NULL, &logDebug);
    napi_set_named_property(env, log, "debug", logDebug);
    napi_create_function(env, NULL, 0, log_verbose, NULL, &logVerbose);
    napi_set_named_property(env, log, "verbose", logVerbose);

    // bind dataset objects to datamodel object
    napi_set_named_property(env, dataModel, "FillValve", FillValve.value); 
    napi_set_named_property(env, dataModel, "EnableHeater", EnableHeater.value); 
    napi_set_named_property(env, dataModel, "HeaterConfig", HeaterConfig.value); 
    napi_set_named_property(env, dataModel, "Status", Status.value); 
    napi_set_named_property(env, watertank.value, "datamodel", dataModel); 
    napi_create_function(env, NULL, 0, watertank_connonchange_init, NULL, &watertank_conn_change); 
    napi_set_named_property(env, watertank.value, "onConnectionChange", watertank_conn_change); 
    napi_set_named_property(env, watertank.value, "connectionState", def_string);
    napi_set_named_property(env, watertank.value, "isConnected", def_bool);
    napi_set_named_property(env, watertank.value, "isOperational", def_bool);
    napi_create_function(env, NULL, 0, watertank_onprocessed_init, NULL, &watertank_onprocessed); 
    napi_set_named_property(env, watertank.value, "onProcessed", watertank_onprocessed); 
    napi_create_function(env, NULL, 0, get_net_time, NULL, &getNetTime);
    napi_set_named_property(env, watertank.value, "nettime", getNetTime);
    napi_set_named_property(env, watertank.value, "log", log);
    // export application object
    napi_set_named_property(env, exports, "WaterTank", watertank.value); 

    // save references to object as globals for this C-file
    if (napi_ok != napi_create_reference(env, watertank.value, watertank.ref_count, &watertank.ref)) 
    {
                    
        napi_throw_error(env, "EINVAL", "Can't create watertank reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, FillValve.value, FillValve.ref_count, &FillValve.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create FillValve reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, EnableHeater.value, EnableHeater.ref_count, &EnableHeater.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create EnableHeater reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, HeaterConfig.value, HeaterConfig.ref_count, &HeaterConfig.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create HeaterConfig reference"); 
        return NULL; 
    } 
    if (napi_ok != napi_create_reference(env, Status.value, Status.ref_count, &Status.ref)) 
    {
        napi_throw_error(env, "EINVAL", "Can't create Status reference"); 
        return NULL; 
    } 

    // register clean up hook
    if (napi_ok != napi_add_env_cleanup_hook(env, cleanup_watertank, env)) 
    {
        napi_throw_error(env, "EINVAL", "Can't register cleanup hook"); 
        return NULL; 
    } 

    // exOS
    // exOS inits
    if (EXOS_ERROR_OK != exos_datamodel_init(&watertank_datamodel, "WaterTank_0", "gWaterTank_0")) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize WaterTank"); 
    } 
    watertank_datamodel.user_context = NULL; 
    watertank_datamodel.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&FillValve_dataset, &watertank_datamodel, "FillValve", &exos_data.FillValve, sizeof(exos_data.FillValve))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize FillValve"); 
    }
    FillValve_dataset.user_context = NULL; 
    FillValve_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&EnableHeater_dataset, &watertank_datamodel, "EnableHeater", &exos_data.EnableHeater, sizeof(exos_data.EnableHeater))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize EnableHeater"); 
    }
    EnableHeater_dataset.user_context = NULL; 
    EnableHeater_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&HeaterConfig_dataset, &watertank_datamodel, "HeaterConfig", &exos_data.HeaterConfig, sizeof(exos_data.HeaterConfig))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize HeaterConfig"); 
    }
    HeaterConfig_dataset.user_context = NULL; 
    HeaterConfig_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_dataset_init(&Status_dataset, &watertank_datamodel, "Status", &exos_data.Status, sizeof(exos_data.Status))) 
    {
        napi_throw_error(env, "EINVAL", "Can't initialize Status"); 
    }
    Status_dataset.user_context = NULL; 
    Status_dataset.user_tag = 0; 

    if (EXOS_ERROR_OK != exos_log_init(&logger, "WaterTank_0"))
    {
        napi_throw_error(env, "EINVAL", "Can't register logger for WaterTank"); 
    } 

    INFO("WaterTank starting!")
    // exOS register datamodel
    if (EXOS_ERROR_OK != exos_datamodel_connect_watertank(&watertank_datamodel, datamodelEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect WaterTank"); 
    } 

    // exOS register datasets
    if (EXOS_ERROR_OK != exos_dataset_connect(&FillValve_dataset, EXOS_DATASET_SUBSCRIBE, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect FillValve"); 
    }

    if (EXOS_ERROR_OK != exos_dataset_connect(&EnableHeater_dataset, EXOS_DATASET_SUBSCRIBE, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect EnableHeater"); 
    }

    if (EXOS_ERROR_OK != exos_dataset_connect(&HeaterConfig_dataset, EXOS_DATASET_SUBSCRIBE + EXOS_DATASET_PUBLISH, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect HeaterConfig"); 
    }

    if (EXOS_ERROR_OK != exos_dataset_connect(&Status_dataset, EXOS_DATASET_PUBLISH, datasetEvent)) 
    {
        napi_throw_error(env, "EINVAL", "Can't connect Status"); 
    }

    // start up module

    uv_idle_init(uv_default_loop(), &cyclic_h); 
    uv_idle_start(&cyclic_h, cyclic); 

    SUCCESS("WaterTank started!")
    return exports; 
} 

// hook for Node-API
NAPI_MODULE(NODE_GYP_MODULE_NAME, init_watertank);

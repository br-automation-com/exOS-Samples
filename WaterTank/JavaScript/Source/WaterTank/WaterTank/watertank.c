#include <WaterTank.h>

#define EXOS_ASSERT_LOG &handle->logger
#define EXOS_ASSERT_CALLBACK inst->_state = 255;
#include "exos_log.h"
#include "exos_watertank.h"
#include <string.h>

#define SUCCESS(_format_, ...) exos_log_success(&handle->logger, EXOS_LOG_TYPE_USER, _format_, ##__VA_ARGS__);
#define INFO(_format_, ...) exos_log_info(&handle->logger, EXOS_LOG_TYPE_USER, _format_, ##__VA_ARGS__);
#define VERBOSE(_format_, ...) exos_log_debug(&handle->logger, EXOS_LOG_TYPE_USER + EXOS_LOG_TYPE_VERBOSE, _format_, ##__VA_ARGS__);
#define ERROR(_format_, ...) exos_log_error(&handle->logger, _format_, ##__VA_ARGS__);

typedef struct
{
    void *self;
    exos_log_handle_t logger;
    WaterTank data;

    exos_datamodel_handle_t watertank;

    exos_dataset_handle_t fillvalve;
    exos_dataset_handle_t enableheater;
    exos_dataset_handle_t heaterconfig;
    exos_dataset_handle_t status;
} WaterTankHandle_t;

static void datasetEvent(exos_dataset_handle_t *dataset, EXOS_DATASET_EVENT_TYPE event_type, void *info)
{
    struct WaterTankCyclic *inst = (struct WaterTankCyclic *)dataset->datamodel->user_context;
    WaterTankHandle_t *handle = (WaterTankHandle_t *)inst->Handle;

    switch (event_type)
    {
    case EXOS_DATASET_EVENT_UPDATED:
        VERBOSE("dataset %s updated! latency (us):%i", dataset->name, (exos_datamodel_get_nettime(dataset->datamodel) - dataset->nettime));
        //handle each subscription dataset separately
        if (0 == strcmp(dataset->name, "HeaterConfig"))
        {
            SUCCESS("dataset config changed, check your new datasets");
            memcpy(&inst->pWaterTank->HeaterConfig, dataset->data, dataset->size);
        }
        else if (0 == strcmp(dataset->name, "Status"))
        {
            WaterTankStatus *status = (WaterTankStatus *)dataset->data;
            inst->pWaterTank->StatusDelay = exos_datamodel_get_nettime(dataset->datamodel) - dataset->nettime;
            inst->pWaterTank->StatusUpdates++;

            //check for changes in the received data
            if (status->LevelHigh > inst->pWaterTank->Status.LevelHigh)
            {
                SUCCESS("Level High reached");
            }
            if (status->LevelLow > inst->pWaterTank->Status.LevelLow)
            {
                SUCCESS("Level Low reached");
            }
            if (status->Heater.WaterTemperature != inst->pWaterTank->Status.Heater.WaterTemperature)
            {
                VERBOSE("Water temperature is now: %f nettime delay: %d us", status->Heater.WaterTemperature, inst->pWaterTank->StatusDelay);
            }
            //copy the status data to the local structure
            memcpy(&inst->pWaterTank->Status, dataset->data, dataset->size);
        }
        break;

    case EXOS_DATASET_EVENT_PUBLISHED:
        VERBOSE("dataset %s published to local server for distribution! send buffer free:%i", dataset->name, dataset->send_buffer.free);
        //handle each published dataset separately
        if (0 == strcmp(dataset->name, "FillValve"))
        {
            // VALVE_STATE *fillvalve = (VALVE_STATE *)dataset->data;
        }
        else if (0 == strcmp(dataset->name, "EnableHeater"))
        {
            // BOOL *enableheater = (BOOL *)dataset->data;
        }
        else if (0 == strcmp(dataset->name, "HeaterConfig"))
        {
            // WaterTankHeaterConfig *heaterconfig = (WaterTankHeaterConfig *)dataset->data;
        }
        break;

    case EXOS_DATASET_EVENT_DELIVERED:
        VERBOSE("dataset %s delivered to remote server for distribution! send buffer free:%i", dataset->name, dataset->send_buffer.free);
        //handle each published dataset separately
        if (0 == strcmp(dataset->name, "FillValve"))
        {
            // VALVE_STATE *fillvalve = (VALVE_STATE *)dataset->data;
        }
        else if (0 == strcmp(dataset->name, "EnableHeater"))
        {
            // BOOL *enableheater = (BOOL *)dataset->data;
        }
        else if (0 == strcmp(dataset->name, "HeaterConfig"))
        {
            // WaterTankHeaterConfig *heaterconfig = (WaterTankHeaterConfig *)dataset->data;
        }
        break;

    case EXOS_DATASET_EVENT_CONNECTION_CHANGED:
        INFO("dataset %s changed state to %s", dataset->name, exos_get_state_string(dataset->connection_state));

        switch (dataset->connection_state)
        {
        case EXOS_STATE_DISCONNECTED:
            break;
        case EXOS_STATE_CONNECTED:
            //call the dataset changed event to update the dataset when connected
            //datasetEvent(dataset,EXOS_DATASET_UPDATED,info);
            break;
        case EXOS_STATE_OPERATIONAL:
            break;
        case EXOS_STATE_ABORTED:
            ERROR("dataset %s error %d (%s) occured", dataset->name, dataset->error, exos_get_error_string(dataset->error));
            break;
        }
        break;
    }
}

static void datamodelEvent(exos_datamodel_handle_t *datamodel, const EXOS_DATAMODEL_EVENT_TYPE event_type, void *info)
{
    struct WaterTankCyclic *inst = (struct WaterTankCyclic *)datamodel->user_context;
    WaterTankHandle_t *handle = (WaterTankHandle_t *)inst->Handle;

    switch (event_type)
    {
    case EXOS_DATAMODEL_EVENT_CONNECTION_CHANGED:
        INFO("application changed state to %s", exos_get_state_string(datamodel->connection_state));

        inst->Disconnected = 0;
        inst->Connected = 0;
        inst->Operational = 0;
        inst->Aborted = 0;

        switch (datamodel->connection_state)
        {
        case EXOS_STATE_DISCONNECTED:
            inst->Disconnected = 1;
            inst->_state = 255;
            break;
        case EXOS_STATE_CONNECTED:
            inst->Connected = 1;
            break;
        case EXOS_STATE_OPERATIONAL:
            SUCCESS("WaterTank operational!");
            inst->Operational = 1;
            break;
        case EXOS_STATE_ABORTED:
            ERROR("application error %d (%s) occured", datamodel->error, exos_get_error_string(datamodel->error));
            inst->_state = 255;
            inst->Aborted = 1;
            break;
        }
        break;
    case EXOS_DATAMODEL_EVENT_SYNC_STATE_CHANGED:
        break;

    default:
        break;
    }
}

_BUR_PUBLIC void WaterTankInit(struct WaterTankInit *inst)
{
    WaterTankHandle_t *handle;
    TMP_alloc(sizeof(WaterTankHandle_t), (void **)&handle);
    if (NULL == handle)
    {
        inst->Handle = 0;
        return;
    }

    memset(&handle->data, 0, sizeof(handle->data));
    handle->self = handle;

    exos_log_init(&handle->logger, "gWaterTank_0");

    exos_datamodel_handle_t *watertank = &handle->watertank;
    exos_dataset_handle_t *fillvalve = &handle->fillvalve;
    exos_dataset_handle_t *enableheater = &handle->enableheater;
    exos_dataset_handle_t *heaterconfig = &handle->heaterconfig;
    exos_dataset_handle_t *status = &handle->status;
    EXOS_ASSERT_OK(exos_datamodel_init(watertank, "WaterTank_0", "gWaterTank_0"));

    EXOS_ASSERT_OK(exos_dataset_init(fillvalve, watertank, "FillValve", &handle->data.FillValve, sizeof(handle->data.FillValve)));
    EXOS_ASSERT_OK(exos_dataset_init(enableheater, watertank, "EnableHeater", &handle->data.EnableHeater, sizeof(handle->data.EnableHeater)));
    EXOS_ASSERT_OK(exos_dataset_init(heaterconfig, watertank, "HeaterConfig", &handle->data.HeaterConfig, sizeof(handle->data.HeaterConfig)));
    EXOS_ASSERT_OK(exos_dataset_init(status, watertank, "Status", &handle->data.Status, sizeof(handle->data.Status)));

    inst->Handle = (UDINT)handle;
}

_BUR_PUBLIC void WaterTankCyclic(struct WaterTankCyclic *inst)
{
    WaterTankHandle_t *handle = (WaterTankHandle_t *)inst->Handle;

    inst->Error = false;
    if (NULL == handle || NULL == inst->pWaterTank)
    {
        inst->Error = true;
        return;
    }
    if ((void *)handle != handle->self)
    {
        inst->Error = true;
        return;
    }

    WaterTank *data = &handle->data;
    exos_datamodel_handle_t *watertank = &handle->watertank;
    //the user context of the datamodel points to the WaterTankCyclic instance
    watertank->user_context = inst; //set it cyclically in case the program using the FUB is retransferred
    watertank->user_tag = 0;        //user defined
    //handle online download of the library
    if (NULL != watertank->datamodel_event_callback && watertank->datamodel_event_callback != datamodelEvent)
    {
        watertank->datamodel_event_callback = datamodelEvent;
        exos_log_delete(&handle->logger);
        exos_log_init(&handle->logger, "WaterTank_0");
    }

    exos_dataset_handle_t *fillvalve = &handle->fillvalve;
    fillvalve->user_context = NULL; //user defined
    fillvalve->user_tag = 0;        //user defined
    //handle online download of the library
    if (NULL != fillvalve->dataset_event_callback && fillvalve->dataset_event_callback != datasetEvent)
    {
        fillvalve->dataset_event_callback = datasetEvent;
    }

    exos_dataset_handle_t *enableheater = &handle->enableheater;
    enableheater->user_context = NULL; //user defined
    enableheater->user_tag = 0;        //user defined
    //handle online download of the library
    if (NULL != enableheater->dataset_event_callback && enableheater->dataset_event_callback != datasetEvent)
    {
        enableheater->dataset_event_callback = datasetEvent;
    }

    exos_dataset_handle_t *heaterconfig = &handle->heaterconfig;
    heaterconfig->user_context = NULL; //user defined
    heaterconfig->user_tag = 0;        //user defined
    //handle online download of the library
    if (NULL != heaterconfig->dataset_event_callback && heaterconfig->dataset_event_callback != datasetEvent)
    {
        heaterconfig->dataset_event_callback = datasetEvent;
    }

    exos_dataset_handle_t *status = &handle->status;
    status->user_context = NULL; //user defined
    status->user_tag = 0;        //user defined
    //handle online download of the library
    if (NULL != status->dataset_event_callback && status->dataset_event_callback != datasetEvent)
    {
        status->dataset_event_callback = datasetEvent;
    }

    //unregister on disable
    if (inst->_state && !inst->Enable)
    {
        inst->_state = 255;
    }

    switch (inst->_state)
    {
    case 0:
        inst->Disconnected = 1;
        inst->Connected = 0;
        inst->Operational = 0;
        inst->Aborted = 0;

        if (inst->Enable)
        {
            inst->_state = 10;
        }
        break;

    case 10:
        inst->_state = 100;

        SUCCESS("starting WaterTank application..");

        //connect the datamodel, then the datasets
        EXOS_ASSERT_OK(exos_datamodel_connect_watertank(watertank, datamodelEvent));
        EXOS_ASSERT_OK(exos_dataset_connect(fillvalve, EXOS_DATASET_PUBLISH, datasetEvent));
        EXOS_ASSERT_OK(exos_dataset_connect(enableheater, EXOS_DATASET_PUBLISH, datasetEvent));
        EXOS_ASSERT_OK(exos_dataset_connect(heaterconfig, EXOS_DATASET_PUBLISH + EXOS_DATASET_SUBSCRIBE, datasetEvent));
        EXOS_ASSERT_OK(exos_dataset_connect(status, EXOS_DATASET_SUBSCRIBE, datasetEvent));

        inst->Active = true;
        break;

    case 100:
    case 101:
        if (inst->Start)
        {
            if (inst->_state == 100)
            {
                EXOS_ASSERT_OK(exos_datamodel_set_operational(watertank));
                inst->_state = 101;
            }
        }
        else
        {
            inst->_state = 100;
        }

        EXOS_ASSERT_OK(exos_datamodel_process(watertank));
        //put your cyclic code here!

        //publish the fillvalve dataset as soon as there are changes
        if (0 != memcmp(&inst->pWaterTank->FillValve, &data->FillValve, sizeof(data->FillValve)))
        {
            memcpy(&data->FillValve, &inst->pWaterTank->FillValve, sizeof(data->FillValve));
            exos_dataset_publish(fillvalve);
        }
        //publish the enableheater dataset as soon as there are changes
        if (inst->pWaterTank->EnableHeater != data->EnableHeater)
        {
            data->EnableHeater = inst->pWaterTank->EnableHeater;
            exos_dataset_publish(enableheater);
        }
        //publish the heaterconfig dataset as soon as there are changes
        if (0 != memcmp(&inst->pWaterTank->HeaterConfig, &data->HeaterConfig, sizeof(data->HeaterConfig)))
        {
            memcpy(&data->HeaterConfig, &inst->pWaterTank->HeaterConfig, sizeof(data->HeaterConfig));
            exos_dataset_publish(heaterconfig);
        }

        break;

    case 255:
        //disconnect the datamodel
        EXOS_ASSERT_OK(exos_datamodel_disconnect(watertank));

        inst->Active = false;
        inst->_state = 254;
        //no break

    case 254:
        if (!inst->Enable)
            inst->_state = 0;
        break;
    }

    exos_log_process(&handle->logger);
}

_BUR_PUBLIC void WaterTankExit(struct WaterTankExit *inst)
{
    WaterTankHandle_t *handle = (WaterTankHandle_t *)inst->Handle;

    if (NULL == handle)
    {
        ERROR("WaterTankExit: NULL handle, cannot delete resources");
        return;
    }
    if ((void *)handle != handle->self)
    {
        ERROR("WaterTankExit: invalid handle, cannot delete resources");
        return;
    }

    exos_datamodel_handle_t *watertank = &handle->watertank;

    EXOS_ASSERT_OK(exos_datamodel_delete(watertank));

    //finish with deleting the log
    exos_log_delete(&handle->logger);
    //free the allocated handle
    TMP_free(sizeof(WaterTankHandle_t), (void *)handle);
}

#include <unistd.h>
#include <string.h>
#include "termination.h"

#define EXOS_ASSERT_LOG &logger
#include "exos_log.h"
#include "exos_watertank.h"

#define SUCCESS(_format_, ...) exos_log_success(&logger, EXOS_LOG_TYPE_USER, _format_, ##__VA_ARGS__);
#define INFO(_format_, ...) exos_log_info(&logger, EXOS_LOG_TYPE_USER, _format_, ##__VA_ARGS__);
#define VERBOSE(_format_, ...) exos_log_debug(&logger, EXOS_LOG_TYPE_USER + EXOS_LOG_TYPE_VERBOSE, _format_, ##__VA_ARGS__);
#define ERROR(_format_, ...) exos_log_error(&logger, _format_, ##__VA_ARGS__);

exos_log_handle_t logger;
#define EPSILON 0.0001

typedef struct sim_data
{
    bool old_high;
    bool old_low;
    exos_dataset_handle_t *status;
} sim_data_t;

static void check_heaterconfig(exos_dataset_handle_t *dataset);
static void run_tanksimulator(WaterTank *tank, sim_data_t *sim);

static void datasetEvent(exos_dataset_handle_t *dataset, EXOS_DATASET_EVENT_TYPE event_type, void *info)
{
    switch (event_type)
    {
    case EXOS_DATASET_EVENT_UPDATED:
        VERBOSE("dataset %s updated! latency (us):%i", dataset->name, (exos_datamodel_get_nettime(dataset->datamodel) - dataset->nettime));
        //handle each subscription dataset separately
        if (0 == strcmp(dataset->name, "FillValve"))
        {
            VALVE_STATE *fillvalve = (VALVE_STATE *)dataset->data;
            int32_t diff = exos_datamodel_get_nettime(dataset->datamodel) - dataset->nettime;
            INFO("Filling Valve Changed to %d, timedelay %d", *fillvalve, diff);

            //the Status dataset is connected to the FillValve user_context
            exos_dataset_handle_t *status_dataset = dataset->user_context;
            WaterTankStatus *status = (WaterTankStatus *)status_dataset->data;
            //send back the current delay
            status->FillValveDelay = diff;
            exos_dataset_publish(status_dataset);
        }
        else if (0 == strcmp(dataset->name, "EnableHeater"))
        {
            bool *enableheater = (bool *)dataset->data;
            if (*enableheater)
            {
                SUCCESS("Heater Enabled!");
            }
            else
            {
                SUCCESS("Heater Disabled!");
            }
        }
        else if (0 == strcmp(dataset->name, "HeaterConfig"))
        {
            check_heaterconfig(dataset);
        }
        break;

    case EXOS_DATASET_EVENT_PUBLISHED:
        VERBOSE("dataset %s published to local server for distribution! send buffer free:%i", dataset->name, dataset->send_buffer.free);
        //handle each published dataset separately
        if (0 == strcmp(dataset->name, "HeaterConfig"))
        {
            WaterTankHeaterConfig *heaterconfig = (WaterTankHeaterConfig *)dataset->data;
        }
        else if (0 == strcmp(dataset->name, "Status"))
        {
            WaterTankStatus *status = (WaterTankStatus *)dataset->data;
        }
        break;

    case EXOS_DATASET_EVENT_DELIVERED:
        VERBOSE("dataset %s delivered to remote server for distribution! send buffer free:%i", dataset->name, dataset->send_buffer.free);
        //handle each published dataset separately
        if (0 == strcmp(dataset->name, "HeaterConfig"))
        {
            WaterTankHeaterConfig *heaterconfig = (WaterTankHeaterConfig *)dataset->data;
        }
        else if (0 == strcmp(dataset->name, "Status"))
        {
            WaterTankStatus *status = (WaterTankStatus *)dataset->data;
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
    switch (event_type)
    {
    case EXOS_DATAMODEL_EVENT_CONNECTION_CHANGED:
        INFO("application changed state to %s", exos_get_state_string(datamodel->connection_state));

        switch (datamodel->connection_state)
        {
        case EXOS_STATE_DISCONNECTED:
            break;
        case EXOS_STATE_CONNECTED:
            break;
        case EXOS_STATE_OPERATIONAL:
            SUCCESS("WaterTank operational!");
            break;
        case EXOS_STATE_ABORTED:
            ERROR("application error %d (%s) occured", datamodel->error, exos_get_error_string(datamodel->error));
            break;
        }
        break;
    case EXOS_DATAMODEL_EVENT_SYNC_STATE_CHANGED:
        break;

    default:
        break;
    }
}

int main()
{
    WaterTank data;

    exos_datamodel_handle_t watertank;

    exos_dataset_handle_t fillvalve;
    exos_dataset_handle_t enableheater;
    exos_dataset_handle_t heaterconfig;
    exos_dataset_handle_t status;

    exos_log_init(&logger, "gWaterTank_0");

    SUCCESS("starting WaterTank application..");

    EXOS_ASSERT_OK(exos_datamodel_init(&watertank, "WaterTank_0", "gWaterTank_0"));

    //set the user_context to access custom data in the callbacks
    watertank.user_context = NULL; //user defined
    watertank.user_tag = 0;        //user defined

    EXOS_ASSERT_OK(exos_dataset_init(&fillvalve, &watertank, "FillValve", &data.FillValve, sizeof(data.FillValve)));
    fillvalve.user_context = &status; //used for updating the Status.FillValveDelay
    fillvalve.user_tag = 0;           //user defined

    EXOS_ASSERT_OK(exos_dataset_init(&enableheater, &watertank, "EnableHeater", &data.EnableHeater, sizeof(data.EnableHeater)));
    enableheater.user_context = NULL; //user defined
    enableheater.user_tag = 0;        //user defined

    EXOS_ASSERT_OK(exos_dataset_init(&heaterconfig, &watertank, "HeaterConfig", &data.HeaterConfig, sizeof(data.HeaterConfig)));
    heaterconfig.user_context = NULL; //user defined
    heaterconfig.user_tag = 0;        //user defined

    EXOS_ASSERT_OK(exos_dataset_init(&status, &watertank, "Status", &data.Status, sizeof(data.Status)));
    status.user_context = NULL; //user defined
    status.user_tag = 0;        //user defined

    //connect the datamodel
    EXOS_ASSERT_OK(exos_datamodel_connect_watertank(&watertank, datamodelEvent));

    //connect datasets
    EXOS_ASSERT_OK(exos_dataset_connect(&fillvalve, EXOS_DATASET_SUBSCRIBE, datasetEvent));
    EXOS_ASSERT_OK(exos_dataset_connect(&enableheater, EXOS_DATASET_SUBSCRIBE, datasetEvent));
    EXOS_ASSERT_OK(exos_dataset_connect(&heaterconfig, EXOS_DATASET_PUBLISH + EXOS_DATASET_SUBSCRIBE, datasetEvent));
    EXOS_ASSERT_OK(exos_dataset_connect(&status, EXOS_DATASET_PUBLISH, datasetEvent));

    catch_termination();
    sim_data_t sim;
    sim.old_low = false;
    sim.old_high = false;
    sim.status = &status;

    while (true)
    {
        EXOS_ASSERT_OK(exos_datamodel_process(&watertank));
        exos_log_process(&logger);

        //put your cyclic code here!
        if (EXOS_STATE_OPERATIONAL == watertank.connection_state)
        {
            check_heaterconfig(&heaterconfig);
            run_tanksimulator(&data, &sim);
        }

        if (is_terminated())
        {
            SUCCESS("WaterTank application terminated, closing..");
            break;
        }
    }

    EXOS_ASSERT_OK(exos_datamodel_delete(&watertank));

    //finish with deleting the log
    exos_log_delete(&logger);
    return 0;
}

static void check_heaterconfig(exos_dataset_handle_t *dataset)
{
    WaterTankHeaterConfig *heaterconfig = (WaterTankHeaterConfig *)dataset->data;
    bool values_changed = false;

    //limit incoming config values, and publish if changed

    if (heaterconfig->MaxPower > 1.0)
    {
        values_changed = true;
        heaterconfig->MaxPower = 1.0;
    }

    if (heaterconfig->MaxPower < EPSILON)
    {
        values_changed = true;
        heaterconfig->MaxPower = 0.5;
    }

    if (heaterconfig->MaxTemperature > 90)
    {
        values_changed = true;
        heaterconfig->MaxTemperature = 90;
    }

    if (heaterconfig->MaxTemperature < EPSILON)
    {
        values_changed = true;
        heaterconfig->MaxTemperature = 45;
    }

    if (values_changed)
    {
        SUCCESS("HeaterConfig changed, values limited");
        exos_dataset_publish(dataset);
    }
}

static void run_tanksimulator(WaterTank *tank, sim_data_t *sim)
{
    tank->Status.LevelLow = (tank->Status.WaterLevel < 200);
    tank->Status.LevelHigh = (tank->Status.WaterLevel > 800);

    //publish values on change
    if (sim->old_low != tank->Status.LevelLow)
    {
        INFO("Level LOW changed to %d, Water Level: %d", tank->Status.LevelLow, tank->Status.WaterLevel);
        exos_dataset_publish(sim->status);
    }
    if (sim->old_high != tank->Status.LevelHigh)
    {
        INFO("Level HIGH changed to %d, Water Level: %d", tank->Status.LevelHigh, tank->Status.WaterLevel);
        exos_dataset_publish(sim->status);
    }

    sim->old_low = tank->Status.LevelLow;
    sim->old_high = tank->Status.LevelHigh;

    switch (tank->FillValve)
    {
    case VALVE_OPEN:
        if (tank->Status.WaterLevel < 1000)
        {
            tank->Status.WaterLevel += 5;
            exos_dataset_publish(sim->status);
        }
        break;
    case VALVE_CLOSED:
        if (tank->Status.WaterLevel > 0)
        {
            tank->Status.WaterLevel -= 2;
            exos_dataset_publish(sim->status);
        }
        break;
    }

    if (tank->EnableHeater)
    {
        if (tank->Status.Heater.WaterTemperature < tank->HeaterConfig.MaxTemperature)
        {
            tank->Status.Heater.WaterTemperature += tank->HeaterConfig.MaxPower;
            tank->Status.Heater.HeatingActive = true;
            exos_dataset_publish(sim->status);
        }
        else
        {
            tank->Status.Heater.WaterTemperature -= 3 * tank->HeaterConfig.MaxPower;
            tank->Status.Heater.HeatingActive = false;
            exos_dataset_publish(sim->status);
        }
    }
    else
    {
        tank->Status.Heater.HeatingActive = false;
        if (tank->Status.Heater.WaterTemperature >= 20.1)
        {
            tank->Status.Heater.WaterTemperature -= 0.1;
            exos_dataset_publish(sim->status);
        }
    }
}
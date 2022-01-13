#include <unistd.h>
#include "libwatertank.h"
#include "termination.h"
#include <stdio.h>

/* libWaterTank_t datamodel features:

main methods:
    watertank->connect()
    watertank->disconnect()
    watertank->process()
    watertank->set_operational()
    watertank->dispose()
    watertank->get_nettime() : (int32_t) get current nettime

void(void) user callbacks:
    watertank->on_connected
    watertank->on_disconnected
    watertank->on_operational

boolean values:
    watertank->is_connected
    watertank->is_operational

logging methods:
    watertank->log.error(char *)
    watertank->log.warning(char *)
    watertank->log.success(char *)
    watertank->log.info(char *)
    watertank->log.debug(char *)
    watertank->log.verbose(char *)

dataset FillValve:
    watertank->FillValve.on_change : void(void) user callback function
    watertank->FillValve.nettime : (int32_t) nettime @ time of publish
    watertank->FillValve.value : (VALVE_STATE)  actual dataset values

dataset EnableHeater:
    watertank->EnableHeater.on_change : void(void) user callback function
    watertank->EnableHeater.nettime : (int32_t) nettime @ time of publish
    watertank->EnableHeater.value : (bool)  actual dataset value

dataset HeaterConfig:
    watertank->HeaterConfig.publish()
    watertank->HeaterConfig.on_change : void(void) user callback function
    watertank->HeaterConfig.nettime : (int32_t) nettime @ time of publish
    watertank->HeaterConfig.value : (WaterTankHeaterConfig)  actual dataset values

dataset Status:
    waterwatertank->Status.value.publish()
    waterwatertank->Status.value.value : (WaterTankStatus)  actual dataset values
*/

#define EPSILON 0.0001

static libWaterTank_t *watertank;
typedef struct sim_data
{
    bool old_high;
    bool old_low;
} sim_data_t;

static void check_heaterconfig(void);
static void run_tanksimulator(sim_data_t *sim);

static void on_change_fillvalve(void)
{
    watertank->log.verbose("watertank->FillValve changed!");
    int32_t diff = watertank->get_nettime() - watertank->FillValve.nettime;

    char message[256];
    snprintf(message, 256, "Filling Valve Changed to %d, timedelay %d", watertank->FillValve.value, diff);
    watertank->log.info(message);

    watertank->Status.value.FillValveDelay = diff;
    watertank->Status.publish();
}

static void on_change_enableheater(void)
{
    watertank->log.verbose("watertank->EnableHeater changed!");
    if (watertank->EnableHeater.value)
    {
        watertank->log.success("Heater Enabled!");
    }
    else
    {
        watertank->log.success("Heater Disabled!");
    }
}

static void on_change_heaterconfig(void)
{
    watertank->log.verbose("watertank->HeaterConfig changed!");
    check_heaterconfig();
}

int main()
{
    //retrieve the watertank structure
    watertank = libWaterTank_init();

    //setup callbacks
    watertank->FillValve.on_change = on_change_fillvalve;
    watertank->EnableHeater.on_change = on_change_enableheater;
    watertank->HeaterConfig.on_change = on_change_heaterconfig;

    //connect to the server
    watertank->connect();

    catch_termination();
    sim_data_t sim;
    sim.old_low = false;
    sim.old_high = false;

    while (!is_terminated())
    {
        //trigger callbacks and synchronize with AR
        watertank->process();

        if (watertank->is_operational)
        {
            check_heaterconfig();
            run_tanksimulator(&sim);
        }
    }

    //shutdown
    watertank->disconnect();
    watertank->dispose();

    return 0;
}

static void check_heaterconfig(void)
{
    bool values_changed = false;

    //limit incoming config values, and publish if changed
    if (watertank->HeaterConfig.value.MaxPower > 1.0)
    {
        values_changed = true;
        watertank->HeaterConfig.value.MaxPower = 1.0;
    }

    if (watertank->HeaterConfig.value.MaxPower < EPSILON)
    {
        values_changed = true;
        watertank->HeaterConfig.value.MaxPower = 0.5;
    }

    if (watertank->HeaterConfig.value.MaxTemperature > 90)
    {
        values_changed = true;
        watertank->HeaterConfig.value.MaxTemperature = 90;
    }

    if (watertank->HeaterConfig.value.MaxTemperature < EPSILON)
    {
        values_changed = true;
        watertank->HeaterConfig.value.MaxTemperature = 45;
    }

    if (values_changed)
    {
        watertank->log.success("HeaterConfig changed, values limited");
        watertank->HeaterConfig.publish();
    }
}

static void run_tanksimulator(sim_data_t *sim)
{
    watertank->Status.value.LevelLow = (watertank->Status.value.WaterLevel < 200);
    watertank->Status.value.LevelHigh = (watertank->Status.value.WaterLevel > 800);

    //publish values on change
    if (sim->old_low != watertank->Status.value.LevelLow)
    {
        char message[256];
        snprintf(message, 256, "Level LOW changed to %d, Water Level: %d", watertank->Status.value.LevelLow, watertank->Status.value.WaterLevel);
        watertank->log.info(message);

        watertank->Status.publish();
    }
    if (sim->old_high != watertank->Status.value.LevelHigh)
    {
        char message[256];
        snprintf(message, 256, "Level HIGH changed to %d, Water Level: %d", watertank->Status.value.LevelHigh, watertank->Status.value.WaterLevel);
        watertank->log.info(message);

        watertank->Status.publish();
    }

    sim->old_low = watertank->Status.value.LevelLow;
    sim->old_high = watertank->Status.value.LevelHigh;

    switch (watertank->FillValve.value)
    {
    case VALVE_OPEN:
        if (watertank->Status.value.WaterLevel < 1000)
        {
            watertank->Status.value.WaterLevel += 5;
            watertank->Status.publish();
        }
        break;
    case VALVE_CLOSED:
        if (watertank->Status.value.WaterLevel > 0)
        {
            watertank->Status.value.WaterLevel -= 2;
            watertank->Status.publish();
        }
        break;
    }

    if (watertank->EnableHeater.value)
    {
        if (watertank->Status.value.Heater.WaterTemperature < watertank->HeaterConfig.value.MaxTemperature)
        {
            watertank->Status.value.Heater.WaterTemperature += watertank->HeaterConfig.value.MaxPower;
            watertank->Status.value.Heater.HeatingActive = true;
            watertank->Status.publish();
        }
        else
        {
            watertank->Status.value.Heater.WaterTemperature -= 3 * watertank->HeaterConfig.value.MaxPower;
            watertank->Status.value.Heater.HeatingActive = false;
            watertank->Status.publish();
        }
    }
    else
    {
        watertank->Status.value.Heater.HeatingActive = false;
        if (watertank->Status.value.Heater.WaterTemperature >= 20.1)
        {
            watertank->Status.value.Heater.WaterTemperature -= 0.1;
            watertank->Status.publish();
        }
    }
}
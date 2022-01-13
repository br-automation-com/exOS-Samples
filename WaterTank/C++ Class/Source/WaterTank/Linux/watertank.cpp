#include <string>
#include <csignal>
#include "WaterTankDatamodel.hpp"
#include "termination.h"

/* datamodel features:

main methods:
    watertank.connect()
    watertank.disconnect()
    watertank.process()
    watertank.setOperational()
    watertank.dispose()
    watertank.getNettime() : (int32_t) get current nettime

void(void) user lambda callback:
    watertank.onConnectionChange([&] () {
        // watertank.connectionState ...
    })

boolean values:
    watertank.isConnected
    watertank.isOperational

logging methods:
    watertank.log.error << "some value:" << 1 << std::endl;
    watertank.log.warning << "some value:" << 1 << std::endl;
    watertank.log.success << "some value:" << 1 << std::endl;
    watertank.log.info << "some value:" << 1 << std::endl;
    watertank.log.debug << "some value:" << 1 << std::endl;
    watertank.log.verbose << "some value:" << 1 << std::endl;

dataset FillValve:
    watertank.FillValve.onChange([&] () {
        watertank.FillValve.value ...
    })
    watertank.FillValve.nettime : (int32_t) nettime @ time of publish
    watertank.FillValve.value : (VALVE_STATE)  actual dataset values

dataset EnableHeater:
    watertank.EnableHeater.onChange([&] () {
        watertank.EnableHeater.value ...
    })
    watertank.EnableHeater.nettime : (int32_t) nettime @ time of publish
    watertank.EnableHeater.value : (bool)  actual dataset value

dataset HeaterConfig:
    watertank.HeaterConfig.publish()
    watertank.HeaterConfig.onChange([&] () {
        watertank.HeaterConfig.value ...
    })
    watertank.HeaterConfig.nettime : (int32_t) nettime @ time of publish
    watertank.HeaterConfig.value : (WaterTankHeaterConfig)  actual dataset values

dataset Status:
    watertank.Status.publish()
    watertank.Status.value : (WaterTankStatus)  actual dataset values
*/
#define EPSILON 0.0001
typedef struct sim_data
{
    bool old_high;
    bool old_low;
} sim_data_t;

static void check_heaterconfig(WaterTankDatamodel *watertank);
static void run_tanksimulator(WaterTankDatamodel *watertank, sim_data_t *sim);

int main(int argc, char ** argv) {

    catch_termination();
    
    WaterTankDatamodel watertank;
    watertank.connect();
    
    watertank.onConnectionChange([&] () {
        if (watertank.connectionState == EXOS_STATE_OPERATIONAL) {    
            watertank.log.success << "WaterTank operational!" << std::endl;
        }
    });

    watertank.FillValve.onChange([&] () {
        int32_t diff = watertank.getNettime() - watertank.FillValve.nettime;
        watertank.log.info << "Filling Valve Changed to " << watertank.FillValve.value << ", timedelay " << diff << std::endl;

        watertank.Status.value.FillValveDelay = diff;
        watertank.Status.publish();
    });

    watertank.EnableHeater.onChange([&] () {
        if (watertank.EnableHeater.value)
        {
            watertank.log.success << "Heater Enabled!" << std::endl;
        }
        else
        {
            watertank.log.success << "Heater Disabled!" << std::endl;
        }
    });

    watertank.HeaterConfig.onChange([&] () {
        check_heaterconfig(&watertank);
    });

    sim_data_t sim;
    sim.old_low = false;
    sim.old_high = false;

    while(!is_terminated()) {
        // trigger callbacks
        watertank.process();
                
        if (watertank.isOperational) {
            check_heaterconfig(&watertank);
            run_tanksimulator(&watertank, &sim);
        }
    }

    return 0;
}

static void check_heaterconfig(WaterTankDatamodel *watertank) {
    bool values_changed = false;

    //limit incoming config values, and publish if changed
    if (watertank->HeaterConfig.value.MaxPower > 1.0) {
        values_changed = true;
        watertank->HeaterConfig.value.MaxPower = 1.0;
    }

    if (watertank->HeaterConfig.value.MaxPower < EPSILON) {
        values_changed = true;
        watertank->HeaterConfig.value.MaxPower = 0.5;
    }

    if (watertank->HeaterConfig.value.MaxTemperature > 90) {
        values_changed = true;
        watertank->HeaterConfig.value.MaxTemperature = 90;
    }

    if (watertank->HeaterConfig.value.MaxTemperature < EPSILON) {
        values_changed = true;
        watertank->HeaterConfig.value.MaxTemperature = 45;
    }

    if (values_changed) {
        watertank->log.success << "HeaterConfig changed, values limited";
        watertank->HeaterConfig.publish();
    }
}

static void run_tanksimulator(WaterTankDatamodel *watertank, sim_data_t *sim) {
    watertank->Status.value.LevelLow = (watertank->Status.value.WaterLevel < 200);
    watertank->Status.value.LevelHigh = (watertank->Status.value.WaterLevel > 800);

    //publish values on change
    if (sim->old_low != watertank->Status.value.LevelLow) {
        watertank->log.info << "Level LOW changed to "<< watertank->Status.value.LevelLow << ", Water Level: " << watertank->Status.value.WaterLevel << std::endl;
        watertank->Status.publish();
    }
    if (sim->old_high != watertank->Status.value.LevelHigh) {
        watertank->log.info << "Level HIGH changed to "<< watertank->Status.value.LevelHigh << ", Water Level: " << watertank->Status.value.WaterLevel << std::endl;
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
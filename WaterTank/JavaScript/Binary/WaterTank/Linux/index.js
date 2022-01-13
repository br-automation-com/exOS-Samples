/**
 * @callback WaterTankDataModelCallback
 * @returns {function()}
 * 
 * @typedef {Object} WaterTankHeaterConfigDataSetValue
 * @property {number} MaxTemperature 
 * @property {number} MaxPower 
 * 
 * @typedef {Object} WaterTankStatusHeaterDataSetValue
 * @property {number} WaterTemperature 
 * @property {boolean} HeatingActive 
 * 
 * @typedef {Object} WaterTankStatusDataSetValue
 * @property {boolean} LevelHigh 
 * @property {boolean} LevelLow 
 * @property {number} FillValveDelay 
 * @property {number} WaterLevel 
 * @property {WaterTankStatusHeaterDataSetValue} Heater 
 * 
 * @typedef {Object} FillValveDataSet
 * @property {number} value 
 * @property {WaterTankDataModelCallback} onChange event fired when `value` changes
 * @property {number} nettime used in the `onChange` event: nettime @ time of publish
 * @property {number} latency used in the `onChange` event: time in us between publish and arrival
 * @property {WaterTankDataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} EnableHeaterDataSet
 * @property {boolean} value 
 * @property {WaterTankDataModelCallback} onChange event fired when `value` changes
 * @property {number} nettime used in the `onChange` event: nettime @ time of publish
 * @property {number} latency used in the `onChange` event: time in us between publish and arrival
 * @property {WaterTankDataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} HeaterConfigDataSet
 * @property {WaterTankHeaterConfigDataSetValue} value  
 * @property {function()} publish publish the value
 * @property {WaterTankDataModelCallback} onChange event fired when `value` changes
 * @property {number} nettime used in the `onChange` event: nettime @ time of publish
 * @property {number} latency used in the `onChange` event: time in us between publish and arrival
 * @property {WaterTankDataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} StatusDataSet
 * @property {WaterTankStatusDataSetValue} value 
 * @property {function()} publish publish the value
 * @property {WaterTankDataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * 
 * @typedef {Object} WaterTankDatamodel
 * @property {FillValveDataSet} FillValve
 * @property {EnableHeaterDataSet} EnableHeater
 * @property {HeaterConfigDataSet} HeaterConfig
 * @property {StatusDataSet} Status
 * 
 * @callback WaterTankDatamodelLogMethod
 * @param {string} message
 * 
 * @typedef {Object} WaterTankDatamodelLog
 * @property {WaterTankDatamodelLogMethod} warning
 * @property {WaterTankDatamodelLogMethod} success
 * @property {WaterTankDatamodelLogMethod} info
 * @property {WaterTankDatamodelLogMethod} debug
 * @property {WaterTankDatamodelLogMethod} verbose
 * 
 * @typedef {Object} WaterTank
 * @property {function():number} nettime get current nettime
 * @property {WaterTankDataModelCallback} onConnectionChange event fired when `connectionState` changes 
 * @property {string} connectionState `Connected`|`Operational`|`Disconnected`|`Aborted` - used in the `onConnectionChange` event
 * @property {boolean} isConnected
 * @property {boolean} isOperational
 * @property {WaterTankDatamodelLog} log
 * @property {WaterTankDatamodel} datamodel
 * 
 */

/**
 * @type {WaterTank}
 */
let watertank = require('./l_WaterTank.node').WaterTank;

/* datamodel features:

main methods:
    watertank.nettime() : (int32_t) get current nettime

state change events:
    watertank.onConnectionChange(() => {
        watertank.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted" 
    })

boolean values:
    watertank.isConnected
    watertank.isOperational

logging methods:
    watertank.log.error(string)
    watertank.log.warning(string)
    watertank.log.success(string)
    watertank.log.info(string)
    watertank.log.debug(string)
    watertank.log.verbose(string)

dataset FillValve:
    watertank.datamodel.FillValve.value : (int32_t)  actual dataset value
    watertank.datamodel.FillValve.onChange(() => {
        watertank.datamodel.FillValve.value ...
        watertank.datamodel.FillValve.nettime : (int32_t) nettime @ time of publish
        watertank.datamodel.FillValve.latency : (int32_t) time in us between publish and arrival
    })
    watertank.datamodel.FillValve.onConnectionChange(() => {
        watertank.datamodel.FillValve.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });

dataset EnableHeater:
    watertank.datamodel.EnableHeater.value : (bool)  actual dataset value
    watertank.datamodel.EnableHeater.onChange(() => {
        watertank.datamodel.EnableHeater.value ...
        watertank.datamodel.EnableHeater.nettime : (int32_t) nettime @ time of publish
        watertank.datamodel.EnableHeater.latency : (int32_t) time in us between publish and arrival
    })
    watertank.datamodel.EnableHeater.onConnectionChange(() => {
        watertank.datamodel.EnableHeater.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });

dataset HeaterConfig:
    watertank.datamodel.HeaterConfig.value : (WaterTankHeaterConfig)  actual dataset values
    watertank.datamodel.HeaterConfig.publish()
    watertank.datamodel.HeaterConfig.onChange(() => {
        watertank.datamodel.HeaterConfig.value ...
        watertank.datamodel.HeaterConfig.nettime : (int32_t) nettime @ time of publish
        watertank.datamodel.HeaterConfig.latency : (int32_t) time in us between publish and arrival
    })
    watertank.datamodel.HeaterConfig.onConnectionChange(() => {
        watertank.datamodel.HeaterConfig.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });

dataset Status:
    watertank.datamodel.Status.value : (WaterTankStatus)  actual dataset values
    watertank.datamodel.Status.publish()
    watertank.datamodel.Status.onConnectionChange(() => {
        watertank.datamodel.Status.connectionState : (string) "Connected", "Operational", "Disconnected" or "Aborted"
    });
*/

//connection state changes
watertank.onConnectionChange(() => {
    switch (watertank.connectionState) {
        case "Operational":
            watertank.log.success("WaterTank operational!");
            break;
    }
});

//value change events
watertank.datamodel.FillValve.onChange(() => {
    watertank.log.info(`Filling Valve Changed to ${watertank.datamodel.FillValve.value}, timedelay ${watertank.datamodel.FillValve.latency}`);

    watertank.datamodel.Status.value.FillValveDelay = watertank.datamodel.FillValve.latency;
    watertank.datamodel.Status.publish();
});
watertank.datamodel.EnableHeater.onChange(() => {
    if (watertank.datamodel.EnableHeater.value) {
        watertank.log.success("Heater Enabled!");
    }
    else {
        watertank.log.success("Heater Disabled!");
    }
});
watertank.datamodel.HeaterConfig.onChange(() => {
    watertank.log.verbose("watertank.datamodel.HeaterConfig changed!");
    checkHeaterConfig();
});

//Cyclic call triggered from the Component Server
watertank.onProcessed(() => {
    //Publish values
    if (watertank.isOperational) {
        checkHeaterConfig();
        runTankSimulator();
    }
});

const EPSILON = 0.0001

function checkHeaterConfig() {

    let values_changed = false;

    //limit incoming config values, and publish if changed
    if (watertank.datamodel.HeaterConfig.value.MaxPower > 1.0) {
        values_changed = true;
        watertank.datamodel.HeaterConfig.value.MaxPower = 1.0;
    }

    if (watertank.datamodel.HeaterConfig.value.MaxPower < EPSILON) {
        values_changed = true;
        watertank.datamodel.HeaterConfig.value.MaxPower = 0.5;
    }

    if (watertank.datamodel.HeaterConfig.value.MaxTemperature > 90) {
        values_changed = true;
        watertank.datamodel.HeaterConfig.value.MaxTemperature = 90;
    }

    if (watertank.datamodel.HeaterConfig.value.MaxTemperature < EPSILON) {
        values_changed = true;
        watertank.datamodel.HeaterConfig.value.MaxTemperature = 45;
    }

    if (values_changed) {
        watertank.log.success("HeaterConfig changed, values limited");
        watertank.datamodel.HeaterConfig.publish();
    }
}

let sim = {
    old_low: false,
    old_high: false
}

const VALVE_OPEN = 1
const VALVE_CLOSED = 0

function runTankSimulator() {
    watertank.datamodel.Status.value.LevelLow = (watertank.datamodel.Status.value.WaterLevel < 200);
    watertank.datamodel.Status.value.LevelHigh = (watertank.datamodel.Status.value.WaterLevel > 800);

    //publish values on change
    if (sim.old_low != watertank.datamodel.Status.value.LevelLow) {
        watertank.log.info(`Level LOW changed to ${watertank.datamodel.Status.value.LevelLow}, Water Level: ${watertank.datamodel.Status.value.WaterLevel}`);
        watertank.datamodel.Status.publish();
    }
    if (sim.old_high != watertank.datamodel.Status.value.LevelHigh) {
        watertank.log.info(`Level HIGH changed to ${watertank.datamodel.Status.value.LevelHigh}, Water Level: ${watertank.datamodel.Status.value.WaterLevel}`);
        watertank.datamodel.Status.publish();
    }

    sim.old_low = watertank.datamodel.Status.value.LevelLow;
    sim.old_high = watertank.datamodel.Status.value.LevelHigh;

    switch (watertank.datamodel.FillValve.value) {
        case VALVE_OPEN:
            if (watertank.datamodel.Status.value.WaterLevel < 1000) {
                watertank.datamodel.Status.value.WaterLevel += 5;
                watertank.datamodel.Status.publish();
            }
            break;
        case VALVE_CLOSED:
            if (watertank.datamodel.Status.value.WaterLevel > 0) {
                watertank.datamodel.Status.value.WaterLevel -= 2;
                watertank.datamodel.Status.publish();
            }
            break;
    }

    if (watertank.datamodel.EnableHeater.value) {
        if (watertank.datamodel.Status.value.Heater.WaterTemperature < watertank.datamodel.HeaterConfig.value.MaxTemperature) {
            watertank.datamodel.Status.value.Heater.WaterTemperature += watertank.datamodel.HeaterConfig.value.MaxPower;
            watertank.datamodel.Status.value.Heater.HeatingActive = true;
            watertank.datamodel.Status.publish();
        }
        else {
            watertank.datamodel.Status.value.Heater.WaterTemperature -= 3 * watertank.datamodel.HeaterConfig.value.MaxPower;
            watertank.datamodel.Status.value.Heater.HeatingActive = false;
            watertank.datamodel.Status.publish();
        }
    }
    else {
        watertank.datamodel.Status.value.Heater.HeatingActive = false;
        if (watertank.datamodel.Status.value.Heater.WaterTemperature >= 20.1) {
            watertank.datamodel.Status.value.Heater.WaterTemperature -= 0.1;
            watertank.datamodel.Status.publish();
        }
    }
}
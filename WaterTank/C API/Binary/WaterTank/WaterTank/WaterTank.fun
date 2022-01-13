FUNCTION_BLOCK WaterTankInit
	VAR_OUTPUT
		Handle : UDINT;
	END_VAR
	VAR
		_state : USINT;
	END_VAR
END_FUNCTION_BLOCK

FUNCTION_BLOCK WaterTankCyclic
	VAR_INPUT
		Enable : BOOL;
		Handle : UDINT;
		Start : BOOL;
		pWaterTank : REFERENCE TO WaterTank;
	END_VAR
	VAR_OUTPUT
		Active : BOOL;
		Error : BOOL;
		Disconnected : BOOL;
		Connected : BOOL;
		Operational : BOOL;
		Aborted : BOOL;
	END_VAR
	VAR
		_state : USINT;
	END_VAR
END_FUNCTION_BLOCK

FUNCTION_BLOCK WaterTankExit
	VAR_INPUT
		Handle : UDINT;
	END_VAR
	VAR
		_state : USINT;
	END_VAR
END_FUNCTION_BLOCK

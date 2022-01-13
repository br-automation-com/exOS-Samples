FUNCTION_BLOCK WaterTankInit
    VAR_OUTPUT
        Handle : UDINT;
    END_VAR
END_FUNCTION_BLOCK

FUNCTION_BLOCK WaterTankCyclic
    VAR_INPUT
        Enable : BOOL;
        Start : BOOL;
        Handle : UDINT;
        pWaterTank : REFERENCE TO WaterTank;
    END_VAR
    VAR_OUTPUT
        Connected : BOOL;
        Operational : BOOL;
        Error : BOOL;
    END_VAR
    VAR
        _Start : BOOL;
        _Enable : BOOL;
    END_VAR
END_FUNCTION_BLOCK

FUNCTION_BLOCK WaterTankExit
    VAR_INPUT
        Handle : UDINT;
    END_VAR
END_FUNCTION_BLOCK


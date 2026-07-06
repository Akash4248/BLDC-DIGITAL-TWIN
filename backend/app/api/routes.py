from __future__ import annotations

from fastapi import APIRouter, Depends

from backend.app.models.motor_parameters import MotorParameters
from backend.app.schemas.simulation import (
    ControlResponse,
    FaultInjectionRequest,
    FaultStatusResponse,
    LogEntryResponse,
    MotorParametersUpdate,
    SimulationControlAction,
    SimulationStateResponse,
    TelemetryStepRequest,
)
from backend.app.services.simulation_service import SimulationService, SimulationServiceError
from backend.app.api.deps import get_simulation_service


router = APIRouter(prefix="/api/v1")


@router.get("/health")
def health() -> dict[str, str]:
    return {"status": "ok"}


@router.get("/simulation/state", response_model=SimulationStateResponse)
def simulation_state(service: SimulationService = Depends(get_simulation_service)) -> SimulationStateResponse:
    return SimulationStateResponse(**service.state())


@router.post("/simulation/control/{action}", response_model=ControlResponse)
def simulation_control(
    action: SimulationControlAction,
    service: SimulationService = Depends(get_simulation_service),
) -> ControlResponse:
    if action == SimulationControlAction.start:
        service.start()
    elif action == SimulationControlAction.stop:
        service.stop()
    elif action == SimulationControlAction.reset:
        service.reset()
    return ControlResponse(status=action.value, running=service.running)


@router.put("/parameters", response_model=dict)
def update_parameters(
    payload: MotorParametersUpdate,
    service: SimulationService = Depends(get_simulation_service),
) -> dict:
    data = payload.dict() if hasattr(payload, "dict") else payload.model_dump()
    parameters = MotorParameters(**data)
    return service.update_parameters(parameters)


@router.post("/faults/inject", response_model=FaultStatusResponse)
def inject_fault(
    payload: FaultInjectionRequest,
    service: SimulationService = Depends(get_simulation_service),
) -> FaultStatusResponse:
    faults = service.inject_fault(payload)
    return FaultStatusResponse(status="ok", faults=faults)


@router.delete("/faults", response_model=FaultStatusResponse)
def clear_faults(service: SimulationService = Depends(get_simulation_service)) -> FaultStatusResponse:
    faults = service.clear_faults()
    return FaultStatusResponse(status="ok", faults=faults)


@router.get("/logs", response_model=list[LogEntryResponse])
def logs(service: SimulationService = Depends(get_simulation_service)) -> list[LogEntryResponse]:
    return [LogEntryResponse(**entry) for entry in service.list_logs()]


@router.post("/simulation/step")
def simulation_step(
    payload: TelemetryStepRequest,
    service: SimulationService = Depends(get_simulation_service),
) -> dict:
    try:
        result = service.step_from_telemetry(payload)
    except SimulationServiceError as exc:
        return {"status": "error", "detail": str(exc)}
    return {
        "status": "ok",
        "feedback": result["feedback"],
        "hall": result["hall"],
        "encoder": result["encoder"],
        "current_sensors": result["current_sensors"],
    }

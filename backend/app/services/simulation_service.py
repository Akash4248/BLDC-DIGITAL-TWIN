from __future__ import annotations

from dataclasses import asdict
from threading import RLock
from typing import Any

from backend.app.core.logging import EventLogBuffer
from backend.app.models.motor_parameters import MotorParameterError, MotorParameters
from backend.app.serial.protocol import TelemetryPacket, encode_twin_state
from backend.app.schemas.simulation import FaultInjectionRequest, TelemetryStepRequest
from backend.app.services.faults import FaultManager, FaultType
from backend.app.services.ws_broadcaster import WebSocketBroadcaster
from backend.app.twin.engine import TwinSimulationEngine, TwinStepResult


class SimulationServiceError(ValueError):
    """Raised when simulation operations fail."""


class SimulationService:
    """
    Application service that owns the simulation state.

    Networking layers call into this service. The twin engine remains isolated
    from FastAPI, WebSockets, and REST transport concerns.
    """

    def __init__(self, engine: TwinSimulationEngine) -> None:
        self._engine = engine
        self._lock = RLock()
        self._running = False
        self._faults = FaultManager()
        self._logs = EventLogBuffer()
        self._broadcaster = WebSocketBroadcaster()

    @property
    def broadcaster(self) -> WebSocketBroadcaster:
        return self._broadcaster

    @property
    def running(self) -> bool:
        with self._lock:
            return self._running

    def start(self) -> None:
        with self._lock:
            self._running = True
            self._logs.append("info", "simulation.start", {"running": True})

    def stop(self) -> None:
        with self._lock:
            self._running = False
            self._logs.append("info", "simulation.stop", {"running": False})

    def reset(self) -> None:
        with self._lock:
            self._engine.reset()
            self._running = False
            self._logs.append("info", "simulation.reset", {"step_index": 0})

    def update_parameters(self, parameters: MotorParameters) -> dict[str, Any]:
        parameters.validate()
        with self._lock:
            self._engine.parameters = parameters
            self._engine.parameters.validate()
            self._engine.electrical_model.parameters = parameters
            self._engine.mechanical_model.parameters = parameters
            self._logs.append("info", "parameters.update", parameters.to_dict())
            return parameters.to_dict()

    def inject_fault(self, request: FaultInjectionRequest) -> list[dict[str, Any]]:
        with self._lock:
            payload = request.dict() if hasattr(request, "dict") else request.model_dump()
            self._faults.upsert(
                FaultType(request.fault_type.value),
                enabled=request.enabled,
                value=request.value,
            )
            self._logs.append(
                "warning" if request.enabled else "info",
                "fault.update",
                payload,
            )
            return self._faults.list()

    def clear_faults(self) -> list[dict[str, Any]]:
        with self._lock:
            self._faults.clear()
            self._logs.append("info", "fault.clear", {})
            return self._faults.list()

    def list_faults(self) -> list[dict[str, Any]]:
        with self._lock:
            return self._faults.list()

    def list_logs(self) -> list[dict[str, Any]]:
        return self._logs.list()

    def state(self) -> dict[str, Any]:
        with self._lock:
            return {
                "running": self._running,
                "sample_rate_hz": self._engine.config.SampleRateHz,
                "step_index": self._engine.step_index,
                "parameters": self._engine.parameters.to_dict(),
                "faults": self._faults.list(),
                "clients": self._broadcaster.client_count,
            }

    def step_from_telemetry(self, request: TelemetryStepRequest) -> dict[str, Any]:
        packet = TelemetryPacket(
            sequence=request.sequence,
            timestamp_us=request.timestamp_us,
            duty_a=request.duty_a,
            duty_b=request.duty_b,
            duty_c=request.duty_c,
            vdc=request.vdc,
        )
        return self._step(packet)

    def step_packet(self, packet: TelemetryPacket) -> dict[str, Any]:
        return self._step(packet)

    def step_bytes(self, frame: bytes) -> bytes:
        from backend.app.serial.protocol import decode_frame

        packet = decode_frame(frame)
        if not isinstance(packet, TelemetryPacket):
            raise SimulationServiceError("expected telemetry packet")
        result = self._step(packet)
        return encode_twin_state(result["feedback"])

    def _step(self, packet: TelemetryPacket) -> dict[str, Any]:
        with self._lock:
            if not self._running:
                self._logs.append("warning", "simulation.rejected", {"reason": "stopped"})
                raise SimulationServiceError("simulation is stopped")

            packet = self._faults.apply_telemetry(packet)

            original_load_torque = self._engine.mechanical_model.parameters.LoadTorque
            self._engine.mechanical_model.parameters.LoadTorque = original_load_torque + self._faults.load_torque_delta()
            if self._faults.has(FaultType.locked_rotor):
                self._engine.mechanical_model.state = type(self._engine.mechanical_model.state)(
                    Speed=0.0,
                    MechanicalAngle=self._engine.mechanical_model.state.MechanicalAngle,
                )

            try:
                result = self._engine.step(packet)
            finally:
                self._engine.mechanical_model.parameters.LoadTorque = original_load_torque
            result = self._faults.apply(result)
            if self._faults.has(FaultType.locked_rotor):
                self._engine.mechanical_model.state = type(self._engine.mechanical_model.state)(
                    Speed=result.mechanical.Speed,
                    MechanicalAngle=result.mechanical.MechanicalAngle,
                )
            self._logs.append(
                "info",
                "simulation.step",
                {
                    "sequence": packet.sequence,
                    "step_index": self._engine.step_index,
                },
            )
            return {
                "telemetry": result.telemetry,
                "inverter": result.inverter,
                "electrical": result.electrical,
                "mechanical": result.mechanical,
                "hall": result.hall,
                "encoder": result.encoder,
                "current_sensors": result.current_sensors,
                "feedback": result.feedback,
                "electromagnetic_torque": result.electromagnetic_torque,
            }

    async def step_and_broadcast(self, packet: TelemetryPacket) -> TwinStepResult:
        with self._lock:
            if not self._running:
                self._logs.append("warning", "simulation.rejected", {"reason": "stopped"})
                raise SimulationServiceError("simulation is stopped")
            result = self._engine.step(packet)
            result = self._faults.apply(result)
            self._logs.append(
                "info",
                "simulation.step",
                {"sequence": packet.sequence, "step_index": self._engine.step_index},
            )
        await self._broadcaster.broadcast(result)
        return result

from __future__ import annotations

import asyncio
from dataclasses import asdict
from typing import Any

from fastapi import WebSocket

from backend.app.schemas.ws import TelemetrySnapshot
from backend.app.twin.engine import TwinStepResult


class WebSocketBroadcastError(RuntimeError):
    """Raised when websocket broadcast fails."""


class WebSocketBroadcaster:
    """
    Fan-out broadcaster for dashboard websocket clients.

    This component is transport-only. It does not step the simulation.
    It consumes simulation snapshots and sends them to every connected client.
    """

    def __init__(self) -> None:
        self._clients: set[WebSocket] = set()
        self._lock = asyncio.Lock()

    @property
    def client_count(self) -> int:
        return len(self._clients)

    async def connect(self, websocket: WebSocket) -> None:
        await websocket.accept()
        async with self._lock:
            self._clients.add(websocket)

    async def disconnect(self, websocket: WebSocket) -> None:
        async with self._lock:
            self._clients.discard(websocket)

    def _serialize(self, result: TwinStepResult) -> dict[str, Any]:
        return {
            "telemetry": {
                "sequence": result.telemetry.sequence,
                "timestamp_us": result.telemetry.timestamp_us,
                "duty_a": result.telemetry.duty_a,
                "duty_b": result.telemetry.duty_b,
                "duty_c": result.telemetry.duty_c,
                "vdc": result.telemetry.vdc,
            },
            "current": {
                "ia": result.electrical.Ia,
                "ib": result.electrical.Ib,
                "ic": result.electrical.Ic,
                "adc_a": result.current_sensors.ADCA,
                "adc_b": result.current_sensors.ADCB,
                "adc_c": result.current_sensors.ADCC,
            },
            "speed": result.mechanical.Speed,
            "torque": result.electromagnetic_torque,
            "rotor_angle": result.mechanical.ElectricalAngle,
            "hall": {
                "a": result.hall.HallA,
                "b": result.hall.HallB,
                "c": result.hall.HallC,
            },
            "encoder": {
                "a": result.encoder.EncoderA,
                "b": result.encoder.EncoderB,
                "index": result.encoder.IndexPulse,
            },
        }

    async def broadcast(self, result: TwinStepResult) -> None:
        payload = self._serialize(result)
        stale_clients: list[WebSocket] = []
        for client in list(self._clients):
            try:
                await client.send_json(payload)
            except Exception:
                stale_clients.append(client)

        if stale_clients:
            async with self._lock:
                for client in stale_clients:
                    self._clients.discard(client)

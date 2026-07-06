from __future__ import annotations

from fastapi import APIRouter, Depends, WebSocket, WebSocketDisconnect

from backend.app.serial.protocol import TelemetryPacket, decode_frame
from backend.app.services.simulation_service import SimulationService, SimulationServiceError
from backend.app.api.deps import get_simulation_service


router = APIRouter(prefix="/api/v1")


@router.websocket("/ws/telemetry")
async def websocket_telemetry(
    websocket: WebSocket,
    service: SimulationService = Depends(get_simulation_service),
) -> None:
    await service.broadcaster.connect(websocket)
    try:
        while True:
            frame = await websocket.receive_bytes()
            try:
                packet = decode_frame(frame)
                if not isinstance(packet, TelemetryPacket):
                    await websocket.send_json({"status": "error", "detail": "expected telemetry packet"})
                    continue
                await service.step_and_broadcast(packet)
            except SimulationServiceError as exc:
                await websocket.send_json({"status": "error", "detail": str(exc)})
            except Exception as exc:
                await websocket.send_json({"status": "error", "detail": str(exc)})
    except WebSocketDisconnect:
        await service.broadcaster.disconnect(websocket)


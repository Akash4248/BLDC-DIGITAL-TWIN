from __future__ import annotations

import asyncio

from fastapi import FastAPI

from backend.app.api.ws import router as ws_router
from backend.app.api.routes import router
from backend.app.core.logging import configure_logging
from backend.app.services.usb_bridge import DigitalTwinUsbBridge, UsbBridgeConfig
from backend.app.api.deps import get_simulation_service


def create_app() -> FastAPI:
    configure_logging()
    app = FastAPI(title="BLDC Digital Twin Backend", version="1.0.0")
    app.include_router(router)
    app.include_router(ws_router)

    @app.on_event("startup")
    async def _startup() -> None:
        config = UsbBridgeConfig.from_env()
        if config is None:
            return
        bridge = DigitalTwinUsbBridge(get_simulation_service(), config)
        bridge.start(asyncio.get_running_loop())
        app.state.usb_bridge = bridge

    @app.on_event("shutdown")
    async def _shutdown() -> None:
        bridge = getattr(app.state, "usb_bridge", None)
        if bridge is not None:
            bridge.stop()

    return app


app = create_app()

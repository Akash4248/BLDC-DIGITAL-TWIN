import type { FaultItem, LogEntry, MotorParameters, SimulationState } from '../app/types'

const HTTP_BASE = import.meta.env.VITE_BACKEND_HTTP_URL ?? 'http://localhost:8000'
const WS_BASE = import.meta.env.VITE_BACKEND_WS_URL ?? 'ws://localhost:8000/api/v1/ws/telemetry'

async function requestJson<T>(path: string, init?: RequestInit): Promise<T> {
  const response = await fetch(`${HTTP_BASE}${path}`, {
    headers: {
      'Content-Type': 'application/json',
      ...(init?.headers ?? {}),
    },
    ...init,
  })

  if (!response.ok) {
    throw new Error(`Request failed: ${response.status}`)
  }

  return response.json() as Promise<T>
}

export const backendClient = {
  wsUrl: WS_BASE,
  getState(): Promise<SimulationState> {
    return requestJson<SimulationState>('/api/v1/simulation/state')
  },
  start(): Promise<{ status: string; running: boolean }> {
    return requestJson('/api/v1/simulation/control/start', { method: 'POST' })
  },
  stop(): Promise<{ status: string; running: boolean }> {
    return requestJson('/api/v1/simulation/control/stop', { method: 'POST' })
  },
  reset(): Promise<{ status: string; running: boolean }> {
    return requestJson('/api/v1/simulation/control/reset', { method: 'POST' })
  },
  updateParameters(parameters: MotorParameters): Promise<MotorParameters> {
    return requestJson('/api/v1/parameters', {
      method: 'PUT',
      body: JSON.stringify(parameters),
    })
  },
  injectFault(payload: { fault_type: string; enabled: boolean; value?: number | null }): Promise<{ status: string; faults: FaultItem[] }> {
    return requestJson('/api/v1/faults/inject', {
      method: 'POST',
      body: JSON.stringify(payload),
    })
  },
  clearFaults(): Promise<{ status: string; faults: FaultItem[] }> {
    return requestJson('/api/v1/faults', { method: 'DELETE' })
  },
  logs(): Promise<LogEntry[]> {
    return requestJson('/api/v1/logs')
  },
}

import type { TelemetrySnapshot } from './types'

export function deriveDashboardMetrics(snapshot: TelemetrySnapshot) {
  const rpm = snapshot.speed * 9.5493
  const current = Math.max(
    Math.abs(snapshot.current.ia),
    Math.abs(snapshot.current.ib),
    Math.abs(snapshot.current.ic),
  )
  const voltage = snapshot.telemetry.vdc
  const inputPower = Math.max(0.01, voltage * current * 1.2)
  const mechanicalPower = Math.max(0, snapshot.torque * snapshot.speed)
  const efficiency = Math.min(100, Math.max(0, (mechanicalPower / inputPower) * 100))
  const temperature = Math.min(140, Math.max(20, 28 + current * 3.5 + Math.abs(snapshot.torque) * 4.5))

  return {
    rpm,
    torque: snapshot.torque,
    voltage,
    current,
    efficiency,
    temperature,
  }
}


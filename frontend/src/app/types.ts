import type { FocState } from './focMath'

export type TelemetrySnapshot = {
  telemetry: {
    sequence: number
    timestamp_us: number
    duty_a: number
    duty_b: number
    duty_c: number
    vdc: number
  }
  current: {
    ia: number
    ib: number
    ic: number
    adc_a: number
    adc_b: number
    adc_c: number
  }
  speed: number
  torque: number
  rotor_angle: number
  hall: {
    a: number
    b: number
    c: number
  }
  encoder: {
    a: number
    b: number
    index: number
  }
  temperature?: number
  pwm?: {
    ah: number
    al: number
    bh: number
    bl: number
    ch: number
    cl: number
  }
  // ===== WEBSITE VISUALIZATION START =====
  RotorAngle?: number
  StatorFieldAngle?: number
  RotorFieldAngle?: number
  FieldAngleDifference?: number
  MotorMode?: string
  ControllerMode?: string
  Ia?: number
  Ib?: number
  Ic?: number
  RPM?: number
  Voltage?: number
  PolePairs?: number
  // ===== WEBSITE VISUALIZATION END =====
}

export type PwmState = {
  ah: number
  al: number
  bh: number
  bl: number
  ch: number
  cl: number
}

export type MotorState = {
  rotorAngle: number
  rotorSpeed: number
  electricalAngle: number
  ia: number
  ib: number
  ic: number
  torque: number
  temperature: number
  hallState: {
    a: number
    b: number
    c: number
  }
  encoderPosition: number
  encoderPhase: {
    a: number
    b: number
    index: number
  }
  pwm: PwmState
  dutyCycle: {
    a: number
    b: number
    c: number
  }
  foc: FocState
}

export type SimulationState = {
  running: boolean
  clients: number
  sample_rate_hz: number
  step_index: number
  faults?: FaultItem[]
}

export type MotorParameters = {
  Rs: number
  Ld: number
  Lq: number
  PolePairs: number
  FluxLinkage: number
  Kt: number
  Ke: number
  RotorInertia: number
  Friction: number
  LoadTorque: number
  Vdc: number
}

export type FaultItem = {
  fault_type: string
  enabled: boolean
  value?: number | null
}

export type LogEntry = {
  timestamp: string
  level: string
  event: string
  detail: Record<string, unknown>
}

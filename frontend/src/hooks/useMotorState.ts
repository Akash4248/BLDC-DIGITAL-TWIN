import { useEffect, useRef, useState } from 'react'
import type { MotorState, TelemetrySnapshot } from '../app/types'
import { backendClient } from '../services/backendClient'

type ConnectionState = 'connecting' | 'connected' | 'disconnected' | 'mock'

function createMockSnapshot(t: number): TelemetrySnapshot {
  const angle = (t * 0.8) % (Math.PI * 2)
  const speed = 1200 + Math.sin(t * 0.6) * 120
  const torque = 1.8 + Math.cos(t * 0.8) * 0.25
  const ia = Math.sin(angle) * 9
  const ib = Math.sin(angle - (2 * Math.PI) / 3) * 9
  const ic = Math.sin(angle + (2 * Math.PI) / 3) * 9
  const hall = {
    a: Math.floor((angle / (Math.PI / 3)) % 6) % 2,
    b: Math.floor(((angle + Math.PI / 3) / (Math.PI / 3)) % 6) % 2,
    c: Math.floor(((angle + (2 * Math.PI) / 3) / (Math.PI / 3)) % 6) % 2,
  }

  return {
    telemetry: {
      sequence: Math.floor(t * 25),
      timestamp_us: Math.floor(t * 1000000),
      duty_a: 0.5 + Math.sin(angle) * 0.18,
      duty_b: 0.5 + Math.sin(angle - (2 * Math.PI) / 3) * 0.18,
      duty_c: 0.5 + Math.sin(angle + (2 * Math.PI) / 3) * 0.18,
      vdc: 24 + Math.sin(t * 0.2) * 0.4,
    },
    current: {
      ia,
      ib,
      ic,
      adc_a: Math.round((ia + 12) * 120),
      adc_b: Math.round((ib + 12) * 120),
      adc_c: Math.round((ic + 12) * 120),
    },
    speed,
    torque,
    rotor_angle: angle,
    hall,
    encoder: {
      a: Number(Math.sin(angle) >= 0),
      b: Number(Math.sin(angle + Math.PI / 2) >= 0),
      index: Number(angle < 0.08),
    },
  }
}

function deriveMotorState(snapshot: TelemetrySnapshot): MotorState {
  const currentMagnitude = Math.max(
    Math.abs(snapshot.current.ia),
    Math.abs(snapshot.current.ib),
    Math.abs(snapshot.current.ic),
  )
  const dutyA = snapshot.telemetry.duty_a
  const dutyB = snapshot.telemetry.duty_b
  const dutyC = snapshot.telemetry.duty_c

  return {
    rotorAngle: snapshot.rotor_angle,
    rotorSpeed: snapshot.speed,
    electricalAngle: snapshot.rotor_angle * 4,
    ia: snapshot.current.ia,
    ib: snapshot.current.ib,
    ic: snapshot.current.ic,
    torque: snapshot.torque,
    temperature: snapshot.temperature ?? 40 + currentMagnitude * 0.35 + Math.abs(snapshot.torque) * 1.15,
    hallState: snapshot.hall,
    encoderPosition: Math.round((snapshot.rotor_angle / (Math.PI * 2)) * 8192),
    encoderPhase: snapshot.encoder,
    pwm: snapshot.pwm ?? {
      ah: dutyA,
      al: 1 - dutyA,
      bh: dutyB,
      bl: 1 - dutyB,
      ch: dutyC,
      cl: 1 - dutyC,
    },
    dutyCycle: {
      a: dutyA,
      b: dutyB,
      c: dutyC,
    },
  }
}

function normalizePayload(payload: {
  clients?: number
  telemetry?: TelemetrySnapshot['telemetry']
  current?: TelemetrySnapshot['current']
  speed?: number
  torque?: number
  rotor_angle?: number
  rotorAngle?: number
  rotor_speed?: number
  rotorSpeed?: number
  hall?: TelemetrySnapshot['hall']
  encoder?: TelemetrySnapshot['encoder']
  temperature?: number
  pwm?: TelemetrySnapshot['pwm']
  pwmA?: number
  pwmB?: number
  pwmC?: number
  ia?: number
  ib?: number
  ic?: number
  currentA?: number
  currentB?: number
  currentC?: number
}) {
  const telemetry = payload.telemetry ?? {
    sequence: 0,
    timestamp_us: 0,
    duty_a: payload.pwmA ?? 0.5,
    duty_b: payload.pwmB ?? 0.5,
    duty_c: payload.pwmC ?? 0.5,
    vdc: 24,
  }
  const current = payload.current ?? {
    ia: payload.ia ?? payload.currentA ?? 0,
    ib: payload.ib ?? payload.currentB ?? 0,
    ic: payload.ic ?? payload.currentC ?? 0,
    adc_a: 0,
    adc_b: 0,
    adc_c: 0,
  }
  const rotorSpeed = payload.speed ?? payload.rotor_speed ?? payload.rotorSpeed ?? 0
  const rotorAngle = payload.rotor_angle ?? payload.rotorAngle ?? 0
  return {
    telemetry,
    current,
    speed: rotorSpeed,
    torque: payload.torque ?? 0,
    rotor_angle: rotorAngle,
    hall: payload.hall ?? { a: 0, b: 0, c: 0 },
    encoder: payload.encoder ?? { a: 0, b: 0, index: 0 },
    temperature: payload.temperature,
    pwm: payload.pwm,
    clients: payload.clients,
  }
}

export function useMotorState() {
  const [snapshot, setSnapshot] = useState<TelemetrySnapshot>(() => createMockSnapshot(0))
  const [motorState, setMotorState] = useState<MotorState>(() => deriveMotorState(createMockSnapshot(0)))
  const [connection, setConnection] = useState<ConnectionState>('connecting')
  const [connectedClients, setConnectedClients] = useState(0)
  const [history, setHistory] = useState<TelemetrySnapshot[]>([createMockSnapshot(0)])
  const mockTimer = useRef<number | null>(null)
  const tick = useRef(0)
  const mounted = useRef(true)

  function commitSnapshot(next: TelemetrySnapshot) {
    setSnapshot(next)
    setMotorState(deriveMotorState(next))
    setHistory((current) => [...current.slice(-119), next])
  }

  function startMockStream() {
    if (!mounted.current || mockTimer.current !== null) return
    setConnection('mock')
    mockTimer.current = window.setInterval(() => {
      tick.current += 0.04
      const next = createMockSnapshot(tick.current)
      commitSnapshot(next)
      setConnectedClients(1)
    }, 40)
  }

  function stopMockStream() {
    if (mockTimer.current === null) return
    window.clearInterval(mockTimer.current)
    mockTimer.current = null
  }

  useEffect(() => {
    mounted.current = true
    startMockStream()

    const socket = new WebSocket(backendClient.wsUrl)
    socket.binaryType = 'arraybuffer'

    socket.onopen = () => {
      if (!mounted.current) return
      setConnection('connected')
    }
    socket.onclose = () => {
      if (!mounted.current) return
      setConnection('disconnected')
      startMockStream()
    }
    socket.onerror = () => {
      if (!mounted.current) return
      socket.close()
      setConnection('disconnected')
    }

    socket.onmessage = (event) => {
      if (typeof event.data !== 'string') return
      try {
        const payload = normalizePayload(JSON.parse(event.data) as Record<string, unknown> as any)

        if (!mounted.current) return
        if (typeof payload.clients === 'number') setConnectedClients(payload.clients)

        if (
          payload.current &&
          typeof payload.speed === 'number' &&
          typeof payload.torque === 'number' &&
          payload.telemetry
        ) {
          stopMockStream()
          commitSnapshot({
            telemetry: payload.telemetry,
            current: payload.current,
            speed: payload.speed,
            torque: payload.torque,
            rotor_angle: payload.rotor_angle ?? 0,
            hall: payload.hall ?? { a: 0, b: 0, c: 0 },
            encoder: payload.encoder ?? { a: 0, b: 0, index: 0 },
            temperature: payload.temperature,
            pwm: payload.pwm,
          })
        }
      } catch {
        if (!mounted.current) return
        setConnection('disconnected')
        startMockStream()
      }
    }

    return () => {
      mounted.current = false
      stopMockStream()
      socket.close()
    }
  }, [])

  useEffect(
    () => () => {
      stopMockStream()
    },
    [],
  )

  return {
    snapshot,
    motorState,
    history,
    connection,
    connectedClients,
  }
}

import { useEffect, useRef, useState } from 'react'
import type { TelemetrySnapshot } from '../app/types'
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

export function useTelemetrySocket() {
  const [snapshot, setSnapshot] = useState<TelemetrySnapshot>(() => createMockSnapshot(0))
  const [connection, setConnection] = useState<ConnectionState>('connecting')
  const [connectedClients, setConnectedClients] = useState(0)
  const [history, setHistory] = useState<TelemetrySnapshot[]>([createMockSnapshot(0)])
  const mockTimer = useRef<number | null>(null)
  const tick = useRef(0)
  const mounted = useRef(true)

  function startMockStream() {
    if (!mounted.current || mockTimer.current !== null) return
    setConnection('mock')
    mockTimer.current = window.setInterval(() => {
      tick.current += 0.04
      const next = createMockSnapshot(tick.current)
      setSnapshot(next)
      setHistory((current) => [...current.slice(-119), next])
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
      if (typeof event.data === 'string') {
        try {
          const payload = JSON.parse(event.data) as {
            clients?: number
            telemetry?: TelemetrySnapshot['telemetry']
            current?: TelemetrySnapshot['current']
            speed?: number
            torque?: number
            rotor_angle?: number
            hall?: TelemetrySnapshot['hall']
            encoder?: TelemetrySnapshot['encoder']
          }
          if (!mounted.current) return
          if (typeof payload.clients === 'number') setConnectedClients(payload.clients)
          if (payload.current && typeof payload.speed === 'number' && typeof payload.torque === 'number' && payload.telemetry) {
            stopMockStream()
            const next: TelemetrySnapshot = {
              telemetry: payload.telemetry,
              current: payload.current,
              speed: payload.speed,
              torque: payload.torque,
              rotor_angle: payload.rotor_angle ?? 0,
              hall: payload.hall ?? { a: 0, b: 0, c: 0 },
              encoder: payload.encoder ?? { a: 0, b: 0, index: 0 },
            }
            setSnapshot(next)
            setHistory((current) => [...current.slice(-119), next])
          }
        } catch {
          if (!mounted.current) return
          setConnection('disconnected')
          startMockStream()
        }
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
    history,
    connection,
    connectedClients,
  }
}

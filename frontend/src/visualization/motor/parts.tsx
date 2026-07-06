import { Grid, Html, MeshReflectorMaterial, OrbitControls, Text, useGLTF } from '@react-three/drei'
import { useFrame, useThree } from '@react-three/fiber'
import { useEffect, useMemo, useRef } from 'react'
import * as THREE from 'three'
import type { MotorState } from '../../app/types'
import type { MotorViewMode } from './modelMapper'
import type { HoverTarget } from './types'
import type { RefObject } from 'react'

const MOTOR_MODEL_URL = '/models/electric_motor.glb'

useGLTF.preload(MOTOR_MODEL_URL)

type HoverCallback = (target: HoverTarget | null, position?: [number, number, number]) => void

function hoverMaterial(color = '#6EE7F9') {
  return (
    <meshStandardMaterial
      color={color}
      emissive={color}
      emissiveIntensity={0.95}
      transparent
      opacity={0.16}
      depthWrite={false}
    />
  )
}

export function CameraController({ controlsRef }: { controlsRef: RefObject<any> }) {
  const { camera, gl } = useThree()

  useEffect(() => {
    controlsRef.current?.saveState?.()
  }, [controlsRef])

  return (
    <OrbitControls
      ref={controlsRef}
      args={[camera, gl.domElement]}
      enableDamping
      dampingFactor={0.06}
      enablePan
      enableZoom
      minDistance={3.2}
      maxDistance={13}
      maxPolarAngle={Math.PI * 0.48}
      target={[0, 0.1, 0]}
    />
  )
}

export function Lighting() {
  return (
    <>
      <color attach="background" args={['#061018']} />
      <ambientLight intensity={0.7} color="#b7d7e6" />
      <directionalLight position={[6, 8, 5]} intensity={2.7} color="#f3f7fb" castShadow shadow-mapSize={[2048, 2048]} />
      <directionalLight position={[-5, 3, -4]} intensity={1.2} color="#6EE7F9" />
      <pointLight position={[0, 4, 3]} intensity={0.8} color="#FFB86B" distance={18} />
      <hemisphereLight intensity={0.45} groundColor="#071218" color="#cce8ff" />
    </>
  )
}

export function Ground() {
  return (
    <group rotation={[-Math.PI / 2, 0, 0]} position={[0, -1.95, 0]}>
      <mesh receiveShadow>
        <planeGeometry args={[18, 18]} />
        <MeshReflectorMaterial
          resolution={1024}
          mirror={0.15}
          mixStrength={7}
          blur={[500, 120]}
          minDepthThreshold={0.75}
          maxDepthThreshold={1.4}
          color="#071017"
          metalness={0.25}
          roughness={0.78}
        />
      </mesh>
      <Grid
        infiniteGrid
        fadeDistance={18}
        fadeStrength={3}
        cellSize={0.45}
        sectionSize={2.25}
        cellColor="#12303a"
        sectionColor="#1b5563"
      />
    </group>
  )
}

export function Rotor({ state, onHover, mode }: { state: MotorState; onHover: HoverCallback; mode: MotorViewMode }) {
  const group = useRef<THREE.Group>(null)
  const magnetStrength = Math.min(1.2, 0.18 + Math.abs(state.torque) * 0.16 + Math.max(Math.abs(state.ia), Math.abs(state.ib), Math.abs(state.ic)) * 0.012)
  const magnetAngles = useMemo(() => Array.from({ length: 8 }, (_, index) => (index / 8) * Math.PI * 2), [])

  useFrame((_, delta) => {
    if (!group.current) return
    const target = state.rotorAngle
    group.current.rotation.y = THREE.MathUtils.damp(group.current.rotation.y, target, 8, delta)
    const targetPosition = mode === 'exploded' ? new THREE.Vector3(0, 0.14, 0) : new THREE.Vector3(0, 0, 0)
    group.current.position.lerp(targetPosition, 1 - Math.exp(-delta * 8))
  })

  return (
    <group ref={group}>
      <mesh castShadow receiveShadow>
        <cylinderGeometry args={[0.84, 0.84, 2.36, 48]} />
        <meshStandardMaterial color="#0a1116" metalness={0.94} roughness={0.14} />
      </mesh>
      {magnetAngles.map((angle, index) => {
        const x = Math.cos(angle) * 0.74
        const z = Math.sin(angle) * 0.74
        const positive = index % 2 === 0
        return (
          <mesh key={angle} position={[x, 0, z]} rotation={[0, -angle, 0]} castShadow>
            <boxGeometry args={[0.16, 0.34, 0.07]} />
            <meshStandardMaterial
              color={positive ? '#7fdff1' : '#d28c46'}
              emissive={positive ? '#6EE7F9' : '#FFB86B'}
              emissiveIntensity={magnetStrength}
              metalness={0.18}
              roughness={0.3}
            />
          </mesh>
        )
      })}
      <mesh
        castShadow
        receiveShadow
        onPointerOver={(event) => {
          event.stopPropagation()
          onHover('rotor', [0, 0, 0])
        }}
        onPointerOut={() => onHover(null)}
      >
        <cylinderGeometry args={[0.88, 0.88, 2.42, 48]} />
        {hoverMaterial('#6EE7F9')}
      </mesh>
    </group>
  )
}

export function Shaft({ state, onHover, mode }: { state: MotorState; onHover: HoverCallback; mode: MotorViewMode }) {
  const shaft = useRef<THREE.Group>(null)

  useFrame((_, delta) => {
    if (!shaft.current) return
    shaft.current.rotation.y = THREE.MathUtils.damp(shaft.current.rotation.y, state.rotorAngle, 9.5, delta)
    const targetPosition = mode === 'exploded' ? new THREE.Vector3(0, 0.2, 0) : new THREE.Vector3(0, 0, 0)
    shaft.current.position.lerp(targetPosition, 1 - Math.exp(-delta * 8))
  })

  return (
    <group ref={shaft}>
      <mesh castShadow receiveShadow>
        <cylinderGeometry args={[0.12, 0.12, 3.6, 28]} />
        <meshStandardMaterial color="#aeb8bf" metalness={0.92} roughness={0.13} />
      </mesh>
      <mesh
        onPointerOver={(event) => {
          event.stopPropagation()
          onHover('shaft', [0, 0, 0])
        }}
        onPointerOut={() => onHover(null)}
      >
        <cylinderGeometry args={[0.18, 0.18, 3.7, 28]} />
        {hoverMaterial('#8dcff2')}
      </mesh>
    </group>
  )
}

export function Bearing({
  position,
  state,
  onHover,
  mode,
}: {
  position: [number, number, number]
  state: MotorState
  onHover: HoverCallback
  mode: MotorViewMode
}) {
  const ring = useRef<THREE.Group>(null)

  useFrame((_, delta) => {
    if (!ring.current) return
    ring.current.rotation.y += delta * (0.18 + Math.abs(state.rotorSpeed) * 0.00008)
    const exploded = mode === 'exploded' ? 0.24 : 0
    const target = new THREE.Vector3(position[0], position[1] + Math.sign(position[1] || 1) * exploded, position[2])
    ring.current.position.lerp(target, 1 - Math.exp(-delta * 8))
  })

  return (
    <group position={position} ref={ring}>
      <mesh castShadow receiveShadow>
        <cylinderGeometry args={[0.34, 0.34, 0.2, 40]} />
        <meshStandardMaterial color="#1c2c34" metalness={0.9} roughness={0.18} />
      </mesh>
      <mesh castShadow receiveShadow>
        <cylinderGeometry args={[0.2, 0.2, 0.22, 40]} />
        <meshStandardMaterial color="#0b1115" metalness={0.95} roughness={0.12} />
      </mesh>
      <mesh
        onPointerOver={(event) => {
          event.stopPropagation()
          onHover('bearing', position)
        }}
        onPointerOut={() => onHover(null)}
      >
        <cylinderGeometry args={[0.38, 0.38, 0.24, 40]} />
        {hoverMaterial('#8dcff2')}
      </mesh>
    </group>
  )
}

function WindingArc({
  radius,
  angle,
  color,
  current,
  label,
  onHover,
  target,
}: {
  radius: number
  angle: number
  color: string
  current: number
  label: HoverTarget
  onHover: HoverCallback
  target: HoverTarget
}) {
  const polarity = current >= 0 ? 1 : -1
  const magnitude = Math.min(1, Math.abs(current) / 12)
  const emissive = polarity > 0 ? '#6EE7F9' : '#FFB86B'
  const baseColor = magnitude < 0.08 ? '#51626b' : color

  return (
    <group rotation={[0, angle, Math.PI / 2]}>
      <mesh castShadow receiveShadow>
        <torusGeometry args={[radius, 0.08, 14, 60, Math.PI * 1.2]} />
        <meshStandardMaterial
          color={baseColor}
          emissive={emissive}
          emissiveIntensity={0.2 + magnitude * 1.4}
          metalness={0.55}
          roughness={0.28}
        />
      </mesh>
      <mesh
        onPointerOver={(event) => {
          event.stopPropagation()
          onHover(target, [Math.cos(angle) * 1.1, 0.15, Math.sin(angle) * 1.1])
        }}
        onPointerOut={() => onHover(null)}
      >
        <torusGeometry args={[radius + 0.02, 0.14, 12, 44, Math.PI * 1.1]} />
        {hoverMaterial(color)}
      </mesh>
      <Html position={[Math.cos(angle) * 1.22, 0.14, Math.sin(angle) * 1.22]} center>
        <div
          style={{
            color: '#d9edf4',
            fontSize: 10,
            letterSpacing: '0.16em',
            textTransform: 'uppercase',
            opacity: 0.48 + magnitude * 0.4,
            mixBlendMode: 'screen',
          }}
        >
          {label === 'phaseA' ? 'phase a' : label === 'phaseB' ? 'phase b' : 'phase c'}
        </div>
      </Html>
    </group>
  )
}

export function Windings({ state, onHover, mode }: { state: MotorState; onHover: HoverCallback; mode: MotorViewMode }) {
  const group = useRef<THREE.Group>(null)

  useFrame((_, delta) => {
    if (!group.current) return
    const targetPosition = mode === 'exploded' ? new THREE.Vector3(0, 0.08, 0) : new THREE.Vector3(0, 0, 0)
    group.current.position.lerp(targetPosition, 1 - Math.exp(-delta * 8))
  })

  return (
    <group ref={group} position={[0, 0, 0]}>
      <WindingArc radius={1.22} angle={0} color="#6EE7F9" current={state.ia} label="phaseA" onHover={onHover} target="phaseA" />
      <WindingArc radius={1.22} angle={(2 * Math.PI) / 3} color="#FFB86B" current={state.ib} label="phaseB" onHover={onHover} target="phaseB" />
      <WindingArc radius={1.22} angle={(4 * Math.PI) / 3} color="#5FD38A" current={state.ic} label="phaseC" onHover={onHover} target="phaseC" />
      <mesh castShadow receiveShadow>
        <cylinderGeometry args={[1.52, 1.52, 2.85, 64, 1, true]} />
        <meshStandardMaterial color="#20313a" metalness={0.22} roughness={0.74} transparent opacity={0.72} side={THREE.DoubleSide} />
      </mesh>
    </group>
  )
}

function PulseDot({
  angle,
  radius,
  speed,
  color,
}: {
  angle: number
  radius: number
  speed: number
  color: string
}) {
  const ref = useRef<THREE.Mesh>(null)

  useFrame((state, delta) => {
    if (!ref.current) return
    const t = state.clock.elapsedTime * speed + angle
    ref.current.position.x = Math.cos(t) * radius
    ref.current.position.z = Math.sin(t) * radius
    ref.current.position.y = Math.sin(t * 2.2) * 0.18
    ref.current.scale.setScalar(0.7 + Math.sin(t * 3.6) * 0.12)
  })

  return (
    <mesh ref={ref} castShadow>
      <sphereGeometry args={[0.045, 16, 16]} />
      <meshStandardMaterial color={color} emissive={color} emissiveIntensity={1.4} />
    </mesh>
  )
}

export function CurrentFlow({ state }: { state: MotorState }) {
  const mag = Math.min(1, Math.max(Math.abs(state.ia), Math.abs(state.ib), Math.abs(state.ic)) / 12)
  return (
    <group position={[0, 0, 0]}>
      <PulseDot angle={0} radius={1.13} speed={1.8 + mag * 0.5} color={state.ia >= 0 ? '#6EE7F9' : '#FFB86B'} />
      <PulseDot angle={2} radius={1.13} speed={1.6 + mag * 0.4} color={state.ib >= 0 ? '#6EE7F9' : '#FFB86B'} />
      <PulseDot angle={4} radius={1.13} speed={1.7 + mag * 0.45} color={state.ic >= 0 ? '#5FD38A' : '#FFB86B'} />
    </group>
  )
}

export function MagneticField({ state, onHover }: { state: MotorState; onHover: HoverCallback }) {
  const group = useRef<THREE.Group>(null)
  const strength = Math.min(1.2, 0.2 + Math.max(Math.abs(state.ia), Math.abs(state.ib), Math.abs(state.ic)) * 0.02)

  useFrame((_, delta) => {
    if (!group.current) return
    group.current.rotation.y = THREE.MathUtils.damp(group.current.rotation.y, state.electricalAngle, 7.5, delta)
  })

  return (
    <group ref={group}>
      {Array.from({ length: 10 }, (_, index) => {
        const angle = (index / 10) * Math.PI * 2
        const x = Math.cos(angle) * 1.72
        const z = Math.sin(angle) * 1.72
        return (
          <mesh key={angle} position={[x, 0.02 * Math.sin(angle * 3), z]} rotation={[0, -angle, 0]} castShadow>
            <torusGeometry args={[0.38, 0.018, 10, 42, Math.PI * 0.95]} />
            <meshStandardMaterial
              color={index % 2 === 0 ? '#6EE7F9' : '#FFB86B'}
              emissive={index % 2 === 0 ? '#6EE7F9' : '#FFB86B'}
              emissiveIntensity={0.14 + strength * 0.6}
              transparent
              opacity={0.95}
            />
          </mesh>
        )
      })}
      <mesh
        onPointerOver={(event) => {
          event.stopPropagation()
          onHover('magneticField', [0, 1.2, 0])
        }}
        onPointerOut={() => onHover(null)}
      >
        <torusGeometry args={[1.78, 0.12, 18, 64]} />
        {hoverMaterial('#6EE7F9')}
      </mesh>
    </group>
  )
}

export function TorqueArrow({ state }: { state: MotorState }) {
  const group = useRef<THREE.Group>(null)
  const length = Math.min(1.65, 0.6 + Math.abs(state.torque) * 0.5)

  useFrame((_, delta) => {
    if (!group.current) return
    const target = state.torque >= 0 ? 0 : Math.PI
    group.current.rotation.y = THREE.MathUtils.dampAngle(group.current.rotation.y, target, 5.5, delta)
  })

  return (
    <group ref={group} position={[0, 1.72, 0]}>
      <mesh castShadow>
        <cylinderGeometry args={[0.045, 0.045, length, 12]} />
        <meshStandardMaterial color="#FFB86B" emissive="#FFB86B" emissiveIntensity={1.2} />
      </mesh>
      <mesh position={[0, length / 2 + 0.12, 0]} castShadow>
        <coneGeometry args={[0.14, 0.22, 12]} />
        <meshStandardMaterial color="#FFB86B" emissive="#FFB86B" emissiveIntensity={1.3} />
      </mesh>
      <Text position={[0, 0.2, 0]} fontSize={0.1} color="#ffddb7" anchorX="center" anchorY="bottom">
        Torque
      </Text>
    </group>
  )
}

export function HallIndicators({ state, onHover }: { state: MotorState; onHover: HoverCallback }) {
  const leds = [
    { label: 'HA', active: state.hallState.a > 0, color: '#6EE7F9', pos: [-1.95, 1.1, 0.55] as [number, number, number] },
    { label: 'HB', active: state.hallState.b > 0, color: '#FFB86B', pos: [-1.95, 0.92, 0] as [number, number, number] },
    { label: 'HC', active: state.hallState.c > 0, color: '#5FD38A', pos: [-1.95, 1.1, -0.55] as [number, number, number] },
  ]

  return (
    <group position={[0, 0, 0]}>
      <mesh
        onPointerOver={(event) => {
          event.stopPropagation()
          onHover('hall', [-1.8, 1.05, 0])
        }}
        onPointerOut={() => onHover(null)}
      >
        <torusGeometry args={[1.95, 0.055, 12, 36]} />
        {hoverMaterial('#6EE7F9')}
      </mesh>
      {leds.map((led) => (
        <group key={led.label} position={led.pos}>
          <mesh castShadow>
            <sphereGeometry args={[0.095, 20, 20]} />
            <meshStandardMaterial
              color={led.active ? led.color : '#1a2730'}
              emissive={led.color}
              emissiveIntensity={led.active ? 2.1 : 0.12}
              metalness={0.15}
              roughness={0.32}
            />
          </mesh>
          <Html position={[0.18, 0, 0]} center>
            <div style={{ color: '#d7e9f2', fontSize: 10, letterSpacing: '0.14em' }}>{led.label}</div>
          </Html>
        </group>
      ))}
    </group>
  )
}

export function Encoder({ state, onHover }: { state: MotorState; onHover: HoverCallback }) {
  const group = useRef<THREE.Group>(null)
  const pulseCount = 48

  useFrame((_, delta) => {
    if (!group.current) return
    group.current.rotation.y = THREE.MathUtils.damp(group.current.rotation.y, state.rotorAngle, 9, delta)
  })

  return (
    <group ref={group} position={[0, 0, 1.72]}>
      <mesh castShadow receiveShadow>
        <cylinderGeometry args={[0.28, 0.28, 0.14, 48]} />
        <meshStandardMaterial color="#101a20" metalness={0.75} roughness={0.24} />
      </mesh>
      {Array.from({ length: pulseCount }, (_, index) => {
        const angle = (index / pulseCount) * Math.PI * 2
        const x = Math.cos(angle) * 0.34
        const y = Math.sin(angle) * 0.34
        return (
          <mesh key={angle} position={[x, y, 0.09]} rotation={[0, 0, angle]} castShadow>
            <boxGeometry args={[0.025, 0.07, 0.01]} />
            <meshStandardMaterial
              color={index % 4 === 0 ? '#6EE7F9' : '#27424d'}
              emissive="#6EE7F9"
              emissiveIntensity={index % 4 === 0 ? 0.95 : 0.1}
            />
          </mesh>
        )
      })}
      <mesh
        onPointerOver={(event) => {
          event.stopPropagation()
          onHover('encoder', [0, 0, 1.78])
        }}
        onPointerOut={() => onHover(null)}
      >
        <cylinderGeometry args={[0.42, 0.42, 0.2, 48]} />
        {hoverMaterial('#6EE7F9')}
      </mesh>
      <Html position={[0.54, 0.22, 0]} center>
        <div style={{ color: '#9edbf1', fontSize: 10, letterSpacing: '0.14em' }}>
          {state.encoderPosition}
        </div>
      </Html>
    </group>
  )
}

export function MotorLabels({
  active,
  position,
}: {
  active: HoverTarget | null
  position: [number, number, number]
}) {
  if (!active) return null

  const labels: Record<HoverTarget, string> = {
    housing: 'Motor housing',
    stator: 'Stator',
    rotor: 'Rotor',
    shaft: 'Shaft',
    bearing: 'Bearing',
    phaseA: 'Phase A winding',
    phaseB: 'Phase B winding',
    phaseC: 'Phase C winding',
    hall: 'Hall sensors',
    encoder: 'Encoder disk',
    magneticField: 'Magnetic field',
  }

  return (
    <Html position={position} center>
      <div
        style={{
          padding: '6px 10px',
          borderRadius: 999,
          background: 'rgba(7,16,22,0.94)',
          color: '#E6F4FA',
          border: '1px solid rgba(110,231,249,0.22)',
          fontSize: 11,
          letterSpacing: '0.08em',
          textTransform: 'uppercase',
          boxShadow: '0 10px 30px rgba(0,0,0,0.35)',
          whiteSpace: 'nowrap',
        }}
      >
        {labels[active]}
      </div>
    </Html>
  )
}

export function BaseMotorShell({
  onHover,
}: {
  onHover: HoverCallback
}) {
  return (
    <group position={[0, 0, 0]}>
      <mesh castShadow receiveShadow>
        <cylinderGeometry args={[1.88, 1.88, 3.18, 72]} />
        <meshStandardMaterial color="#10191f" metalness={0.95} roughness={0.3} />
      </mesh>
      <mesh castShadow receiveShadow>
        <cylinderGeometry args={[1.6, 1.6, 2.82, 72]} />
        <meshStandardMaterial color="#26343c" metalness={0.22} roughness={0.7} transparent opacity={0.88} side={THREE.DoubleSide} />
      </mesh>
      <mesh
        onPointerOver={(event) => {
          event.stopPropagation()
          onHover('stator', [0, 0.15, 0])
        }}
        onPointerOut={() => onHover(null)}
      >
        <cylinderGeometry args={[1.7, 1.7, 3.0, 72, 1, true]} />
        {hoverMaterial('#6EE7F9')}
      </mesh>
      <mesh
        onPointerOver={(event) => {
          event.stopPropagation()
          onHover('housing', [0, 1.5, 0])
        }}
        onPointerOut={() => onHover(null)}
      >
        <cylinderGeometry args={[1.96, 1.96, 3.28, 72, 1, true]} />
        {hoverMaterial('#8b9ca8')}
      </mesh>
      <Text position={[0, 2.22, 0]} fontSize={0.13} color="#b2d8ea" anchorX="center" anchorY="middle">
        BLDC / PMSM Digital Twin
      </Text>
    </group>
  )
}

export function MotorAsset() {
  const { scene } = useGLTF(MOTOR_MODEL_URL) as any
  return <primitive object={scene} scale={1.32} rotation={[0, Math.PI / 2, 0]} position={[0, -0.02, 0]} />
}

function temperatureColor(temperature: number) {
  const normalized = THREE.MathUtils.clamp((temperature - 35) / 75, 0, 1)
  const cold = new THREE.Color('#4aa3ff')
  const warm = new THREE.Color('#f6a04d')
  const hot = new THREE.Color('#ff4d4d')
  if (normalized < 0.55) {
    return cold.clone().lerp(warm, normalized / 0.55)
  }
  return warm.clone().lerp(hot, (normalized - 0.55) / 0.45)
}

export function TemperatureShell({
  temperature,
  mode,
}: {
  temperature: number
  mode: MotorViewMode
}) {
  const color = temperatureColor(temperature)
  const opacity = mode === 'transparent' ? 0.1 : mode === 'cutaway' ? 0.16 : 0.28

  return (
    <group>
      <mesh castShadow receiveShadow>
        <cylinderGeometry args={[1.74, 1.74, 3.24, 64, 1, true]} />
        <meshPhysicalMaterial
          color={color}
          emissive={color}
          emissiveIntensity={0.12 + THREE.MathUtils.clamp((temperature - 45) / 45, 0, 1) * 0.7}
          transparent
          opacity={opacity}
          roughness={0.16}
          metalness={0.08}
          clearcoat={0.6}
          clearcoatRoughness={0.35}
          depthWrite={false}
          side={THREE.DoubleSide}
        />
      </mesh>
      <mesh>
        <cylinderGeometry args={[1.52, 1.52, 2.92, 64, 1, true]} />
        <meshStandardMaterial
          color={color}
          emissive={color}
          emissiveIntensity={0.06 + THREE.MathUtils.clamp((temperature - 35) / 65, 0, 1) * 0.55}
          transparent
          opacity={mode === 'standard' ? 0.06 : 0.08}
          side={THREE.DoubleSide}
          depthWrite={false}
        />
      </mesh>
    </group>
  )
}

export function PwmIndicators({
  pwm,
}: {
  pwm: MotorState['pwm']
}) {
  const items = [
    { label: 'AH', key: 'ah' as const, color: '#6EE7F9' },
    { label: 'AL', key: 'al' as const, color: '#94a3b8' },
    { label: 'BH', key: 'bh' as const, color: '#FFB86B' },
    { label: 'BL', key: 'bl' as const, color: '#94a3b8' },
    { label: 'CH', key: 'ch' as const, color: '#5FD38A' },
    { label: 'CL', key: 'cl' as const, color: '#94a3b8' },
  ]

  return (
    <group position={[2.0, 1.4, 0]}>
      {items.map((item, index) => {
        const intensity = THREE.MathUtils.clamp(pwm[item.key], 0.03, 1)
        const x = index < 3 ? 0 : 0.36
        const y = 0.26 - (index % 3) * 0.26
        return (
          <group key={item.label} position={[x, y, 0]}>
            <mesh castShadow>
              <sphereGeometry args={[0.06, 18, 18]} />
              <meshStandardMaterial
                color={intensity > 0.5 ? item.color : '#20313a'}
                emissive={item.color}
                emissiveIntensity={intensity * 2.0}
                metalness={0.15}
                roughness={0.24}
              />
            </mesh>
            <Html position={[0.16, 0, 0]} center>
              <div style={{ color: '#d8edf8', fontSize: 10, letterSpacing: '0.16em' }}>{item.label}</div>
            </Html>
          </group>
        )
      })}
    </group>
  )
}

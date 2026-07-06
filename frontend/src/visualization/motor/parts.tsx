import { Grid, Html, MeshReflectorMaterial, OrbitControls, Text, useGLTF } from '@react-three/drei'
import { useFrame, useThree } from '@react-three/fiber'
import { useEffect, useMemo, useRef } from 'react'
import * as THREE from 'three'
import type { MotorState } from '../../app/types'
import type { MotorViewMode } from './modelMapper'
import type { HoverTarget } from './types'
import type { RefObject } from 'react'

const MOTOR_MODEL_URL = '/models/dji_phantom_4_bldc_motor.glb'

useGLTF.preload(MOTOR_MODEL_URL)

type HoverCallback = (target: HoverTarget | null, position?: [number, number, number]) => void

function hoverMaterial(color = '#b6c2cb') {
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
      <color attach="background" args={['#080c10']} />
      <ambientLight intensity={0.85} color="#c3ccd4" />
      <directionalLight position={[6, 8, 5]} intensity={3} color="#f3f7fb" castShadow shadow-mapSize={[2048, 2048]} />
      <directionalLight position={[-5, 3, -4]} intensity={1.0} color="#c98b4d" />
      <pointLight position={[0, 4, 3]} intensity={0.55} color="#b9c4cb" distance={18} />
      <hemisphereLight intensity={0.38} groundColor="#0f1418" color="#d5dde3" />
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
          color="#0a0f13"
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
        cellColor="#1a2329"
        sectionColor="#2d363d"
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
        <meshStandardMaterial color="#141a1f" metalness={0.94} roughness={0.14} />
      </mesh>
      {magnetAngles.map((angle, index) => {
        const x = Math.cos(angle) * 0.74
        const z = Math.sin(angle) * 0.74
        const positive = index % 2 === 0
        return (
          <mesh key={angle} position={[x, 0, z]} rotation={[0, -angle, 0]} castShadow>
            <boxGeometry args={[0.16, 0.34, 0.07]} />
            <meshStandardMaterial
              color={positive ? '#b9c4cb' : '#c98b4d'}
              emissive={positive ? '#b9c4cb' : '#c98b4d'}
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
        {hoverMaterial('#b6c2cb')}
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
        <meshStandardMaterial color="#d7dde2" metalness={0.92} roughness={0.13} />
      </mesh>
      <mesh
        onPointerOver={(event) => {
          event.stopPropagation()
          onHover('shaft', [0, 0, 0])
        }}
        onPointerOut={() => onHover(null)}
      >
        <cylinderGeometry args={[0.18, 0.18, 3.7, 28]} />
        {hoverMaterial('#b6c2cb')}
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
        {hoverMaterial('#b6c2cb')}
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
  const emissive = polarity > 0 ? '#b8c9d6' : '#c98b4d'
  const baseColor = magnitude < 0.08 ? '#5c6970' : color

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
      <WindingArc radius={1.22} angle={0} color="#b8c9d6" current={state.ia} label="phaseA" onHover={onHover} target="phaseA" />
      <WindingArc radius={1.22} angle={(2 * Math.PI) / 3} color="#c98b4d" current={state.ib} label="phaseB" onHover={onHover} target="phaseB" />
      <WindingArc radius={1.22} angle={(4 * Math.PI) / 3} color="#9ba7b1" current={state.ic} label="phaseC" onHover={onHover} target="phaseC" />
      <mesh castShadow receiveShadow>
        <cylinderGeometry args={[1.52, 1.52, 2.85, 64, 1, true]} />
        <meshStandardMaterial color="#273138" metalness={0.22} roughness={0.74} transparent opacity={0.72} side={THREE.DoubleSide} />
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
      <PulseDot angle={0} radius={1.13} speed={1.8 + mag * 0.5} color={state.ia >= 0 ? '#b8c9d6' : '#c98b4d'} />
      <PulseDot angle={2} radius={1.13} speed={1.6 + mag * 0.4} color={state.ib >= 0 ? '#c98b4d' : '#b8c9d6'} />
      <PulseDot angle={4} radius={1.13} speed={1.7 + mag * 0.45} color={state.ic >= 0 ? '#9ba7b1' : '#c98b4d'} />
    </group>
  )
}

export function MagneticField({ state, onHover }: { state: MotorState; onHover: HoverCallback }) {
  const group = useRef<THREE.Group>(null)
  const strength = Math.min(1.2, 0.2 + Math.max(Math.abs(state.ia), Math.abs(state.ib), Math.abs(state.ic)) * 0.02)

  const statorAngleRef = useRef(0)
  const rotorAngleRef = useRef(0)
  
  // Magnitudes for arrows
  const statorMag = Math.sqrt(state.foc.Ialpha ** 2 + state.foc.Ibeta ** 2)
  const arrowLengthStator = 1.0 + (statorMag / 10) // Scale length by current
  const arrowLengthRotor = 2.0 // Permanent magnet flux is constant

  useFrame((_, delta) => {
    if (!group.current) return
    group.current.rotation.y = THREE.MathUtils.damp(group.current.rotation.y, state.electricalAngle, 7.5, delta)
    
    // Update vector angles
    const targetStatorAngle = Math.atan2(state.foc.Ibeta, state.foc.Ialpha)
    statorAngleRef.current = THREE.MathUtils.damp(statorAngleRef.current, targetStatorAngle, 10, delta)
    rotorAngleRef.current = THREE.MathUtils.damp(rotorAngleRef.current, state.electricalAngle, 10, delta)
  })

  // Reusable 3D arrow
  const Arrow = ({ color, length, angle }: { color: string, length: number, angle: number }) => (
    <group rotation={[0, -angle, 0]} position={[0, 0.5, 0]}>
      {/* shaft */}
      <mesh position={[length / 2, 0, 0]} rotation={[0, 0, Math.PI / 2]}>
        <cylinderGeometry args={[0.05, 0.05, length, 16]} />
        <meshStandardMaterial color={color} emissive={color} emissiveIntensity={0.5} transparent opacity={0.8} />
      </mesh>
      {/* head */}
      <mesh position={[length, 0, 0]} rotation={[0, 0, -Math.PI / 2]}>
        <coneGeometry args={[0.15, 0.3, 16]} />
        <meshStandardMaterial color={color} emissive={color} emissiveIntensity={0.5} transparent opacity={0.8} />
      </mesh>
    </group>
  )

  return (
    <group>
      {/* The original decorative field lines */}
      <group ref={group}>
        {Array.from({ length: 10 }, (_, index) => {
          const angle = (index / 10) * Math.PI * 2
          const x = Math.cos(angle) * 1.72
          const z = Math.sin(angle) * 1.72
          return (
            <mesh key={angle} position={[x, 0.02 * Math.sin(angle * 3), z]} rotation={[0, -angle, 0]} castShadow>
              <torusGeometry args={[0.38, 0.018, 10, 42, Math.PI * 0.95]} />
              <meshStandardMaterial
                color={index % 2 === 0 ? '#b8c9d6' : '#c98b4d'}
                emissive={index % 2 === 0 ? '#b8c9d6' : '#c98b4d'}
                emissiveIntensity={0.14 + strength * 0.6}
                transparent
                opacity={0.95}
              />
            </mesh>
          )
        })}
      </group>

      {/* NEW: FOC Magnetic Field Vectors (Stator=Red, Rotor=Blue) */}
      <Arrow color="#d32f2f" length={arrowLengthStator} angle={statorAngleRef.current} />
      <Arrow color="#1976d2" length={arrowLengthRotor} angle={rotorAngleRef.current} />

      <mesh
        onPointerOver={(event) => {
          event.stopPropagation()
          onHover('magneticField', [0, 1.2, 0])
        }}
        onPointerOut={() => onHover(null)}
      >
        <torusGeometry args={[1.78, 0.12, 18, 64]} />
        {hoverMaterial('#b6c2cb')}
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
    group.current.rotation.y = THREE.MathUtils.lerp(group.current.rotation.y, target, 5.5 * delta)
  })

  return (
    <group ref={group} position={[0, 1.72, 0]}>
      <mesh castShadow>
        <cylinderGeometry args={[0.045, 0.045, length, 12]} />
        <meshStandardMaterial color="#c98b4d" emissive="#c98b4d" emissiveIntensity={1.15} />
      </mesh>
      <mesh position={[0, length / 2 + 0.12, 0]} castShadow>
        <coneGeometry args={[0.14, 0.22, 12]} />
        <meshStandardMaterial color="#c98b4d" emissive="#c98b4d" emissiveIntensity={1.25} />
      </mesh>
      <Text position={[0, 0.2, 0]} fontSize={0.1} color="#e6e0d4" anchorX="center" anchorY="bottom">
        Torque
      </Text>
    </group>
  )
}

export function HallIndicators({ state, onHover }: { state: MotorState; onHover: HoverCallback }) {
  const leds = [
    { label: 'HA', active: state.hallState.a > 0, color: '#b8c9d6', pos: [-1.95, 1.1, 0.55] as [number, number, number] },
    { label: 'HB', active: state.hallState.b > 0, color: '#c98b4d', pos: [-1.95, 0.92, 0] as [number, number, number] },
    { label: 'HC', active: state.hallState.c > 0, color: '#9ba7b1', pos: [-1.95, 1.1, -0.55] as [number, number, number] },
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
        {hoverMaterial('#b6c2cb')}
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
        <div style={{ color: '#d7dde2', fontSize: 10, letterSpacing: '0.14em' }}>{led.label}</div>
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
        <meshStandardMaterial color="#12181d" metalness={0.75} roughness={0.24} />
      </mesh>
      {Array.from({ length: pulseCount }, (_, index) => {
        const angle = (index / pulseCount) * Math.PI * 2
        const x = Math.cos(angle) * 0.34
        const y = Math.sin(angle) * 0.34
        return (
          <mesh key={angle} position={[x, y, 0.09]} rotation={[0, 0, angle]} castShadow>
            <boxGeometry args={[0.025, 0.07, 0.01]} />
            <meshStandardMaterial
              color={index % 4 === 0 ? '#b8c9d6' : '#2b353b'}
              emissive="#b8c9d6"
              emissiveIntensity={index % 4 === 0 ? 0.9 : 0.08}
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
        {hoverMaterial('#b6c2cb')}
      </mesh>
      <Html position={[0.54, 0.22, 0]} center>
        <div style={{ color: '#d7dde2', fontSize: 10, letterSpacing: '0.14em' }}>
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
        <meshStandardMaterial color="#10161a" metalness={0.95} roughness={0.3} />
      </mesh>
      <mesh castShadow receiveShadow>
        <cylinderGeometry args={[1.6, 1.6, 2.82, 72]} />
        <meshStandardMaterial color="#273239" metalness={0.22} roughness={0.7} transparent opacity={0.88} side={THREE.DoubleSide} />
      </mesh>
      <mesh
        onPointerOver={(event) => {
          event.stopPropagation()
          onHover('stator', [0, 0.15, 0])
        }}
        onPointerOut={() => onHover(null)}
      >
        <cylinderGeometry args={[1.7, 1.7, 3.0, 72, 1, true]} />
        {hoverMaterial('#b6c2cb')}
      </mesh>
      <mesh
        onPointerOver={(event) => {
          event.stopPropagation()
          onHover('housing', [0, 1.5, 0])
        }}
        onPointerOut={() => onHover(null)}
      >
        <cylinderGeometry args={[1.96, 1.96, 3.28, 72, 1, true]} />
        {hoverMaterial('#b1b8bd')}
      </mesh>
      <Text position={[0, 2.22, 0]} fontSize={0.13} color="#d5dde2" anchorX="center" anchorY="middle">
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
  const cold = new THREE.Color('#7f8f9a')
  const warm = new THREE.Color('#c58b53')
  const hot = new THREE.Color('#ff6a4d')
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
    { label: 'AH', key: 'ah' as const, color: '#b8c9d6' },
    { label: 'AL', key: 'al' as const, color: '#87939c' },
    { label: 'BH', key: 'bh' as const, color: '#c98b4d' },
    { label: 'BL', key: 'bl' as const, color: '#87939c' },
    { label: 'CH', key: 'ch' as const, color: '#9ba7b1' },
    { label: 'CL', key: 'cl' as const, color: '#87939c' },
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
                color={intensity > 0.5 ? item.color : '#2a3339'}
                emissive={item.color}
                emissiveIntensity={intensity * 2.0}
                metalness={0.15}
                roughness={0.24}
              />
            </mesh>
            <Html position={[0.16, 0, 0]} center>
        <div style={{ color: '#d8dee4', fontSize: 10, letterSpacing: '0.16em' }}>{item.label}</div>
            </Html>
          </group>
        )
      })}
    </group>
  )
}

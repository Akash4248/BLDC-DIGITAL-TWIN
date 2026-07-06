import { useState } from 'react'
import type { MotorState } from '../../app/types'
import type { HoverTarget } from './types'
import type { MotorViewMode } from './modelMapper'
import {
  Bearing,
  CurrentFlow,
  Encoder,
  HallIndicators,
  MagneticField,
  MotorLabels,
  Rotor,
  Shaft,
  TorqueArrow,
  TemperatureShell,
  PwmIndicators,
  Windings,
} from './parts'
import { MotorGlbAssembly } from './MotorGlbAssembly'

type HoverState = {
  target: HoverTarget | null
  position: [number, number, number]
}

const labelPositions: Record<HoverTarget, [number, number, number]> = {
  housing: [0, 2.1, 0],
  stator: [0, 1.45, 0],
  rotor: [0, 0.35, 0],
  shaft: [0.35, -0.05, 0.6],
  bearing: [1.1, -1.15, 0],
  phaseA: [1.6, 0.9, 0.35],
  phaseB: [1.55, 0.9, 0],
  phaseC: [1.6, 0.9, -0.35],
  hall: [-1.6, 1.15, 0],
  encoder: [0.7, 0.45, 1.6],
  magneticField: [0, 1.65, 0],
}

export function MotorModel({ state, mode }: { state: MotorState; mode: MotorViewMode }) {
  const [hover, setHover] = useState<HoverState>({ target: null, position: [0, 0, 0] })
  const sectionClipping = mode === 'cutaway'

  const handleHover = (target: HoverTarget | null, position?: [number, number, number]) => {
    if (!target) {
      setHover({ target: null, position: [0, 0, 0] })
      return
    }
    setHover({ target, position: position ?? labelPositions[target] })
  }

  return (
    <group position={[0, -0.1, 0]} scale={1.08}>
      <MotorGlbAssembly state={state} mode={mode} sectionClipping={sectionClipping} />
      <TemperatureShell temperature={state.temperature} mode={mode} />
      <Rotor state={state} onHover={handleHover} mode={mode} />
      <Shaft state={state} onHover={handleHover} mode={mode} />
      <Bearing position={[0, -1.58, 0]} state={state} onHover={handleHover} mode={mode} />
      <Bearing position={[0, 1.58, 0]} state={state} onHover={handleHover} mode={mode} />
      <Windings state={state} onHover={handleHover} mode={mode} />
      <CurrentFlow state={state} />
      <MagneticField state={state} onHover={handleHover} />
      <TorqueArrow state={state} />
      <HallIndicators state={state} onHover={handleHover} />
      <Encoder state={state} onHover={handleHover} />
      <PwmIndicators pwm={state.pwm} />
      <MotorLabels active={hover.target} position={hover.position} />
    </group>
  )
}

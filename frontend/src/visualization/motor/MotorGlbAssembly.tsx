import { useEffect, useMemo, useRef } from 'react'
import * as THREE from 'three'
import { useFrame } from '@react-three/fiber'
import { useGLTF } from '@react-three/drei'
import * as SkeletonUtils from 'three/examples/jsm/utils/SkeletonUtils.js'
import type { MotorState } from '../../app/types'
import { analyzeMotorModel, roleOffset, type MotorMeshDescriptor, type MotorViewMode } from './modelMapper'

const MOTOR_MODEL_URL = '/models/dji_phantom_4_bldc_motor.glb'

type MotorGlbAssemblyProps = {
  state: MotorState
  mode: MotorViewMode
  sectionClipping: boolean
  onHover: (target: HoverTarget | null, position?: [number, number, number]) => void
}

type PaletteEntry = {
  material: THREE.Material
  opacity: number
  clipping: boolean
}

function temperatureBlend(temperature: number) {
  const normalized = THREE.MathUtils.clamp((temperature - 35) / 80, 0, 1)
  const cold = new THREE.Color('#7f8f9a')
  const warm = new THREE.Color('#c58b53')
  const hot = new THREE.Color('#ff6a4d')
  if (normalized < 0.52) {
    return cold.clone().lerp(warm, normalized / 0.52)
  }
  return warm.clone().lerp(hot, (normalized - 0.52) / 0.48)
}

function makeMaterial(role: string, temperature: number): THREE.MeshStandardMaterial | THREE.MeshPhysicalMaterial {
  const tempColor = temperatureBlend(temperature)
  switch (role) {
    case 'housing':
      return new THREE.MeshPhysicalMaterial({
        color: new THREE.Color('#d3d8dd'),
        metalness: 0.98,
        roughness: 0.16,
        clearcoat: 0.55,
        clearcoatRoughness: 0.2,
        reflectivity: 0.82,
        emissive: tempColor.clone().multiplyScalar(0.05),
      })
    case 'stator':
      return new THREE.MeshPhysicalMaterial({
        color: new THREE.Color('#46515a'),
        metalness: 0.52,
        roughness: 0.54,
        clearcoat: 0.08,
        emissive: tempColor.clone().multiplyScalar(0.08),
      })
    case 'rotor':
      return new THREE.MeshPhysicalMaterial({
        color: new THREE.Color('#2c343a'),
        metalness: 0.98,
        roughness: 0.14,
        clearcoat: 0.28,
        emissive: tempColor.clone().multiplyScalar(0.05),
      })
    case 'shaft':
      return new THREE.MeshPhysicalMaterial({
        color: new THREE.Color('#edf2f5'),
        metalness: 1,
        roughness: 0.12,
        reflectivity: 0.76,
      })
    case 'bearings':
      return new THREE.MeshPhysicalMaterial({
        color: new THREE.Color('#bcc3c8'),
        metalness: 0.94,
        roughness: 0.16,
      })
    case 'windings':
      return new THREE.MeshPhysicalMaterial({
        color: new THREE.Color('#c98b4d'),
        metalness: 0.86,
        roughness: 0.24,
        emissive: tempColor.clone().multiplyScalar(0.16),
      })
    case 'endcap':
      return new THREE.MeshPhysicalMaterial({
        color: new THREE.Color('#d8dce0'),
        metalness: 0.88,
        roughness: 0.24,
      })
    default:
      return new THREE.MeshStandardMaterial({
        color: new THREE.Color('#515b62'),
        metalness: 0.5,
        roughness: 0.5,
      })
  }
}

function assignRoleMaterial(descriptor: MotorMeshDescriptor, temperature: number, mode: MotorViewMode, sectionClipping: boolean, plane: THREE.Plane | null): PaletteEntry {
  const material = makeMaterial(descriptor.role, temperature)
  const transparentHousing = mode === 'transparent' && (descriptor.role === 'housing' || descriptor.role === 'endcap')
  const cutawayHousing = mode === 'cutaway' && (descriptor.role === 'housing' || descriptor.role === 'endcap')
  const opacity = transparentHousing ? 0.22 : cutawayHousing ? 0.42 : 1
  material.transparent = opacity < 1
  material.opacity = opacity
  material.depthWrite = opacity >= 1
  material.side = descriptor.role === 'housing' || descriptor.role === 'stator' ? THREE.DoubleSide : THREE.FrontSide
  material.clippingPlanes = sectionClipping && plane ? [plane] : null
  material.clipShadows = sectionClipping
  ;(material as THREE.MeshStandardMaterial & { skinning?: boolean }).skinning = true
  material.needsUpdate = true
  return { material, opacity, clipping: sectionClipping }
}

export function MotorGlbAssembly({ state, mode, sectionClipping, onHover }: MotorGlbAssemblyProps) {
  const { scene } = useGLTF(MOTOR_MODEL_URL) as { scene: THREE.Group }
  const clonedScene = useMemo(() => SkeletonUtils.clone(scene) as THREE.Group, [scene])
  const mapping = useMemo(() => analyzeMotorModel(clonedScene), [clonedScene])
  const descriptorsRef = useRef<MotorMeshDescriptor[]>(mapping.descriptors)
  const planeRef = useRef(new THREE.Plane(new THREE.Vector3(1, 0, 0), 0.08))
  const centeredRef = useRef(false)
  const fitScale = useMemo(() => {
    const maxDimension = Math.max(mapping.size.x, mapping.size.y, mapping.size.z)
    if (!Number.isFinite(maxDimension) || maxDimension <= 0) {
      return 1
    }
    return THREE.MathUtils.clamp(4.0 / maxDimension, 0.03, 3.5)
  }, [mapping.size.x, mapping.size.y, mapping.size.z])

  useEffect(() => {
    descriptorsRef.current = mapping.descriptors
    if (!centeredRef.current) {
      clonedScene.position.set(-mapping.center.x, -mapping.center.y, -mapping.center.z)
      clonedScene.updateMatrixWorld(true)
      centeredRef.current = true
    }

    if (mode === 'cutaway') {
      planeRef.current.setFromNormalAndCoplanarPoint(new THREE.Vector3(1, 0, 0), new THREE.Vector3(0.07, 0, 0))
    } else {
      planeRef.current.setFromNormalAndCoplanarPoint(new THREE.Vector3(1, 0, 0), new THREE.Vector3(999, 0, 0))
    }

    for (const descriptor of mapping.descriptors) {
      const palette = assignRoleMaterial(descriptor, state.temperature, mode, sectionClipping, mode === 'cutaway' ? planeRef.current : null)
      descriptor.mesh.material = palette.material
      descriptor.mesh.castShadow = true
      descriptor.mesh.receiveShadow = true
      descriptor.mesh.frustumCulled = false
      descriptor.mesh.userData.motorRole = descriptor.role
      descriptor.mesh.userData.basePosition = descriptor.basePosition.clone()
      descriptor.mesh.userData.baseRotation = descriptor.baseRotation.clone()
      descriptor.mesh.userData.baseScale = descriptor.baseScale.clone()

      // Assign interactive pointer events directly to standard Three.js meshes
      descriptor.mesh.onPointerOver = (event: any) => {
        event.stopPropagation()
        let targetRole: HoverTarget | null = null
        if (descriptor.role === 'bearings') targetRole = 'bearing'
        else if (descriptor.role === 'windings') targetRole = 'phaseA' // map to winding phase
        else if (descriptor.role === 'endcap') targetRole = 'housing'
        else if (descriptor.role === 'misc') targetRole = 'housing'
        else targetRole = descriptor.role as HoverTarget

        if (targetRole) {
          onHover(targetRole, [event.point.x, event.point.y, event.point.z])
        }
      }
      descriptor.mesh.onPointerOut = (event: any) => {
        onHover(null)
      }
    }
  }, [mapping.descriptors, mode, sectionClipping, state.temperature, onHover])

  useFrame((_, delta) => {
    const smoothing = 1 - Math.exp(-delta * 8)
    const axis = mapping.dominantAxis

    for (const descriptor of descriptorsRef.current) {
      const offset = roleOffset(descriptor.role, mode, axis)
      const targetPosition = descriptor.basePosition.clone().add(offset)
      descriptor.mesh.position.lerp(targetPosition, smoothing)
      descriptor.mesh.scale.lerp(descriptor.baseScale, smoothing)

      if (descriptor.role === 'rotor' || descriptor.role === 'shaft') {
        // Rotate the rotor and shaft meshes in standard, exploded, and cutaway modes
        // based on the actual physical rotorAngle
        if (axis === 'y') {
          descriptor.mesh.rotation.y = THREE.MathUtils.damp(descriptor.mesh.rotation.y, state.rotorAngle, 9.5, delta)
        } else if (axis === 'z') {
          descriptor.mesh.rotation.z = THREE.MathUtils.damp(descriptor.mesh.rotation.z, state.rotorAngle, 9.5, delta)
        } else {
          descriptor.mesh.rotation.x = THREE.MathUtils.damp(descriptor.mesh.rotation.x, state.rotorAngle, 9.5, delta)
        }
      } else {
        descriptor.mesh.rotation.x = THREE.MathUtils.lerp(descriptor.mesh.rotation.x, descriptor.baseRotation.x, smoothing)
        descriptor.mesh.rotation.y = THREE.MathUtils.lerp(descriptor.mesh.rotation.y, descriptor.baseRotation.y, smoothing)
        descriptor.mesh.rotation.z = THREE.MathUtils.lerp(descriptor.mesh.rotation.z, descriptor.baseRotation.z, smoothing)
      }
    }
  })

  return (
    <group scale={fitScale} position={[0, 0.12, 0]}>
      <primitive object={clonedScene} dispose={null} />
    </group>
  )
}

useGLTF.preload(MOTOR_MODEL_URL)

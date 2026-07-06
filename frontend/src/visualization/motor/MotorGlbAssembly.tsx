import { useEffect, useMemo, useRef } from 'react'
import * as THREE from 'three'
import { useFrame } from '@react-three/fiber'
import { Center, useGLTF } from '@react-three/drei'
import type { MotorState } from '../../app/types'
import { analyzeMotorModel, roleOffset, type MotorMeshDescriptor, type MotorViewMode } from './modelMapper'

const MOTOR_MODEL_URL = '/models/dji_phantom_4_bldc_motor.glb'

type MotorGlbAssemblyProps = {
  state: MotorState
  mode: MotorViewMode
  sectionClipping: boolean
}

type PaletteEntry = {
  material: THREE.Material
  opacity: number
  clipping: boolean
}

function temperatureBlend(temperature: number) {
  const normalized = THREE.MathUtils.clamp((temperature - 35) / 80, 0, 1)
  const cold = new THREE.Color('#1a86ff')
  const warm = new THREE.Color('#f4a04a')
  const hot = new THREE.Color('#ff5447')
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
        color: new THREE.Color('#9aa7b3'),
        metalness: 0.92,
        roughness: 0.22,
        clearcoat: 0.4,
        clearcoatRoughness: 0.28,
        reflectivity: 0.65,
        emissive: tempColor.clone().multiplyScalar(0.08),
      })
    case 'stator':
      return new THREE.MeshPhysicalMaterial({
        color: new THREE.Color('#1a252c'),
        metalness: 0.58,
        roughness: 0.58,
        clearcoat: 0.08,
        emissive: tempColor.clone().multiplyScalar(0.05),
      })
    case 'rotor':
      return new THREE.MeshPhysicalMaterial({
        color: new THREE.Color('#101820'),
        metalness: 0.96,
        roughness: 0.16,
        clearcoat: 0.28,
        emissive: tempColor.clone().multiplyScalar(0.025),
      })
    case 'shaft':
      return new THREE.MeshPhysicalMaterial({
        color: new THREE.Color('#b8c3cb'),
        metalness: 1,
        roughness: 0.12,
        reflectivity: 0.7,
      })
    case 'bearings':
      return new THREE.MeshPhysicalMaterial({
        color: new THREE.Color('#7d8991'),
        metalness: 0.94,
        roughness: 0.16,
      })
    case 'windings':
      return new THREE.MeshPhysicalMaterial({
        color: new THREE.Color('#c97b31'),
        metalness: 0.82,
        roughness: 0.26,
        emissive: tempColor.clone().multiplyScalar(0.12),
      })
    case 'endcap':
      return new THREE.MeshPhysicalMaterial({
        color: new THREE.Color('#a4b0b9'),
        metalness: 0.88,
        roughness: 0.24,
      })
    default:
      return new THREE.MeshStandardMaterial({
        color: new THREE.Color('#31414a'),
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
  material.needsUpdate = true
  return { material, opacity, clipping: sectionClipping }
}

export function MotorGlbAssembly({ state, mode, sectionClipping }: MotorGlbAssemblyProps) {
  const { scene } = useGLTF(MOTOR_MODEL_URL) as { scene: THREE.Group }
  const clonedScene = useMemo(() => scene.clone(true), [scene])
  const mapping = useMemo(() => analyzeMotorModel(clonedScene), [clonedScene])
  const descriptorsRef = useRef<MotorMeshDescriptor[]>(mapping.descriptors)
  const planeRef = useRef(new THREE.Plane(new THREE.Vector3(1, 0, 0), 0.08))
  const fitScale = useMemo(() => {
    const maxDimension = Math.max(mapping.size.x, mapping.size.y, mapping.size.z)
    if (!Number.isFinite(maxDimension) || maxDimension <= 0) {
      return 1
    }
    return THREE.MathUtils.clamp(3.2 / maxDimension, 0.02, 3)
  }, [mapping.size.x, mapping.size.y, mapping.size.z])

  useEffect(() => {
    descriptorsRef.current = mapping.descriptors

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
    }
  }, [mapping.descriptors, mode, sectionClipping, state.temperature])

  useFrame((_, delta) => {
    const smoothing = 1 - Math.exp(-delta * 8)
    const axis = mapping.dominantAxis

    for (const descriptor of descriptorsRef.current) {
      const offset = roleOffset(descriptor.role, mode, axis)
      const targetPosition = descriptor.basePosition.clone().add(offset)
      descriptor.mesh.position.lerp(targetPosition, smoothing)
      descriptor.mesh.scale.lerp(descriptor.baseScale, smoothing)
      descriptor.mesh.rotation.x = THREE.MathUtils.lerp(descriptor.mesh.rotation.x, descriptor.baseRotation.x, smoothing)
      descriptor.mesh.rotation.y = THREE.MathUtils.lerp(descriptor.mesh.rotation.y, descriptor.baseRotation.y, smoothing)
      descriptor.mesh.rotation.z = THREE.MathUtils.lerp(descriptor.mesh.rotation.z, descriptor.baseRotation.z, smoothing)
    }
  })

  return (
    <group scale={fitScale} position={[0, 0, 0]}>
      <Center top>
        <primitive object={clonedScene} dispose={null} />
      </Center>
    </group>
  )
}

useGLTF.preload(MOTOR_MODEL_URL)

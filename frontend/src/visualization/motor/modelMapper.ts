import * as THREE from 'three'

export type MotorViewMode = 'standard' | 'exploded' | 'cutaway' | 'transparent'

export type MotorMeshRole =
  | 'housing'
  | 'stator'
  | 'rotor'
  | 'shaft'
  | 'bearings'
  | 'windings'
  | 'endcap'
  | 'misc'

export type MotorMeshDescriptor = {
  mesh: THREE.Mesh
  role: MotorMeshRole
  sourceName: string
  parentName: string
  center: THREE.Vector3
  size: THREE.Vector3
  basePosition: THREE.Vector3
  baseRotation: THREE.Euler
  baseScale: THREE.Vector3
}

export type MotorModelMapping = {
  descriptors: MotorMeshDescriptor[]
  dominantAxis: 'x' | 'y' | 'z'
  center: THREE.Vector3
  size: THREE.Vector3
}

const ROLE_KEYWORDS: Record<MotorMeshRole, RegExp[]> = {
  housing: [/housing/i, /body/i, /shell/i, /case/i, /cover/i, /closing/i, /end/i],
  stator: [/stator/i, /lamination/i, /core/i, /body/i],
  rotor: [/rotor/i, /magnet/i, /inner/i, /parts?/i],
  shaft: [/shaft/i, /axle/i, /spindle/i, /hub/i],
  bearings: [/bearing/i, /ring/i, /seal/i, /race/i],
  windings: [/winding/i, /coil/i, /wire/i, /copper/i, /phase/i, /parts?/i],
  endcap: [/cap/i, /closing/i, /cover/i, /plate/i, /end/i],
  misc: [],
}

const ROLE_PRIORITY: MotorMeshRole[] = ['housing', 'stator', 'rotor', 'shaft', 'bearings', 'windings', 'endcap', 'misc']

function classifyByName(name: string, parentName: string): MotorMeshRole | null {
  const haystack = `${name} ${parentName}`
  for (const role of ROLE_PRIORITY) {
    if (role === 'misc') continue
    if (ROLE_KEYWORDS[role].some((pattern) => pattern.test(haystack))) {
      return role
    }
  }
  return null
}

function dominantAxis(size: THREE.Vector3): 'x' | 'y' | 'z' {
  const values: Array<['x' | 'y' | 'z', number]> = [
    ['x', size.x],
    ['y', size.y],
    ['z', size.z],
  ]
  values.sort((a, b) => b[1] - a[1])
  return values[0][0]
}

function heuristicRole(name: string, size: THREE.Vector3, center: THREE.Vector3, modelSize: THREE.Vector3): MotorMeshRole {
  const lower = name.toLowerCase()
  const axial = dominantAxis(size)
  const slender = size[axial] / Math.max(size.x, size.y, size.z) > 0.86
  const nearCenter = center.length() < Math.max(modelSize.x, modelSize.y, modelSize.z) * 0.08
  const isOuterShell = size.x > modelSize.x * 0.75 || size.y > modelSize.y * 0.75 || size.z > modelSize.z * 0.75

  if (lower.includes('part')) {
    return 'rotor'
  }
  if (lower.includes('body')) {
    return 'stator'
  }
  if (lower.includes('closing')) {
    return 'housing'
  }
  if (slender && nearCenter) {
    return 'shaft'
  }
  if (isOuterShell) {
    return 'housing'
  }
  if (center.length() > Math.max(modelSize.x, modelSize.y, modelSize.z) * 0.22 && size.y < modelSize.y * 0.55) {
    return 'windings'
  }
  return 'misc'
}

export function analyzeMotorModel(scene: THREE.Object3D): MotorModelMapping {
  const box = new THREE.Box3().setFromObject(scene)
  const center = new THREE.Vector3()
  const size = new THREE.Vector3()
  box.getCenter(center)
  box.getSize(size)

  const descriptors: MotorMeshDescriptor[] = []

  scene.traverse((object) => {
    if (!object.isMesh) return
    const mesh = object as THREE.Mesh
    const meshBox = new THREE.Box3().setFromObject(mesh)
    const meshCenter = new THREE.Vector3()
    const meshSize = new THREE.Vector3()
    meshBox.getCenter(meshCenter)
    meshBox.getSize(meshSize)
    const sourceName = mesh.name || mesh.parent?.name || 'mesh'
    const parentName = mesh.parent?.name || ''
    const byName = classifyByName(sourceName, parentName)
    const role = byName ?? heuristicRole(sourceName, meshSize, meshCenter, size)

    descriptors.push({
      mesh,
      role,
      sourceName,
      parentName,
      center: meshCenter,
      size: meshSize,
      basePosition: mesh.position.clone(),
      baseRotation: mesh.rotation.clone(),
      baseScale: mesh.scale.clone(),
    })
  })

  const axis = dominantAxis(size)
  return { descriptors, dominantAxis: axis, center, size }
}

export function roleOffset(role: MotorMeshRole, mode: MotorViewMode, axis: 'x' | 'y' | 'z'): THREE.Vector3 {
  const axisVector = new THREE.Vector3()
  axisVector[axis] = 1
  const opposite = axisVector.clone().multiplyScalar(-1)

  if (mode === 'standard') return new THREE.Vector3()
  if (mode === 'transparent') {
    if (role === 'housing' || role === 'endcap') return new THREE.Vector3()
    return new THREE.Vector3()
  }
  if (mode === 'cutaway') {
    if (role === 'housing' || role === 'endcap') return axisVector.clone().multiplyScalar(0.08)
    if (role === 'stator') return axisVector.clone().multiplyScalar(0.04)
    return new THREE.Vector3()
  }
  const explodeMap: Record<MotorMeshRole, number> = {
    housing: 0.12,
    stator: 0.03,
    rotor: 0.26,
    shaft: 0.42,
    bearings: 0.24,
    windings: 0.08,
    endcap: 0.16,
    misc: 0.06,
  }
  return axis === 'y'
    ? axisVector.clone().multiplyScalar(explodeMap[role])
    : opposite.clone().multiplyScalar(0).add(axisVector.clone().multiplyScalar(explodeMap[role]))
}


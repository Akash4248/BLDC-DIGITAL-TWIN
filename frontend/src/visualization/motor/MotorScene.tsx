import { Suspense, useRef, useState } from 'react'
import { Box, Button, Stack, ToggleButton, ToggleButtonGroup, Typography } from '@mui/material'
import FullscreenIcon from '@mui/icons-material/Fullscreen'
import RestartAltIcon from '@mui/icons-material/RestartAlt'
import { Canvas } from '@react-three/fiber'
import { ContactShadows, Environment } from '@react-three/drei'
import * as THREE from 'three'
import type { MotorState } from '../../app/types'
import { ErrorBoundary } from './ErrorBoundary'
import { LoadingOverlay } from './LoadingOverlay'
import { MotorFallback } from './MotorFallback'
import { CameraController, Ground, Lighting } from './parts'
import { MotorModel } from './MotorModel'

type MotorSceneProps = {
  snapshot: MotorState
}

export function MotorScene({ snapshot }: MotorSceneProps) {
  const containerRef = useRef<HTMLDivElement>(null)
  const controlsRef = useRef<any>(null)
  const [mode, setMode] = useState<'standard' | 'exploded' | 'cutaway' | 'transparent'>('standard')

  function resetView() {
    const controls = controlsRef.current
    if (!controls) return
    controls.reset?.()
    controls.target?.set?.(0, 0, 0)
    controls.update?.()
  }

  async function toggleFullscreen() {
    const element = containerRef.current
    if (!element) return
    if (document.fullscreenElement) {
      await document.exitFullscreen()
      return
    }
    await element.requestFullscreen?.()
  }

  return (
    <Box
      ref={containerRef}
      sx={{
        position: 'relative',
        width: '100%',
        height: '100%',
        borderRadius: 4,
        overflow: 'hidden',
        border: '1px solid rgba(201,139,77,0.12)',
        background:
          'radial-gradient(circle at 50% 28%, rgba(201,139,77,0.10), transparent 35%), linear-gradient(180deg, rgba(5,9,12,0.96), rgba(7,10,13,0.99))',
      }}
      onDoubleClick={resetView}
    >
      <ErrorBoundary fallback={<MotorFallback />}>
        <Canvas
          shadows
          dpr={[1, 1.6]}
          gl={{ antialias: true, alpha: true, powerPreference: 'high-performance' }}
          camera={{ position: [4.8, 2.8, 4.8], fov: 35, near: 0.1, far: 100 }}
          onCreated={({ gl, scene }) => {
            gl.toneMapping = THREE.ACESFilmicToneMapping
            gl.toneMappingExposure = 1.35
            gl.shadowMap.enabled = true
            scene.fog = new THREE.Fog('#081015', 8, 22)
          }}
        >
          <Lighting />
          <CameraController controlsRef={controlsRef} />
          <Suspense fallback={<LoadingOverlay />}>
            <Environment preset="warehouse" />
            <Ground />
            <MotorModel state={snapshot} mode={mode} />
            <ContactShadows opacity={0.34} scale={10} blur={2.4} far={4.8} resolution={1024} color="#0a0f13" />
          </Suspense>
        </Canvas>
      </ErrorBoundary>

      <Stack
        direction="row"
        spacing={1}
        sx={{
          position: 'absolute',
          top: 12,
          left: 12,
          zIndex: 2,
          flexWrap: 'wrap',
        }}
      >
        <ToggleButtonGroup
          exclusive
          size="small"
          value={mode}
          onChange={(_, next) => next && setMode(next)}
          sx={{
            bgcolor: 'rgba(5,10,14,0.7)',
            backdropFilter: 'blur(12px)',
            borderRadius: 2,
            '& .MuiToggleButton-root': {
              color: '#b7c1c8',
              borderColor: 'rgba(201,139,77,0.12)',
            },
            '& .Mui-selected': {
              color: '#f0ece6 !important',
              bgcolor: 'rgba(201,139,77,0.12) !important',
            },
          }}
        >
          <ToggleButton value="standard">Standard</ToggleButton>
          <ToggleButton value="exploded">Exploded</ToggleButton>
          <ToggleButton value="cutaway">Cutaway</ToggleButton>
          <ToggleButton value="transparent">Transparent</ToggleButton>
        </ToggleButtonGroup>
        <Button
          size="small"
          variant="outlined"
          startIcon={<RestartAltIcon />}
          onClick={resetView}
          sx={{
            pointerEvents: 'auto',
            bgcolor: 'rgba(5,10,14,0.7)',
            backdropFilter: 'blur(12px)',
          }}
        >
          Reset
        </Button>
        <Button
          size="small"
          variant="outlined"
          startIcon={<FullscreenIcon />}
          onClick={toggleFullscreen}
          sx={{
            pointerEvents: 'auto',
            bgcolor: 'rgba(5,10,14,0.7)',
            backdropFilter: 'blur(12px)',
          }}
          >
          Fullscreen
        </Button>
      </Stack>

      <Box
        sx={{
          position: 'absolute',
          left: 14,
          bottom: 14,
          zIndex: 2,
          pointerEvents: 'none',
          maxWidth: 420,
          borderRadius: 3,
          px: 1.5,
          py: 1,
          background: 'rgba(7,16,22,0.72)',
          border: '1px solid rgba(201,139,77,0.10)',
          backdropFilter: 'blur(12px)',
        }}
      >
        <Typography variant="caption" color="text.secondary" sx={{ letterSpacing: '0.18em' }}>
          interactive digital twin
        </Typography>
        <Typography variant="body2" sx={{ mt: 0.5 }}>
          Hover motor components for labels. Double click to refocus the viewer.
        </Typography>
      </Box>
    </Box>
  )
}

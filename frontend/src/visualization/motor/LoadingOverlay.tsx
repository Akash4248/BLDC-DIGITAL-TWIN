import { Html, useProgress } from '@react-three/drei'

export function LoadingOverlay() {
  const { progress, loaded, total } = useProgress()

  return (
    <Html center>
      <div
        style={{
          width: 280,
          padding: '18px 20px',
          borderRadius: 16,
          border: '1px solid rgba(110,231,249,0.18)',
          background: 'rgba(7,16,22,0.9)',
          boxShadow: '0 18px 60px rgba(0,0,0,0.45)',
          color: '#E6F4FA',
          fontFamily: 'Inter, system-ui, sans-serif',
        }}
      >
        <div style={{ fontSize: 12, letterSpacing: '0.18em', textTransform: 'uppercase', color: '#8CA5B3' }}>
          loading motor asset
        </div>
        <div style={{ marginTop: 10, fontSize: 18, fontWeight: 700 }}>{Math.round(progress)}%</div>
        <div style={{ marginTop: 8, height: 7, borderRadius: 999, background: 'rgba(255,255,255,0.06)', overflow: 'hidden' }}>
          <div
            style={{
              height: '100%',
              width: `${progress}%`,
              background: 'linear-gradient(90deg, #6EE7F9, #8B5CF6)',
              boxShadow: '0 0 18px rgba(110,231,249,0.45)',
            }}
          />
        </div>
        <div style={{ marginTop: 8, fontSize: 11, color: '#8CA5B3' }}>
          {loaded}/{total || 1} resources
        </div>
      </div>
    </Html>
  )
}


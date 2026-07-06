import React from 'react';
import { Box, Typography } from '@mui/material';
import type { FocState } from '../app/focMath';

export const SvpwmHexagon: React.FC<{ foc: FocState }> = ({ foc }) => {
  const cx = 150;
  const cy = 150;
  const radius = 100;

  // The 6 base vectors for SVPWM in alpha-beta
  // V1 (100) -> 0 deg
  // V2 (110) -> 60 deg
  // V3 (010) -> 120 deg
  // V4 (011) -> 180 deg
  // V5 (001) -> 240 deg
  // V6 (101) -> 300 deg
  const angles = [0, 60, 120, 180, 240, 300].map(a => (a * Math.PI) / 180);
  
  const points = angles.map(a => {
    return {
      x: cx + radius * Math.cos(a),
      y: cy - radius * Math.sin(a),
    };
  });

  const polygonPoints = points.map(p => `${p.x},${p.y}`).join(' ');

  // Voltage vector in the hexagon (scaled up for visibility)
  const vScale = 3;
  const vX = cx + foc.Valpha * vScale;
  const vY = cy - foc.Vbeta * vScale;

  // Find the active sector (1 to 6)
  const sectorIndex = foc.sector - 1; // 0 to 5
  const nextSectorIndex = (sectorIndex + 1) % 6;

  // Active sector triangle points
  const activeTriangle = `150,150 ${points[sectorIndex]?.x},${points[sectorIndex]?.y} ${points[nextSectorIndex]?.x},${points[nextSectorIndex]?.y}`;

  return (
    <Box sx={{ width: '100%', height: 300, position: 'relative' }}>
      <Typography variant="subtitle2" sx={{ position: 'absolute', top: 10, left: 10, color: 'text.secondary' }}>
        SVPWM Hexagon (Sector {foc.sector})
      </Typography>
      <svg width="100%" height="100%" viewBox="0 0 300 300">
        {/* Hexagon Background */}
        <polygon points={polygonPoints} fill="none" stroke="#4b5257" strokeWidth="2" />
        
        {/* Sector Lines */}
        {points.map((p, i) => (
          <line key={i} x1={cx} y1={cy} x2={p.x} y2={p.y} stroke="#4b5257" strokeWidth="1" strokeDasharray="4 4" />
        ))}

        {/* Active Sector Highlight */}
        <polygon points={activeTriangle} fill="rgba(201,139,77,0.15)" stroke="none" />

        {/* Base Vectors Labels */}
        <text x={points[0].x + 10} y={points[0].y + 5} fill="#4b5257" fontSize="10">V1 (100)</text>
        <text x={points[1].x + 5} y={points[1].y - 5} fill="#4b5257" fontSize="10">V2 (110)</text>
        <text x={points[2].x - 45} y={points[2].y - 5} fill="#4b5257" fontSize="10">V3 (010)</text>
        <text x={points[3].x - 45} y={points[3].y + 5} fill="#4b5257" fontSize="10">V4 (011)</text>
        <text x={points[4].x - 45} y={points[4].y + 15} fill="#4b5257" fontSize="10">V5 (001)</text>
        <text x={points[5].x + 5} y={points[5].y + 15} fill="#4b5257" fontSize="10">V6 (101)</text>

        {/* Current Voltage Vector */}
        <line x1={cx} y1={cy} x2={vX} y2={vY} stroke="#c98b4d" strokeWidth="3" />
        <circle cx={vX} cy={vY} r="4" fill="#c98b4d" />
      </svg>
    </Box>
  );
};

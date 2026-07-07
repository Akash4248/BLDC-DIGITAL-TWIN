import React from 'react';
import { Box, Typography, Stack, Chip } from '@mui/material';
import type { FocState } from '../app/focMath';
import type { TelemetrySnapshot } from '../app/types';

export const ClarkeTransform: React.FC<{ foc: FocState }> = ({ foc }) => {
  // Map vector from math coordinates to SVG coordinates
  // center is at (150, 150)
  // scale factor: e.g. 100 pixels = 10 Amps
  const scale = 10;
  const cx = 150;
  const cy = 150;
  
  const vX = cx + foc.Ialpha * scale;
  const vY = cy - foc.Ibeta * scale; // SVG y axis is inverted

  return (
    <Box sx={{ width: '100%', height: 300, position: 'relative' }}>
      <Typography variant="subtitle2" sx={{ position: 'absolute', top: 10, left: 10, color: 'text.secondary' }}>
        Clarke Transform (α-β)
      </Typography>
      <svg width="100%" height="100%" viewBox="0 0 300 300">
        {/* Axes */}
        <line x1="150" y1="20" x2="150" y2="280" stroke="#4b5257" strokeWidth="1" strokeDasharray="4 4" />
        <line x1="20" y1="150" x2="280" y2="150" stroke="#4b5257" strokeWidth="1" strokeDasharray="4 4" />
        <text x="285" y="154" fill="#4b5257" fontSize="12">α</text>
        <text x="146" y="15" fill="#4b5257" fontSize="12">β</text>

        {/* Vector */}
        <line x1={cx} y1={cy} x2={vX} y2={vY} stroke="#c98b4d" strokeWidth="3" />
        <circle cx={vX} cy={vY} r="4" fill="#c98b4d" />
        
        {/* Vector Components */}
        <line x1={vX} y1={cy} x2={vX} y2={vY} stroke="rgba(201,139,77,0.4)" strokeWidth="1" strokeDasharray="2 2" />
        <line x1={cx} y1={vY} x2={vX} y2={vY} stroke="rgba(201,139,77,0.4)" strokeWidth="1" strokeDasharray="2 2" />
      </svg>
    </Box>
  );
};

export const ParkTransform: React.FC<{ foc: FocState }> = ({ foc }) => {
  const scale = 10;
  const cx = 150;
  const cy = 150;
  
  const vX = cx + foc.Id * scale;
  const vY = cy - foc.Iq * scale; 

  return (
    <Box sx={{ width: '100%', height: 300, position: 'relative' }}>
      <Typography variant="subtitle2" sx={{ position: 'absolute', top: 10, left: 10, color: 'text.secondary' }}>
        Park Transform (d-q)
      </Typography>
      <svg width="100%" height="100%" viewBox="0 0 300 300">
        {/* Axes */}
        <line x1="150" y1="20" x2="150" y2="280" stroke="#4b5257" strokeWidth="1" strokeDasharray="4 4" />
        <line x1="20" y1="150" x2="280" y2="150" stroke="#4b5257" strokeWidth="1" strokeDasharray="4 4" />
        <text x="285" y="154" fill="#4b5257" fontSize="12">d</text>
        <text x="146" y="15" fill="#4b5257" fontSize="12">q</text>

        {/* Vector */}
        <line x1={cx} y1={cy} x2={vX} y2={vY} stroke="#4caf50" strokeWidth="3" />
        <circle cx={vX} cy={vY} r="4" fill="#4caf50" />
        
        {/* Vector Components */}
        <line x1={vX} y1={cy} x2={vX} y2={vY} stroke="rgba(76,175,80,0.4)" strokeWidth="1" strokeDasharray="2 2" />
        <line x1={cx} y1={vY} x2={vX} y2={vY} stroke="rgba(76,175,80,0.4)" strokeWidth="1" strokeDasharray="2 2" />
      </svg>
    </Box>
  );
};

export const FieldAngleVisualizer: React.FC<{ snapshot: TelemetrySnapshot }> = ({ snapshot }) => {
  const cx = 150;
  const cy = 150;
  const radius = 95;
  const arcRadius = 45;
  
  const diff = snapshot.FieldAngleDifference ?? 0;
  const angleRad = (diff * Math.PI) / 180;
  
  // Stator vector endpoint
  const sX = cx + radius * Math.cos(angleRad);
  const sY = cy - radius * Math.sin(angleRad);
  
  // Rotor vector endpoint (fixed at 0 deg, horizontal)
  const rX = cx + radius;
  const rY = cy;
  
  // Arc path
  const arcX = cx + arcRadius * Math.cos(angleRad);
  const arcY = cy - arcRadius * Math.sin(angleRad);
  const arcPath = `M ${cx + arcRadius} ${cy} A ${arcRadius} ${arcRadius} 0 0 0 ${arcX} ${arcY}`;

  return (
    <Box sx={{ width: '100%', height: 320, position: 'relative', display: 'flex', flexDirection: 'column', alignItems: 'center' }}>
      <Typography variant="subtitle2" sx={{ position: 'absolute', top: 10, left: 10, color: 'text.secondary' }}>
        Field Alignment (Rotor Fixed at 0°)
      </Typography>
      
      <svg width="100%" height="240" viewBox="0 0 300 300" style={{ marginTop: 25 }}>
        {/* Background circular grid */}
        <circle cx={cx} cy={cy} r={radius} fill="none" stroke="rgba(255,255,255,0.06)" strokeWidth="1.5" />
        <circle cx={cx} cy={cy} r={radius * 0.7} fill="none" stroke="rgba(255,255,255,0.04)" strokeWidth="1" strokeDasharray="3 3" />
        <circle cx={cx} cy={cy} r={radius * 0.4} fill="none" stroke="rgba(255,255,255,0.04)" strokeWidth="1" strokeDasharray="3 3" />
        
        {/* Coordinate axes */}
        <line x1="30" y1={cy} x2="270" y2={cy} stroke="rgba(255,255,255,0.12)" strokeWidth="1" strokeDasharray="4 4" />
        <line x1={cx} y1="30" x2={cx} y2="270" stroke="rgba(255,255,255,0.12)" strokeWidth="1" strokeDasharray="4 4" />
        
        {/* Markings */}
        <text x={cx - 9} y="22" fill="rgba(255,255,255,0.3)" fontSize="10" fontFamily="monospace">90°</text>
        <text x="278" y={cy + 3} fill="rgba(255,255,255,0.3)" fontSize="10" fontFamily="monospace">0°</text>
        <text x={cx - 15} y="288" fill="rgba(255,255,255,0.3)" fontSize="10" fontFamily="monospace">-90°</text>

        {/* Arc indicating the angle difference */}
        {diff > 0 && (
          <path d={arcPath} fill="none" stroke="#c98b4d" strokeWidth="2.5" strokeDasharray="3 3" />
        )}
        
        {/* Angle Text Label inside the arc */}
        <text 
          x={cx + (arcRadius + 18) * Math.cos(angleRad / 2)} 
          y={cy - (arcRadius + 18) * Math.sin(angleRad / 2) + 4} 
          fill="#c98b4d" 
          fontSize="11" 
          fontWeight="bold" 
          fontFamily="monospace"
          textAnchor="middle"
        >
          {diff.toFixed(1)}°
        </text>

        {/* Rotor Field Vector (Fixed at 0 degrees, pointing right) */}
        <line x1={cx} y1={cy} x2={rX} y2={rY} stroke="#2196f3" strokeWidth="4.5" strokeLinecap="round" />
        {/* Arrow head for Rotor */}
        <polygon 
          points={`${rX},${rY} ${rX-10},${rY-5} ${rX-7},${rY} ${rX-10},${rY+5}`} 
          fill="#2196f3" 
        />
        <text x={rX - 52} y={rY + 18} fill="#2196f3" fontSize="10" fontWeight="bold" fontFamily="monospace">Rotor (Br)</text>

        {/* Stator Field Vector (Varying with difference angle) */}
        <line x1={cx} y1={cy} x2={sX} y2={sY} stroke="#4caf50" strokeWidth="4.5" strokeLinecap="round" />
        {/* Arrow head for Stator */}
        <polygon 
          points={`${sX},${sY} 
                  ${sX - 10 * Math.cos(angleRad - 0.45)},${sY + 10 * Math.sin(angleRad - 0.45)} 
                  ${sX - 7 * Math.cos(angleRad)},${sY + 7 * Math.sin(angleRad)} 
                  ${sX - 10 * Math.cos(angleRad + 0.45)},${sY + 10 * Math.sin(angleRad + 0.45)}`} 
          fill="#4caf50" 
        />
        <text 
          x={sX + 12 * Math.cos(angleRad)} 
          y={sY - 12 * Math.sin(angleRad) + 3} 
          fill="#4caf50" 
          fontSize="10" 
          fontWeight="bold" 
          fontFamily="monospace"
          textAnchor={Math.cos(angleRad) > 0 ? "start" : "end"}
        >
          Stator (Bs)
        </text>

        {/* Center pivot point */}
        <circle cx={cx} cy={cy} r="6" fill="#091015" stroke="rgba(255,255,255,0.6)" strokeWidth="2.5" />
      </svg>
      
      {/* Legend and stats */}
      <Stack direction="row" spacing={1.5} sx={{ position: 'absolute', bottom: 12 }}>
        <Chip size="small" label={`Diff: ${diff.toFixed(1)}°`} color="warning" variant="outlined" sx={{ borderRadius: 1.5, height: 22, fontSize: '0.75rem' }} />
        <Chip 
          size="small" 
          label={snapshot.MotorMode === "Open_Loop" ? "Open Loop (45°-73°)" : "Closed Loop (75°-88°)"} 
          color={snapshot.MotorMode === "Open_Loop" ? "warning" : "success"} 
          sx={{ borderRadius: 1.5, height: 22, fontSize: '0.75rem' }}
        />
      </Stack>
    </Box>
  );
};

import React from 'react';
import { Box, Typography } from '@mui/material';
import type { FocState } from '../app/focMath';

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

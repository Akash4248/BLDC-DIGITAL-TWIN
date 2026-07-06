import React from 'react';
import { Box, Typography } from '@mui/material';
import type { FocState } from '../app/focMath';

export const FocBlockDiagram: React.FC<{ foc: FocState }> = ({ foc }) => {
  return (
    <Box sx={{ width: '100%', height: '100%', minHeight: 300, position: 'relative', overflow: 'hidden' }}>
      <svg width="100%" height="100%" viewBox="0 0 800 300" preserveAspectRatio="xMidYMid meet">
        <defs>
          <marker id="arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="6" markerHeight="6" orient="auto">
            <path d="M 0 0 L 10 5 L 0 10 z" fill="#c98b4d" />
          </marker>
        </defs>

        {/* Connections */}
        <path d="M 80 150 L 150 150" stroke="#c98b4d" strokeWidth="2" markerEnd="url(#arrow)" />
        <path d="M 230 150 L 300 150" stroke="#c98b4d" strokeWidth="2" markerEnd="url(#arrow)" />
        <path d="M 380 150 L 450 150" stroke="#c98b4d" strokeWidth="2" markerEnd="url(#arrow)" />
        <path d="M 530 150 L 600 150" stroke="#c98b4d" strokeWidth="2" markerEnd="url(#arrow)" />
        <path d="M 680 150 L 750 150" stroke="#c98b4d" strokeWidth="2" markerEnd="url(#arrow)" />

        {/* Blocks */}
        <g transform="translate(150, 110)">
          <rect width="80" height="80" rx="4" fill="#0d1419" stroke="#4b5257" strokeWidth="2" />
          <text x="40" y="35" fill="#fff" fontSize="12" textAnchor="middle" dominantBaseline="middle">Clarke</text>
          <text x="40" y="55" fill="#4b5257" fontSize="10" textAnchor="middle" dominantBaseline="middle">abc → αβ</text>
        </g>

        <g transform="translate(300, 110)">
          <rect width="80" height="80" rx="4" fill="#0d1419" stroke="#4b5257" strokeWidth="2" />
          <text x="40" y="35" fill="#fff" fontSize="12" textAnchor="middle" dominantBaseline="middle">Park</text>
          <text x="40" y="55" fill="#4b5257" fontSize="10" textAnchor="middle" dominantBaseline="middle">αβ → dq</text>
        </g>

        <g transform="translate(450, 110)">
          <rect width="80" height="80" rx="4" fill="#0d1419" stroke="#4b5257" strokeWidth="2" />
          <text x="40" y="35" fill="#fff" fontSize="12" textAnchor="middle" dominantBaseline="middle">Inv. Park</text>
          <text x="40" y="55" fill="#4b5257" fontSize="10" textAnchor="middle" dominantBaseline="middle">dq → αβ</text>
        </g>

        <g transform="translate(600, 110)">
          <rect width="80" height="80" rx="4" fill="#0d1419" stroke="#4b5257" strokeWidth="2" />
          <text x="40" y="35" fill="#fff" fontSize="12" textAnchor="middle" dominantBaseline="middle">SVPWM</text>
          <text x="40" y="55" fill="#4b5257" fontSize="10" textAnchor="middle" dominantBaseline="middle">αβ → ABC</text>
        </g>

        {/* Labels & Live Data */}
        <g transform="translate(40, 150)">
          <text x="0" y="-10" fill="#c98b4d" fontSize="12" textAnchor="middle">Ia, Ib, Ic</text>
        </g>

        <g transform="translate(265, 140)">
          <text x="0" y="-10" fill="#c98b4d" fontSize="12" textAnchor="middle">Iα: {foc.Ialpha.toFixed(2)}</text>
          <text x="0" y="5" fill="#c98b4d" fontSize="12" textAnchor="middle">Iβ: {foc.Ibeta.toFixed(2)}</text>
        </g>

        <g transform="translate(415, 140)">
          <text x="0" y="-10" fill="#c98b4d" fontSize="12" textAnchor="middle">Id: {foc.Id.toFixed(2)}</text>
          <text x="0" y="5" fill="#c98b4d" fontSize="12" textAnchor="middle">Iq: {foc.Iq.toFixed(2)}</text>
        </g>

        <g transform="translate(565, 140)">
          <text x="0" y="-10" fill="#c98b4d" fontSize="12" textAnchor="middle">Vα: {foc.Valpha.toFixed(2)}</text>
          <text x="0" y="5" fill="#c98b4d" fontSize="12" textAnchor="middle">Vβ: {foc.Vbeta.toFixed(2)}</text>
        </g>

        <g transform="translate(715, 140)">
          <text x="0" y="-10" fill="#c98b4d" fontSize="12" textAnchor="middle">Sector</text>
          <text x="0" y="5" fill="#c98b4d" fontSize="14" textAnchor="middle" fontWeight="bold">{foc.sector}</text>
        </g>

        {/* Rotor Angle Input */}
        <g transform="translate(340, 50)">
          <circle cx="0" cy="0" r="25" fill="#0d1419" stroke="#4b5257" strokeWidth="2" />
          <text x="0" y="-5" fill="#fff" fontSize="12" textAnchor="middle">θ</text>
          <text x="0" y="10" fill="#c98b4d" fontSize="10" textAnchor="middle">{(foc.theta * 180 / Math.PI).toFixed(0)}°</text>
          <path d="M 0 25 L 0 110" stroke="#4b5257" strokeWidth="1" strokeDasharray="4 4" />
          <path d="M 0 50 L 150 50 L 150 110" stroke="#4b5257" strokeWidth="1" strokeDasharray="4 4" />
        </g>

      </svg>
    </Box>
  );
};

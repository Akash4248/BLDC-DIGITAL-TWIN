import React from 'react';
import { Box } from '@mui/material';
import type { FocState } from '../app/focMath';

export const FocBlockDiagram: React.FC<{ foc: FocState }> = ({ foc }) => {
  const viewBox = "0 0 1400 550";

  // Helper for drawing standard blocks
  const Block = ({ x, y, w = 90, h = 60, title, subtitle, value }: any) => (
    <g transform={`translate(${x}, ${y})`}>
      <rect width={w} height={h} rx="4" fill="#0d1419" stroke="#4b5257" strokeWidth="2" />
      <text x={w / 2} y={subtitle || value ? h / 2 - 8 : h / 2 + 4} fill="#fff" fontSize="12" textAnchor="middle" dominantBaseline="middle" fontWeight="500">{title}</text>
      {subtitle && <text x={w / 2} y={h / 2 + 10} fill="#8b959e" fontSize="10" textAnchor="middle" dominantBaseline="middle">{subtitle}</text>}
      {value && <text x={w / 2} y={h / 2 + 15} fill="#c98b4d" fontSize="12" textAnchor="middle" dominantBaseline="middle">{value}</text>}
    </g>
  );

  // Summing junction
  const SumJunction = ({ x, y }: any) => (
    <g transform={`translate(${x}, ${y})`}>
      <circle cx="0" cy="0" r="14" fill="#0d1419" stroke="#4b5257" strokeWidth="2" />
      <path d="M -6 -6 L 6 6 M -6 6 L 6 -6" stroke="#4b5257" strokeWidth="1" />
      <text x="-18" y="0" fill="#fff" fontSize="10" dominantBaseline="middle">+</text>
      <text x="0" y="20" fill="#fff" fontSize="10" textAnchor="middle">-</text>
    </g>
  );

  return (
    <Box sx={{ width: '100%', height: '100%', minHeight: 400, position: 'relative', overflow: 'hidden' }}>
      <svg width="100%" height="100%" viewBox={viewBox} preserveAspectRatio="xMidYMid meet" style={{ fontFamily: 'sans-serif' }}>
        <defs>
          <marker id="arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="5" markerHeight="5" orient="auto">
            <path d="M 0 0 L 10 5 L 0 10 z" fill="#4b5257" />
          </marker>
          <marker id="arrowLive" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="5" markerHeight="5" orient="auto">
            <path d="M 0 0 L 10 5 L 0 10 z" fill="#c98b4d" />
          </marker>
        </defs>

        {/* Background Zones */}
        <rect x="0" y="0" width="320" height="550" fill="rgba(100, 150, 255, 0.05)" />
        <text x="160" y="30" fill="rgba(100, 150, 255, 0.4)" fontSize="20" textAnchor="middle" fontWeight="bold">Position control</text>

        <rect x="320" y="0" width="280" height="550" fill="rgba(100, 255, 100, 0.05)" />
        <text x="460" y="30" fill="rgba(100, 255, 100, 0.4)" fontSize="20" textAnchor="middle" fontWeight="bold">Speed control</text>

        <rect x="600" y="0" width="800" height="550" fill="rgba(255, 150, 50, 0.05)" />
        <text x="900" y="30" fill="rgba(255, 150, 50, 0.4)" fontSize="20" textAnchor="middle" fontWeight="bold">Current control</text>

        {/* ========================================================= */}
        {/* WIRING PATHS */}
        {/* ========================================================= */}
        <g stroke="#4b5257" strokeWidth="2" fill="none" markerEnd="url(#arrow)">
          {/* Position Loop */}
          <path d="M 120 120 L 176 120" />
          <path d="M 204 120 L 230 120" />
          <path d="M 310 120 L 350 120" />
          
          {/* Speed Loop */}
          <path d="M 430 120 L 476 120" />
          <path d="M 504 120 L 530 120" />
          <path d="M 620 120 L 666 120" /> {/* to iq sum */}
          
          {/* Current Loop (Iq) */}
          <path d="M 694 120 L 730 120" />
          <path d="M 810 120 L 860 120" />
          
          {/* Current Loop (Id) */}
          <path d="M 620 180 L 666 180" /> {/* id ref to sum */}
          <path d="M 694 180 L 730 180" />
          <path d="M 810 180 L 860 180" />
          
          {/* FOC Transforms to PWM */}
          <path d="M 940 150 L 980 150" />
          <path d="M 1080 150 L 1120 150" />
          <path d="M 1190 150 L 1230 150" />
          <path d="M 1300 80 L 1300 100" /> {/* Power Supply -> Inverter */}
          
          {/* Inverter to Motor (Three Phases) */}
          <path d="M 1250 220 L 1250 350" stroke="#c98b4d" markerEnd="url(#arrowLive)" />
          <path d="M 1280 220 L 1280 350" stroke="#c98b4d" markerEnd="url(#arrowLive)" />
          <path d="M 1310 220 L 1310 350" stroke="#c98b4d" markerEnd="url(#arrowLive)" />
          
          {/* Motor -> Position Sensing -> Speed Calc */}
          <path d="M 1280 410 L 1280 470 L 970 470" />
          <path d="M 880 470 L 850 470" />
          
          {/* Speed feedback line */}
          <path d="M 750 470 L 490 470 L 490 134" />
          
          {/* Position feedback line */}
          <path d="M 880 470 L 880 520 L 190 520 L 190 134" />

          {/* Current Feedback -> Clarke/Park */}
          <path d="M 1250 310 L 950 310" stroke="#c98b4d" markerEnd="url(#arrowLive)" />
          <path d="M 1280 290 L 950 290" stroke="#c98b4d" markerEnd="url(#arrowLive)" />
          <path d="M 1310 270 L 950 270" stroke="#c98b4d" markerEnd="url(#arrowLive)" />
          
          {/* Theta -> Clarke/Park */}
          <path d="M 880 470 L 880 340 L 900 340" />

          {/* Clarke/Park -> iq sum */}
          <path d="M 850 280 L 680 280 L 680 134" stroke="#c98b4d" markerEnd="url(#arrowLive)" />
          
          {/* Clarke/Park -> id sum */}
          <path d="M 850 300 L 680 300 L 680 194" stroke="#c98b4d" markerEnd="url(#arrowLive)" />
        </g>

        {/* Text Labels for signals */}
        <g fill="#c98b4d" fontSize="12" fontWeight="bold">
          <text x="635" y="110">iq ref = 0</text>
          <text x="635" y="170">id ref = 0</text>
          
          <text x="835" y="110">Vq: {foc.Vbeta.toFixed(2)}</text>
          <text x="835" y="170">Vd: {foc.Valpha.toFixed(2)}</text>
          
          <text x="960" y="130">Vα, Vβ</text>
          
          <text x="1100" y="140">Duty Cycles</text>
          <text x="1205" y="140">Pulses</text>
          
          <text x="1170" y="305">ia: {foc.Ialpha.toFixed(1)}</text>
          <text x="1170" y="285">ib: {foc.Ibeta.toFixed(1)}</text>
          <text x="1170" y="265">ic: {-(foc.Ialpha + foc.Ibeta).toFixed(1)}</text>
          
          <text x="760" y="275">iq: {foc.Iq.toFixed(2)}</text>
          <text x="760" y="315">id: {foc.Id.toFixed(2)}</text>
          
          <text x="350" y="460">Speed feedback</text>
          <text x="195" y="510">Position feedback</text>
          <text x="885" y="420">θ = {(foc.theta * 180 / Math.PI).toFixed(0)}°</text>
        </g>

        {/* ========================================================= */}
        {/* BLOCKS */}
        {/* ========================================================= */}

        {/* Position */}
        <Block x={40} y={90} w={80} h={60} title="Position" subtitle="Reference" />
        <SumJunction x={190} y={120} />
        <Block x={230} y={90} w={80} h={60} title="Position" subtitle="Controller" />
        
        {/* Speed */}
        <Block x={350} y={90} w={80} h={60} title="Speed" subtitle="Reference" />
        <SumJunction x={490} y={120} />
        <Block x={530} y={90} w={90} h={60} title="Speed" subtitle="Controller" />

        {/* Current / ID IQ */}
        <SumJunction x={680} y={120} />
        <Block x={730} y={90} w={80} h={50} title="PI-cont." subtitle="(Iq)" />
        
        <SumJunction x={680} y={180} />
        <Block x={730} y={155} w={80} h={50} title="PI-cont." subtitle="(Id)" />
        
        {/* Transforms */}
        <Block x={860} y={90} w={80} h={120} title="Inverse Park" subtitle="dq → αβ" />
        <Block x={980} y={90} w={100} h={120} title="SPWM" subtitle="Generator" />
        <Block x={1120} y={105} w={70} h={90} title="PWM" subtitle="driver" />
        
        <Block x={1230} y={20} w={100} h={60} title="Power supply" />
        
        {/* Inverter & Motor */}
        <g transform="translate(1230, 100)">
          <rect width="100" height="120" rx="4" fill="#0d1419" stroke="#4b5257" strokeWidth="2" />
          <text x="50" y="50" fill="#fff" fontSize="14" textAnchor="middle" fontWeight="bold">Three-Phase</text>
          <text x="50" y="70" fill="#fff" fontSize="14" textAnchor="middle" fontWeight="bold">Inverter</text>
        </g>
        
        <g transform="translate(1230, 350)">
          <rect width="100" height="60" rx="4" fill="#0d1419" stroke="#4b5257" strokeWidth="2" />
          <text x="50" y="35" fill="#fff" fontSize="16" textAnchor="middle" fontWeight="bold">Motor</text>
        </g>

        {/* Feedback Processing */}
        <Block x={850} y={250} w={100} h={100} title="Clarke/Park" subtitle="abc → αβ → dq" />
        
        <Block x={880} y={440} w={90} h={60} title="Position" subtitle="sensing" />
        <Block x={750} y={440} w={100} h={60} title="Speed" subtitle="calculation" />

      </svg>
    </Box>
  );
};

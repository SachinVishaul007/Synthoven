import React, { useState } from 'react';
import { Waveform } from '../ui/Waveform';
import { Tag } from '../ui/Tag';

function formatDuration(s) {
  const ms = Math.round(s * 1000);
  return ms >= 1000 ? `${s.toFixed(2)}s` : `${ms}ms`;
}

export function SampleRow({ sample, isPlaying, onPlay, onSave, onToggleFavorite }) {
  const [hovered, setHovered] = useState(false);
  const [saved, setSaved] = useState(false);
  const isLib = sample.type === 'library';
  const isGen = sample.type === 'generated';

  const handleSave = (e) => {
    e.stopPropagation();
    setSaved(true);
    onSave?.(sample);
    setTimeout(() => setSaved(false), 2000);
  };

  const accent      = isLib ? 'var(--accent-amber)'        : 'var(--accent-gen)';
  const accentGlow  = isLib ? 'var(--accent-amber-glow)'   : 'var(--accent-gen-glow)';
  const accentBorder= isLib ? 'var(--accent-amber-border)' : 'var(--accent-gen-border)';

  return (
    <div
      onClick={() => onPlay(sample)}
      onMouseEnter={() => setHovered(true)}
      onMouseLeave={() => setHovered(false)}
      style={{
        display: 'grid',
        gridTemplateColumns: '32px 1fr 56px 80px auto',
        alignItems: 'center', gap: '12px',
        padding: '10px 16px',
        borderBottom: '1px solid var(--border-subtle)',
        cursor: 'pointer',
        background: hovered ? 'var(--bg-elevated)' : 'transparent',
        transition: 'background var(--t-fast)',
      }}
    >
      <button
        onClick={e => { e.stopPropagation(); onPlay(sample); }}
        style={{
          width: 32, height: 32, borderRadius: '50%', flexShrink: 0,
          border: `1px solid ${isPlaying ? accent : 'var(--border-default)'}`,
          background: isPlaying ? accentGlow : 'transparent',
          color: isPlaying ? accent : 'var(--text-tertiary)',
          display: 'flex', alignItems: 'center', justifyContent: 'center',
          transition: 'all var(--t-base)',
        }}
      >
        {isPlaying
          ? <svg width="10" height="10" viewBox="0 0 10 10" fill="currentColor"><rect x="1" y="1" width="3" height="8" rx="1"/><rect x="6" y="1" width="3" height="8" rx="1"/></svg>
          : <svg width="10" height="10" viewBox="0 0 10 10" fill="currentColor"><path d="M2 1.5L8.5 5 2 8.5V1.5z"/></svg>
        }
      </button>

      <div style={{ overflow: 'hidden' }}>
        <div style={{
          fontSize: '12px', fontFamily: 'var(--font-mono)',
          color: isPlaying ? accent : 'var(--text-primary)',
          whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis', marginBottom: '2px',
          transition: 'color var(--t-base)',
        }}>
          {sample.name}
        </div>
        <div style={{
          fontSize: '11px', fontFamily: 'var(--font-mono)', color: 'var(--text-tertiary)',
          whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis',
        }}>
          {isLib ? sample.pack : sample.model}
        </div>
      </div>

      <Waveform data={sample.waveform} isPlaying={isPlaying} type={sample.type} />

      <div style={{ textAlign: 'right' }}>
        <div style={{ fontSize: '11px', fontFamily: 'var(--font-mono)', color: 'var(--text-secondary)' }}>
          {formatDuration(sample.duration)}
        </div>
        {sample.bpm && (
          <div style={{ fontSize: '10px', fontFamily: 'var(--font-mono)', color: 'var(--text-tertiary)' }}>
            {sample.bpm} bpm
          </div>
        )}
      </div>

      <div style={{ display: 'flex', gap: '6px', opacity: hovered || isPlaying ? 1 : 0, transition: 'opacity var(--t-base)' }}>
        {isGen && (
          <button onClick={handleSave} style={{
            fontSize: '10px', fontFamily: 'var(--font-mono)', padding: '4px 8px',
            borderRadius: 'var(--r-sm)', whiteSpace: 'nowrap', cursor: 'pointer',
            border: saved ? '1px solid rgba(100,200,120,0.4)' : `1px solid ${accentBorder}`,
            background: saved ? 'rgba(100,200,120,0.1)' : accentGlow,
            color: saved ? '#64c878' : 'var(--text-gen)',
          }}>
            {saved ? '✓ saved' : '+ save'}
          </button>
        )}
        <button
          onClick={(e) => { e.stopPropagation(); onToggleFavorite?.(sample); }}
          style={{
            width: 26, height: 26, borderRadius: 'var(--r-sm)',
            border: sample.favorited ? '1px solid var(--accent-red-border)' : '1px solid var(--border-default)',
            background: sample.favorited ? 'var(--accent-red-glow)' : 'var(--bg-overlay)',
            color: sample.favorited ? 'var(--accent-red)' : 'var(--text-tertiary)',
            display: 'flex', alignItems: 'center', justifyContent: 'center', cursor: 'pointer',
            transition: 'all var(--t-base)',
          }}
          title={sample.favorited ? "Remove from Favorites" : "Add to Favorites"}
        >
          <svg width="11" height="11" viewBox="0 0 11 11" fill={sample.favorited ? "currentColor" : "none"} stroke="currentColor" strokeWidth="1.4">
            <path d="M5.5 9.5s-4-2.8-4-5.6a2.4 2.4 0 0 1 4.8 0 2.4 2.4 0 0 1 4.8 0c0 2.8-4 5.6-4 5.6z" strokeLinejoin="round"/>
          </svg>
        </button>
        <button style={{
          width: 26, height: 26, borderRadius: 'var(--r-sm)',
          border: '1px solid var(--border-default)', background: 'var(--bg-overlay)',
          color: 'var(--text-tertiary)', display: 'flex', alignItems: 'center', justifyContent: 'center', cursor: 'pointer',
        }}>
          <svg width="11" height="11" viewBox="0 0 11 11" fill="none" stroke="currentColor" strokeWidth="1.4">
            <path d="M5.5 1v7M2 5l3.5 3.5L9 5"/><path d="M1 10h9"/>
          </svg>
        </button>
      </div>
    </div>
  );
}
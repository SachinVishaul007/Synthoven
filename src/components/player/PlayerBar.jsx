import React from 'react';
import { Waveform } from '../ui/Waveform';

export function PlayerBar({ sample, isPlaying, onToggle, onStop }) {
  if (!sample) {
    return (
      <div style={{
        height: 'var(--player-height)', borderTop: '1px solid var(--border-subtle)',
        background: 'var(--bg-surface)', display: 'flex', alignItems: 'center', padding: '0 24px',
      }}>
        <div style={{ fontSize: '11px', fontFamily: 'var(--font-mono)', color: 'var(--text-tertiary)', letterSpacing: '0.06em' }}>
          — no sample loaded
        </div>
      </div>
    );
  }

  const isLib       = sample.type === 'library';
  const accent      = isLib ? 'var(--accent-amber)'        : 'var(--accent-gen)';
  const accentBorder= isLib ? 'var(--accent-amber-border)' : 'var(--accent-gen-border)';
  const accentGlow  = isLib ? 'var(--accent-amber-glow)'   : 'var(--accent-gen-glow)';

  return (
    <div style={{
      height: 'var(--player-height)',
      borderTop: `1px solid ${accentBorder}`,
      background: 'var(--bg-surface)',
      display: 'flex', alignItems: 'center', padding: '0 24px', gap: '20px',
    }}>
      <button onClick={onToggle} style={{
        width: 36, height: 36, borderRadius: '50%', flexShrink: 0,
        border: `1px solid ${accent}`,
        background: isPlaying ? accent : 'transparent',
        color: isPlaying ? 'var(--bg-base)' : accent,
        display: 'flex', alignItems: 'center', justifyContent: 'center',
        cursor: 'pointer', transition: 'all var(--t-base)',
      }}>
        {isPlaying
          ? <svg width="11" height="11" viewBox="0 0 11 11" fill="currentColor"><rect x="1" y="1" width="3.5" height="9" rx="1"/><rect x="6.5" y="1" width="3.5" height="9" rx="1"/></svg>
          : <svg width="11" height="11" viewBox="0 0 11 11" fill="currentColor"><path d="M2 1.5L9.5 5.5 2 9.5V1.5z"/></svg>
        }
      </button>

      <div style={{ flex: 1, overflow: 'hidden' }}>
        <div style={{
          fontSize: '12px', fontFamily: 'var(--font-mono)', color: accent,
          whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis', marginBottom: '3px',
        }}>
          {sample.name}
        </div>
        <div style={{ display: 'flex', gap: '12px', alignItems: 'center' }}>
          <span style={{ fontSize: '10px', fontFamily: 'var(--font-mono)', color: 'var(--text-tertiary)' }}>
            {isLib ? sample.pack : sample.model}
          </span>
          {!isLib && (
            <span style={{
              fontSize: '9px', fontFamily: 'var(--font-mono)', padding: '1px 5px', borderRadius: '2px',
              background: accentGlow, color: 'var(--text-gen)', border: `1px solid ${accentBorder}`,
            }}>
              AI
            </span>
          )}
        </div>
      </div>

      <Waveform data={sample.waveform} isPlaying={isPlaying} type={sample.type} />

      <div style={{ fontSize: '12px', fontFamily: 'var(--font-mono)', color: 'var(--text-tertiary)', flexShrink: 0, minWidth: '40px', textAlign: 'right' }}>
        {sample.duration.toFixed(2)}s
      </div>

      <div style={{ display: 'flex', gap: '4px', flexShrink: 0 }}>
        {sample.tags.slice(0, 3).map(t => (
          <span key={t} style={{
            fontSize: '9px', fontFamily: 'var(--font-mono)', padding: '2px 5px', borderRadius: '2px',
            background: 'var(--bg-overlay)', color: 'var(--text-tertiary)', border: '1px solid var(--border-subtle)',
          }}>
            {t}
          </span>
        ))}
      </div>
    </div>
  );
}
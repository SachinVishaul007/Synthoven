import React from 'react';
import { SampleRow } from './SampleRow';

function SpinnerDots({ accent }) {
  return (
    <span style={{ display: 'flex', gap: '3px', alignItems: 'center' }}>
      {[0, 1, 2].map(i => (
        <span key={i} style={{
          width: 3, height: 3, borderRadius: '50%', background: accent,
          display: 'inline-block',
          animation: `pulse 1.2s ease-in-out ${i * 0.2}s infinite`,
        }} />
      ))}
      <style>{`
        @keyframes pulse {
          0%, 80%, 100% { opacity: 0.2; transform: scale(0.8); }
          40%            { opacity: 1;   transform: scale(1);   }
        }
      `}</style>
    </span>
  );
}

function SectionHeader({ label, count, accent, isLoading, badge }) {
  return (
    <div style={{
      display: 'flex', alignItems: 'center', gap: '10px',
      padding: '10px 16px', borderBottom: '1px solid var(--border-subtle)',
      background: 'var(--bg-surface)', position: 'sticky', top: 0, zIndex: 1,
    }}>
      <div style={{ width: 6, height: 6, borderRadius: '50%', background: accent, flexShrink: 0 }} />
      <span style={{
        fontSize: '10px', fontFamily: 'var(--font-mono)',
        letterSpacing: '0.1em', color: 'var(--text-tertiary)',
        textTransform: 'uppercase', flex: 1,
      }}>
        {label}
      </span>
      {isLoading ? (
        <span style={{ fontSize: '10px', fontFamily: 'var(--font-mono)', color: 'var(--text-tertiary)', display: 'flex', alignItems: 'center', gap: '6px' }}>
          <SpinnerDots accent={accent} /> loading
        </span>
      ) : (
        <span style={{ fontSize: '10px', fontFamily: 'var(--font-mono)', color: 'var(--text-tertiary)' }}>
          {count !== null ? `${count} result${count !== 1 ? 's' : ''}` : ''}
        </span>
      )}
      {badge && (
        <span style={{
          fontSize: '9px', fontFamily: 'var(--font-mono)', letterSpacing: '0.06em',
          padding: '2px 6px', borderRadius: '2px',
          background: 'rgba(108,143,255,0.1)', color: 'var(--text-gen)',
          border: '1px solid var(--accent-gen-border)', textTransform: 'uppercase',
        }}>
          {badge}
        </span>
      )}
    </div>
  );
}

function EmptySearch() {
  const hints = ['punchy techno kick', 'dark atmospheric pad', 'filtered acid bass', 'vinyl crackle loop', '909 rimshot dry'];
  return (
    <div style={{ flex: 1, display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', gap: '24px' }}>
      <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'center', gap: '8px' }}>
        <div style={{ fontSize: '11px', fontFamily: 'var(--font-mono)', letterSpacing: '0.1em', textTransform: 'uppercase', color: 'var(--text-tertiary)', marginBottom: '4px' }}>
          try searching for
        </div>
        {hints.map(h => (
          <div key={h} style={{
            fontSize: '12px', fontFamily: 'var(--font-mono)', color: 'var(--text-tertiary)',
            padding: '4px 12px', borderRadius: 'var(--r-sm)',
            border: '1px solid var(--border-subtle)', background: 'var(--bg-elevated)',
          }}>
            {h}
          </div>
        ))}
      </div>
    </div>
  );
}

export function ResultsPanel({ libraryResults, generatedResults, isSearching, isGenerating, hasSearched, submittedQuery, playingId, onPlay, onSave }) {
  if (!hasSearched) return <EmptySearch />;

  return (
    <div style={{ display: 'flex', flex: 1, overflow: 'hidden' }}>
      <div style={{ flex: 1, display: 'flex', flexDirection: 'column', overflow: 'hidden' }}>
        <SectionHeader label="Your Library" count={isSearching ? null : libraryResults.length} accent="var(--accent-amber)" isLoading={isSearching} />
        <div style={{ flex: 1, overflowY: 'auto' }}>
          {!isSearching && libraryResults.length === 0 && (
            <div style={{ padding: '40px 24px', textAlign: 'center', color: 'var(--text-tertiary)', fontSize: '12px', fontFamily: 'var(--font-mono)' }}>no library results</div>
          )}
          {libraryResults.map(s => (
            <SampleRow key={s.id} sample={s} isPlaying={playingId === s.id} onPlay={onPlay} onSave={onSave} />
          ))}
        </div>
      </div>

      <div style={{ width: '1px', background: 'var(--border-subtle)', flexShrink: 0 }} />

      <div style={{ flex: 1, display: 'flex', flexDirection: 'column', overflow: 'hidden' }}>
        <SectionHeader label="AI Generated" count={isGenerating ? null : generatedResults.length} accent="var(--accent-gen)" isLoading={isGenerating} badge="live" />
        <div style={{ flex: 1, overflowY: 'auto' }}>
          {!isGenerating && generatedResults.length === 0 && (
            <div style={{ padding: '40px 24px', textAlign: 'center', color: 'var(--text-tertiary)', fontSize: '12px', fontFamily: 'var(--font-mono)' }}>no generated results</div>
          )}
          {generatedResults.map(s => (
            <SampleRow key={s.id} sample={s} isPlaying={playingId === s.id} onPlay={onPlay} onSave={onSave} />
          ))}
        </div>
      </div>
    </div>
  );
}
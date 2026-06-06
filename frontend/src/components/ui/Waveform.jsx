import React from 'react';

const styles = {
  container: {
    display: 'flex',
    alignItems: 'center',
    gap: '1px',
    height: '28px',
  },
  bar: (height, isPlaying, isLibrary) => ({
    width: '2px',
    borderRadius: '1px',
    transition: 'height 80ms ease, background 80ms ease',
    height: `${Math.max(3, height * 28)}px`,
    background: isPlaying
      ? (isLibrary ? 'var(--accent-amber)' : 'var(--accent-gen)')
      : 'var(--border-strong)',
    flexShrink: 0,
  }),
};

export function Waveform({ data = [], isPlaying = false, type = 'library' }) {
  const isLibrary = type === 'library';
  return (
    <div style={styles.container}>
      {data.map((v, i) => (
        <div key={i} style={styles.bar(v, isPlaying, isLibrary)} />
      ))}
    </div>
  );
}
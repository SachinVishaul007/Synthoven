import React from 'react';

export function Tag({ label, variant = 'default' }) {
  const variants = {
    default: {
      background: 'var(--bg-subtle)',
      color: 'var(--text-tertiary)',
      border: '1px solid var(--border-subtle)',
    },
    amber: {
      background: 'rgba(232, 160, 32, 0.08)',
      color: 'var(--accent-amber-dim)',
      border: '1px solid var(--accent-amber-border)',
    },
    gen: {
      background: 'rgba(108, 143, 255, 0.08)',
      color: 'var(--text-gen)',
      border: '1px solid var(--accent-gen-border)',
    },
  };

  return (
    <span style={{
      ...(variants[variant] || variants.default),
      display: 'inline-block',
      fontSize: '10px',
      fontFamily: 'var(--font-mono)',
      letterSpacing: '0.04em',
      padding: '2px 6px',
      borderRadius: 'var(--r-sm)',
      whiteSpace: 'nowrap',
    }}>
      {label}
    </span>
  );
}
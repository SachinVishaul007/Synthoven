import React from 'react';

const NAV_ITEMS = [
  { id: 'library',   label: 'Library'   },
  { id: 'packs',     label: 'Packs'     },
  { id: 'favorites', label: 'Favorites' },
  { id: 'generated', label: 'Generated' },
  { id: 'recent',    label: 'Recent'    },
];

const NAV_ICONS = {
  library: (
    <svg width="14" height="14" viewBox="0 0 14 14" fill="none" stroke="currentColor" strokeWidth="1.4">
      <rect x="1" y="1" width="5" height="5" rx="1"/>
      <rect x="8" y="1" width="5" height="5" rx="1"/>
      <rect x="1" y="8" width="5" height="5" rx="1"/>
      <rect x="8" y="8" width="5" height="5" rx="1"/>
    </svg>
  ),
  packs: (
    <svg width="14" height="14" viewBox="0 0 14 14" fill="none" stroke="currentColor" strokeWidth="1.4">
      <rect x="1.5" y="4" width="11" height="9" rx="1"/>
      <path d="M4 4V3a3 3 0 016 0v1"/>
    </svg>
  ),
  favorites: (
    <svg width="14" height="14" viewBox="0 0 14 14" fill="none" stroke="currentColor" strokeWidth="1.4">
      <path d="M7 12s-5-3.5-5-7a3 3 0 016 0 3 3 0 016 0c0 3.5-5 7-5 7z" strokeLinejoin="round"/>
    </svg>
  ),
  generated: (
    <svg width="14" height="14" viewBox="0 0 14 14" fill="none" stroke="currentColor" strokeWidth="1.4">
      <path d="M7 1v2M7 11v2M1 7h2M11 7h2M3 3l1.5 1.5M9.5 9.5L11 11M3 11l1.5-1.5M9.5 4.5L11 3"/>
      <circle cx="7" cy="7" r="2"/>
    </svg>
  ),
  recent: (
    <svg width="14" height="14" viewBox="0 0 14 14" fill="none" stroke="currentColor" strokeWidth="1.4">
      <circle cx="7" cy="7" r="5.5"/>
      <path d="M7 4v3l2 2"/>
    </svg>
  ),
};

export function Sidebar({ activeSection, onNavigate, libraryCount = 0 }) {
  return (
    <div style={{
      width: 'var(--sidebar-width)',
      height: '100%',
      background: 'var(--bg-surface)',
      borderRight: '1px solid var(--border-subtle)',
      display: 'flex',
      flexDirection: 'column',
      flexShrink: 0,
    }}>
      <div style={{
        height: 'var(--topbar-height)',
        display: 'flex',
        alignItems: 'center',
        padding: '0 20px',
        borderBottom: '1px solid var(--border-subtle)',
      }}>
        <div style={{ display: 'flex', alignItems: 'baseline', gap: '4px' }}>
          <span style={{
            fontSize: '18px', fontWeight: 600,
            fontFamily: 'var(--font-mono)',
            color: 'var(--accent-amber)',
            letterSpacing: '-0.02em',
          }}>CHOP</span>
          <span style={{
            fontSize: '9px',
            fontFamily: 'var(--font-mono)',
            color: 'var(--text-tertiary)',
            letterSpacing: '0.06em',
          }}>v0.1</span>
        </div>
      </div>

      <nav style={{ flex: 1, padding: '12px 8px', display: 'flex', flexDirection: 'column', gap: '2px' }}>
        {NAV_ITEMS.map(item => {
          const active = activeSection === item.id;
          return (
            <button
              key={item.id}
              onClick={() => onNavigate(item.id)}
              style={{
                display: 'flex', alignItems: 'center', gap: '10px',
                padding: '8px 12px', borderRadius: 'var(--r-md)',
                background: active ? 'var(--bg-overlay)' : 'transparent',
                border: active ? '1px solid var(--border-subtle)' : '1px solid transparent',
                color: active ? 'var(--text-primary)' : 'var(--text-tertiary)',
                fontSize: '12px', fontWeight: active ? 500 : 400,
                cursor: 'pointer', textAlign: 'left', width: '100%',
                transition: 'all var(--t-base)',
              }}
              onMouseEnter={e => { if (!active) e.currentTarget.style.color = 'var(--text-secondary)'; }}
              onMouseLeave={e => { if (!active) e.currentTarget.style.color = 'var(--text-tertiary)'; }}
            >
              <span style={{ opacity: active ? 1 : 0.6 }}>{NAV_ICONS[item.id]}</span>
              {item.label}
            </button>
          );
        })}
      </nav>

      <div style={{ padding: '16px 20px', borderTop: '1px solid var(--border-subtle)' }}>
        <div style={{
          fontSize: '10px', fontFamily: 'var(--font-mono)',
          color: 'var(--text-tertiary)', marginBottom: '6px',
          display: 'flex', justifyContent: 'space-between',
        }}>
          <span>library</span>
          <span style={{ color: 'var(--accent-amber)' }}>{libraryCount} sample{libraryCount !== 1 ? 's' : ''}</span>
        </div>
        <div style={{ height: '2px', background: 'var(--bg-overlay)', borderRadius: '1px', overflow: 'hidden' }}>
          <div style={{ width: '12%', height: '100%', background: 'var(--accent-amber)', borderRadius: '1px' }} />
        </div>
      </div>
    </div>
  );
}
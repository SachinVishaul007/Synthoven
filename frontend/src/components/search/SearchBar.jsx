import React, { useRef, useState } from 'react';
import { RECENT_SEARCHES } from '../../data/samples';

export function SearchBar({ query, onChange, onSearch, onClear, hasSearched, submittedQuery }) {
  const inputRef = useRef(null);
  const [focused, setFocused] = useState(false);
  const [showRecent, setShowRecent] = useState(false);

  const handleKeyDown = (e) => {
    if (e.key === 'Enter' && query.trim()) {
      onSearch(query);
      setShowRecent(false);
      inputRef.current?.blur();
    }
    if (e.key === 'Escape') {
      setShowRecent(false);
      inputRef.current?.blur();
    }
  };

  const handleRecentClick = (s) => {
    onChange(s);
    onSearch(s);
    setShowRecent(false);
  };

  return (
    <div style={{ position: 'relative', flex: 1 }}>
      <div style={{
        display: 'flex', alignItems: 'center', gap: '12px',
        padding: '0 16px', height: '48px', borderRadius: 'var(--r-md)',
        border: focused ? '1px solid var(--accent-amber-border)' : '1px solid var(--border-default)',
        background: focused ? 'var(--bg-elevated)' : 'var(--bg-surface)',
        transition: 'border-color var(--t-base), background var(--t-base)',
      }}>
        <svg width="16" height="16" viewBox="0 0 16 16" fill="none" stroke="var(--text-tertiary)" strokeWidth="1.5">
          <circle cx="7" cy="7" r="5"/><path d="M11 11l3 3"/>
        </svg>

        <input
          ref={inputRef}
          value={query}
          onChange={e => { onChange(e.target.value); if (e.target.value) setShowRecent(false); }}
          onKeyDown={handleKeyDown}
          onFocus={() => { setFocused(true); if (!query) setShowRecent(true); }}
          onBlur={() => { setFocused(false); setTimeout(() => setShowRecent(false), 150); }}
          placeholder="Describe a sound..."
          style={{
            flex: 1, background: 'none', border: 'none', outline: 'none',
            color: 'var(--text-primary)', fontSize: '15px', fontFamily: 'var(--font-sans)',
          }}
        />

        {hasSearched && submittedQuery && (
          <span style={{ fontSize: '10px', fontFamily: 'var(--font-mono)', color: 'var(--text-tertiary)', flexShrink: 0 }}>
            "{submittedQuery}"
          </span>
        )}

        {query && (
          <button
            onClick={() => { onClear(); inputRef.current?.focus(); setShowRecent(true); }}
            style={{
              width: 20, height: 20, borderRadius: '50%',
              background: 'var(--bg-overlay)', border: '1px solid var(--border-subtle)',
              color: 'var(--text-tertiary)', display: 'flex', alignItems: 'center',
              justifyContent: 'center', flexShrink: 0, cursor: 'pointer',
            }}
          >
            <svg width="8" height="8" viewBox="0 0 8 8" stroke="currentColor" strokeWidth="1.5">
              <path d="M1 1l6 6M7 1L1 7"/>
            </svg>
          </button>
        )}

        <button
          onClick={() => query.trim() && onSearch(query)}
          disabled={!query.trim()}
          style={{
            padding: '5px 14px', borderRadius: 'var(--r-sm)',
            border: '1px solid var(--accent-amber-border)',
            background: query.trim() ? 'var(--accent-amber-glow)' : 'transparent',
            color: query.trim() ? 'var(--accent-amber)' : 'var(--text-tertiary)',
            fontSize: '11px', fontFamily: 'var(--font-mono)', letterSpacing: '0.04em',
            cursor: query.trim() ? 'pointer' : 'default', flexShrink: 0,
            transition: 'all var(--t-base)',
          }}
        >
          search
        </button>
      </div>

      {showRecent && !query && (
        <div style={{
          position: 'absolute', top: 'calc(100% + 6px)', left: 0, right: 0,
          background: 'var(--bg-elevated)', border: '1px solid var(--border-default)',
          borderRadius: 'var(--r-md)', overflow: 'hidden', zIndex: 100,
        }}>
          <div style={{
            padding: '8px 12px 4px', fontSize: '9px',
            fontFamily: 'var(--font-mono)', letterSpacing: '0.1em',
            color: 'var(--text-tertiary)', textTransform: 'uppercase',
          }}>
            Recent
          </div>
          {RECENT_SEARCHES.map(s => (
            <button
              key={s}
              onMouseDown={() => handleRecentClick(s)}
              style={{
                display: 'flex', alignItems: 'center', gap: '10px',
                width: '100%', padding: '8px 12px', background: 'none', border: 'none',
                textAlign: 'left', cursor: 'pointer', color: 'var(--text-secondary)',
                fontSize: '12px', fontFamily: 'var(--font-mono)',
              }}
              onMouseEnter={e => e.currentTarget.style.background = 'var(--bg-overlay)'}
              onMouseLeave={e => e.currentTarget.style.background = 'none'}
            >
              <svg width="12" height="12" viewBox="0 0 12 12" fill="none" stroke="currentColor" strokeWidth="1.2" style={{ flexShrink: 0, opacity: 0.4 }}>
                <circle cx="6" cy="6" r="4.5"/><path d="M6 3.5V6l1.5 1.5"/>
              </svg>
              {s}
            </button>
          ))}
        </div>
      )}
    </div>
  );
}
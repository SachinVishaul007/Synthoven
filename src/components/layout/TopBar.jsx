import React from 'react';
import { SearchBar } from '../search/SearchBar';

export function TopBar({ query, onChange, onSearch, onClear, hasSearched, submittedQuery }) {
  return (
    <div style={{
      height: 'var(--topbar-height)',
      borderBottom: '1px solid var(--border-subtle)',
      display: 'flex', alignItems: 'center',
      padding: '0 20px', gap: '16px',
      background: 'var(--bg-base)', flexShrink: 0,
    }}>
      <SearchBar
        query={query} onChange={onChange}
        onSearch={onSearch} onClear={onClear}
        hasSearched={hasSearched} submittedQuery={submittedQuery}
      />

      <div style={{ display: 'flex', gap: '8px', flexShrink: 0 }}>
        <button style={{
          width: 32, height: 32, borderRadius: 'var(--r-md)',
          border: '1px solid var(--border-subtle)', background: 'var(--bg-elevated)',
          color: 'var(--text-tertiary)', display: 'flex', alignItems: 'center', justifyContent: 'center', cursor: 'pointer',
        }} title="Filter">
          <svg width="13" height="13" viewBox="0 0 13 13" fill="none" stroke="currentColor" strokeWidth="1.4">
            <path d="M1 3h11M3 6.5h7M5 10h3"/>
          </svg>
        </button>

        <button style={{
          width: 32, height: 32, borderRadius: 'var(--r-md)',
          border: '1px solid var(--border-subtle)', background: 'var(--bg-elevated)',
          color: 'var(--text-tertiary)', display: 'flex', alignItems: 'center', justifyContent: 'center', cursor: 'pointer',
        }} title="Import">
          <svg width="13" height="13" viewBox="0 0 13 13" fill="none" stroke="currentColor" strokeWidth="1.4">
            <path d="M6.5 1v8M3 6l3.5 3.5L10 6"/>
            <path d="M1 11.5h11"/>
          </svg>
        </button>
      </div>
    </div>
  );
}
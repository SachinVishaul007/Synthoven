import React, { useRef, useState } from 'react';
import { SearchBar } from '../search/SearchBar';

export function TopBar({ query, onChange, onSearch, onClear, hasSearched, submittedQuery, onUpload, onSelectFolder }) {
  const fileInputRef = useRef(null);
  const [uploading, setUploading] = useState(false);
  const [selectingFolder, setSelectingFolder] = useState(false);

  const handleUploadClick = async () => {
    if (uploading) return;
    setUploading(true);
    try { await onUpload?.(); } finally { setUploading(false); }
  };

  const handleSelectFolderClick = async () => {
    if (selectingFolder) return;
    setSelectingFolder(true);
    try { await onSelectFolder?.(); } finally { setSelectingFolder(false); }
  };

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

        <button
          onClick={handleSelectFolderClick}
          disabled={selectingFolder}
          style={{
            width: 32, height: 32, borderRadius: 'var(--r-md)',
            border: `1px solid ${selectingFolder ? 'var(--accent-amber-border)' : 'var(--border-subtle)'}`,
            background: selectingFolder ? 'var(--bg-overlay)' : 'var(--bg-elevated)',
            color: selectingFolder ? 'var(--accent-amber)' : 'var(--text-tertiary)',
            display: 'flex', alignItems: 'center', justifyContent: 'center',
            cursor: selectingFolder ? 'default' : 'pointer',
            transition: 'all var(--t-base)',
          }}
          title="Select/Scan library folder"
        >
          {selectingFolder
            ? <svg width="13" height="13" viewBox="0 0 13 13" fill="none" stroke="currentColor" strokeWidth="1.4" style={{ animation: 'spin 1s linear infinite' }}>
                <path d="M6.5 1a5.5 5.5 0 1 1-3.89 1.61"/>
                <style>{`@keyframes spin { to { transform: rotate(360deg); } }`}</style>
              </svg>
            : <svg width="13" height="13" viewBox="0 0 13 13" fill="none" stroke="currentColor" strokeWidth="1.4">
                <path d="M1 2.5h3.5l1.5 1.5h6v7.5H1V2.5z"/>
                <path d="M6.5 6v4M4.5 8h4"/>
              </svg>
          }
        </button>

        <button
          onClick={handleUploadClick}
          disabled={uploading}
          style={{
            width: 32, height: 32, borderRadius: 'var(--r-md)',
            border: `1px solid ${uploading ? 'var(--accent-amber-border)' : 'var(--border-subtle)'}`,
            background: uploading ? 'var(--bg-overlay)' : 'var(--bg-elevated)',
            color: uploading ? 'var(--accent-amber)' : 'var(--text-tertiary)',
            display: 'flex', alignItems: 'center', justifyContent: 'center',
            cursor: uploading ? 'default' : 'pointer',
            transition: 'all var(--t-base)',
          }}
          title="Import files"
        >
          {uploading
            ? <svg width="13" height="13" viewBox="0 0 13 13" fill="none" stroke="currentColor" strokeWidth="1.4" style={{ animation: 'spin 1s linear infinite' }}>
                <path d="M6.5 1a5.5 5.5 0 1 1-3.89 1.61"/>
                <style>{`@keyframes spin { to { transform: rotate(360deg); } }`}</style>
              </svg>
            : <svg width="13" height="13" viewBox="0 0 13 13" fill="none" stroke="currentColor" strokeWidth="1.4">
                <path d="M6.5 9V1M3 4l3.5-3.5L10 4"/>
                <path d="M1 11.5h11"/>
              </svg>
          }
        </button>
      </div>
    </div>
  );
}
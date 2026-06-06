import React, { useRef, useEffect } from 'react';
import { SearchBar } from '../search/SearchBar';

const AUDIO_RE = /\.(wav|mp3|aiff?|flac)$/i;

export function TopBar({ query, onChange, onSearch, onClear, hasSearched, submittedQuery, onImportFolder, isImporting }) {
  const folderInputRef = useRef(null);

  // webkitdirectory / directory aren't standard JSX props, so set them on the
  // element directly — this turns the file picker into a folder picker.
  useEffect(() => {
    const input = folderInputRef.current;
    if (input) {
      input.setAttribute('webkitdirectory', '');
      input.setAttribute('directory', '');
    }
  }, []);

  const handleFolderPick = (e) => {
    const all = Array.from(e.target.files || []);
    const audio = all.filter(f => AUDIO_RE.test(f.name));
    const folderName = all[0]?.webkitRelativePath?.split('/')[0] || '';
    e.target.value = ''; // allow re-picking the same folder
    if (audio.length === 0) {
      onImportFolder?.([], folderName, all.length);
      return;
    }
    onImportFolder?.(audio, folderName, all.length);
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

        <input
          ref={folderInputRef}
          type="file"
          multiple
          accept=".wav,.mp3,.aiff,.aif,.flac,audio/*"
          onChange={handleFolderPick}
          style={{ display: 'none' }}
        />
        <button
          onClick={() => folderInputRef.current?.click()}
          disabled={isImporting}
          style={{
            height: 32, borderRadius: 'var(--r-md)', padding: '0 12px', gap: '6px',
            border: '1px solid var(--border-subtle)', background: 'var(--bg-elevated)',
            color: 'var(--text-secondary)', display: 'flex', alignItems: 'center', justifyContent: 'center',
            cursor: isImporting ? 'wait' : 'pointer', fontSize: '11px', fontFamily: 'var(--font-mono)',
            whiteSpace: 'nowrap', opacity: isImporting ? 0.6 : 1,
          }}
          title="Import a folder of audio files"
        >
          <svg width="13" height="13" viewBox="0 0 13 13" fill="none" stroke="currentColor" strokeWidth="1.4">
            <path d="M6.5 1v8M3 6l3.5 3.5L10 6"/>
            <path d="M1 11.5h11"/>
          </svg>
          {isImporting ? 'importing…' : 'Import folder'}
        </button>
      </div>
    </div>
  );
}

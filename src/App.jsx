import React, { useState, useEffect, useMemo } from 'react';
import './styles/global.css';
import { Sidebar } from './components/layout/Sidebar';
import { TopBar } from './components/layout/TopBar';
import { ResultsPanel } from './components/results/ResultsPanel';
import { SampleRow } from './components/results/SampleRow';
import { PlayerBar } from './components/player/PlayerBar';
import { useSearch } from './hooks/useSearch';
import { usePlayer } from './hooks/usePlayer';
import bridge from './bridge';

export default function App() {
  const [activeSection, setActiveSection] = useState('library');
  const [libraryCount, setLibraryCount] = useState(0);
  const [selectedPack, setSelectedPack] = useState(null);
  
  const search = useSearch();
  const player = usePlayer();

  useEffect(() => {
    search.loadAll().then(() => {
      bridge.send('getSamples').then(res => {
        if (res.ok) setLibraryCount(res.data.length);
      });
    });
  }, []);

  const handleSave = async (sample) => {
    const res = await bridge.send('addSample', { filePath: sample.filePath });
    if (res.ok) {
      search.loadAll().then(() => {
        bridge.send('getSamples').then(r => {
          if (r.ok) setLibraryCount(r.data.length);
        });
      });
    }
  };

  const handleUpload = async () => {
    const res = await bridge.send('browseForFiles', {});
    if (res.ok) {
      search.loadAll().then(() => {
        bridge.send('getSamples').then(r => {
          if (r.ok) setLibraryCount(r.data.length);
        });
      });
    }
    return res;
  };

  const handleSelectFolder = async () => {
    const res = await bridge.send('selectLibraryFolder', {});
    if (res.ok && res.data) {
      search.setLibraryResults(res.data);
      setLibraryCount(res.data.length);
      setSelectedPack(null);
    }
    return res;
  };

  const handleToggleFavorite = async (sample) => {
    const res = await bridge.send('toggleFavorite', { id: sample.id });
    if (res.ok) {
      // Toggle favorite state in search results so the UI updates immediately!
      search.setLibraryResults(prev => prev.map(s => {
        if (s.id === sample.id) {
          return { ...s, favorited: !s.favorited };
        }
        return s;
      }));
    }
  };

  // 1. Favorites view list
  const favoritesList = useMemo(() => {
    return search.libraryResults.filter(s => s.favorited);
  }, [search.libraryResults]);

  // 2. Recent view list (sorted by addition timestamp)
  const recentList = useMemo(() => {
    return [...search.libraryResults].sort((a, b) => b.addedAt - a.addedAt);
  }, [search.libraryResults]);

  // 3. Pack data grouping
  const packsData = useMemo(() => {
    const map = {};
    search.libraryResults.forEach(s => {
      const p = s.pack || 'Unsorted';
      if (!map[p]) map[p] = [];
      map[p].push(s);
    });
    return Object.entries(map).map(([name, files]) => ({
      name,
      count: files.length,
      files,
    }));
  }, [search.libraryResults]);

  // View content renderer
  const renderMainContent = () => {
    switch (activeSection) {
      case 'library':
        return (
          <ResultsPanel
            libraryResults={search.libraryResults}
            generatedResults={search.generatedResults}
            isSearching={search.isSearching}
            isGenerating={search.isGenerating}
            hasSearched={search.hasSearched}
            submittedQuery={search.submittedQuery}
            playingId={player.playing}
            onPlay={player.toggle}
            onSave={handleSave}
            onToggleFavorite={handleToggleFavorite}
          />
        );

      case 'packs':
        if (selectedPack) {
          const pack = packsData.find(p => p.name === selectedPack);
          const files = pack ? pack.files : [];
          return (
            <div style={{ flex: 1, display: 'flex', flexDirection: 'column', overflow: 'hidden' }}>
              <div style={{
                display: 'flex', alignItems: 'center', gap: '12px',
                padding: '16px', borderBottom: '1px solid var(--border-subtle)',
                background: 'var(--bg-surface)',
              }}>
                <button
                  onClick={() => setSelectedPack(null)}
                  style={{
                    fontSize: '11px', fontFamily: 'var(--font-mono)', color: 'var(--text-secondary)',
                    padding: '6px 12px', borderRadius: 'var(--r-sm)', border: '1px solid var(--border-default)',
                    background: 'var(--bg-elevated)', cursor: 'pointer', transition: 'all var(--t-base)',
                  }}
                  onMouseEnter={e => e.currentTarget.style.color = 'var(--text-primary)'}
                  onMouseLeave={e => e.currentTarget.style.color = 'var(--text-secondary)'}
                >
                  ← Back to Packs
                </button>
                <span style={{ fontSize: '13px', fontWeight: 600, color: 'var(--accent-amber)', fontFamily: 'var(--font-mono)' }}>
                  {selectedPack}
                </span>
                <span style={{ fontSize: '11px', color: 'var(--text-tertiary)', fontFamily: 'var(--font-mono)' }}>
                  ({files.length} sample{files.length !== 1 ? 's' : ''})
                </span>
              </div>
              <div style={{ flex: 1, overflowY: 'auto' }}>
                {files.length === 0 && (
                  <div style={{ padding: '60px 24px', textAlign: 'center', color: 'var(--text-tertiary)', fontSize: '12px', fontFamily: 'var(--font-mono)' }}>
                    no samples found
                  </div>
                )}
                {files.map(s => (
                  <SampleRow
                    key={s.id}
                    sample={s}
                    isPlaying={player.playing === s.id}
                    onPlay={player.toggle}
                    onSave={handleSave}
                    onToggleFavorite={handleToggleFavorite}
                  />
                ))}
              </div>
            </div>
          );
        }

        return (
          <div style={{ flex: 1, overflowY: 'auto', padding: '24px' }}>
            <div style={{
              fontSize: '11px', fontFamily: 'var(--font-mono)', letterSpacing: '0.1em',
              textTransform: 'uppercase', color: 'var(--text-tertiary)', marginBottom: '16px',
            }}>
              Sample Packs ({packsData.length})
            </div>
            <div style={{
              display: 'grid',
              gridTemplateColumns: 'repeat(auto-fill, minmax(180px, 1fr))',
              gap: '16px',
            }}>
              {packsData.map(pack => (
                <div
                  key={pack.name}
                  onClick={() => setSelectedPack(pack.name)}
                  style={{
                    background: 'var(--bg-surface)',
                    border: '1px solid var(--border-subtle)',
                    borderRadius: 'var(--r-md)',
                    padding: '16px',
                    cursor: 'pointer',
                    transition: 'all var(--t-base)',
                    display: 'flex',
                    flexDirection: 'column',
                    gap: '12px',
                  }}
                  onMouseEnter={e => {
                    e.currentTarget.style.border = '1px solid var(--accent-amber-border)';
                    e.currentTarget.style.background = 'var(--bg-elevated)';
                  }}
                  onMouseLeave={e => {
                    e.currentTarget.style.border = '1px solid var(--border-subtle)';
                    e.currentTarget.style.background = 'var(--bg-surface)';
                  }}
                >
                  <div style={{
                    width: 36, height: 36, borderRadius: 'var(--r-md)',
                    background: 'var(--accent-amber-glow)', color: 'var(--accent-amber)',
                    display: 'flex', alignItems: 'center', justifyContent: 'center',
                  }}>
                    <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                      <path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"/>
                    </svg>
                  </div>
                  <div>
                    <div style={{
                      fontSize: '13px', fontWeight: 500, color: 'var(--text-primary)',
                      whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis',
                      marginBottom: '4px',
                    }}>
                      {pack.name}
                    </div>
                    <div style={{ fontSize: '11px', color: 'var(--text-secondary)', fontFamily: 'var(--font-mono)' }}>
                      {pack.count} sample{pack.count !== 1 ? 's' : ''}
                    </div>
                  </div>
                </div>
              ))}
            </div>
          </div>
        );

      case 'favorites':
        return renderSinglePanel('Favorites', 'var(--accent-red)', favoritesList, false);

      case 'recent':
        return renderSinglePanel('Recent Additions', 'var(--accent-amber)', recentList, true);

      case 'generated':
        return renderSinglePanel('AI Generated Archive', 'var(--accent-gen)', search.generatedResults, false);

      default:
        return null;
    }
  };

  const renderSinglePanel = (title, accentColor, list, showAddHint) => {
    return (
      <div style={{ flex: 1, display: 'flex', flexDirection: 'column', overflow: 'hidden' }}>
        <div style={{
          display: 'flex', alignItems: 'center', gap: '10px',
          padding: '10px 16px', borderBottom: '1px solid var(--border-subtle)',
          background: 'var(--bg-surface)', position: 'sticky', top: 0, zIndex: 1,
        }}>
          <div style={{ width: 6, height: 6, borderRadius: '50%', background: accentColor, flexShrink: 0 }} />
          <span style={{
            fontSize: '10px', fontFamily: 'var(--font-mono)',
            letterSpacing: '0.1em', color: 'var(--text-tertiary)',
            textTransform: 'uppercase', flex: 1,
          }}>
            {title}
          </span>
          <span style={{ fontSize: '10px', fontFamily: 'var(--font-mono)', color: 'var(--text-tertiary)' }}>
            {list.length} sample{list.length !== 1 ? 's' : ''}
          </span>
        </div>
        <div style={{ flex: 1, overflowY: 'auto' }}>
          {list.length === 0 && (
            <div style={{ padding: '60px 24px', textAlign: 'center', color: 'var(--text-tertiary)', fontSize: '12px', fontFamily: 'var(--font-mono)' }}>
              {showAddHint ? 'no samples found' : 'nothing here yet'}
            </div>
          )}
          {list.map(s => (
            <SampleRow
              key={s.id}
              sample={s}
              isPlaying={player.playing === s.id}
              onPlay={player.toggle}
              onSave={handleSave}
              onToggleFavorite={handleToggleFavorite}
            />
          ))}
        </div>
      </div>
    );
  };

  return (
    <div style={{
      display: 'flex',
      height: '100vh',
      width: '100vw',
      overflow: 'hidden',
      background: 'var(--bg-base)',
    }}>
      <Sidebar
        activeSection={activeSection}
        onNavigate={(sect) => {
          setActiveSection(sect);
          setSelectedPack(null);
        }}
        libraryCount={libraryCount}
      />

      <div style={{
        flex: 1,
        display: 'flex',
        flexDirection: 'column',
        overflow: 'hidden',
        minWidth: 0,
      }}>
        <TopBar
          query={search.query}
          onChange={search.setQuery}
          onSearch={search.search}
          onClear={search.clear}
          hasSearched={search.hasSearched}
          submittedQuery={search.submittedQuery}
          onUpload={handleUpload}
          onSelectFolder={handleSelectFolder}
        />

        <div style={{ flex: 1, overflow: 'hidden', display: 'flex', flexDirection: 'column' }}>
          {renderMainContent()}
        </div>

        <PlayerBar
          sample={player.currentSample}
          isPlaying={player.playing !== null}
          onToggle={() => player.currentSample && player.toggle(player.currentSample)}
          onStop={player.stop}
        />
      </div>
    </div>
  );
}

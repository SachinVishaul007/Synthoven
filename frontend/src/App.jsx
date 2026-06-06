import React, { useState } from 'react';
import './styles/global.css';
import { Sidebar } from './components/layout/Sidebar';
import { TopBar } from './components/layout/TopBar';
import { ResultsPanel } from './components/results/ResultsPanel';
import { PlayerBar } from './components/player/PlayerBar';
import { useSearch } from './hooks/useSearch';
import { usePlayer } from './hooks/usePlayer';
import { api } from './api/client';
import { mapSamples } from './api/mapper';

export default function App() {
  const [activeSection, setActiveSection] = useState('library');
  const [isImporting, setIsImporting] = useState(false);
  const search = useSearch();
  const player = usePlayer();

  const handlePlay = (sample) => {
    player.toggle(sample);
  };

  const handleSave = (sample) => {
    // Generated samples are already persisted by the backend; "save" pins one
    // by favoriting it so it surfaces under the Favorites view.
    api.toggleFavorite(sample.id).catch(err =>
      console.error('Failed to save sample:', err.message),
    );
  };

  const handleImportFolder = async (audioFiles, folderName) => {
    if (!audioFiles || audioFiles.length === 0) {
      search.showSamples([], folderName || 'no audio files');
      return;
    }
    setIsImporting(true);
    try {
      const imported = await api.uploadFiles(audioFiles);
      const label = folderName
        ? `${folderName} (${imported.length}/${audioFiles.length} files)`
        : `${imported.length} files`;
      search.showSamples(mapSamples(imported), label);
    } catch (err) {
      console.error('Import failed:', err.message);
      search.showSamples([], `import failed — ${err.message}`);
    } finally {
      setIsImporting(false);
    }
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
        onNavigate={setActiveSection}
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
          onImportFolder={handleImportFolder}
          isImporting={isImporting}
        />

        {search.error && (
          <div style={{
            padding: '8px 20px',
            fontSize: '11px',
            fontFamily: 'var(--font-mono)',
            color: '#ff6b6b',
            background: 'rgba(255,107,107,0.08)',
            borderBottom: '1px solid rgba(255,107,107,0.25)',
          }}>
            ⚠ {search.error}
          </div>
        )}

        <div style={{ flex: 1, overflow: 'hidden', display: 'flex', flexDirection: 'column' }}>
          <ResultsPanel
            libraryResults={search.libraryResults}
            generatedResults={search.generatedResults}
            isSearching={search.isSearching}
            isGenerating={search.isGenerating}
            hasSearched={search.hasSearched}
            submittedQuery={search.submittedQuery}
            mode={search.mode}
            playingId={player.playing}
            onPlay={handlePlay}
            onSave={handleSave}
          />
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
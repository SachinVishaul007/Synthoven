import React, { useState, useEffect } from 'react';
import './styles/global.css';
import { Sidebar } from './components/layout/Sidebar';
import { TopBar } from './components/layout/TopBar';
import { ResultsPanel } from './components/results/ResultsPanel';
import { PlayerBar } from './components/player/PlayerBar';
import { useSearch } from './hooks/useSearch';
import { usePlayer } from './hooks/usePlayer';
import bridge from './bridge';

export default function App() {
  const [activeSection, setActiveSection] = useState('library');
  const [libraryCount, setLibraryCount] = useState(0);
  const search = useSearch();
  const player = usePlayer();

  useEffect(() => {
    bridge.send('getSamples').then(res => {
      if (res.ok) setLibraryCount(res.data.length);
    });
  }, []);

  const handleSave = async (sample) => {
    const res = await bridge.send('addSample', { filePath: sample.filePath });
    if (res.ok) setLibraryCount(c => c + 1);
  };

  const handleUpload = async () => {
    const res = await bridge.send('browseForFiles', {});
    if (res.ok) setLibraryCount(c => c + (res.data?.length ?? 0));
    return res;
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
        />

        <div style={{ flex: 1, overflow: 'hidden', display: 'flex', flexDirection: 'column' }}>
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

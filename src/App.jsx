import React, { useState } from 'react';
import './styles/global.css';
import { Sidebar } from './components/layout/Sidebar';
import { TopBar } from './components/layout/TopBar';
import { ResultsPanel } from './components/results/ResultsPanel';
import { PlayerBar } from './components/player/PlayerBar';
import { useSearch } from './hooks/useSearch';
import { usePlayer } from './hooks/usePlayer';

export default function App() {
  const [activeSection, setActiveSection] = useState('library');
  const search = useSearch();
  const player = usePlayer();

  const handlePlay = (sample) => {
    player.toggle(sample);
  };

  const handleSave = (sample) => {
    // In production: POST to API, add to library
    console.log('Saving generated sample to library:', sample.id);
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
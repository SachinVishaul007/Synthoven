import { useState, useCallback, useRef } from 'react';
import { MOCK_LIBRARY_SAMPLES, MOCK_GENERATED_SAMPLES } from '../data/samples';

const GENERATION_DELAY = 2200;

export function useSearch() {
  const [query, setQuery] = useState('');
  const [submittedQuery, setSubmittedQuery] = useState('');
  const [libraryResults, setLibraryResults] = useState([]);
  const [generatedResults, setGeneratedResults] = useState([]);
  const [isSearching, setIsSearching] = useState(false);
  const [isGenerating, setIsGenerating] = useState(false);
  const [hasSearched, setHasSearched] = useState(false);

  const generationTimer = useRef(null);

  const search = useCallback((q) => {
    if (!q.trim()) return;

    const trimmed = q.trim().toLowerCase();
    setSubmittedQuery(q.trim());
    setHasSearched(true);
    setIsSearching(true);
    setIsGenerating(true);
    setLibraryResults([]);
    setGeneratedResults([]);

    if (generationTimer.current) clearTimeout(generationTimer.current);

    // Library: near-instant
    setTimeout(() => {
      const filtered = MOCK_LIBRARY_SAMPLES.filter(s =>
        s.tags.some(t => trimmed.split(' ').some(w => t.includes(w))) ||
        s.name.toLowerCase().includes(trimmed)
      );
      setLibraryResults(filtered);
      setIsSearching(false);
    }, 320);

    // Generated: simulated AI latency
    generationTimer.current = setTimeout(() => {
      setGeneratedResults(MOCK_GENERATED_SAMPLES);
      setIsGenerating(false);
    }, GENERATION_DELAY);
  }, []);

  const clear = useCallback(() => {
    setQuery('');
    setSubmittedQuery('');
    setLibraryResults([]);
    setGeneratedResults([]);
    setHasSearched(false);
    setIsSearching(false);
    setIsGenerating(false);
    if (generationTimer.current) clearTimeout(generationTimer.current);
  }, []);

  return {
    query, setQuery,
    submittedQuery,
    libraryResults, generatedResults,
    isSearching, isGenerating,
    hasSearched,
    search, clear,
  };
}
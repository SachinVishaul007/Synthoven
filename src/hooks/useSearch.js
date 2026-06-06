import { useState, useCallback, useRef } from 'react';
import { MOCK_GENERATED_SAMPLES } from '../data/samples';
import bridge from '../bridge';

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

  const loadAll = useCallback(async () => {
    setIsSearching(true);
    setHasSearched(true);
    const res = await bridge.send('getSamples');
    setLibraryResults(res.ok ? res.data : []);
    setIsSearching(false);
  }, []);

  const search = useCallback(async (q) => {
    if (!q.trim()) return;

    setSubmittedQuery(q.trim());
    setHasSearched(true);
    setIsSearching(true);
    setIsGenerating(true);
    setLibraryResults([]);
    setGeneratedResults([]);

    if (generationTimer.current) clearTimeout(generationTimer.current);

    const res = await bridge.send('search', { query: q.trim() });
    setLibraryResults(res.ok ? res.data : []);
    setIsSearching(false);

    // AI generation wired in a later phase — mock for now
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
    loadAll, search, clear,
  };
}

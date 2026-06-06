import { useState, useCallback, useRef } from 'react';
import { api } from '../api/client';
import { mapSamples } from '../api/mapper';

const POLL_INTERVAL_MS = 2000;
const MAX_POLL_ATTEMPTS = 40; // ~80s ceiling before we give up on a job

export function useSearch() {
  const [query, setQuery] = useState('');
  const [submittedQuery, setSubmittedQuery] = useState('');
  const [libraryResults, setLibraryResults] = useState([]);
  const [generatedResults, setGeneratedResults] = useState([]);
  const [isSearching, setIsSearching] = useState(false);
  const [isGenerating, setIsGenerating] = useState(false);
  const [hasSearched, setHasSearched] = useState(false);
  const [error, setError] = useState(null);
  // 'search' (two panels: library + AI) or 'library' (single list, e.g. imports)
  const [mode, setMode] = useState('search');

  // Bumped on every new search/clear so stale in-flight requests and polls can
  // detect they've been superseded and bail out instead of clobbering state.
  const searchSeq = useRef(0);
  const pollTimer = useRef(null);

  const stopPolling = useCallback(() => {
    if (pollTimer.current) {
      clearTimeout(pollTimer.current);
      pollTimer.current = null;
    }
  }, []);

  const pollJob = useCallback((jobId, seq, attempt = 0) => {
    if (seq !== searchSeq.current) return;

    api.getGenerationJob(jobId)
      .then(job => {
        if (seq !== searchSeq.current) return;

        if (job.status === 'complete') {
          setGeneratedResults(mapSamples(job.samples));
          setIsGenerating(false);
        } else if (job.status === 'failed') {
          setError(job.error || 'Generation failed');
          setIsGenerating(false);
        } else if (attempt >= MAX_POLL_ATTEMPTS) {
          setError('Generation timed out');
          setIsGenerating(false);
        } else {
          pollTimer.current = setTimeout(
            () => pollJob(jobId, seq, attempt + 1),
            POLL_INTERVAL_MS,
          );
        }
      })
      .catch(err => {
        if (seq !== searchSeq.current) return;
        setError(err.message);
        setIsGenerating(false);
      });
  }, []);

  const search = useCallback((q) => {
    if (!q.trim()) return;
    const trimmed = q.trim();
    const seq = ++searchSeq.current;
    stopPolling();

    setMode('search');
    setSubmittedQuery(trimmed);
    setHasSearched(true);
    setIsSearching(true);
    setIsGenerating(true);
    setLibraryResults([]);
    setGeneratedResults([]);
    setError(null);

    api.search(trimmed, true)
      .then(res => {
        if (seq !== searchSeq.current) return;

        setLibraryResults(mapSamples(res.libraryResults));
        setIsSearching(false);

        if (res.generationJobId) {
          pollJob(res.generationJobId, seq);
        } else {
          setIsGenerating(false);
        }
      })
      .catch(err => {
        if (seq !== searchSeq.current) return;
        setError(err.message);
        setIsSearching(false);
        setIsGenerating(false);
      });
  }, [pollJob, stopPolling]);

  /** Display a ready-made list of samples (e.g. freshly imported files). */
  const showSamples = useCallback((samples, label = '') => {
    searchSeq.current++; // invalidate any in-flight request/poll
    stopPolling();
    setMode('library');
    setSubmittedQuery(label);
    setHasSearched(true);
    setIsSearching(false);
    setIsGenerating(false);
    setGeneratedResults([]);
    setError(null);
    setLibraryResults(samples);
  }, [stopPolling]);

  const clear = useCallback(() => {
    searchSeq.current++; // invalidate any in-flight request/poll
    stopPolling();
    setMode('search');
    setQuery('');
    setSubmittedQuery('');
    setLibraryResults([]);
    setGeneratedResults([]);
    setHasSearched(false);
    setIsSearching(false);
    setIsGenerating(false);
    setError(null);
  }, [stopPolling]);

  return {
    query, setQuery,
    submittedQuery,
    libraryResults, generatedResults,
    isSearching, isGenerating,
    hasSearched, error, mode,
    search, clear, showSamples,
  };
}

import { useState, useCallback, useRef, useEffect } from 'react';
import { api } from '../api/client';

export function usePlayer() {
  const [playing, setPlaying] = useState(null);
  const [currentSample, setCurrentSample] = useState(null);
  const audioRef = useRef(null);

  // One reusable <audio> element for the whole app session.
  if (!audioRef.current && typeof Audio !== 'undefined') {
    audioRef.current = new Audio();
  }

  useEffect(() => {
    const audio = audioRef.current;
    if (!audio) return;
    const onEnded = () => setPlaying(null);
    const onError = () => setPlaying(null);
    audio.addEventListener('ended', onEnded);
    audio.addEventListener('error', onError);
    return () => {
      audio.removeEventListener('ended', onEnded);
      audio.removeEventListener('error', onError);
    };
  }, []);

  const play = useCallback((sample) => {
    const audio = audioRef.current;
    setCurrentSample(sample);
    setPlaying(sample.id);

    if (audio) {
      const url = api.streamUrl(sample.id);
      // Only reload (and re-record a play) when switching samples; otherwise
      // resume the already-loaded one.
      if (!audio.src.endsWith(url)) {
        audio.src = url;
        api.recordPlay(sample.id).catch(() => { /* best-effort */ });
      }
      audio.play().catch(() => setPlaying(null));
    }
  }, []);

  const stop = useCallback(() => {
    const audio = audioRef.current;
    if (audio) audio.pause();
    setPlaying(null);
  }, []);

  const toggle = useCallback((sample) => {
    if (playing === sample.id) { stop(); }
    else { play(sample); }
  }, [playing, play, stop]);

  const isPlaying = useCallback((id) => playing === id, [playing]);

  return { playing, currentSample, play, stop, toggle, isPlaying };
}

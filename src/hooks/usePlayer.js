import { useState, useCallback } from 'react';

export function usePlayer() {
  const [playing, setPlaying] = useState(null);
  const [currentSample, setCurrentSample] = useState(null);

  const play = useCallback((sample) => {
    setPlaying(sample.id);
    setCurrentSample(sample);
    // TODO: trigger Web Audio API here
  }, []);

  const stop = useCallback(() => {
    setPlaying(null);
    // TODO: stop Web Audio API here
  }, []);

  const toggle = useCallback((sample) => {
    if (playing === sample.id) { stop(); }
    else { play(sample); }
  }, [playing, play, stop]);

  const isPlaying = useCallback((id) => playing === id, [playing]);

  return { playing, currentSample, play, stop, toggle, isPlaying };
}
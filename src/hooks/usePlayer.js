import { useState, useCallback } from 'react';
import bridge from '../bridge';

export function usePlayer() {
  const [playing, setPlaying] = useState(null);
  const [currentSample, setCurrentSample] = useState(null);

  const play = useCallback(async (sample) => {
    setPlaying(sample.id);
    setCurrentSample(sample);
    await bridge.send('play', { id: sample.id });
  }, []);

  const stop = useCallback(async () => {
    setPlaying(null);
    await bridge.send('stop');
  }, []);

  const toggle = useCallback((sample) => {
    if (playing === sample.id) stop();
    else play(sample);
  }, [playing, play, stop]);

  return { playing, currentSample, play, stop, toggle };
}

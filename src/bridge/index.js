import { MOCK_LIBRARY_SAMPLES, MOCK_GENERATED_SAMPLES } from '../data/samples';

const inJUCE = () => typeof window.__JUCE__ !== 'undefined' &&
                     typeof window.__JUCE__.postMessage === 'function';

function callNative(name, ...args) {
  return new Promise((resolve) => {
    const resultId = Math.floor(Math.random() * Number.MAX_SAFE_INTEGER);

    const token = window.__JUCE__.backend.addEventListener('__juce__complete', (event) => {
      if (event.promiseId === resultId) {
        window.__JUCE__.backend.removeEventListener(token);
        resolve(event.result);
      }
    });

    window.__JUCE__.backend.emitEvent('__juce__invoke', {
      name,
      params: args,
      resultId,
    });
  });
}

async function send(action, payload = {}) {
  if (inJUCE()) {
    try {
      return await callNative('chopSend', action, payload);
    } catch (e) {
      console.warn('[bridge] native call failed:', action, e);
    }
  }
  return devFallback(action, payload);
}

function devFallback(action, payload) {
  switch (action) {
    case 'getSamples':
      return { ok: true, data: MOCK_LIBRARY_SAMPLES };

    case 'search': {
      const q = (payload.query ?? '').toLowerCase().trim();
      const words = q.split(' ').filter(Boolean);
      const results = MOCK_LIBRARY_SAMPLES.filter(s =>
        words.some(w =>
          s.name.toLowerCase().includes(w) ||
          s.pack.toLowerCase().includes(w) ||
          s.tags.some(t => t.includes(w))
        )
      );
      return { ok: true, data: results };
    }

    case 'browseForFiles':
      return { ok: false, error: 'File browser only available inside the plugin' };

    case 'play':
    case 'stop':
    case 'addSample':
    case 'deleteSample':
    case 'updateSample':
    case 'toggleFavorite':
      return { ok: true };

    case 'getPlaybackState':
      return { ok: true, data: { isPlaying: false, position: 0, duration: 0 } };

    default:
      return { ok: false, error: `No dev fallback for action: ${action}` };
  }
}

export default { send, inJUCE };

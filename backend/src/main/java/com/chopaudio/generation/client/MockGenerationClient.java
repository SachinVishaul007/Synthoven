package com.chopaudio.generation.client;

import com.chopaudio.library.model.Sample;
import com.chopaudio.library.model.SampleType;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Component;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.ThreadLocalRandom;

/**
 * Offline stand-in for a real generation provider. Produces placeholder samples
 * with plausible metadata so the full search → generate → library flow can be
 * exercised without any API key.
 *
 * <p>Selected by {@link com.chopaudio.generation.service.GenerationService} when
 * {@code chop.generation.provider=mock} (the default).
 */
@Component
@Slf4j
public class MockGenerationClient {

    public String provider() {
        return "mock";
    }

    /**
     * Generate {@code count} placeholder samples for the prompt. Blocking —
     * callers run this on an async executor.
     */
    public List<Sample> generate(String prompt, int count) {
        log.info("[mock] Generating {} sample(s) for prompt: {}", count, prompt);

        // Simulate provider latency so polling behaves realistically.
        try {
            Thread.sleep(1500);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }

        List<Sample> results = new ArrayList<>();
        for (int i = 0; i < count; i++) {
            String id = UUID.randomUUID().toString().substring(0, 8);
            results.add(Sample.builder()
                    .name(slug(prompt) + "_" + (i + 1))
                    .filePath("mock://generated/" + id + ".wav")
                    .type(SampleType.GENERATED)
                    .format("wav")
                    .durationMs(ThreadLocalRandom.current().nextLong(1000, 8000))
                    .sizeBytes(ThreadLocalRandom.current().nextLong(200_000, 2_000_000))
                    .sampleRate(44100)
                    .channels(2)
                    .bpm(ThreadLocalRandom.current().nextInt(80, 160))
                    .tags(deriveTags(prompt))
                    .prompt(prompt)
                    .provider(provider())
                    .build());
        }
        return results;
    }

    private Set<String> deriveTags(String prompt) {
        Set<String> tags = new HashSet<>();
        for (String token : prompt.toLowerCase().split("[\\s_\\-.]+")) {
            if (token.length() >= 3) {
                tags.add(token);
            }
        }
        tags.addAll(Arrays.asList("generated", "mock"));
        return tags;
    }

    private String slug(String prompt) {
        String s = prompt.toLowerCase().replaceAll("[^a-z0-9]+", "_").replaceAll("^_+|_+$", "");
        return s.isEmpty() ? "sample" : s.substring(0, Math.min(s.length(), 40));
    }
}

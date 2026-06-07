package com.chopaudio.generation.client;

import com.chopaudio.generation.service.GeneratedFileStore;
import com.chopaudio.library.model.Sample;
import com.chopaudio.library.model.SampleType;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.MediaType;
import org.springframework.http.client.MultipartBodyBuilder;
import org.springframework.stereotype.Component;
import org.springframework.web.reactive.function.BodyInserters;
import org.springframework.web.reactive.function.client.WebClient;

import java.nio.file.Path;
import java.time.Duration;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Real generation provider backed by Stability AI's Stable Audio text-to-audio
 * API. Unlike Suno's poll-then-fetch flow, Stable Audio returns the rendered
 * audio bytes synchronously in the HTTP response, so each request maps directly
 * to one finished sample written to disk by {@link GeneratedFileStore}.
 *
 * <p>The endpoint path, model, duration, step count and output format are all
 * configured via {@code chop.generation.stableaudio.*} so the same client can
 * target Stable Audio 2 / 2.5 / 3 (or a partner gateway) without code changes.
 * Selected by {@link com.chopaudio.generation.service.GenerationService} when
 * {@code chop.generation.provider=stableaudio}.
 */
@Component
@Slf4j
public class StableAudioClient {

    private final WebClient webClient;
    private final GeneratedFileStore fileStore;
    private final String endpointPath;
    private final int durationSeconds;
    private final int steps;
    private final double cfgScale;
    private final String outputFormat;
    private final long requestTimeoutMs;

    public StableAudioClient(@Qualifier("stableAudioWebClient") WebClient webClient,
                             GeneratedFileStore fileStore,
                             @Value("${chop.generation.stableaudio.endpoint-path}") String endpointPath,
                             @Value("${chop.generation.stableaudio.duration-seconds}") int durationSeconds,
                             @Value("${chop.generation.stableaudio.steps}") int steps,
                             @Value("${chop.generation.stableaudio.cfg-scale}") double cfgScale,
                             @Value("${chop.generation.stableaudio.output-format}") String outputFormat,
                             @Value("${chop.generation.stableaudio.request-timeout-ms}") long requestTimeoutMs) {
        this.webClient = webClient;
        this.fileStore = fileStore;
        this.endpointPath = endpointPath;
        this.durationSeconds = durationSeconds;
        this.steps = steps;
        this.cfgScale = cfgScale;
        this.outputFormat = outputFormat;
        this.requestTimeoutMs = requestTimeoutMs;
    }

    public String provider() {
        return "stableaudio";
    }

    public List<Sample> generate(String prompt, int count) {
        return generate(prompt, count, null);
    }

    /**
     * Generate {@code count} samples for the prompt. Each is a separate request
     * (Stable Audio returns one clip per call); a failure on any single request
     * aborts the job. Blocking — callers run this on an async executor.
     */
    public List<Sample> generate(String prompt, int count, String initAudioPath) {
        log.info("[stableaudio] Generating {} sample(s) for prompt: {} (initAudioPath: {})", count, prompt, initAudioPath);

        List<Sample> samples = new ArrayList<>();
        for (int i = 0; i < count; i++) {
            byte[] audio = requestAudio(prompt, initAudioPath);
            if (audio == null || audio.length == 0) {
                throw new IllegalStateException("Stable Audio returned no audio bytes");
            }

            String name = slug(prompt) + "_" + (i + 1);
            Path saved;
            try {
                saved = fileStore.save(audio, name, outputFormat);
            } catch (Exception e) {
                throw new IllegalStateException("Could not save generated audio: " + e.getMessage(), e);
            }

            samples.add(Sample.builder()
                    .name(name)
                    .filePath(saved.toString())
                    .type(SampleType.GENERATED)
                    .format(outputFormat)
                    .durationMs((long) durationSeconds * 1000)
                    .sizeBytes((long) audio.length)
                    .tags(deriveTags(prompt))
                    .prompt(prompt)
                    .provider(provider())
                    .build());
        }
        return samples;
    }

    private byte[] requestAudio(String prompt, String initAudioPath) {
        MultipartBodyBuilder body = new MultipartBodyBuilder();
        body.part("prompt", prompt);
        body.part("duration", String.valueOf(durationSeconds));
        body.part("output_format", outputFormat);
        body.part("steps", String.valueOf(steps));
        body.part("cfg_scale", String.valueOf(cfgScale));

        String uriPath = endpointPath;
        if (initAudioPath != null && !initAudioPath.isBlank()) {
            body.part("audio", new org.springframework.core.io.FileSystemResource(Path.of(initAudioPath)));
            body.part("audio_strength", "0.7");
            if (uriPath.contains("text-to-audio")) {
                uriPath = uriPath.replace("text-to-audio", "audio-to-audio");
            }
        }

        return webClient.post()
                .uri(uriPath)
                .accept(MediaType.parseMediaType("audio/*"))
                .contentType(MediaType.MULTIPART_FORM_DATA)
                .body(BodyInserters.fromMultipartData(body.build()))
                .retrieve()
                .bodyToMono(byte[].class)
                .block(Duration.ofMillis(requestTimeoutMs));
    }

    private Set<String> deriveTags(String prompt) {
        Set<String> tags = new HashSet<>();
        for (String token : prompt.toLowerCase().split("[\\s_\\-.]+")) {
            if (token.length() >= 3) {
                tags.add(token);
            }
        }
        tags.add("generated");
        tags.add("stableaudio");
        return tags;
    }

    private String slug(String prompt) {
        String s = prompt.toLowerCase().replaceAll("[^a-z0-9]+", "_").replaceAll("^_+|_+$", "");
        return s.isEmpty() ? "sample" : s.substring(0, Math.min(s.length(), 40));
    }
}

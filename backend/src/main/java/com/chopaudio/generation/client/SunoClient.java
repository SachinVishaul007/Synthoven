package com.chopaudio.generation.client;

import com.chopaudio.library.model.Sample;
import com.chopaudio.library.model.SampleType;
import com.fasterxml.jackson.databind.JsonNode;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;
import org.springframework.web.reactive.function.client.WebClient;

import java.time.Duration;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Real generation provider backed by the Suno API. Submits a prompt, polls the
 * job until the clips are ready, then maps each returned clip to a GENERATED
 * {@link Sample} pointing at its hosted audio URL.
 *
 * <p>Endpoint shapes are intentionally defensive (multiple field-name fallbacks)
 * so the client degrades gracefully if the provider schema shifts. Selected by
 * {@link com.chopaudio.generation.service.GenerationService} only when
 * {@code chop.generation.provider=suno}.
 */
@Component
@Slf4j
public class SunoClient {

    private final WebClient webClient;
    private final long pollIntervalMs;
    private final int maxPollAttempts;

    public SunoClient(@Qualifier("sunoWebClient") WebClient webClient,
                      @Value("${chop.generation.suno.poll-interval-ms}") long pollIntervalMs,
                      @Value("${chop.generation.suno.max-poll-attempts}") int maxPollAttempts) {
        this.webClient = webClient;
        this.pollIntervalMs = pollIntervalMs;
        this.maxPollAttempts = maxPollAttempts;
    }

    public String provider() {
        return "suno";
    }

    public List<Sample> generate(String prompt, int count) {
        log.info("[suno] Submitting generation for prompt: {}", prompt);

        Map<String, Object> body = new HashMap<>();
        body.put("prompt", prompt);
        body.put("make_instrumental", true);

        JsonNode submission = webClient.post()
                .uri("/v1/generate")
                .bodyValue(body)
                .retrieve()
                .bodyToMono(JsonNode.class)
                .block(Duration.ofSeconds(30));

        String jobId = extractJobId(submission);
        if (jobId == null) {
            throw new IllegalStateException("Suno did not return a job id");
        }

        JsonNode completed = pollUntilComplete(jobId);
        return mapClips(completed, prompt, count);
    }

    private JsonNode pollUntilComplete(String jobId) {
        for (int attempt = 0; attempt < maxPollAttempts; attempt++) {
            sleep(pollIntervalMs);
            JsonNode status = webClient.get()
                    .uri("/v1/generate/{id}", jobId)
                    .retrieve()
                    .bodyToMono(JsonNode.class)
                    .block(Duration.ofSeconds(30));

            String state = status != null && status.hasNonNull("status")
                    ? status.get("status").asText()
                    : "";
            if (state.equalsIgnoreCase("complete") || state.equalsIgnoreCase("succeeded")) {
                return status;
            }
            if (state.equalsIgnoreCase("failed") || state.equalsIgnoreCase("error")) {
                throw new IllegalStateException("Suno generation failed for job " + jobId);
            }
            log.debug("[suno] job {} still {} (attempt {}/{})", jobId, state, attempt + 1, maxPollAttempts);
        }
        throw new IllegalStateException("Suno generation timed out for job " + jobId);
    }

    private List<Sample> mapClips(JsonNode root, String prompt, int count) {
        JsonNode clips = root.has("clips") ? root.get("clips") : root.get("data");
        List<Sample> samples = new ArrayList<>();
        if (clips != null && clips.isArray()) {
            for (JsonNode clip : clips) {
                if (samples.size() >= count) {
                    break;
                }
                String audioUrl = firstNonNull(clip, "audio_url", "audioUrl", "url");
                if (audioUrl == null) {
                    continue;
                }
                samples.add(Sample.builder()
                        .name(firstNonNull(clip, "title", "name") != null
                                ? firstNonNull(clip, "title", "name")
                                : prompt)
                        .filePath(audioUrl)
                        .type(SampleType.GENERATED)
                        .format("mp3")
                        .tags(deriveTags(prompt))
                        .prompt(prompt)
                        .provider(provider())
                        .build());
            }
        }
        if (samples.isEmpty()) {
            throw new IllegalStateException("Suno returned no playable clips");
        }
        return samples;
    }

    private String extractJobId(JsonNode node) {
        if (node == null) {
            return null;
        }
        String direct = firstNonNull(node, "id", "job_id", "jobId");
        if (direct != null) {
            return direct;
        }
        // Some responses wrap the job under "data".
        if (node.has("data")) {
            return firstNonNull(node.get("data"), "id", "job_id", "jobId");
        }
        return null;
    }

    private String firstNonNull(JsonNode node, String... fields) {
        if (node == null) {
            return null;
        }
        for (String field : fields) {
            if (node.hasNonNull(field)) {
                return node.get(field).asText();
            }
        }
        return null;
    }

    private Set<String> deriveTags(String prompt) {
        Set<String> tags = new HashSet<>();
        for (String token : prompt.toLowerCase().split("[\\s_\\-.]+")) {
            if (token.length() >= 3) {
                tags.add(token);
            }
        }
        tags.add("generated");
        tags.add("suno");
        return tags;
    }

    private void sleep(long ms) {
        try {
            Thread.sleep(ms);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }
}

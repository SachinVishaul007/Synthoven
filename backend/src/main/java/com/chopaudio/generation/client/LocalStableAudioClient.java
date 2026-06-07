package com.chopaudio.generation.client;

import com.chopaudio.generation.service.GeneratedFileStore;
import com.chopaudio.library.model.Sample;
import com.chopaudio.library.model.SampleType;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.TimeUnit;

/**
 * Generation provider that runs the open-source Stable Audio 3 model locally via
 * its {@code stable-audio} CLI (no API key, offline). Each request shells out to
 *
 * <pre>uv run --directory &lt;repo&gt; stable-audio --model &lt;model&gt; -p &lt;prompt&gt;
 *   --duration &lt;n&gt; --steps &lt;n&gt; --cfg-scale &lt;n&gt; -o &lt;generated-folder&gt;/file.wav</pre>
 *
 * so the CLI writes the WAV straight into the generated folder. Selected by
 * {@link com.chopaudio.generation.service.GenerationService} when
 * {@code chop.generation.provider=localstableaudio}.
 *
 * <p>The Small-Music / Small-SFX weights are gated on HuggingFace: accept the
 * model licence once and provide an HF token (env {@code HF_TOKEN} or
 * {@code chop.generation.localstableaudio.hf-token}).
 */
@Component
@Slf4j
public class LocalStableAudioClient {

    private final GeneratedFileStore fileStore;
    private final String uvPath;
    private final String repoDir;
    private final String model;
    private final int durationSeconds;
    private final int steps;
    private final double cfgScale;
    private final String device;
    private final String hfToken;
    private final long timeoutMs;

    public LocalStableAudioClient(
            GeneratedFileStore fileStore,
            @Value("${chop.generation.localstableaudio.uv-path}") String uvPath,
            @Value("${chop.generation.localstableaudio.repo-dir}") String repoDir,
            @Value("${chop.generation.localstableaudio.model}") String model,
            @Value("${chop.generation.localstableaudio.duration-seconds}") int durationSeconds,
            @Value("${chop.generation.localstableaudio.steps}") int steps,
            @Value("${chop.generation.localstableaudio.cfg-scale}") double cfgScale,
            @Value("${chop.generation.localstableaudio.device}") String device,
            @Value("${chop.generation.localstableaudio.hf-token}") String hfToken,
            @Value("${chop.generation.localstableaudio.timeout-ms}") long timeoutMs) {
        this.fileStore = fileStore;
        this.uvPath = expandHome(uvPath);
        this.repoDir = expandHome(repoDir);
        this.model = model;
        this.durationSeconds = durationSeconds;
        this.steps = steps;
        this.cfgScale = cfgScale;
        this.device = device;
        this.hfToken = hfToken;
        this.timeoutMs = timeoutMs;
    }

    public String provider() {
        return "localstableaudio";
    }

    public List<Sample> generate(String prompt, int count) {
        return generate(prompt, count, null, null);
    }

    /**
     * @param requestedDuration desired length in seconds (configured default when null)
     * @param requestedCfg      CFG / "creativity" scale (configured default when null)
     */
    public List<Sample> generate(String prompt, int count, Double requestedDuration, Double requestedCfg) {
        double dur = requestedDuration != null && requestedDuration > 0 ? requestedDuration : durationSeconds;
        double cfg = requestedCfg != null && requestedCfg > 0 ? requestedCfg : cfgScale;
        log.info("[local-sa3] Generating {} sample(s) (duration={}s, cfg={}) for prompt: {}",
                count, dur, cfg, prompt);

        List<Sample> samples = new ArrayList<>();
        for (int i = 0; i < count; i++) {
            String name = slug(prompt) + "_" + (i + 1);
            Path out;
            try {
                out = fileStore.reserve(name, "wav");
            } catch (Exception e) {
                throw new IllegalStateException("Could not prepare output path: " + e.getMessage(), e);
            }

            runCli(prompt, out, dur, cfg);

            if (!Files.isReadable(out)) {
                throw new IllegalStateException("stable-audio CLI did not produce " + out);
            }

            long size;
            try {
                size = Files.size(out);
            } catch (Exception e) {
                size = 0;
            }

            samples.add(Sample.builder()
                    .name(name)
                    .filePath(out.toString())
                    .type(SampleType.GENERATED)
                    .format("wav")
                    .durationMs((long) (dur * 1000))
                    .sizeBytes(size)
                    .sampleRate(44100)
                    .channels(2)
                    .tags(deriveTags(prompt))
                    .prompt(prompt)
                    .provider(provider())
                    .build());
        }
        return samples;
    }

    private void runCli(String prompt, Path out, double dur, double cfg) {
        List<String> cmd = new ArrayList<>(List.of(
                uvPath, "run", "--directory", repoDir,
                "stable-audio",
                "--model", model,
                "-p", prompt,
                "--duration", String.valueOf(dur),
                "--steps", String.valueOf(steps),
                "--cfg-scale", String.valueOf(cfg),
                "-o", out.toString()));
        if (device != null && !device.isBlank()) {
            cmd.add("--device");
            cmd.add(device);
        }

        ProcessBuilder pb = new ProcessBuilder(cmd);
        pb.redirectErrorStream(true);
        if (hfToken != null && !hfToken.isBlank()) {
            pb.environment().put("HF_TOKEN", hfToken);
            pb.environment().put("HUGGING_FACE_HUB_TOKEN", hfToken);
        }

        try {
            Process process = pb.start();
            StringBuilder tail = new StringBuilder();
            try (BufferedReader reader = new BufferedReader(
                    new InputStreamReader(process.getInputStream(), StandardCharsets.UTF_8))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    log.debug("[local-sa3] {}", line);
                    tail.append(line).append('\n');
                    if (tail.length() > 4000) {
                        tail.delete(0, tail.length() - 4000);
                    }
                }
            }
            boolean finished = process.waitFor(timeoutMs, TimeUnit.MILLISECONDS);
            if (!finished) {
                process.destroyForcibly();
                throw new IllegalStateException("stable-audio CLI timed out after " + timeoutMs + "ms");
            }
            if (process.exitValue() != 0) {
                throw new IllegalStateException("stable-audio CLI failed (exit "
                        + process.exitValue() + "):\n" + tail);
            }
        } catch (IllegalStateException e) {
            throw e;
        } catch (Exception e) {
            throw new IllegalStateException("Could not run stable-audio CLI: " + e.getMessage(), e);
        }
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

    private static String expandHome(String path) {
        if (path != null && path.startsWith("~")) {
            return System.getProperty("user.home") + path.substring(1);
        }
        return path;
    }
}

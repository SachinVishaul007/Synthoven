package com.chopaudio.generation.service;

import com.chopaudio.library.model.Sample;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

import java.io.InputStream;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;
import java.time.Duration;
import java.util.Locale;
import java.util.UUID;

/**
 * Owns the local "generated samples" folder ({@code chop.generation.output-path},
 * default {@code ~/Music/Chop/Generated}) and writes generated audio into it so
 * every generated sample lives on disk — streamable by the player without a
 * redirect and draggable straight into a DAW. The folder is created on demand.
 *
 * <p>Two entry points: {@link #save(byte[], String, String)} for providers that
 * return raw audio bytes (Stable Audio), and {@link #localize(Sample)} for
 * providers that return a hosted URL (Suno).
 */
@Component
@Slf4j
public class GeneratedFileStore {

    private final Path outputDir;
    private final HttpClient http = HttpClient.newBuilder()
            .followRedirects(HttpClient.Redirect.NORMAL)
            .connectTimeout(Duration.ofSeconds(20))
            .build();

    public GeneratedFileStore(@Value("${chop.generation.output-path}") String outputPath) {
        this.outputDir = Paths.get(expandHome(outputPath)).toAbsolutePath().normalize();
    }

    public Path outputDir() {
        return outputDir;
    }

    /** Write audio bytes into the generated folder and return the file path. */
    public Path save(byte[] audio, String name, String ext) throws Exception {
        Path dest = reserve(name, ext);
        Files.write(dest, audio);
        log.info("[generated] saved '{}' ({} bytes) -> {}", name, audio.length, dest);
        return dest;
    }

    /**
     * Ensure the generated folder exists and return a fresh unique target path
     * inside it (without writing). For providers like a local CLI that write the
     * file themselves via an output-path flag.
     */
    public Path reserve(String name, String ext) throws Exception {
        Files.createDirectories(outputDir);
        return outputDir.resolve(buildFileName(name, ext));
    }

    /**
     * If the sample points at a remote URL, download it into the generated
     * folder and rewrite the sample to reference the local file. Returns the
     * same instance. On any failure the sample is left as-is so the existing
     * remote-redirect playback path still works.
     */
    public Sample localize(Sample sample) {
        String src = sample.getFilePath();
        if (src == null || !(src.startsWith("http://") || src.startsWith("https://"))) {
            return sample; // already local (Stable Audio / mock) — nothing to download
        }

        try {
            Files.createDirectories(outputDir);
            String ext = resolveExtension(sample.getFormat(), src);
            Path dest = outputDir.resolve(buildFileName(sample.getName(), ext));

            HttpRequest request = HttpRequest.newBuilder(URI.create(src))
                    .timeout(Duration.ofMinutes(2))
                    .GET()
                    .build();
            HttpResponse<InputStream> response =
                    http.send(request, HttpResponse.BodyHandlers.ofInputStream());

            if (response.statusCode() / 100 != 2) {
                log.warn("[generated] download for '{}' returned HTTP {} — keeping remote URL",
                        sample.getName(), response.statusCode());
                return sample;
            }
            try (InputStream in = response.body()) {
                Files.copy(in, dest, StandardCopyOption.REPLACE_EXISTING);
            }

            sample.setFilePath(dest.toString());
            sample.setFormat(ext);
            sample.setSizeBytes(Files.size(dest));
            log.info("[generated] downloaded '{}' -> {}", sample.getName(), dest);
        } catch (Exception e) {
            log.warn("[generated] could not download '{}' ({}): {} — keeping remote URL",
                    sample.getName(), src, e.getMessage());
        }
        return sample;
    }

    private String buildFileName(String name, String ext) {
        String base = (name == null || name.isBlank()) ? "generated" : name;
        base = base.toLowerCase(Locale.ROOT)
                .replaceAll("[^a-z0-9]+", "-")
                .replaceAll("(^-+)|(-+$)", "");
        if (base.isBlank()) {
            base = "generated";
        }
        if (base.length() > 48) {
            base = base.substring(0, 48);
        }
        return base + "_" + UUID.randomUUID().toString().substring(0, 8) + "." + ext;
    }

    private String resolveExtension(String format, String url) {
        if (format != null && !format.isBlank()) {
            return format.toLowerCase(Locale.ROOT).replace(".", "");
        }
        String path = url.split("\\?", 2)[0];
        int dot = path.lastIndexOf('.');
        if (dot > 0 && dot < path.length() - 1) {
            String ext = path.substring(dot + 1).toLowerCase(Locale.ROOT);
            if (ext.matches("[a-z0-9]{2,5}")) {
                return ext;
            }
        }
        return "mp3";
    }

    private static String expandHome(String path) {
        if (path != null && path.startsWith("~")) {
            return System.getProperty("user.home") + path.substring(1);
        }
        return path;
    }
}

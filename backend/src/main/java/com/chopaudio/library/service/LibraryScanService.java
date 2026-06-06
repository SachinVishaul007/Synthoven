package com.chopaudio.library.service;

import com.chopaudio.library.model.Sample;
import com.chopaudio.library.model.ScanFolder;
import com.chopaudio.library.repository.SampleRepository;
import com.chopaudio.library.repository.ScanFolderRepository;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.scheduling.annotation.Async;
import org.springframework.stereotype.Service;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.time.Instant;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import java.util.stream.Stream;

/**
 * Walks watched folders for supported audio files and persists newly discovered
 * samples. De-duplicates by absolute file path so repeated scans are idempotent.
 */
@Service
@Slf4j
public class LibraryScanService {

    private final SampleRepository sampleRepository;
    private final ScanFolderRepository scanFolderRepository;
    private final AudioMetadataService metadataService;

    private final Set<String> supportedFormats;
    private final String[] defaultScanPaths;

    public LibraryScanService(SampleRepository sampleRepository,
                              ScanFolderRepository scanFolderRepository,
                              AudioMetadataService metadataService,
                              @Value("${chop.library.supported-formats}") String formats,
                              @Value("${chop.library.scan-paths}") String scanPaths) {
        this.sampleRepository = sampleRepository;
        this.scanFolderRepository = scanFolderRepository;
        this.metadataService = metadataService;
        this.supportedFormats = new HashSet<>(Arrays.asList(formats.toLowerCase().split(",")));
        this.defaultScanPaths = scanPaths.split(",");
    }

    /**
     * Scan all configured default paths, registering each as a watched folder.
     * Invoked once at startup.
     */
    public void scanDefaultPaths() {
        for (String raw : defaultScanPaths) {
            String path = expandHome(raw.trim());
            if (path.isEmpty()) {
                continue;
            }
            Path dir = Paths.get(path);
            if (!Files.isDirectory(dir)) {
                log.info("Skipping scan path (not a directory): {}", path);
                continue;
            }
            ScanFolder folder = scanFolderRepository.findByPath(path)
                    .orElseGet(() -> scanFolderRepository.save(
                            ScanFolder.builder().path(path).enabled(true).build()));
            scanFolder(folder);
        }
    }

    /**
     * Scan a single folder asynchronously and update its statistics.
     */
    @Async("taskExecutor")
    public void scanFolderAsync(ScanFolder folder) {
        scanFolder(folder);
    }

    public int scanFolder(ScanFolder folder) {
        Path dir = Paths.get(folder.getPath());
        if (!Files.isDirectory(dir)) {
            log.warn("Cannot scan {} — not a directory", folder.getPath());
            return 0;
        }

        int added = 0;
        try (Stream<Path> stream = Files.walk(dir)) {
            for (Path file : (Iterable<Path>) stream.filter(this::isSupportedAudioFile)::iterator) {
                if (persistIfNew(file)) {
                    added++;
                }
            }
        } catch (IOException e) {
            log.error("Failed to walk {}: {}", folder.getPath(), e.getMessage());
        }

        folder.setLastScannedAt(Instant.now());
        folder.setSampleCount((int) sampleRepository.count());
        scanFolderRepository.save(folder);
        log.info("Scanned {} — {} new sample(s)", folder.getPath(), added);
        return added;
    }

    private boolean persistIfNew(Path file) {
        String abs = file.toAbsolutePath().toString();
        if (sampleRepository.existsByFilePath(abs)) {
            return false;
        }
        try {
            Sample sample = metadataService.fromFile(file);
            sampleRepository.save(sample);
            return true;
        } catch (Exception e) {
            log.warn("Could not import {}: {}", abs, e.getMessage());
            return false;
        }
    }

    private boolean isSupportedAudioFile(Path path) {
        if (!Files.isRegularFile(path)) {
            return false;
        }
        String name = path.getFileName().toString().toLowerCase();
        int dot = name.lastIndexOf('.');
        if (dot < 0) {
            return false;
        }
        return supportedFormats.contains(name.substring(dot + 1));
    }

    private String expandHome(String path) {
        if (path.startsWith("~")) {
            return System.getProperty("user.home") + path.substring(1);
        }
        return path;
    }
}

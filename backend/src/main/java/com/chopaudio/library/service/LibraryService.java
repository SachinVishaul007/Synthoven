package com.chopaudio.library.service;

import com.chopaudio.common.exception.ResourceNotFoundException;
import com.chopaudio.library.dto.AddFolderRequest;
import com.chopaudio.library.dto.SampleDto;
import com.chopaudio.library.dto.ScanFolderDto;
import com.chopaudio.library.model.Sample;
import com.chopaudio.library.model.SampleType;
import com.chopaudio.library.model.ScanFolder;
import com.chopaudio.library.repository.SampleRepository;
import com.chopaudio.library.repository.ScanFolderRepository;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.data.domain.PageRequest;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.web.multipart.MultipartFile;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;

/**
 * Library use-cases: browsing, favoriting, play tracking, folder management
 * and statistics.
 */
@Service
@Slf4j
@Transactional
public class LibraryService {

    private static final int RECENT_LIMIT = 30;

    private final SampleRepository sampleRepository;
    private final ScanFolderRepository scanFolderRepository;
    private final LibraryScanService scanService;
    private final AudioMetadataService metadataService;
    private final SampleMapper mapper;
    private final String uploadPath;
    private final Set<String> supportedFormats;

    public LibraryService(SampleRepository sampleRepository,
                          ScanFolderRepository scanFolderRepository,
                          LibraryScanService scanService,
                          AudioMetadataService metadataService,
                          SampleMapper mapper,
                          @Value("${chop.library.upload-path}") String uploadPath,
                          @Value("${chop.library.supported-formats}") String formats) {
        this.sampleRepository = sampleRepository;
        this.scanFolderRepository = scanFolderRepository;
        this.scanService = scanService;
        this.metadataService = metadataService;
        this.mapper = mapper;
        this.uploadPath = uploadPath;
        this.supportedFormats = new HashSet<>(Arrays.asList(formats.toLowerCase().split(",")));
    }

    // ─── Samples ─────────────────────────────────────────────────────────────

    @Transactional(readOnly = true)
    public List<SampleDto> getAllSamples() {
        return mapper.toDtoList(sampleRepository.findAll());
    }

    @Transactional(readOnly = true)
    public List<SampleDto> getByType(SampleType type) {
        return mapper.toDtoList(sampleRepository.findByTypeOrderByCreatedAtDesc(type));
    }

    @Transactional(readOnly = true)
    public List<SampleDto> getFavorites() {
        return mapper.toDtoList(sampleRepository.findByFavoriteTrueOrderByCreatedAtDesc());
    }

    @Transactional(readOnly = true)
    public List<SampleDto> getRecent() {
        return mapper.toDtoList(sampleRepository
                .findByLastPlayedAtNotNullOrderByLastPlayedAtDesc(PageRequest.of(0, RECENT_LIMIT)));
    }

    @Transactional(readOnly = true)
    public SampleDto getById(Long id) {
        return mapper.toDto(findSample(id));
    }

    public SampleDto toggleFavorite(Long id) {
        Sample sample = findSample(id);
        sample.setFavorite(!sample.isFavorite());
        return mapper.toDto(sampleRepository.save(sample));
    }

    public SampleDto recordPlay(Long id) {
        Sample sample = findSample(id);
        sample.setPlayCount(sample.getPlayCount() + 1);
        sample.setLastPlayedAt(Instant.now());
        return mapper.toDto(sampleRepository.save(sample));
    }

    public void deleteSample(Long id) {
        Sample sample = findSample(id);
        sampleRepository.delete(sample);
        log.info("Deleted sample {} ({})", id, sample.getName());
    }

    // ─── Uploads ─────────────────────────────────────────────────────────────

    /**
     * Store uploaded audio files under the configured upload directory and
     * import each as a LIBRARY sample. Non-audio files are skipped. Idempotent:
     * re-uploading the same file (same name) does not create a duplicate, but
     * the existing sample is still returned so the caller always sees every
     * file it uploaded.
     *
     * @return one sample per uploaded audio file (newly imported or pre-existing)
     */
    public List<SampleDto> importUploads(MultipartFile[] files) {
        if (files == null || files.length == 0) {
            throw new IllegalArgumentException("No files were uploaded");
        }

        Path uploadDir = Paths.get(expandHome(uploadPath));
        try {
            Files.createDirectories(uploadDir);
        } catch (IOException e) {
            throw new IllegalStateException("Could not create upload directory: " + uploadDir, e);
        }

        List<SampleDto> result = new ArrayList<>();
        int newlyImported = 0;
        for (MultipartFile file : files) {
            String original = file.getOriginalFilename();
            if (original == null || original.isBlank()) {
                continue;
            }
            // Strip any directory components the browser may include.
            String fileName = Paths.get(original).getFileName().toString();
            if (!isSupportedAudio(fileName)) {
                log.debug("Skipping unsupported upload: {}", fileName);
                continue;
            }

            Path dest = uploadDir.resolve(fileName);
            String abs = dest.toAbsolutePath().toString();

            try {
                if (!Files.exists(dest)) {
                    file.transferTo(dest);
                }
            } catch (IOException e) {
                log.warn("Failed to store upload {}: {}", fileName, e.getMessage());
                continue;
            }

            try {
                Optional<Sample> existing = sampleRepository.findByFilePath(abs);
                if (existing.isPresent()) {
                    result.add(mapper.toDto(existing.get()));
                } else {
                    result.add(mapper.toDto(sampleRepository.save(metadataService.fromFile(dest))));
                    newlyImported++;
                }
            } catch (Exception e) {
                log.warn("Could not import {}: {}", abs, e.getMessage());
            }
        }

        log.info("Upload: {} file(s) -> {} returned ({} newly imported)",
                files.length, result.size(), newlyImported);
        return result;
    }

    private boolean isSupportedAudio(String fileName) {
        int dot = fileName.lastIndexOf('.');
        if (dot < 0 || dot == fileName.length() - 1) {
            return false;
        }
        return supportedFormats.contains(fileName.substring(dot + 1).toLowerCase());
    }

    // ─── Folders ─────────────────────────────────────────────────────────────

    @Transactional(readOnly = true)
    public List<ScanFolderDto> getFolders() {
        return mapper.toFolderDtoList(scanFolderRepository.findAll());
    }

    public ScanFolderDto addFolder(AddFolderRequest request) {
        String path = expandHome(request.path().trim());
        if (scanFolderRepository.existsByPath(path)) {
            throw new IllegalArgumentException("Folder is already watched: " + path);
        }
        ScanFolder folder = scanFolderRepository.save(
                ScanFolder.builder().path(path).enabled(true).build());
        // Kick off an async scan so the response returns immediately.
        scanService.scanFolderAsync(folder);
        return mapper.toDto(folder);
    }

    public void removeFolder(Long id) {
        ScanFolder folder = scanFolderRepository.findById(id)
                .orElseThrow(() -> new ResourceNotFoundException("Folder", id));
        scanFolderRepository.delete(folder);
        log.info("Stopped watching folder {} ({})", id, folder.getPath());
    }

    // ─── Stats ───────────────────────────────────────────────────────────────

    @Transactional(readOnly = true)
    public Map<String, Object> getStats() {
        Map<String, Object> stats = new LinkedHashMap<>();
        stats.put("totalSamples", sampleRepository.count());
        stats.put("librarySamples", sampleRepository.countByType(SampleType.LIBRARY));
        stats.put("generatedSamples", sampleRepository.countByType(SampleType.GENERATED));
        stats.put("favorites", sampleRepository.findByFavoriteTrueOrderByCreatedAtDesc().size());
        stats.put("watchedFolders", scanFolderRepository.count());
        return stats;
    }

    // ─── Helpers ─────────────────────────────────────────────────────────────

    private Sample findSample(Long id) {
        return sampleRepository.findById(id)
                .orElseThrow(() -> new ResourceNotFoundException("Sample", id));
    }

    private String expandHome(String path) {
        if (path.startsWith("~")) {
            return System.getProperty("user.home") + path.substring(1);
        }
        return path;
    }
}

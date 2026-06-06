package com.chopaudio.player.controller;

import com.chopaudio.common.exception.ResourceNotFoundException;
import com.chopaudio.library.dto.SampleDto;
import com.chopaudio.library.service.LibraryService;
import org.springframework.core.io.FileSystemResource;
import org.springframework.core.io.Resource;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.net.URI;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Map;

/**
 * Serves audio for playback. Local library files are streamed from disk;
 * remotely hosted generated samples (e.g. Suno URLs) are redirected to their
 * origin.
 */
@RestController
@RequestMapping("/api/player")
public class PlayerController {

    private final LibraryService libraryService;

    public PlayerController(LibraryService libraryService) {
        this.libraryService = libraryService;
    }

    /** Stream a sample's audio bytes (or redirect to a remote source). */
    @GetMapping("/stream/{id}")
    public ResponseEntity<Resource> stream(@PathVariable Long id) {
        SampleDto sample = libraryService.getById(id);
        String filePath = sample.filePath();

        if (isRemote(filePath)) {
            return ResponseEntity.status(HttpStatus.FOUND)
                    .location(URI.create(filePath))
                    .build();
        }

        Path path = Paths.get(filePath);
        if (!Files.isReadable(path)) {
            throw new ResourceNotFoundException("Audio file is missing on disk for sample " + id);
        }

        Resource resource = new FileSystemResource(path);
        long length;
        try {
            length = Files.size(path);
        } catch (Exception e) {
            length = -1;
        }

        return ResponseEntity.ok()
                .contentType(mediaTypeFor(sample.format()))
                .header(HttpHeaders.ACCEPT_RANGES, "bytes")
                .header(HttpHeaders.CONTENT_LENGTH, String.valueOf(length))
                .header(HttpHeaders.CONTENT_DISPOSITION,
                        "inline; filename=\"" + sample.name() + "." + safeFormat(sample.format()) + "\"")
                .body(resource);
    }

    /** Return a playable URL for a sample without transferring the bytes. */
    @GetMapping("/url/{id}")
    public Map<String, Object> url(@PathVariable Long id) {
        SampleDto sample = libraryService.getById(id);
        String filePath = sample.filePath();
        String url = isRemote(filePath) ? filePath : "/api/player/stream/" + id;
        return Map.of(
                "id", id,
                "url", url,
                "remote", isRemote(filePath),
                "format", safeFormat(sample.format()));
    }

    private boolean isRemote(String filePath) {
        return filePath != null && (filePath.startsWith("http://") || filePath.startsWith("https://"));
    }

    private MediaType mediaTypeFor(String format) {
        if (format == null) {
            return MediaType.APPLICATION_OCTET_STREAM;
        }
        return switch (format.toLowerCase()) {
            case "mp3" -> MediaType.parseMediaType("audio/mpeg");
            case "wav" -> MediaType.parseMediaType("audio/wav");
            case "aiff", "aif" -> MediaType.parseMediaType("audio/aiff");
            case "flac" -> MediaType.parseMediaType("audio/flac");
            default -> MediaType.APPLICATION_OCTET_STREAM;
        };
    }

    private String safeFormat(String format) {
        return format == null || format.isBlank() ? "audio" : format;
    }
}

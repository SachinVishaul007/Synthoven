package com.chopaudio.generation.controller;

import com.chopaudio.generation.dto.GenerationJobDto;
import com.chopaudio.generation.dto.GenerationRequest;
import com.chopaudio.generation.service.GenerationService;
import jakarta.validation.Valid;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.multipart.MultipartFile;
import org.springframework.http.MediaType;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

@RestController
@RequestMapping("/api/generation")
public class GenerationController {

    private final GenerationService generationService;

    public GenerationController(GenerationService generationService) {
        this.generationService = generationService;
    }

    /** Start a generation job; returns immediately with a job id to poll. */
    @PostMapping
    public ResponseEntity<GenerationJobDto> startJob(@Valid @RequestBody GenerationRequest request) {
        GenerationJobDto job = generationService.startJob(
                request.prompt(), request.count(),
                request.durationSeconds(), request.cfgScale(), request.category());
        return ResponseEntity.status(HttpStatus.ACCEPTED).body(job);
    }

    /** Start an audio-to-audio generation job; returns immediately with a job id to poll. */
    @PostMapping(value = "/audio", consumes = MediaType.MULTIPART_FORM_DATA_VALUE)
    public ResponseEntity<GenerationJobDto> startAudioToAudioJob(
            @RequestParam("prompt") String prompt,
            @RequestParam(value = "durationSeconds", required = false) Double durationSeconds,
            @RequestParam(value = "cfgScale", required = false) Double cfgScale,
            @RequestParam(value = "category", required = false) String category,
            @RequestParam("audio") MultipartFile audio) throws IOException {
        
        Path tempFile = Files.createTempFile("sa_init_", ".wav");
        // Copy the upload's stream rather than MultipartFile.transferTo(): with
        // Spring Boot's default file-size-threshold=0 the part is disk-backed, so
        // transferTo() does a move/rename that fails because createTempFile has
        // already created the destination. Files.copy + REPLACE_EXISTING is safe.
        try (java.io.InputStream in = audio.getInputStream()) {
            Files.copy(in, tempFile, java.nio.file.StandardCopyOption.REPLACE_EXISTING);
        }

        // Name the generated output after the source file (+ "_generated") so it's
        // easy to trace back to its origin, e.g. "warm_bass" -> "warm_bass_generated".
        String original = audio.getOriginalFilename();
        String base = (original == null || original.isBlank()) ? "audio" : original;
        int dot = base.lastIndexOf('.');
        if (dot > 0) {
            base = base.substring(0, dot);
        }
        String outputNameHint = base + "_generated";

        GenerationJobDto job = generationService.startJob(
                prompt, 1,
                durationSeconds, cfgScale, category,
                tempFile.toAbsolutePath().toString(),
                outputNameHint);
        return ResponseEntity.status(HttpStatus.ACCEPTED).body(job);
    }

    /** Poll a generation job's status and (once complete) its samples. */
    @GetMapping("/{jobId}")
    public GenerationJobDto getJob(@PathVariable String jobId) {
        return generationService.getJob(jobId);
    }
}

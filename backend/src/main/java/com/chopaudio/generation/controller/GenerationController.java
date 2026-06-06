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
        GenerationJobDto job = generationService.startJob(request.prompt(), request.count());
        return ResponseEntity.status(HttpStatus.ACCEPTED).body(job);
    }

    /** Poll a generation job's status and (once complete) its samples. */
    @GetMapping("/{jobId}")
    public GenerationJobDto getJob(@PathVariable String jobId) {
        return generationService.getJob(jobId);
    }
}

package com.chopaudio.generation.service;

import com.chopaudio.common.exception.ResourceNotFoundException;
import com.chopaudio.generation.client.MockGenerationClient;
import com.chopaudio.generation.client.SunoClient;
import com.chopaudio.generation.dto.GenerationJobDto;
import com.chopaudio.library.dto.SampleDto;
import com.chopaudio.library.model.Sample;
import com.chopaudio.library.repository.SampleRepository;
import com.chopaudio.library.service.SampleMapper;
import jakarta.annotation.PostConstruct;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executor;

/**
 * Orchestrates asynchronous sample generation. A job is created synchronously
 * (returning immediately with status {@code queued}), the configured provider
 * runs on a background thread, and finished samples are persisted to the library
 * as {@link com.chopaudio.library.model.SampleType#GENERATED}. Job state lives
 * in an in-memory map and is polled via {@link #getJob(String)}.
 */
@Service
@Slf4j
public class GenerationService {

    private static final int DEFAULT_COUNT = 2;
    private static final int MAX_COUNT = 5;

    private final MockGenerationClient mockClient;
    private final SunoClient sunoClient;
    private final SampleRepository sampleRepository;
    private final SampleMapper mapper;
    private final Executor executor;
    private final String activeProvider;

    private final Map<String, JobState> jobs = new ConcurrentHashMap<>();

    public GenerationService(MockGenerationClient mockClient,
                             SunoClient sunoClient,
                             SampleRepository sampleRepository,
                             SampleMapper mapper,
                             @Qualifier("taskExecutor") Executor executor,
                             @Value("${chop.generation.provider}") String activeProvider) {
        this.mockClient = mockClient;
        this.sunoClient = sunoClient;
        this.sampleRepository = sampleRepository;
        this.mapper = mapper;
        this.executor = executor;
        this.activeProvider = activeProvider;
    }

    @PostConstruct
    void logProvider() {
        if (!"mock".equals(activeProvider) && !"suno".equals(activeProvider)) {
            log.warn("Unknown generation provider '{}' — falling back to mock", activeProvider);
        } else {
            log.info("Generation provider: {}", activeProvider);
        }
    }

    /** Generate using whichever provider is configured. Blocking. */
    private List<Sample> invokeProvider(String prompt, int count) {
        if ("suno".equals(activeProvider)) {
            return sunoClient.generate(prompt, count);
        }
        return mockClient.generate(prompt, count);
    }

    /**
     * Create a job and start generation in the background. Returns immediately.
     */
    public GenerationJobDto startJob(String prompt, Integer requestedCount) {
        int count = clamp(requestedCount);
        String jobId = activeProvider + "_" + UUID.randomUUID().toString().substring(0, 8);

        JobState state = new JobState(jobId, prompt, activeProvider);
        jobs.put(jobId, state);

        // Run on the shared executor — submitting via the injected bean (rather
        // than an @Async self-invocation) keeps the work genuinely asynchronous.
        executor.execute(() -> runGeneration(jobId));
        return toDto(state);
    }

    void runGeneration(String jobId) {
        JobState state = jobs.get(jobId);
        if (state == null) {
            return;
        }

        state.status = "generating";
        try {
            List<Sample> generated = invokeProvider(state.prompt, DEFAULT_COUNT);
            List<SampleDto> persisted = persist(generated);
            state.complete(persisted);
            log.info("Generation job {} complete — {} sample(s)", jobId, persisted.size());
        } catch (Exception e) {
            log.error("Generation job {} failed: {}", jobId, e.getMessage());
            state.fail(e.getMessage());
        }
    }

    /**
     * Persist each generated sample. Spring Data wraps every {@code save} in its
     * own transaction; the in-memory entities already carry their (eager) tags,
     * so mapping to DTOs needs no surrounding transaction.
     */
    private List<SampleDto> persist(List<Sample> generated) {
        List<SampleDto> result = new ArrayList<>();
        for (Sample s : generated) {
            // Skip if we somehow already stored this exact URL/path.
            if (s.getFilePath() != null && sampleRepository.existsByFilePath(s.getFilePath())) {
                continue;
            }
            result.add(mapper.toDto(sampleRepository.save(s)));
        }
        return result;
    }

    public GenerationJobDto getJob(String jobId) {
        JobState state = jobs.get(jobId);
        if (state == null) {
            throw new ResourceNotFoundException("Generation job", jobId);
        }
        return toDto(state);
    }

    private GenerationJobDto toDto(JobState s) {
        return new GenerationJobDto(
                s.jobId, s.status, s.prompt, s.provider,
                s.samples, s.error, s.createdAt, s.completedAt);
    }

    private int clamp(Integer requested) {
        if (requested == null || requested < 1) {
            return DEFAULT_COUNT;
        }
        return Math.min(requested, MAX_COUNT);
    }

    /**
     * Mutable per-job state held in memory for the lifetime of the process.
     */
    private static final class JobState {
        final String jobId;
        final String prompt;
        final String provider;
        final Instant createdAt = Instant.now();
        volatile String status = "queued";
        volatile List<SampleDto> samples = List.of();
        volatile String error;
        volatile Instant completedAt;

        JobState(String jobId, String prompt, String provider) {
            this.jobId = jobId;
            this.prompt = prompt;
            this.provider = provider;
        }

        void complete(List<SampleDto> samples) {
            this.samples = samples;
            this.status = "complete";
            this.completedAt = Instant.now();
        }

        void fail(String error) {
            this.error = error;
            this.status = "failed";
            this.completedAt = Instant.now();
        }
    }
}

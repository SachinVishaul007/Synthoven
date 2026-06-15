package com.chopaudio.search.service;

import com.chopaudio.generation.dto.GenerationJobDto;
import com.chopaudio.generation.service.GenerationService;
import com.chopaudio.library.dto.SampleDto;
import com.chopaudio.library.repository.SampleRepository;
import com.chopaudio.library.service.SampleMapper;
import com.chopaudio.search.dto.SearchResponse;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.data.domain.PageRequest;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;

/**
 * Bridges library search and on-demand generation. A search returns matching
 * library samples immediately and, when requested, starts a generation job so
 * the client can fill gaps while it browses existing hits.
 */
@Service
@Slf4j
@Transactional(readOnly = true)
public class SearchService {

    private final SampleRepository sampleRepository;
    private final SampleMapper mapper;
    private final GenerationService generationService;
    private final int maxResults;

    public SearchService(SampleRepository sampleRepository,
                         SampleMapper mapper,
                         GenerationService generationService,
                         @Value("${chop.search.max-results}") int maxResults) {
        this.sampleRepository = sampleRepository;
        this.mapper = mapper;
        this.generationService = generationService;
        this.maxResults = maxResults;
    }

    /**
     * Quick library-only search used by GET /api/search?q=...
     */
    public List<SampleDto> quickSearch(String query) {
        if (query == null || query.isBlank()) {
            return List.of();
        }
        return mapper.toDtoList(
                sampleRepository.search(query.trim(), PageRequest.of(0, maxResults)));
    }

    /**
     * Full search used by POST /api/search: returns library hits and, if
     * requested, the id of a freshly started generation job.
     */
    public SearchResponse search(String query, boolean generate) {
        List<SampleDto> libraryResults = quickSearch(query);
        log.info("Search '{}' — {} library hit(s), generate={}",
                query, libraryResults.size(), generate);

        if (!generate) {
            return new SearchResponse(query, libraryResults, null, "done");
        }

        GenerationJobDto job = generationService.startJob(query, null);
        return new SearchResponse(query, libraryResults, job.jobId(), "searching");
    }
}

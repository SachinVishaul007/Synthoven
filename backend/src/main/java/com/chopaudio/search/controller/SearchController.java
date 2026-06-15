package com.chopaudio.search.controller;

import com.chopaudio.library.dto.SampleDto;
import com.chopaudio.search.dto.SearchRequest;
import com.chopaudio.search.dto.SearchResponse;
import com.chopaudio.search.service.SearchService;
import jakarta.validation.Valid;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;

@RestController
@RequestMapping("/api/search")
public class SearchController {

    private final SearchService searchService;

    public SearchController(SearchService searchService) {
        this.searchService = searchService;
    }

    /** Search the library and (by default) start a generation job. */
    @PostMapping
    public SearchResponse search(@Valid @RequestBody SearchRequest request) {
        return searchService.search(request.query(), request.shouldGenerate());
    }

    /** Quick library-only search. */
    @GetMapping
    public List<SampleDto> quickSearch(@RequestParam("q") String query) {
        return searchService.quickSearch(query);
    }
}

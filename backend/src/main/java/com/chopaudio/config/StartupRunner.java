package com.chopaudio.config;

import com.chopaudio.library.service.LibraryScanService;
import lombok.extern.slf4j.Slf4j;
import org.springframework.boot.ApplicationArguments;
import org.springframework.boot.ApplicationRunner;
import org.springframework.stereotype.Component;

@Component
@Slf4j
public class StartupRunner implements ApplicationRunner {

    private final LibraryScanService libraryScanService;

    public StartupRunner(LibraryScanService libraryScanService) {
        this.libraryScanService = libraryScanService;
    }

    @Override
    public void run(ApplicationArguments args) {
        log.info("Chop API started — scanning default library paths...");
        libraryScanService.scanDefaultPaths();
    }
}